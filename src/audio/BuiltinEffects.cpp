#include "audio/BuiltinEffects.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

namespace
{
    using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;

    std::unique_ptr<juce::RangedAudioParameter> floatParam (const char* id,
                                                             const char* name,
                                                             float min,
                                                             float max,
                                                             float step,
                                                             float initial,
                                                             const char* suffix = "")
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> { min, max, step }, initial,
            juce::AudioParameterFloatAttributes().withLabel (suffix));
    }

    std::unique_ptr<juce::RangedAudioParameter> choiceParam (const char* id,
                                                              const char* name,
                                                              juce::StringArray choices,
                                                              int initial)
    {
        return std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { id, 1 }, name, std::move (choices), initial);
    }

    class BuiltinProcessorBase : public juce::AudioProcessor
    {
    public:
        BuiltinProcessorBase (const juce::String& processorName, Layout layout)
            : juce::AudioProcessor (BusesProperties()
                  .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                  .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
              name (processorName),
              state (*this, nullptr, "builtin-state", std::move (layout))
        {
        }

        const juce::String getName() const override { return name; }
        double getTailLengthSeconds() const override { return 0.0; }
        bool acceptsMidi() const override { return false; }
        bool producesMidi() const override { return false; }
        bool isMidiEffect() const override { return false; }

        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram (int) override {}
        const juce::String getProgramName (int) override { return {}; }
        void changeProgramName (int, const juce::String&) override {}

        bool hasEditor() const override { return true; }
        juce::AudioProcessorEditor* createEditor() override
        {
            auto* editor = new juce::GenericAudioProcessorEditor (*this);
            editor->setSize (420, juce::jmax (180, editor->getHeight()));
            return editor;
        }

        void getStateInformation (juce::MemoryBlock& dest) override
        {
            if (auto xml = state.copyState().createXml())
                copyXmlToBinary (*xml, dest);
        }

        void setStateInformation (const void* data, int size) override
        {
            if (auto xml = getXmlFromBinary (data, size))
                state.replaceState (juce::ValueTree::fromXml (*xml));
        }

        void releaseResources() override {}

        bool isBusesLayoutSupported (const BusesLayout& layouts) const override
        {
            const auto in  = layouts.getMainInputChannelSet();
            const auto out = layouts.getMainOutputChannelSet();
            if (in != out) return false;
            return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
        }

    protected:
        float value (const char* id) const
        {
            if (auto* p = state.getRawParameterValue (id)) return p->load();
            return 0.0f;
        }

        juce::String name;
        juce::AudioProcessorValueTreeState state;
    };

    class SimpleDelayLine
    {
    public:
        void prepare (int maxDelaySamples)
        {
            data.assign ((size_t) juce::jmax (8, maxDelaySamples + 4), 0.0f);
            writePos = 0;
        }

        void reset()
        {
            std::fill (data.begin(), data.end(), 0.0f);
            writePos = 0;
        }

        float read (float delaySamples) const
        {
            if (data.empty()) return 0.0f;
            delaySamples = juce::jlimit (1.0f, (float) data.size() - 2.0f, delaySamples);
            float pos = (float) writePos - delaySamples;
            while (pos < 0.0f) pos += (float) data.size();
            while (pos >= (float) data.size()) pos -= (float) data.size();
            const int i0 = (int) pos;
            const int i1 = (i0 + 1) % (int) data.size();
            const float frac = pos - (float) i0;
            return data[(size_t) i0] + (data[(size_t) i1] - data[(size_t) i0]) * frac;
        }

        void push (float x)
        {
            if (data.empty()) return;
            data[(size_t) writePos] = x;
            writePos = (writePos + 1) % (int) data.size();
        }

    private:
        std::vector<float> data;
        int writePos = 0;
    };

    // Lightweight dual-read-head time-domain pitch shifter. It is intentionally tuned for
    // voice-chat latency rather than mastering quality: ~11 ms at 48 kHz with the default window.
    class VoicePitchShifter
    {
    public:
        void prepare (double newSampleRate, int channels)
        {
            sampleRate = newSampleRate;
            numChannels = juce::jlimit (1, 2, channels);
            const int wanted = juce::nextPowerOfTwo ((int) std::ceil (sampleRate * 0.30));
            ringSize = juce::jmax (4096, wanted);
            ring.assign ((size_t) numChannels, std::vector<float> ((size_t) ringSize, 0.0f));
            writePos = 0;
            phase = 0.25f;
            windowSamples = juce::jlimit (384.0f, 1536.0f, (float) sampleRate * 0.021333f); // ~1024 @ 48k
            minDelay = juce::jmax (24.0f, (float) sampleRate * 0.0007f);
        }

        void reset()
        {
            for (auto& c : ring) std::fill (c.begin(), c.end(), 0.0f);
            writePos = 0;
            phase = 0.25f;
        }

        int latencySamples() const { return (int) std::lround (minDelay + windowSamples * 0.5f); }

        void process (juce::AudioBuffer<float>& buffer, float semitones, float wet)
        {
            const int channels = juce::jmin (buffer.getNumChannels(), numChannels);
            const int samples = buffer.getNumSamples();
            if (channels <= 0 || samples <= 0) return;

            wet = juce::jlimit (0.0f, 1.0f, wet);
            if (std::abs (semitones) < 0.01f || wet <= 0.0001f) return;
            const float ratio = std::pow (2.0f, semitones / 12.0f);
            const float phaseStep = (1.0f - ratio) / windowSamples;

            for (int i = 0; i < samples; ++i)
            {
                std::array<float, 2> dry { 0.0f, 0.0f };
                for (int ch = 0; ch < channels; ++ch)
                {
                    dry[(size_t) ch] = buffer.getSample (ch, i);
                    ring[(size_t) ch][(size_t) writePos] = dry[(size_t) ch];
                }

                const float p1 = wrap01 (phase);
                const float p2 = wrap01 (phase + 0.5f);
                const float d1 = minDelay + p1 * windowSamples;
                const float d2 = minDelay + p2 * windowSamples;
                const float w1 = std::sin (juce::MathConstants<float>::pi * p1);
                const float w2 = std::sin (juce::MathConstants<float>::pi * p2);
                const float g1 = w1 * w1;
                const float g2 = w2 * w2;
                const float norm = 1.0f / juce::jmax (0.0001f, g1 + g2);

                for (int ch = 0; ch < channels; ++ch)
                {
                    const float shifted = (readDelay (ch, d1) * g1 + readDelay (ch, d2) * g2) * norm;
                    buffer.setSample (ch, i, dry[(size_t) ch] + (shifted - dry[(size_t) ch]) * wet);
                }

                writePos = (writePos + 1) % ringSize;
                phase = wrap01 (phase + phaseStep);
            }
        }

    private:
        static float wrap01 (float v)
        {
            v -= std::floor (v);
            return v;
        }

        float readDelay (int channel, float delay) const
        {
            float pos = (float) writePos - delay;
            while (pos < 0.0f) pos += (float) ringSize;
            while (pos >= (float) ringSize) pos -= (float) ringSize;
            const int i0 = (int) pos;
            const int i1 = (i0 + 1) % ringSize;
            const float frac = pos - (float) i0;
            const auto& r = ring[(size_t) channel];
            return r[(size_t) i0] + (r[(size_t) i1] - r[(size_t) i0]) * frac;
        }

        double sampleRate = 48000.0;
        int numChannels = 2;
        int ringSize = 16384;
        int writePos = 0;
        float phase = 0.25f;
        float windowSamples = 1024.0f;
        float minDelay = 32.0f;
        std::vector<std::vector<float>> ring;
    };

    class PitchDetector
    {
    public:
        static constexpr int historySize = 1024;
        void prepare (double sr)
        {
            sampleRate = sr;
            write = 0;
            decimCounter = 0;
            std::fill (history.begin(), history.end(), 0.0f);
            lastHz = 0.0f;
        }

        void pushBlock (const juce::AudioBuffer<float>& buffer)
        {
            if (buffer.getNumChannels() <= 0) return;
            const int n = buffer.getNumSamples();
            for (int i = 0; i < n; ++i)
            {
                if ((decimCounter++ & 1) != 0) continue;
                float x = buffer.getSample (0, i);
                if (buffer.getNumChannels() > 1) x = 0.5f * (x + buffer.getSample (1, i));
                history[(size_t) write++] = x;
                if (write == (int) history.size())
                {
                    write = 0;
                    analyse();
                }
            }
        }

        float frequency() const { return lastHz; }

    private:
        void analyse()
        {
            constexpr int n = historySize;
            std::array<float, historySize> ordered {};
            for (int i = 0; i < n; ++i)
                ordered[(size_t) i] = history[(size_t) ((write + i) % n)];

            float mean = 0.0f;
            for (float x : ordered) mean += x;
            mean /= (float) n;
            float energy = 0.0f;
            for (auto& x : ordered) { x -= mean; energy += x * x; }
            if (energy < 1.0e-5f) { lastHz = 0.0f; return; }

            const float dsRate = (float) sampleRate * 0.5f;
            const int minLag = juce::jmax (2, (int) std::floor (dsRate / 500.0f));
            const int maxLag = juce::jmin (n / 2, (int) std::ceil (dsRate / 70.0f));

            float best = 0.0f;
            int bestLag = 0;
            for (int lag = minLag; lag <= maxLag; ++lag)
            {
                double xy = 0.0, xx = 0.0, yy = 0.0;
                const int count = n - lag;
                for (int i = 0; i < count; ++i)
                {
                    const float a = ordered[(size_t) i];
                    const float b = ordered[(size_t) (i + lag)];
                    xy += (double) a * b;
                    xx += (double) a * a;
                    yy += (double) b * b;
                }
                const float corr = (float) (xy / std::sqrt (xx * yy + 1.0e-12));
                if (corr > best) { best = corr; bestLag = lag; }
            }

            lastHz = (best > 0.58f && bestLag > 0) ? dsRate / (float) bestLag : 0.0f;
        }

        double sampleRate = 48000.0;
        std::array<float, historySize> history {};
        int write = 0;
        int decimCounter = 0;
        float lastHz = 0.0f;
    };

    float nearestScaleMidi (float midi, int key, int scale)
    {
        static constexpr int major[] = { 0, 2, 4, 5, 7, 9, 11 };
        static constexpr int minor[] = { 0, 2, 3, 5, 7, 8, 10 };
        if (scale == 0) return std::round (midi); // chromatic

        const int centre = (int) std::lround (midi);
        float best = (float) centre;
        float bestDist = 1000.0f;
        for (int note = centre - 12; note <= centre + 12; ++note)
        {
            const int pc = ((note - key) % 12 + 12) % 12;
            bool allowed = false;
            const auto* tones = scale == 1 ? major : minor;
            for (int i = 0; i < 7; ++i) if (pc == tones[i]) { allowed = true; break; }
            if (! allowed) continue;
            const float d = std::abs ((float) note - midi);
            if (d < bestDist) { bestDist = d; best = (float) note; }
        }
        return best;
    }

    class AutoTuneProcessor final : public BuiltinProcessorBase
    {
    public:
        AutoTuneProcessor() : BuiltinProcessorBase ("AutoTune", makeLayout()) {}

        static Layout makeLayout()
        {
            Layout l;
            l.add (floatParam ("strength", "Strength", 0.0f, 100.0f, 1.0f, 100.0f, "%"));
            l.add (floatParam ("speed", "Retune speed", 0.0f, 250.0f, 1.0f, 18.0f, "ms"));
            l.add (choiceParam ("key", "Key", { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));
            l.add (choiceParam ("scale", "Scale", { "Chromatic", "Major", "Minor" }, 0));
            l.add (floatParam ("mix", "Mix", 0.0f, 100.0f, 1.0f, 100.0f, "%"));
            return l;
        }

        void prepareToPlay (double sr, int) override
        {
            sampleRate = sr;
            shifter.prepare (sr, getTotalNumOutputChannels());
            detector.prepare (sr);
            smoothedSemitones = 0.0f;
            setLatencySamples (shifter.latencySamples());
        }

        void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
        {
            juce::ScopedNoDenormals noDenormals;
            detector.pushBlock (buffer);
            const float hz = detector.frequency();
            float wanted = 0.0f;
            if (hz >= 70.0f && hz <= 500.0f)
            {
                const float midi = 69.0f + 12.0f * std::log2 (hz / 440.0f);
                const int key = juce::jlimit (0, 11, (int) std::lround (value ("key")));
                const int scale = juce::jlimit (0, 2, (int) std::lround (value ("scale")));
                const float target = nearestScaleMidi (midi, key, scale);
                wanted = juce::jlimit (-7.0f, 7.0f, (target - midi) * value ("strength") * 0.01f);
            }

            const float speedMs = value ("speed");
            const float tau = juce::jmax (0.001f, speedMs * 0.001f);
            const float alpha = speedMs <= 0.5f ? 1.0f
                : 1.0f - std::exp (-(float) buffer.getNumSamples() / ((float) sampleRate * tau));
            smoothedSemitones += (wanted - smoothedSemitones) * alpha;
            shifter.process (buffer, smoothedSemitones, value ("mix") * 0.01f);
        }

    private:
        double sampleRate = 48000.0;
        PitchDetector detector;
        VoicePitchShifter shifter;
        float smoothedSemitones = 0.0f;
    };

    class PitchProcessor final : public BuiltinProcessorBase
    {
    public:
        PitchProcessor() : BuiltinProcessorBase ("Pitch Shift", makeLayout()) {}
        static Layout makeLayout()
        {
            Layout l;
            l.add (floatParam ("semitones", "Semitones", -12.0f, 12.0f, 0.1f, 0.0f, "st"));
            l.add (floatParam ("mix", "Mix", 0.0f, 100.0f, 1.0f, 100.0f, "%"));
            return l;
        }
        void prepareToPlay (double sr, int) override
        {
            shifter.prepare (sr, getTotalNumOutputChannels());
            setLatencySamples (shifter.latencySamples());
        }
        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
        {
            juce::ScopedNoDenormals n;
            shifter.process (b, value ("semitones"), value ("mix") * 0.01f);
        }
    private:
        VoicePitchShifter shifter;
    };

    class DeepVoiceProcessor final : public BuiltinProcessorBase
    {
    public:
        DeepVoiceProcessor() : BuiltinProcessorBase ("Deep Voice", makeLayout()) {}
        static Layout makeLayout()
        {
            Layout l;
            l.add (floatParam ("depth", "Depth", -12.0f, -1.0f, 0.1f, -5.0f, "st"));
            l.add (floatParam ("warmth", "Warmth", 0.0f, 100.0f, 1.0f, 45.0f, "%"));
            l.add (floatParam ("mix", "Mix", 0.0f, 100.0f, 1.0f, 100.0f, "%"));
            return l;
        }
        void prepareToPlay (double sr, int) override
        {
            sampleRate = (float) sr;
            shifter.prepare (sr, getTotalNumOutputChannels());
            lp.fill (0.0f);
            setLatencySamples (shifter.latencySamples());
        }
        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
        {
            juce::ScopedNoDenormals n;
            shifter.process (b, value ("depth"), value ("mix") * 0.01f);
            const float warmth = value ("warmth") * 0.01f;
            const float cutoff = 9000.0f - warmth * 6000.0f;
            const float a = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * cutoff / sampleRate);
            for (int ch = 0; ch < juce::jmin (2, b.getNumChannels()); ++ch)
            {
                auto* x = b.getWritePointer (ch);
                float z = lp[(size_t) ch];
                for (int i = 0; i < b.getNumSamples(); ++i)
                {
                    z += a * (x[i] - z);
                    x[i] = std::tanh (z * (1.0f + warmth * 1.4f));
                }
                lp[(size_t) ch] = z;
            }
        }
    private:
        VoicePitchShifter shifter;
        float sampleRate = 48000.0f;
        std::array<float, 2> lp { 0.0f, 0.0f };
    };

    class RobotProcessor final : public BuiltinProcessorBase
    {
    public:
        RobotProcessor() : BuiltinProcessorBase ("Robot", makeLayout()) {}
        static Layout makeLayout()
        {
            Layout l;
            l.add (floatParam ("carrier", "Carrier", 35.0f, 320.0f, 1.0f, 92.0f, "Hz"));
            l.add (floatParam ("drive", "Drive", 0.0f, 24.0f, 0.5f, 8.0f, "dB"));
            l.add (floatParam ("mix", "Mix", 0.0f, 100.0f, 1.0f, 85.0f, "%"));
            return l;
        }
        void prepareToPlay (double sr, int) override { sampleRate = (float) sr; phase = 0.0f; }
        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
        {
            juce::ScopedNoDenormals n;
            const float hz = value ("carrier");
            const float inc = hz / sampleRate;
            const float gain = juce::Decibels::decibelsToGain (value ("drive"));
            const float mix = value ("mix") * 0.01f;
            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                const float carrier = 0.7f * std::sin (juce::MathConstants<float>::twoPi * phase)
                                    + 0.3f * std::sin (juce::MathConstants<float>::twoPi * phase * 2.0f);
                phase += inc;
                phase -= std::floor (phase);
                for (int ch = 0; ch < b.getNumChannels(); ++ch)
                {
                    const float dry = b.getSample (ch, i);
                    const float wet = std::tanh (dry * gain) * carrier;
                    b.setSample (ch, i, dry + (wet - dry) * mix);
                }
            }
        }
    private:
        float sampleRate = 48000.0f;
        float phase = 0.0f;
    };

    class RadioProcessor final : public BuiltinProcessorBase
    {
    public:
        RadioProcessor() : BuiltinProcessorBase ("Radio / Walkie-Talkie", makeLayout()) {}
        static Layout makeLayout()
        {
            Layout l;
            l.add (floatParam ("lowcut", "Low cut", 120.0f, 900.0f, 10.0f, 280.0f, "Hz"));
            l.add (floatParam ("highcut", "High cut", 1800.0f, 9000.0f, 50.0f, 3800.0f, "Hz"));
            l.add (floatParam ("drive", "Crunch", 0.0f, 100.0f, 1.0f, 42.0f, "%"));
            l.add (floatParam ("noise", "Static", 0.0f, 20.0f, 0.5f, 2.0f, "%"));
            return l;
        }
        void prepareToPlay (double sr, int) override
        {
            sampleRate = (float) sr;
            hpY.fill (0.0f); hpX.fill (0.0f); lpY.fill (0.0f);
        }
        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
        {
            juce::ScopedNoDenormals n;
            const float dt = 1.0f / sampleRate;
            const float hpRC = 1.0f / (juce::MathConstants<float>::twoPi * value ("lowcut"));
            const float hpA = hpRC / (hpRC + dt);
            const float lpA = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * value ("highcut") / sampleRate);
            const float drive = 1.0f + value ("drive") * 0.055f;
            const float noise = value ("noise") * 0.0009f;

            for (int ch = 0; ch < juce::jmin (2, b.getNumChannels()); ++ch)
            {
                auto* x = b.getWritePointer (ch);
                float hy = hpY[(size_t) ch], hx = hpX[(size_t) ch], ly = lpY[(size_t) ch];
                for (int i = 0; i < b.getNumSamples(); ++i)
                {
                    const float in = x[i];
                    hy = hpA * (hy + in - hx); hx = in;
                    ly += lpA * (hy - ly);
                    x[i] = std::tanh (ly * drive) + randomSigned() * noise;
                }
                hpY[(size_t) ch] = hy; hpX[(size_t) ch] = hx; lpY[(size_t) ch] = ly;
            }
        }
    private:
        float randomSigned()
        {
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            return ((float) (rng & 0x00ffffffu) / 8388607.5f) - 1.0f;
        }
        float sampleRate = 48000.0f;
        std::array<float, 2> hpY {}, hpX {}, lpY {};
        uint32_t rng = 0x5a17c9e3u;
    };

    class BitcrusherProcessor final : public BuiltinProcessorBase
    {
    public:
        BitcrusherProcessor() : BuiltinProcessorBase ("Bitcrusher", makeLayout()) {}
        static Layout makeLayout()
        {
            Layout l;
            l.add (floatParam ("bits", "Bit depth", 2.0f, 16.0f, 1.0f, 8.0f, "bit"));
            l.add (floatParam ("rate", "Sample rate", 1000.0f, 48000.0f, 100.0f, 9000.0f, "Hz"));
            l.add (floatParam ("mix", "Mix", 0.0f, 100.0f, 1.0f, 100.0f, "%"));
            return l;
        }
        void prepareToPlay (double sr, int) override
        {
            sampleRate = (float) sr;
            phase = 1.0f;
            held.fill (0.0f);
        }
        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
        {
            const int bits = juce::jlimit (2, 16, (int) std::lround (value ("bits")));
            const float levels = (float) (1u << (bits - 1));
            const float inc = juce::jlimit (0.001f, 1.0f, value ("rate") / sampleRate);
            const float mix = value ("mix") * 0.01f;
            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                phase += inc;
                const bool capture = phase >= 1.0f;
                if (capture) phase -= std::floor (phase);
                for (int ch = 0; ch < juce::jmin (2, b.getNumChannels()); ++ch)
                {
                    const float dry = b.getSample (ch, i);
                    if (capture) held[(size_t) ch] = std::round (dry * levels) / levels;
                    b.setSample (ch, i, dry + (held[(size_t) ch] - dry) * mix);
                }
            }
        }
    private:
        float sampleRate = 48000.0f;
        float phase = 1.0f;
        std::array<float, 2> held {};
    };

    struct BiquadCoeffs
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    };

    struct BiquadState
    {
        float z1 = 0.0f, z2 = 0.0f;
        void reset() { z1 = z2 = 0.0f; }
        float process (float x, const BiquadCoeffs& c)
        {
            const float y = c.b0 * x + z1;
            z1 = c.b1 * x - c.a1 * y + z2;
            z2 = c.b2 * x - c.a2 * y;
            return y;
        }
    };

    BiquadCoeffs makeBandPass (float sampleRate, float frequency, float q)
    {
        frequency = juce::jlimit (40.0f, sampleRate * 0.45f, frequency);
        q = juce::jlimit (0.25f, 12.0f, q);
        const float w = juce::MathConstants<float>::twoPi * frequency / sampleRate;
        const float s = std::sin (w);
        const float c = std::cos (w);
        const float alpha = s / (2.0f * q);
        const float a0 = 1.0f + alpha;
        BiquadCoeffs r;
        r.b0 = alpha / a0;
        r.b1 = 0.0f;
        r.b2 = -alpha / a0;
        r.a1 = (-2.0f * c) / a0;
        r.a2 = (1.0f - alpha) / a0;
        return r;
    }

    class WahProcessor final : public BuiltinProcessorBase
    {
    public:
        WahProcessor() : BuiltinProcessorBase ("Wah / Auto Wah", makeLayout()) {}

        static Layout makeLayout()
        {
            Layout l;
            l.add (choiceParam ("mode", "Mode", { "Envelope", "LFO", "Manual" }, 0));
            l.add (floatParam ("base", "Base frequency", 180.0f, 1400.0f, 10.0f, 320.0f, "Hz"));
            l.add (floatParam ("range", "Sweep range", 200.0f, 3200.0f, 10.0f, 1800.0f, "Hz"));
            l.add (floatParam ("resonance", "Resonance", 0.4f, 8.0f, 0.1f, 2.4f, "Q"));
            l.add (floatParam ("rate", "LFO rate", 0.10f, 8.0f, 0.05f, 1.6f, "Hz"));
            l.add (floatParam ("sensitivity", "Envelope sensitivity", 0.0f, 100.0f, 1.0f, 55.0f, "%"));
            l.add (floatParam ("position", "Manual position", 0.0f, 100.0f, 1.0f, 50.0f, "%"));
            l.add (floatParam ("mix", "Mix", 0.0f, 100.0f, 1.0f, 78.0f, "%"));
            return l;
        }

        void prepareToPlay (double sr, int) override
        {
            sampleRate = (float) sr;
            phase = 0.0f;
            envelope = 0.0f;
            for (auto& f : filters) f.reset();
        }

        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
        {
            juce::ScopedNoDenormals n;
            const int channels = juce::jmin (2, b.getNumChannels());
            const int mode = juce::jlimit (0, 2, (int) std::lround (value ("mode")));
            const float base = value ("base");
            const float top = juce::jmin (sampleRate * 0.42f, base + value ("range"));
            const float q = value ("resonance");
            const float rate = value ("rate");
            const float sensitivity = 0.5f + value ("sensitivity") * 0.075f;
            const float manual = value ("position") * 0.01f;
            const float mix = value ("mix") * 0.01f;
            const float attack = std::exp (-1.0f / (sampleRate * 0.004f));
            const float release = std::exp (-1.0f / (sampleRate * 0.100f));

            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                float amp = 0.0f;
                for (int ch = 0; ch < channels; ++ch)
                    amp += std::abs (b.getSample (ch, i));
                amp /= (float) juce::jmax (1, channels);
                const float envCoeff = amp > envelope ? attack : release;
                envelope = envCoeff * envelope + (1.0f - envCoeff) * amp;

                float position = manual;
                if (mode == 0)
                    position = juce::jlimit (0.0f, 1.0f, envelope * sensitivity);
                else if (mode == 1)
                    position = 0.5f + 0.5f * std::sin (juce::MathConstants<float>::twoPi * phase);

                const float ratio = juce::jmax (1.001f, top / juce::jmax (40.0f, base));
                const float frequency = base * std::pow (ratio, position);
                const auto coeffs = makeBandPass (sampleRate, frequency, q);

                for (int ch = 0; ch < channels; ++ch)
                {
                    const float dry = b.getSample (ch, i);
                    const float wet = std::tanh (filters[(size_t) ch].process (dry, coeffs) * 2.2f);
                    b.setSample (ch, i, dry + (wet - dry) * mix);
                }

                phase += rate / sampleRate;
                phase -= std::floor (phase);
            }
        }

    private:
        float sampleRate = 48000.0f;
        float phase = 0.0f;
        float envelope = 0.0f;
        std::array<BiquadState, 2> filters;
    };

    class ChorusProcessor final : public BuiltinProcessorBase
    {
    public:
        ChorusProcessor() : BuiltinProcessorBase ("Chorus", makeLayout()) {}

        static Layout makeLayout()
        {
            Layout l;
            l.add (floatParam ("rate", "Rate", 0.05f, 8.0f, 0.05f, 0.80f, "Hz"));
            l.add (floatParam ("depth", "Depth", 0.0f, 15.0f, 0.1f, 5.0f, "ms"));
            l.add (floatParam ("delay", "Base delay", 2.0f, 30.0f, 0.1f, 12.0f, "ms"));
            l.add (floatParam ("feedback", "Feedback", 0.0f, 70.0f, 1.0f, 14.0f, "%"));
            l.add (floatParam ("stereo", "Stereo", 0.0f, 100.0f, 1.0f, 100.0f, "%"));
            l.add (floatParam ("mix", "Mix", 0.0f, 100.0f, 1.0f, 35.0f, "%"));
            return l;
        }

        void prepareToPlay (double sr, int) override
        {
            sampleRate = (float) sr;
            phase = 0.0f;
            for (auto& d : delayLines) d.prepare ((int) std::ceil (sampleRate * 0.080f));
        }

        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
        {
            juce::ScopedNoDenormals n;
            const int channels = juce::jmin (2, b.getNumChannels());
            const float rate = value ("rate");
            const float depthMs = value ("depth");
            const float baseMs = value ("delay");
            const float feedback = value ("feedback") * 0.01f;
            const float stereoPhase = juce::MathConstants<float>::pi * value ("stereo") * 0.01f;
            const float mix = value ("mix") * 0.01f;

            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                for (int ch = 0; ch < channels; ++ch)
                {
                    const float dry = b.getSample (ch, i);
                    const float phaseOffset = ch == 1 ? stereoPhase : 0.0f;
                    const float lfo = 0.5f + 0.5f * std::sin (juce::MathConstants<float>::twoPi * phase + phaseOffset);
                    const float delaySamples = (baseMs + depthMs * lfo) * 0.001f * sampleRate;
                    const float wet = delayLines[(size_t) ch].read (delaySamples);
                    delayLines[(size_t) ch].push (dry + wet * feedback);
                    b.setSample (ch, i, dry + (wet - dry) * mix);
                }
                phase += rate / sampleRate;
                phase -= std::floor (phase);
            }
        }

    private:
        float sampleRate = 48000.0f;
        float phase = 0.0f;
        std::array<SimpleDelayLine, 2> delayLines;
    };

    class UnisonProcessor final : public BuiltinProcessorBase
    {
    public:
        UnisonProcessor() : BuiltinProcessorBase ("Unison", makeLayout()) {}

        static Layout makeLayout()
        {
            Layout l;
            l.add (floatParam ("voices", "Voices", 2.0f, 8.0f, 1.0f, 4.0f));
            l.add (floatParam ("detune", "Detune", 0.0f, 40.0f, 0.5f, 12.0f, "cent"));
            l.add (floatParam ("spread", "Stereo spread", 0.0f, 100.0f, 1.0f, 75.0f, "%"));
            l.add (floatParam ("delay", "Voice stagger", 0.0f, 30.0f, 0.5f, 9.0f, "ms"));
            l.add (floatParam ("mix", "Mix", 0.0f, 100.0f, 1.0f, 60.0f, "%"));
            return l;
        }

        void prepareToPlay (double sr, int samplesPerBlock) override
        {
            sampleRate = (float) sr;
            channels = juce::jlimit (1, 2, getTotalNumOutputChannels());
            for (auto& s : shifters) s.prepare (sr, channels);
            pitchLatency = shifters[0].latencySamples();
            for (auto& voice : voiceDelay)
                for (auto& d : voice)
                    d.prepare ((int) std::ceil (sampleRate * 0.080f) + pitchLatency + 8);
            temp.setSize (channels, juce::jmax (64, samplesPerBlock), false, false, true);
            wet.setSize (channels, juce::jmax (64, samplesPerBlock), false, false, true);
            lastVoices = -1;
            setLatencySamples (pitchLatency);
        }

        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
        {
            juce::ScopedNoDenormals n;
            const int activeChannels = juce::jmin (channels, b.getNumChannels());
            if (activeChannels <= 0) return;
            const int voices = juce::jlimit (2, 8, (int) std::lround (value ("voices")));
            if (voices != lastVoices)
            {
                for (auto& s : shifters) s.reset();
                for (auto& voice : voiceDelay) for (auto& d : voice) d.reset();
                lastVoices = voices;
            }

            if (temp.getNumSamples() < b.getNumSamples())
            {
                temp.setSize (channels, b.getNumSamples(), false, false, true);
                wet.setSize (channels, b.getNumSamples(), false, false, true);
            }
            wet.clear();

            const float detuneSemitones = value ("detune") * 0.01f;
            const float spread = value ("spread") * 0.01f;
            const float staggerMs = value ("delay");

            for (int v = 0; v < voices; ++v)
            {
                temp.makeCopyOf (b, true);
                const float position = voices <= 1 ? 0.0f
                    : ((float) v / (float) (voices - 1)) * 2.0f - 1.0f;
                const float semitones = position * detuneSemitones;
                const bool centreVoice = std::abs (semitones) < 0.01f;
                if (! centreVoice)
                    shifters[(size_t) v].process (temp, semitones, 1.0f);

                const float voiceDelaySamples = ((float) v / (float) juce::jmax (1, voices - 1))
                                              * staggerMs * 0.001f * sampleRate
                                              + (centreVoice ? (float) pitchLatency : 0.0f);
                const float pan = position * spread;
                const float gainL = std::sqrt (juce::jmax (0.0f, 1.0f - pan));
                const float gainR = std::sqrt (juce::jmax (0.0f, 1.0f + pan));

                for (int ch = 0; ch < activeChannels; ++ch)
                {
                    const float panGain = activeChannels == 1 ? 1.0f : (ch == 0 ? gainL : gainR);
                    auto* dst = wet.getWritePointer (ch);
                    const auto* src = temp.getReadPointer (ch);
                    auto& line = voiceDelay[(size_t) v][(size_t) ch];
                    for (int i = 0; i < b.getNumSamples(); ++i)
                    {
                        float sample = src[i];
                        if (voiceDelaySamples >= 1.0f)
                        {
                            const float delayed = line.read (voiceDelaySamples);
                            line.push (sample);
                            sample = delayed;
                        }
                        else
                        {
                            line.push (sample);
                        }
                        dst[i] += sample * panGain / (float) voices;
                    }
                }
            }

            const float mix = value ("mix") * 0.01f;
            for (int ch = 0; ch < activeChannels; ++ch)
            {
                auto* out = b.getWritePointer (ch);
                const auto* w = wet.getReadPointer (ch);
                for (int i = 0; i < b.getNumSamples(); ++i)
                    out[i] += (w[i] - out[i]) * mix;
            }
        }

    private:
        float sampleRate = 48000.0f;
        int channels = 2;
        int pitchLatency = 0;
        int lastVoices = -1;
        std::array<VoicePitchShifter, 8> shifters;
        std::array<std::array<SimpleDelayLine, 2>, 8> voiceDelay;
        juce::AudioBuffer<float> temp, wet;
    };

    class DelayProcessor final : public BuiltinProcessorBase
    {
    public:
        DelayProcessor() : BuiltinProcessorBase ("Delay", makeLayout()) {}
        double getTailLengthSeconds() const override { return 12.0; }

        static Layout makeLayout()
        {
            Layout l;
            l.add (floatParam ("time", "Time", 20.0f, 1500.0f, 1.0f, 280.0f, "ms"));
            l.add (floatParam ("feedback", "Feedback", 0.0f, 90.0f, 1.0f, 32.0f, "%"));
            l.add (choiceParam ("mode", "Mode", { "Stereo", "Ping-Pong" }, 0));
            l.add (floatParam ("lowcut", "Feedback low cut", 20.0f, 1200.0f, 10.0f, 120.0f, "Hz"));
            l.add (floatParam ("highcut", "Feedback high cut", 1000.0f, 20000.0f, 100.0f, 9000.0f, "Hz"));
            l.add (floatParam ("mix", "Mix", 0.0f, 100.0f, 1.0f, 28.0f, "%"));
            return l;
        }

        void prepareToPlay (double sr, int) override
        {
            sampleRate = (float) sr;
            for (auto& d : delayLines) d.prepare ((int) std::ceil (sampleRate * 1.60f));
            hpY.fill (0.0f); hpX.fill (0.0f); lpY.fill (0.0f);
        }

        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
        {
            juce::ScopedNoDenormals n;
            const int channels = juce::jmin (2, b.getNumChannels());
            if (channels <= 0) return;
            const float delaySamples = value ("time") * 0.001f * sampleRate;
            const float feedback = value ("feedback") * 0.01f;
            const bool pingPong = (int) std::lround (value ("mode")) == 1 && channels > 1;
            const float mix = value ("mix") * 0.01f;
            const float dt = 1.0f / sampleRate;
            const float hpRC = 1.0f / (juce::MathConstants<float>::twoPi * value ("lowcut"));
            const float hpA = hpRC / (hpRC + dt);
            const float lpA = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * value ("highcut") / sampleRate);

            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                std::array<float, 2> dry { 0.0f, 0.0f }, delayed { 0.0f, 0.0f };
                for (int ch = 0; ch < channels; ++ch)
                {
                    dry[(size_t) ch] = b.getSample (ch, i);
                    delayed[(size_t) ch] = delayLines[(size_t) ch].read (delaySamples);
                }

                for (int ch = 0; ch < channels; ++ch)
                {
                    const int source = pingPong ? 1 - ch : ch;
                    const float fbIn = delayed[(size_t) source];
                    float hy = hpA * (hpY[(size_t) ch] + fbIn - hpX[(size_t) ch]);
                    hpX[(size_t) ch] = fbIn;
                    hpY[(size_t) ch] = hy;
                    lpY[(size_t) ch] += lpA * (hy - lpY[(size_t) ch]);
                    delayLines[(size_t) ch].push (dry[(size_t) ch] + lpY[(size_t) ch] * feedback);
                    b.setSample (ch, i, dry[(size_t) ch] + (delayed[(size_t) ch] - dry[(size_t) ch]) * mix);
                }
            }
        }

    private:
        float sampleRate = 48000.0f;
        std::array<SimpleDelayLine, 2> delayLines;
        std::array<float, 2> hpY {}, hpX {}, lpY {};
    };

    class ReverbProcessor final : public BuiltinProcessorBase
    {
    public:
        ReverbProcessor() : BuiltinProcessorBase ("Reverb", makeLayout()) {}
        double getTailLengthSeconds() const override { return 10.0; }

        static Layout makeLayout()
        {
            Layout l;
            l.add (floatParam ("size", "Room size", 0.0f, 100.0f, 1.0f, 55.0f, "%"));
            l.add (floatParam ("decay", "Decay", 0.20f, 8.0f, 0.05f, 1.8f, "s"));
            l.add (floatParam ("predelay", "Pre-delay", 0.0f, 120.0f, 1.0f, 18.0f, "ms"));
            l.add (floatParam ("damping", "Damping", 0.0f, 100.0f, 1.0f, 45.0f, "%"));
            l.add (floatParam ("width", "Stereo width", 0.0f, 100.0f, 1.0f, 85.0f, "%"));
            l.add (floatParam ("mix", "Mix", 0.0f, 100.0f, 1.0f, 22.0f, "%"));
            return l;
        }

        void prepareToPlay (double sr, int) override
        {
            sampleRate = (float) sr;
            const int combMax = (int) std::ceil (sampleRate * 0.16f);
            const int allpassMax = (int) std::ceil (sampleRate * 0.07f);
            for (auto& channel : combs) for (auto& d : channel) d.prepare (combMax);
            for (auto& channel : allpasses) for (auto& d : channel) d.prepare (allpassMax);
            for (auto& d : preDelay) d.prepare ((int) std::ceil (sampleRate * 0.13f));
            for (auto& channel : dampState) channel.fill (0.0f);
        }

        void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
        {
            juce::ScopedNoDenormals n;
            const int channels = juce::jmin (2, b.getNumChannels());
            if (channels <= 0) return;
            const float sizeScale = 0.72f + value ("size") * 0.0088f;
            const float decay = value ("decay");
            const float preDelaySamples = value ("predelay") * 0.001f * sampleRate;
            const float damping = value ("damping") * 0.01f;
            const float dampingCutoff = 18000.0f - damping * 15500.0f;
            const float dampA = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * dampingCutoff / sampleRate);
            const float width = value ("width") * 0.01f;
            const float mix = value ("mix") * 0.01f;
            const float srScale = sampleRate / 44100.0f;

            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                std::array<float, 2> dry { 0.0f, 0.0f }, wetSample { 0.0f, 0.0f };
                for (int ch = 0; ch < channels; ++ch)
                {
                    dry[(size_t) ch] = b.getSample (ch, i);
                    float input = dry[(size_t) ch];
                    if (preDelaySamples >= 1.0f)
                    {
                        input = preDelay[(size_t) ch].read (preDelaySamples);
                        preDelay[(size_t) ch].push (dry[(size_t) ch]);
                    }
                    else
                    {
                        preDelay[(size_t) ch].push (dry[(size_t) ch]);
                    }

                    float sum = 0.0f;
                    for (int c = 0; c < numCombs; ++c)
                    {
                        const float delaySamples = ((float) combBase[(size_t) c] * srScale * sizeScale)
                            + (ch == 1 ? (float) combRightOffset[(size_t) c] * srScale : 0.0f);
                        const float y = combs[(size_t) ch][(size_t) c].read (delaySamples);
                        auto& damp = dampState[(size_t) ch][(size_t) c];
                        damp += dampA * (y - damp);
                        const float delaySeconds = delaySamples / sampleRate;
                        const float feedback = std::pow (0.001f, delaySeconds / juce::jmax (0.05f, decay));
                        combs[(size_t) ch][(size_t) c].push (input + damp * feedback);
                        sum += y;
                    }

                    float x = sum * 0.25f;
                    for (int a = 0; a < numAllpasses; ++a)
                    {
                        const float delaySamples = ((float) allpassBase[(size_t) a] * srScale * sizeScale)
                            + (ch == 1 ? 17.0f * (float) (a + 1) * srScale : 0.0f);
                        const float d = allpasses[(size_t) ch][(size_t) a].read (delaySamples);
                        constexpr float g = 0.55f;
                        const float y = d - g * x;
                        allpasses[(size_t) ch][(size_t) a].push (x + g * d);
                        x = y;
                    }
                    wetSample[(size_t) ch] = x * 0.72f;
                }

                if (channels > 1)
                {
                    const float mid = 0.5f * (wetSample[0] + wetSample[1]);
                    const float side = 0.5f * (wetSample[0] - wetSample[1]) * width;
                    wetSample[0] = mid + side;
                    wetSample[1] = mid - side;
                }

                for (int ch = 0; ch < channels; ++ch)
                    b.setSample (ch, i, dry[(size_t) ch] + (wetSample[(size_t) ch] - dry[(size_t) ch]) * mix);
            }
        }

    private:
        static constexpr int numCombs = 4;
        static constexpr int numAllpasses = 2;
        static constexpr std::array<int, numCombs> combBase { 1116, 1188, 1277, 1356 };
        static constexpr std::array<int, numCombs> combRightOffset { 23, 31, 43, 53 };
        static constexpr std::array<int, numAllpasses> allpassBase { 225, 556 };

        float sampleRate = 48000.0f;
        std::array<std::array<SimpleDelayLine, numCombs>, 2> combs;
        std::array<std::array<SimpleDelayLine, numAllpasses>, 2> allpasses;
        std::array<SimpleDelayLine, 2> preDelay;
        std::array<std::array<float, numCombs>, 2> dampState {};
    };

    struct Definition { const char* id; const char* name; const char* category; };
    constexpr Definition defs[] = {
        { BuiltinEffects::autoTuneId,   "AutoTune", "Pitch / Voice" },
        { BuiltinEffects::pitchId,      "Pitch Shift", "Pitch / Voice" },
        { BuiltinEffects::deepVoiceId,  "Deep Voice", "Pitch / Voice" },
        { BuiltinEffects::unisonId,     "Unison", "Pitch / Voice" },
        { BuiltinEffects::robotId,      "Robot", "Character" },
        { BuiltinEffects::radioId,      "Radio / Walkie-Talkie", "Character" },
        { BuiltinEffects::bitcrusherId, "Bitcrusher", "Character" },
        { BuiltinEffects::wahId,        "Wah / Auto Wah", "Modulation" },
        { BuiltinEffects::chorusId,     "Chorus", "Modulation" },
        { BuiltinEffects::delayId,      "Delay", "Space" },
        { BuiltinEffects::reverbId,     "Reverb", "Space" },
    };
}

