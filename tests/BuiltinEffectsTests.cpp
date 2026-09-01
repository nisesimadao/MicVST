#include <juce_audio_utils/juce_audio_utils.h>
#include "audio/BuiltinEffects.h"

#include <array>
#include <cmath>
#include <limits>

namespace
{
    bool setParameter (juce::AudioProcessor& processor, const juce::String& name, float plainValue)
    {
        for (auto* p : processor.getParameters())
        {
            if (p->getName (128) != name) continue;
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            {
                ranged->setValueNotifyingHost (ranged->convertTo0to1 (plainValue));
                return true;
            }
        }
        return false;
    }

    float getParameter (juce::AudioProcessor& processor, const juce::String& name)
    {
        for (auto* p : processor.getParameters())
        {
            if (p->getName (128) != name) continue;
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
                return ranged->convertFrom0to1 (ranged->getValue());
        }
        return std::numeric_limits<float>::quiet_NaN();
    }

    bool bufferIsFinite (const juce::AudioBuffer<float>& b)
    {
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            for (int i = 0; i < b.getNumSamples(); ++i)
                if (! std::isfinite (b.getSample (ch, i))) return false;
        return true;
    }

    double energy (const juce::AudioBuffer<float>& b)
    {
        double e = 0.0;
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                const double x = b.getSample (ch, i);
                e += x * x;
            }
        return e;
    }

    void prepare (juce::AudioProcessor& p, int block = 256)
    {
        p.setPlayConfigDetails (2, 2, 48000.0, block);
        p.prepareToPlay (48000.0, block);
    }

    void fillVoiceLikeSignal (juce::AudioBuffer<float>& b, double phaseOffset = 0.0)
    {
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const double t = (double) i / 48000.0;
            const float x = 0.30f * std::sin ((float) (juce::MathConstants<double>::twoPi * 180.0 * t + phaseOffset))
                          + 0.12f * std::sin ((float) (juce::MathConstants<double>::twoPi * 360.0 * t));
            for (int ch = 0; ch < b.getNumChannels(); ++ch)
                b.setSample (ch, i, x);
        }
    }
}

struct BuiltinEffectsTest : juce::UnitTest
{
    BuiltinEffectsTest() : juce::UnitTest ("Built-in effects") {}

