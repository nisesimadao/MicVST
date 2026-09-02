#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

// SPSC audio bridge between MicVST's primary callback (producer) and a completely
// separate WASAPI output callback (consumer). The two devices have independent clocks,
// so the consumer performs lightweight linear resampling and a tiny adaptive ratio
// correction to keep the queue around its target fill level instead of slowly drifting.
class MonitorBuffer
{
public:
    static constexpr int capacity = 1 << 17; // ~2.7 s at 48 kHz; power of two not required by AbstractFifo.

    MonitorBuffer()
        : fifo (capacity),
          left ((size_t) capacity, 0.0f),
          right ((size_t) capacity, 0.0f),
          scratchLeft (65536, 0.0f),
          scratchRight (65536, 0.0f)
    {
    }

    void setSourceSampleRate (double rate)
    {
        sourceSampleRate.store (juce::jlimit (8000.0, 384000.0, rate), std::memory_order_release);
    }

    // Called while the secondary device is stopped. It is safe for the UI/device thread:
    // producer callbacks are first disabled, then we wait only for an already-running push.
    void prepare (double newOutputSampleRate, int maxOutputBlock)
    {
        enabled.store (false, std::memory_order_release);
        while (writers.load (std::memory_order_acquire) != 0)
            juce::Thread::yield();

        fifo.reset();
        readPhase = 0.0;
        primed = false;
        outputSampleRate = juce::jlimit (8000.0, 384000.0, newOutputSampleRate);

        // Keep this allocation off both audio callbacks. A ratio of 8 covers even very
        // unusual source/output combinations; normal Windows endpoints are much closer.
        const size_t wanted = (size_t) juce::jmax (65536, maxOutputBlock * 8 + 16);
        scratchLeft.resize (wanted);
        scratchRight.resize (wanted);
        enabled.store (true, std::memory_order_release);
    }

    void stop()
    {
        enabled.store (false, std::memory_order_release);
    }

    bool isEnabled() const { return enabled.load (std::memory_order_acquire); }
    int getNumReady() const { return fifo.getNumReady(); }

    // Producer: called after MicVST's DSP/VST graph has rendered its primary output.
    void push (const float* const* channels, int numChannels, int numSamples)
    {
        if (! enabled.load (std::memory_order_acquire) || channels == nullptr || numChannels <= 0 || numSamples <= 0)
            return;

        writers.fetch_add (1, std::memory_order_acq_rel);
        if (! enabled.load (std::memory_order_acquire))
        {
            writers.fetch_sub (1, std::memory_order_acq_rel);
            return;
        }

        const int writable = juce::jmin (numSamples, fifo.getFreeSpace());
        if (writable > 0)
        {
            int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
            fifo.prepareToWrite (writable, start1, size1, start2, size2);
            copyIntoRing (channels, numChannels, 0, start1, size1);
            copyIntoRing (channels, numChannels, size1, start2, size2);
            fifo.finishedWrite (size1 + size2);
            lastProducerBlock.store (numSamples, std::memory_order_relaxed);
        }

        writers.fetch_sub (1, std::memory_order_acq_rel);
    }