namespace BuiltinEffects
{
    bool isEffectId (const juce::String& id)
    {
        for (auto& d : defs) if (id == d.id) return true;
        return false;
    }

    juce::String displayName (const juce::String& id)
    {
        for (auto& d : defs) if (id == d.id) return d.name;
        return {};
    }

    juce::Array<juce::PluginDescription> descriptions()
    {
        juce::Array<juce::PluginDescription> result;
        for (auto& def : defs)
        {
            juce::PluginDescription d;
            d.name = def.name;
            d.descriptiveName = def.name;
            d.manufacturerName = "MicVST Built-ins";
            d.category = def.category;
            d.pluginFormatName = "Built-in";
            d.fileOrIdentifier = def.id;
            d.numInputChannels = 2;
            d.numOutputChannels = 2;
            d.isInstrument = false;
            result.add (d);
        }
        return result;
    }

    std::unique_ptr<juce::AudioProcessor> create (const juce::String& id)
    {
        if (id == autoTuneId)   return std::make_unique<AutoTuneProcessor>();
        if (id == robotId)      return std::make_unique<RobotProcessor>();
        if (id == radioId)      return std::make_unique<RadioProcessor>();
        if (id == bitcrusherId) return std::make_unique<BitcrusherProcessor>();
        if (id == pitchId)      return std::make_unique<PitchProcessor>();
        if (id == deepVoiceId)  return std::make_unique<DeepVoiceProcessor>();
        if (id == wahId)        return std::make_unique<WahProcessor>();
        if (id == unisonId)     return std::make_unique<UnisonProcessor>();
        if (id == chorusId)     return std::make_unique<ChorusProcessor>();
        if (id == delayId)      return std::make_unique<DelayProcessor>();
        if (id == reverbId)     return std::make_unique<ReverbProcessor>();
        return {};
    }
}
