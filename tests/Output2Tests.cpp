#include <juce_core/juce_core.h>
#include "audio/MonitorBuffer.h"
#include "state/Persistence.h"

#include <array>
#include <cmath>

namespace
{
    bool allFinite (const std::vector<float>& v)
    {
        for (float x : v)
            if (! std::isfinite (x)) return false;
        return true;
    }

    float peakAbs (const std::vector<float>& v)
    {
        float p = 0.0f;
        for (float x : v) p = juce::jmax (p, std::abs (x));
        return p;
    }
}

struct Output2MonitorBufferTest : juce::UnitTest
{
    Output2MonitorBufferTest() : juce::UnitTest ("Output2 MonitorBuffer") {}

    void runTest() override
    {
        beginTest ("waits for safety buffer instead of playing partial audio");
        {
            MonitorBuffer b;
            b.setSourceSampleRate (48000.0);
            b.prepare (48000.0, 256);

            std::vector<float> src (100, 0.5f);
            const float* in[] = { src.data() };
            b.push (in, 1, (int) src.size());

            std::vector<float> l (256, 1.0f), r (256, 1.0f);
            float* out[] = { l.data(), r.data() };
            b.render (out, 2, 256);
            expectWithinAbsoluteError (peakAbs (l), 0.0f, 1.0e-6f);
            expectWithinAbsoluteError (peakAbs (r), 0.0f, 1.0e-6f);
        }

        beginTest ("stereo processed audio reaches Output2");
        {
            MonitorBuffer b;
            b.setSourceSampleRate (48000.0);
            b.prepare (48000.0, 256);

            std::vector<float> left (4096, 0.25f), right (4096, -0.5f);
            const float* in[] = { left.data(), right.data() };
            b.push (in, 2, (int) left.size());

            std::vector<float> l (512, 0.0f), r (512, 0.0f);
            float* out[] = { l.data(), r.data() };
            b.render (out, 2, 512);

            expect (allFinite (l));
            expect (allFinite (r));
            expectWithinAbsoluteError (l[100], 0.25f, 0.01f);
            expectWithinAbsoluteError (r[100], -0.5f, 0.01f);
        }

        beginTest ("different Output2 sample rate remains finite and audible");
        {
            MonitorBuffer b;
            b.setSourceSampleRate (48000.0);
            b.prepare (44100.0, 512);

            std::vector<float> source (8192);
            for (int i = 0; i < (int) source.size(); ++i)
                source[(size_t) i] = 0.4f * std::sin (juce::MathConstants<float>::twoPi * 440.0f * (float) i / 48000.0f);
            const float* in[] = { source.data() };
            b.push (in, 1, (int) source.size());

            std::vector<float> l (1024, 0.0f), r (1024, 0.0f);
            float* out[] = { l.data(), r.data() };
            b.render (out, 2, 1024);

            expect (allFinite (l));
            expect (allFinite (r));
            expect (peakAbs (l) > 0.1f);
            expect (peakAbs (r) > 0.1f);
        }
    }
};
static Output2MonitorBufferTest output2MonitorBufferTest;

struct Output2PersistenceTest : juce::UnitTest
{
    Output2PersistenceTest() : juce::UnitTest ("Output2 persistence") {}

    void runTest() override
    {
        beginTest ("Output2 selection round-trips through config tree");
        MicVSTState s;
        s.inputDevice = "USB Mic";
        s.outputDevice = "CABLE Input";
        s.output2Device = "Headphones (USB DAC)";

        const auto restored = fromValueTree (toValueTree (s));
        expectEquals (restored.output2Device, s.output2Device);
    }
};
static Output2PersistenceTest output2PersistenceTest;