    // Consumer: renders processed MicVST audio into Output2. Underflow deliberately
    // produces silence and re-primes instead of replaying stale/partial samples.
    void render (float* const* outputs, int numOutputs, int numSamples)
    {
        if (outputs == nullptr || numOutputs <= 0 || numSamples <= 0)
            return;

        for (int ch = 0; ch < numOutputs; ++ch)
            if (outputs[ch] != nullptr)
                juce::FloatVectorOperations::clear (outputs[ch], numSamples);

        if (! enabled.load (std::memory_order_acquire))
            return;

        const double srcRate = sourceSampleRate.load (std::memory_order_acquire);
        const int producerBlock = juce::jmax (1, lastProducerBlock.load (std::memory_order_relaxed));
        const int target = juce::jlimit (256, capacity / 4,
            juce::jmax ((int) std::lround (srcRate * 0.020), producerBlock * 3));

        int ready = fifo.getNumReady();

        // If clocks ever diverged badly (device sleep/resume etc.), jump back near the
        // live edge instead of letting Output2 accumulate huge audible latency.
        if (ready > capacity * 3 / 4)
        {
            const int drop = juce::jmax (0, ready - target);
            discard (drop);
            ready = fifo.getNumReady();
            readPhase = 0.0;
        }

        if (! primed)
        {
            if (ready < target)
                return;
            primed = true;
            readPhase = 0.0;
        }

        const double baseRatio = juce::jlimit (0.125, 8.0, srcRate / outputSampleRate);
        const double fillError = ((double) ready - (double) target) / (double) juce::jmax (1, target);
        const double correction = juce::jlimit (-0.005, 0.005, fillError * 0.002);
        const double ratio = baseRatio * (1.0 + correction);

        const double endPosition = readPhase + (double) numSamples * ratio;
        const int consumed = (int) std::floor (endPosition);
        const int needed = consumed + 2; // +1 for interpolation, +1 safety.

        if (needed > ready || (size_t) needed > scratchLeft.size())
        {
            // Flush what's left and wait for a clean safety buffer. This avoids a burst
            // of stale audio after a genuine underrun.
            discard (ready);
            primed = false;
            readPhase = 0.0;
            return;
        }

        copyReadyToScratch (needed);

        double pos = readPhase;
        for (int i = 0; i < numSamples; ++i)
        {
            const int index = (int) pos;
            const float frac = (float) (pos - (double) index);
            const float l = scratchLeft[(size_t) index]
                          + (scratchLeft[(size_t) index + 1] - scratchLeft[(size_t) index]) * frac;
            const float r = scratchRight[(size_t) index]
                          + (scratchRight[(size_t) index + 1] - scratchRight[(size_t) index]) * frac;

            if (numOutputs == 1)
            {
                if (outputs[0] != nullptr) outputs[0][i] = 0.5f * (l + r);
            }
            else
            {
                if (outputs[0] != nullptr) outputs[0][i] = l;
                if (outputs[1] != nullptr) outputs[1][i] = r;
                for (int ch = 2; ch < numOutputs; ++ch)
                    if (outputs[ch] != nullptr) outputs[ch][i] = (ch & 1) == 0 ? l : r;
            }
            pos += ratio;
        }

        fifo.finishedRead (consumed);
        readPhase = endPosition - (double) consumed;
    }

private:
    void copyIntoRing (const float* const* channels, int numChannels,
                       int sourceOffset, int destOffset, int count)
    {
        if (count <= 0) return;
        const float* l = channels[0];
        const float* r = numChannels > 1 ? channels[1] : channels[0];
        juce::FloatVectorOperations::copy (left.data() + destOffset, l + sourceOffset, count);
        juce::FloatVectorOperations::copy (right.data() + destOffset, r + sourceOffset, count);
    }

    void copyReadyToScratch (int count)
    {
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        fifo.prepareToRead (count, start1, size1, start2, size2);
        if (size1 > 0)
        {
            juce::FloatVectorOperations::copy (scratchLeft.data(), left.data() + start1, size1);
            juce::FloatVectorOperations::copy (scratchRight.data(), right.data() + start1, size1);
        }
        if (size2 > 0)
        {
            juce::FloatVectorOperations::copy (scratchLeft.data() + size1, left.data() + start2, size2);
            juce::FloatVectorOperations::copy (scratchRight.data() + size1, right.data() + start2, size2);
        }
        // Intentionally do not call finishedRead here. render() consumes only floor(position),
        // leaving interpolation look-ahead samples available for the next callback.
    }

    void discard (int count)
    {
        count = juce::jlimit (0, fifo.getNumReady(), count);
        if (count <= 0) return;
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        fifo.prepareToRead (count, start1, size1, start2, size2);
        fifo.finishedRead (size1 + size2);
    }

    juce::AbstractFifo fifo;
    std::vector<float> left, right;
    std::vector<float> scratchLeft, scratchRight;
    std::atomic<double> sourceSampleRate { 48000.0 };
    std::atomic<int> lastProducerBlock { 480 };
    std::atomic<int> writers { 0 };
    std::atomic<bool> enabled { false };
    double outputSampleRate = 48000.0;
    double readPhase = 0.0;
    bool primed = false;
};
