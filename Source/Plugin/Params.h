// =============================================================================
//  BURNIX by PIRADEX — contrato de parametros (IDs ESTAVEIS: nunca renomear; versao 1 em todos)
//  Ver Docs/PRD.md, secao "Contrato de parametros".
// =============================================================================
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/BandSplitter.h"

namespace bxp
{
inline const juce::StringArray& voicingChoices()
{
    static const juce::StringArray a { "MORPH", "EMBER", "TRIODE BLOOM", "PENTODE EDGE", "IRON CORE", "VELVET", "FIRESTORM" };
    return a;
}
inline const juce::StringArray& voicingOnly()
{
    static const juce::StringArray a { "EMBER", "TRIODE BLOOM", "PENTODE EDGE", "IRON CORE", "VELVET", "FIRESTORM" };
    return a;
}
inline const juce::StringArray& qualityChoices() { static const juce::StringArray a { "ECO 1x", "HQ 4x", "HQ 8x", "ULTRA 16x" }; return a; }
inline int qualityFactor (int q) { static const int f[] { 1, 4, 8, 16 }; return f[juce::jlimit (0, 3, q)]; }

inline juce::String bandId (int b, const char* p) { return "b" + juce::String (b + 1) + "_" + p; }
inline const double kBandFreqDefault[bx::kMaxBands] { 60, 330, 1800, 6000, 11000, 17000 };
inline const double kXoDefault[bx::kMaxX] { 120, 900, 3500, 8000, 14000 };

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout L;
    auto fl = [&] (const String& id, const String& name, float lo, float hi, float def, float skewCentre = 0.f, const String& unit = {}, int dec = 1)
    {
        NormalisableRange<float> r (lo, hi);
        if (skewCentre > 0.f) r.setSkewForCentre (skewCentre);
        L.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def,
            AudioParameterFloatAttributes().withLabel (unit).withStringFromValueFunction ([dec] (float v, int) { return String (v, dec); })));
    };
    auto ch = [&] (const String& id, const String& name, const StringArray& c, int def) { L.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, c, def)); };
    auto bo = [&] (const String& id, const String& name, bool def) { L.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def)); };

    // ---- GLOBAL
    fl ("input", "Input", -24.f, 24.f, 0.f, 0.f, "dB");
    fl ("heat", "Heat", 0.f, 10.f, 5.f);
    fl ("morphX", "Morph X", 0.f, 1.f, 0.5f, 0.f, {}, 2);
    fl ("morphY", "Morph Y", 0.f, 1.f, 0.3f, 0.f, {}, 2);
    ch ("cornerTop", "Morph Top", voicingOnly(), 1);
    ch ("cornerRight", "Morph Right", voicingOnly(), 2);
    ch ("cornerBottom", "Morph Bottom", voicingOnly(), 5);
    ch ("cornerLeft", "Morph Left", voicingOnly(), 4);
    fl ("density", "Density", 0.f, 100.f, 0.f, 0.f, "%", 0);
    fl ("bias", "Bias", -100.f, 100.f, 0.f, 0.f, {}, 0);
    fl ("stageBlend", "Stage Blend", 0.f, 100.f, 0.f, 0.f, "%", 0);
    ch ("topology", "Topology", { "VOICING > PENTODE", "PENTODE > VOICING" }, 0);
    fl ("interstage", "Interstage", -12.f, 12.f, 0.f, 0.f, "dB");
    fl ("transient", "Transient Preserve", 0.f, 100.f, 0.f, 0.f, "%", 0);
    fl ("color", "Color (Tilt)", -6.f, 6.f, 0.f, 0.f, "dB");
    fl ("body", "Body", 0.f, 100.f, 0.f, 0.f, "%", 0);
    fl ("air", "Air", 0.f, 100.f, 0.f, 0.f, "%", 0);
    fl ("width", "Width", 0.f, 200.f, 100.f, 0.f, "%", 0);
    bo ("monoBass", "Mono Bass", false);
    fl ("monoFreq", "Mono Bass Freq", 20.f, 300.f, 120.f, 100.f, "Hz", 0);
    fl ("mix", "Mix", 0.f, 100.f, 100.f, 0.f, "%", 0);
    fl ("output", "Output", -24.f, 24.f, 0.f, 0.f, "dB");
    bo ("autoGain", "Auto Gain", false);
    bo ("delta", "Delta", false);
    bo ("bypass", "Bypass", false);
    ch ("quality", "Quality", qualityChoices(), 1);
    ch ("osPhase", "Oversampling Filter", { "MIN-PHASE", "LINEAR" }, 0);
    L.add (std::make_unique<AudioParameterInt> (ParameterID { "numBands", 1 }, "Bands", 1, bx::kMaxBands, 4));
    ch ("slope", "Crossover Slope", { "24 dB/oct", "48 dB/oct" }, 0);
    for (int k = 0; k < bx::kMaxX; ++k)
        fl ("xo" + String (k + 1), "Crossover " + String (k + 1), 20.f, 20000.f, (float) kXoDefault[k], 1000.f, "Hz", 0);

    // ---- BANDAS
    for (int b = 0; b < bx::kMaxBands; ++b)
    {
        const String n = "Band " + String (b + 1) + " ";
        bo (bandId (b, "on"), n + "On", true);
        bo (bandId (b, "solo"), n + "Solo", false);
        ch (bandId (b, "route"), n + "Route", { "L/R", "MID", "SIDE", "M+S" }, 0);
        fl (bandId (b, "driveM"), n + "Drive Mid", 0.f, 10.f, 3.f);
        fl (bandId (b, "driveS"), n + "Drive Side", 0.f, 10.f, 3.f);
        ch (bandId (b, "voice"), n + "Voicing", voicingChoices(), 0);
        ch (bandId (b, "filter"), n + "Filter", { "NONE", "HP", "LP", "BP", "NOTCH", "SHELF" }, 0);
        fl (bandId (b, "freq"), n + "Filter Freq", 20.f, 20000.f, (float) kBandFreqDefault[b], 1000.f, "Hz", 0);
        fl (bandId (b, "q"), n + "Filter Q", 0.3f, 8.f, 0.7f, 1.2f, {}, 2);
        ch (bandId (b, "mode"), n + "Mode", { "INSERT", "PARALLEL" }, 0);
        fl (bandId (b, "mix"), n + "Mix", 0.f, 100.f, 100.f, 0.f, "%", 0);
    }
    return L;
}
} // namespace bxp
