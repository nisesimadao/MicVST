#include "audio/AudioPadEngine.h"
#include <algorithm>
#include <cmath>
#include <limits>

AudioPadEngine::AudioPadEngine()
{
    formats.registerBasicFormats();
}

void AudioPadEngine::prepare (double sampleRate, int)
{
    engineSampleRate.store (juce::jlimit (8000.0, 384000.0, sampleRate));
    stopAll (true);
}

bool AudioPadEngine::supportsFile (const juce::File& file)
{
    const auto ext = file.getFileExtension().toLowerCase();
    return ext == ".wav" || ext == ".mp3" || ext == ".flac" || ext == ".ogg"
        || ext == ".aif" || ext == ".aiff";
}

juce::String AudioPadEngine::loadFile (int index, const juce::File& file)
{
    if (! validIndex (index)) return "Invalid pad index";
    if (! file.existsAsFile()) return "Audio file does not exist";
    if (! supportsFile (file)) return "Unsupported audio format";

    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr) return "Could not decode this audio file";
    if (reader->lengthInSamples <= 0) return "Audio file is empty";

    const int channels = juce::jlimit (1, 2, (int) reader->numChannels);
    const auto sampleCount64 = reader->lengthInSamples;
    const double bytesNeeded = (double) channels * (double) sampleCount64 * sizeof (float);
    if (sampleCount64 > (juce::int64) std::numeric_limits<int>::max() || bytesNeeded > 512.0 * 1024.0 * 1024.0)
        return "Audio file is too large for the soundboard (512 MB decoded limit)";

    auto clip = std::make_shared<Clip>();
    clip->sampleRate = reader->sampleRate > 0.0 ? reader->sampleRate : 48000.0;
    clip->audio.setSize (channels, (int) sampleCount64, false, true, false);
    if (! reader->read (&clip->audio, 0, (int) sampleCount64, 0, true, channels > 1))
        return "Could not read audio samples";

    std::atomic_store_explicit (&pads[(size_t) index].clip,
                                std::shared_ptr<const Clip> (std::move (clip)),
                                std::memory_order_release);
    pads[(size_t) index].command.store (3, std::memory_order_release);

    {
        const juce::ScopedLock sl (stateLock);
        states[(size_t) index].filePath = file.getFullPathName();
        if (states[(size_t) index].name.trim().isEmpty())
            states[(size_t) index].name = file.getFileNameWithoutExtension();
    }
    return {};
}

void AudioPadEngine::clearPad (int index)
{
    if (! validIndex (index)) return;
    pads[(size_t) index].command.store (3, std::memory_order_release);
    std::atomic_store_explicit (&pads[(size_t) index].clip,
                                std::shared_ptr<const Clip>(),
                                std::memory_order_release);
    {
        const juce::ScopedLock sl (stateLock);
        states[(size_t) index] = {};
    }
    applyRuntimeSettings (index, {});
}

void AudioPadEngine::triggerPad (int index)
{
    if (validIndex (index)) pads[(size_t) index].command.store (1, std::memory_order_release);
}

void AudioPadEngine::stopPad (int index)
{
    if (validIndex (index)) pads[(size_t) index].command.store (2, std::memory_order_release);
}

void AudioPadEngine::stopAll (bool immediate)
{
    for (auto& p : pads)
        p.command.store (immediate ? 3 : 2, std::memory_order_release);
}

void AudioPadEngine::applyRuntimeSettings (int index, const AudioPadState& s)
{
    auto& p = pads[(size_t) index];
    p.volume.store (juce::jlimit (0.0f, 1.5f, s.volume), std::memory_order_relaxed);
    p.loop.store (s.loop ? 1 : 0, std::memory_order_relaxed);
    p.route.store ((int) s.route, std::memory_order_relaxed);
    p.retrigger.store ((int) s.retrigger, std::memory_order_relaxed);
    p.fadeInMs.store (juce::jlimit (0.0f, 5000.0f, s.fadeInMs), std::memory_order_relaxed);
    p.fadeOutMs.store (juce::jlimit (0.0f, 5000.0f, s.fadeOutMs), std::memory_order_relaxed);
}

