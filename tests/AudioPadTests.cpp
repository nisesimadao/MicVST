#include <juce_core/juce_core.h>
#include "audio/AudioPadEngine.h"
#include "state/Persistence.h"
#include <cmath>

namespace
{
    void writeLE16 (juce::OutputStream& out, juce::uint16 v)
    {
        const unsigned char b[] = { (unsigned char) (v & 0xff), (unsigned char) ((v >> 8) & 0xff) };
        out.write (b, 2);
    }

    void writeLE32 (juce::OutputStream& out, juce::uint32 v)
    {
        const unsigned char b[] = {
            (unsigned char) (v & 0xff), (unsigned char) ((v >> 8) & 0xff),
            (unsigned char) ((v >> 16) & 0xff), (unsigned char) ((v >> 24) & 0xff)
        };
        out.write (b, 4);
    }

    juce::File makeTestWav (int samples = 4096)
    {
        auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                        .getNonexistentChildFile ("micvst-pad-test", ".wav", false);
        std::unique_ptr<juce::FileOutputStream> out (file.createOutputStream());
        if (out == nullptr) return {};

        constexpr int sampleRate = 48000;
        constexpr int channels = 1;
        constexpr int bits = 16;
        const juce::uint32 dataBytes = (juce::uint32) (samples * channels * bits / 8);
        out->write ("RIFF", 4); writeLE32 (*out, 36 + dataBytes);
        out->write ("WAVE", 4);
        out->write ("fmt ", 4); writeLE32 (*out, 16);
        writeLE16 (*out, 1); writeLE16 (*out, channels);
        writeLE32 (*out, sampleRate);
        writeLE32 (*out, sampleRate * channels * bits / 8);
        writeLE16 (*out, channels * bits / 8); writeLE16 (*out, bits);
        out->write ("data", 4); writeLE32 (*out, dataBytes);
        for (int i = 0; i < samples; ++i)
        {
            const float x = 0.4f * std::sin (juce::MathConstants<float>::twoPi * 440.0f * (float) i / sampleRate);
            const auto s = (juce::int16) juce::jlimit (-32767, 32767, (int) std::lround (x * 32767.0f));
            writeLE16 (*out, (juce::uint16) s);
        }
        out->flush();
        return file;
    }

    float peak (const juce::AudioBuffer<float>& b)
    {
        float p = 0.0f;
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            p = juce::jmax (p, b.getMagnitude (ch, 0, b.getNumSamples()));
        return p;
    }
}

struct AudioPadEngineTest : juce::UnitTest
{
    AudioPadEngineTest() : juce::UnitTest ("Audio Pad Engine") {}

    void runTest() override
    {
        const auto wav = makeTestWav();
        expect (wav.existsAsFile());

        AudioPadEngine pads;
        pads.prepare (48000.0, 512);
        expect (pads.loadFile (0, wav).isEmpty());

        juce::AudioBuffer<float> pre (2, 512), post (2, 512), mon (2, 512);

        beginTest ("Post FX route outputs only on post bus");
        {
            auto s = pads.getPadState (0);
            s.route = AudioPadRoute::postFx;
            s.retrigger = AudioPadRetrigger::restart;
            pads.setPadState (0, s, false);
            pads.triggerPad (0);
            pads.render (pre, post, mon, 512);
            expect (peak (post) > 0.1f);
            expectWithinAbsoluteError (peak (pre), 0.0f, 1.0e-6f);
            expectWithinAbsoluteError (peak (mon), 0.0f, 1.0e-6f);
        }

        beginTest ("Pre FX route outputs only on pre bus");
        {
            auto s = pads.getPadState (0);
            s.route = AudioPadRoute::preFx;
            pads.setPadState (0, s, false);
            pads.triggerPad (0);
            pads.render (pre, post, mon, 512);
            expect (peak (pre) > 0.1f);
            expectWithinAbsoluteError (peak (post), 0.0f, 1.0e-6f);
            expectWithinAbsoluteError (peak (mon), 0.0f, 1.0e-6f);
        }

        beginTest ("Output2-only route never enters virtual mic buses");
        {
            auto s = pads.getPadState (0);
            s.route = AudioPadRoute::output2Only;
            pads.setPadState (0, s, false);
            pads.triggerPad (0);
            pads.render (pre, post, mon, 512);
            expect (peak (mon) > 0.1f);
            expectWithinAbsoluteError (peak (pre), 0.0f, 1.0e-6f);
            expectWithinAbsoluteError (peak (post), 0.0f, 1.0e-6f);
        }

        beginTest ("Loop keeps a short clip playing");
        {
            const auto shortWav = makeTestWav (96);
            expect (pads.loadFile (1, shortWav).isEmpty());
            auto s = pads.getPadState (1);
            s.loop = true;
            s.route = AudioPadRoute::postFx;
            pads.setPadState (1, s, false);
            pads.triggerPad (1);
            pads.render (pre, post, mon, 512);
            expect (pads.isPlaying (1));
            expect (peak (post) > 0.05f);
            shortWav.deleteFile();
        }

        beginTest ("Fade stop eventually stops playback");
        {
            auto s = pads.getPadState (0);
            s.route = AudioPadRoute::postFx;
            s.fadeOutMs = 5.0f;
            pads.setPadState (0, s, false);
            pads.triggerPad (0);
            pads.render (pre, post, mon, 128);
            pads.stopPad (0);
            for (int i = 0; i < 8; ++i) pads.render (pre, post, mon, 128);
            expect (! pads.isPlaying (0));
        }

        beginTest ("Pad settings round-trip through config tree");
        {
            MicVSTState state;
            AudioPadState p;
            p.name = "Airhorn";
            p.filePath = "C:\\sounds\\airhorn.wav";
            p.volume = 0.72f;
            p.loop = true;
            p.route = AudioPadRoute::preFx;
            p.retrigger = AudioPadRetrigger::ignore;
            p.hotkey = "Ctrl+Shift+1";
            p.fadeInMs = 12.0f;
            p.fadeOutMs = 80.0f;
            p.colourARGB = 0xff35473c;
            state.audioPads.add (p);
            state.audioPadMasterVolume = 0.91f;

            const auto restored = fromValueTree (toValueTree (state));
            expectEquals (restored.audioPads.size(), 1);
            expectEquals (restored.audioPads[0].name, p.name);
            expectEquals (restored.audioPads[0].hotkey, p.hotkey);
            expectEquals ((int) restored.audioPads[0].route, (int) p.route);
            expectWithinAbsoluteError (restored.audioPads[0].volume, p.volume, 0.001f);
            expectWithinAbsoluteError (restored.audioPadMasterVolume, 0.91f, 0.001f);
        }

        wav.deleteFile();
    }
};
static AudioPadEngineTest audioPadEngineTest;