    void runTest() override
    {
        const std::array<const char*, 11> ids {
            BuiltinEffects::autoTuneId,
            BuiltinEffects::pitchId,
            BuiltinEffects::deepVoiceId,
            BuiltinEffects::unisonId,
            BuiltinEffects::robotId,
            BuiltinEffects::radioId,
            BuiltinEffects::bitcrusherId,
            BuiltinEffects::wahId,
            BuiltinEffects::chorusId,
            BuiltinEffects::delayId,
            BuiltinEffects::reverbId,
        };

        beginTest ("all built-ins are registered and constructible");
        {
            const auto descriptions = BuiltinEffects::descriptions();
            expectEquals (descriptions.size(), (int) ids.size());
            for (auto* id : ids)
            {
                expect (BuiltinEffects::isEffectId (id), juce::String ("not registered: ") + id);
                expect (BuiltinEffects::displayName (id).isNotEmpty(), juce::String ("no display name: ") + id);
                expect (BuiltinEffects::create (id) != nullptr, juce::String ("could not create: ") + id);
            }
        }

        beginTest ("every built-in stays finite on voice-like audio");
        {
            for (auto* id : ids)
            {
                auto fx = BuiltinEffects::create (id);
                prepare (*fx);
                juce::AudioBuffer<float> b (2, 256);
                fillVoiceLikeSignal (b);
                juce::MidiBuffer midi;
                for (int block = 0; block < 12; ++block)
                {
                    fx->processBlock (b, midi);
                    expect (bufferIsFinite (b), juce::String ("non-finite output: ") + id);
                    expect (energy (b) < 1.0e7, juce::String ("unstable output: ") + id);
                    fillVoiceLikeSignal (b, block * 0.17);
                }
            }
        }

        beginTest ("new effects expose expected controls");
        {
            auto wah = BuiltinEffects::create (BuiltinEffects::wahId);
            expect (setParameter (*wah, "Mode", 1.0f));
            expect (setParameter (*wah, "LFO rate", 3.0f));
            expect (setParameter (*wah, "Mix", 100.0f));

            auto unison = BuiltinEffects::create (BuiltinEffects::unisonId);
            expect (setParameter (*unison, "Voices", 8.0f));
            expect (setParameter (*unison, "Detune", 24.0f));
            expect (setParameter (*unison, "Stereo spread", 100.0f));

            auto chorus = BuiltinEffects::create (BuiltinEffects::chorusId);
            expect (setParameter (*chorus, "Depth", 12.0f));
            expect (setParameter (*chorus, "Feedback", 40.0f));

            auto delay = BuiltinEffects::create (BuiltinEffects::delayId);
            expect (setParameter (*delay, "Time", 90.0f));
            expect (setParameter (*delay, "Mode", 1.0f));

            auto reverb = BuiltinEffects::create (BuiltinEffects::reverbId);
            expect (setParameter (*reverb, "Decay", 3.0f));
            expect (setParameter (*reverb, "Pre-delay", 25.0f));
        }

        beginTest ("built-in state round-trip preserves parameters");
        {
            auto a = BuiltinEffects::create (BuiltinEffects::wahId);
            expect (setParameter (*a, "Mode", 2.0f));
            expect (setParameter (*a, "Manual position", 73.0f));
            expect (setParameter (*a, "Resonance", 4.2f));

            juce::MemoryBlock state;
            a->getStateInformation (state);
            auto b = BuiltinEffects::create (BuiltinEffects::wahId);
            b->setStateInformation (state.getData(), (int) state.getSize());

            expectWithinAbsoluteError (getParameter (*b, "Mode"), 2.0f, 0.01f);
            expectWithinAbsoluteError (getParameter (*b, "Manual position"), 73.0f, 0.01f);
            expectWithinAbsoluteError (getParameter (*b, "Resonance"), 4.2f, 0.01f);
        }

        beginTest ("delay emits an echo after the dry impulse");
        {
            auto fx = BuiltinEffects::create (BuiltinEffects::delayId);
            prepare (*fx, 256);
            expect (setParameter (*fx, "Time", 20.0f));
            expect (setParameter (*fx, "Feedback", 0.0f));
            expect (setParameter (*fx, "Mix", 100.0f));

            juce::MidiBuffer midi;
            juce::AudioBuffer<float> b (2, 256);
            b.clear();
            b.setSample (0, 0, 1.0f);
            b.setSample (1, 0, 1.0f);
            fx->processBlock (b, midi);
            double later = 0.0;
            for (int block = 0; block < 5; ++block)
            {
                b.clear();
                fx->processBlock (b, midi);
                later += energy (b);
            }
            expect (later > 0.1, "20 ms delay should produce a later impulse");
        }

        beginTest ("reverb produces a decaying tail");
        {
            auto fx = BuiltinEffects::create (BuiltinEffects::reverbId);
            prepare (*fx, 256);
            expect (setParameter (*fx, "Pre-delay", 0.0f));
            expect (setParameter (*fx, "Mix", 100.0f));
            expect (setParameter (*fx, "Decay", 2.0f));

            juce::MidiBuffer midi;
            juce::AudioBuffer<float> b (2, 256);
            b.clear();
            b.setSample (0, 0, 1.0f);
            b.setSample (1, 0, 1.0f);
            fx->processBlock (b, midi);
            double tail = 0.0;
            for (int block = 0; block < 16; ++block)
            {
                b.clear();
                fx->processBlock (b, midi);
                tail += energy (b);
            }
            expect (tail > 1.0e-5, "reverb should have energy after the impulse block");
            expect (bufferIsFinite (b));
        }

        beginTest ("unison can render eight voices without instability");
        {
            auto fx = BuiltinEffects::create (BuiltinEffects::unisonId);
            prepare (*fx, 256);
            expect (setParameter (*fx, "Voices", 8.0f));
            expect (setParameter (*fx, "Detune", 35.0f));
            expect (setParameter (*fx, "Voice stagger", 20.0f));
            expect (setParameter (*fx, "Mix", 100.0f));

            juce::MidiBuffer midi;
            juce::AudioBuffer<float> b (2, 256);
            for (int block = 0; block < 16; ++block)
            {
                fillVoiceLikeSignal (b, block * 0.09);
                fx->processBlock (b, midi);
                expect (bufferIsFinite (b));
                expect (energy (b) < 1.0e7);
            }
        }
    }
};

static BuiltinEffectsTest builtinEffectsTest;