void AudioPadEngine::setPadState (int index, const AudioPadState& newState, bool reloadFile)
{
    if (! validIndex (index)) return;

    juce::String oldPath;
    {
        const juce::ScopedLock sl (stateLock);
        oldPath = states[(size_t) index].filePath;
        states[(size_t) index] = newState;
    }
    applyRuntimeSettings (index, newState);

    if (reloadFile && newState.filePath.isNotEmpty() && newState.filePath != oldPath)
    {
        const auto err = loadFile (index, juce::File (newState.filePath));
        if (err.isNotEmpty()) juce::Logger::writeToLog ("Audio Pad " + juce::String (index + 1) + ": " + err);
    }
    else if (newState.filePath.isEmpty() && oldPath.isNotEmpty())
    {
        std::atomic_store_explicit (&pads[(size_t) index].clip,
                                    std::shared_ptr<const Clip>(),
                                    std::memory_order_release);
        pads[(size_t) index].command.store (3, std::memory_order_release);
    }
}

AudioPadState AudioPadEngine::getPadState (int index) const
{
    if (! validIndex (index)) return {};
    const juce::ScopedLock sl (stateLock);
    return states[(size_t) index];
}

juce::Array<AudioPadState> AudioPadEngine::captureStates() const
{
    juce::Array<AudioPadState> result;
    const juce::ScopedLock sl (stateLock);
    for (const auto& s : states) result.add (s);
    return result;
}

void AudioPadEngine::restoreStates (const juce::Array<AudioPadState>& restored)
{
    stopAll (true);
    for (int i = 0; i < padCount; ++i)
    {
        const auto s = juce::isPositiveAndBelow (i, restored.size()) ? restored.getReference (i) : AudioPadState{};
        {
            const juce::ScopedLock sl (stateLock);
            states[(size_t) i] = s;
        }
        applyRuntimeSettings (i, s);

        if (s.filePath.isNotEmpty())
        {
            const auto err = loadFile (i, juce::File (s.filePath));
            if (err.isNotEmpty())
            {
                const juce::ScopedLock sl (stateLock);
                states[(size_t) i] = s;
                juce::Logger::writeToLog ("Audio Pad " + juce::String (i + 1) + " restore: " + err);
            }
        }
    }
}

bool AudioPadEngine::isPlaying (int index) const
{
    return validIndex (index) && pads[(size_t) index].playingForUi.load (std::memory_order_relaxed) != 0;
}

float AudioPadEngine::progress (int index) const
{
    return validIndex (index) ? pads[(size_t) index].progressForUi.load (std::memory_order_relaxed) : 0.0f;
}

void AudioPadEngine::startPlayback (PadRuntime& p)
{
    p.position = 0.0;
    p.playing = true;
    p.stopping = false;
    p.stopFadeRemaining = 0;
    p.stopFadeTotal = 0;
    p.playingForUi.store (1, std::memory_order_relaxed);
    p.progressForUi.store (0.0f, std::memory_order_relaxed);
}

void AudioPadEngine::beginStop (PadRuntime& p, bool immediate)
{
    if (immediate || ! p.playing)
    {
        p.playing = false;
        p.stopping = false;
        p.position = 0.0;
        p.playingForUi.store (0, std::memory_order_relaxed);
        p.progressForUi.store (0.0f, std::memory_order_relaxed);
        return;
    }

    const auto fadeSamples = (int) std::lround (p.fadeOutMs.load (std::memory_order_relaxed)
                                                  * 0.001 * engineSampleRate.load());
    if (fadeSamples <= 0)
    {
        beginStop (p, true);
        return;
    }
    p.stopping = true;
    p.stopFadeTotal = p.stopFadeRemaining = fadeSamples;
}

void AudioPadEngine::renderPad (PadRuntime& p, juce::AudioBuffer<float>& target, int numSamples)
{
    auto clip = std::atomic_load_explicit (&p.clip, std::memory_order_acquire);
    const int cmd = p.command.exchange (0, std::memory_order_acq_rel);

    if (cmd == 3) beginStop (p, true);
    else if (cmd == 2) beginStop (p, false);
    else if (cmd == 1 && clip != nullptr)
    {
        const auto mode = (AudioPadRetrigger) p.retrigger.load (std::memory_order_relaxed);
        if (mode == AudioPadRetrigger::restart) startPlayback (p);
        else if (mode == AudioPadRetrigger::stop)
        {
            if (p.playing) beginStop (p, false); else startPlayback (p);
        }
        else if (! p.playing) startPlayback (p);
    }

    if (! p.playing || clip == nullptr || clip->audio.getNumSamples() <= 0) return;

    const double dstRate = engineSampleRate.load (std::memory_order_relaxed);
    const double ratio = juce::jlimit (0.02, 50.0, clip->sampleRate / dstRate);
    const int sourceLength = clip->audio.getNumSamples();
    const int sourceChannels = clip->audio.getNumChannels();
    const bool looping = p.loop.load (std::memory_order_relaxed) != 0;
    const float baseGain = p.volume.load (std::memory_order_relaxed) * masterVolume.load (std::memory_order_relaxed);
    const int fadeInSamples = (int) std::lround (p.fadeInMs.load (std::memory_order_relaxed) * 0.001 * dstRate);
    const int fadeOutSamples = (int) std::lround (p.fadeOutMs.load (std::memory_order_relaxed) * 0.001 * dstRate);

    for (int n = 0; n < numSamples; ++n)
    {
        if (! p.playing) break;
        if (p.position >= (double) sourceLength)
        {
            if (looping)
                p.position = std::fmod (p.position, (double) sourceLength);
            else
            {
                beginStop (p, true);
                break;
            }
        }

        const int i0 = juce::jlimit (0, sourceLength - 1, (int) p.position);
        const int i1 = juce::jmin (sourceLength - 1, i0 + 1);
        const float frac = (float) (p.position - (double) i0);
        auto interp = [&] (int ch)
        {
            ch = juce::jlimit (0, sourceChannels - 1, ch);
            const float a = clip->audio.getSample (ch, i0);
            const float b = clip->audio.getSample (ch, i1);
            return a + (b - a) * frac;
        };

        float gain = baseGain;
        const double playedOutputSamples = p.position / ratio;
        if (fadeInSamples > 0 && playedOutputSamples < fadeInSamples)
            gain *= (float) juce::jlimit (0.0, 1.0, playedOutputSamples / (double) fadeInSamples);

        if (! looping && fadeOutSamples > 0)
        {
            const double remainingOutputSamples = ((double) sourceLength - p.position) / ratio;
            if (remainingOutputSamples < fadeOutSamples)
                gain *= (float) juce::jlimit (0.0, 1.0, remainingOutputSamples / (double) fadeOutSamples);
        }

        if (p.stopping && p.stopFadeTotal > 0)
        {
            gain *= (float) p.stopFadeRemaining / (float) p.stopFadeTotal;
            if (--p.stopFadeRemaining <= 0)
            {
                beginStop (p, true);
                break;
            }
        }

        const float l = interp (0) * gain;
        const float r = interp (sourceChannels > 1 ? 1 : 0) * gain;
        if (target.getNumChannels() == 1)
            target.addSample (0, n, 0.5f * (l + r));
        else
        {
            target.addSample (0, n, l);
            target.addSample (1, n, r);
        }

        p.position += ratio;
    }

    p.playingForUi.store (p.playing ? 1 : 0, std::memory_order_relaxed);
    p.progressForUi.store (p.playing && sourceLength > 0
        ? (float) juce::jlimit (0.0, 1.0, p.position / (double) sourceLength) : 0.0f,
        std::memory_order_relaxed);
}

void AudioPadEngine::render (juce::AudioBuffer<float>& preFx,
                             juce::AudioBuffer<float>& postFx,
                             juce::AudioBuffer<float>& output2Only,
                             int numSamples)
{
    const int n = std::min ({ numSamples, preFx.getNumSamples(), postFx.getNumSamples(), output2Only.getNumSamples() });
    preFx.clear (0, n);
    postFx.clear (0, n);
    output2Only.clear (0, n);
    if (n <= 0) return;

    for (int i = 0; i < padCount; ++i)
    {
        auto& p = pads[(size_t) i];
        const auto route = (AudioPadRoute) p.route.load (std::memory_order_relaxed);
        if (route == AudioPadRoute::preFx) renderPad (p, preFx, n);
        else if (route == AudioPadRoute::output2Only) renderPad (p, output2Only, n);
        else renderPad (p, postFx, n);
    }
}
