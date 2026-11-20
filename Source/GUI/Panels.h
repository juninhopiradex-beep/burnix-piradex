// =============================================================================
//  BURNIX by PIRADEX — paineis da direita (372 x 632 px de base): PERFORM, SCULPT e ENGINEER
// =============================================================================
#pragma once
#include "Theme.h"
#include "../Plugin/PluginProcessor.h"

namespace bxui
{
inline void setParam (BurnixProcessor& p, const juce::String& id, float v)
{
    if (auto* r = p.apvts.getParameter (id)) { r->beginChangeGesture(); r->setValueNotifyingHost (r->convertTo0to1 (v)); r->endChangeGesture(); }
}
inline float getParam (BurnixProcessor& p, const juce::String& id) { return p.apvts.getRawParameterValue (id)->load(); }

// ------------------------------------------------------------------------------------------- PERFORM
class PerformPanel : public juce::Component
{
public:
    explicit PerformPanel (BurnixProcessor& pr) : p (pr)
    {
        heat.attach (p.apvts, "heat"); heat.setLabel ("HEAT");
        for (auto* r : { &heat, &density, &color, &air, &input, &mix, &output }) addAndMakeVisible (r);
        density.attach (p.apvts, "density"); color.attach (p.apvts, "color"); air.attach (p.apvts, "air");
        input.attach (p.apvts, "input"); mix.attach (p.apvts, "mix"); output.attach (p.apvts, "output");
        color.setSuffix (" dB"); input.setSuffix (" dB"); output.setSuffix (" dB"); density.setSuffix (" %"); air.setSuffix (" %"); mix.setSuffix (" %");
        route.onPick = [this] (int i) { const int nb = (int) getParam (p, "numBands"); p.undo.beginNewTransaction ("Rota"); for (int b = 0; b < nb; ++b) setParam (p, bxp::bandId (b, "route"), (float) i); refresh(); };
        mode.onPick  = [this] (int i) { const int nb = (int) getParam (p, "numBands"); p.undo.beginNewTransaction ("Modo"); for (int b = 0; b < nb; ++b) setParam (p, bxp::bandId (b, "mode"), (float) i); refresh(); };
        autoG.attach (p.apvts, "autoGain"); delta.attach (p.apvts, "delta");
        for (auto* c : std::initializer_list<juce::Component*> { &route, &mode, &autoG, &delta }) addAndMakeVisible (c);
        relabel();
    }
    void relabel()
    {
        autoG.setTooltip (L ("Iguala o volume de saída ao da entrada (A/B justo). Desligado por omissão.", "Matches output loudness to the input (fair A/B). Off by default."));
        delta.setTooltip (L ("Ouvir só o que o BURNIX acrescenta (harmónicos e mudança de nível), alinhado.", "Hear only what BURNIX adds (harmonics and level change), time-aligned."));
        repaint();
    }
    void refresh()
    {
        const int nb = (int) getParam (p, "numBands"); int r0 = -1, m0 = -1; bool sameR = true, sameM = true;
        for (int b = 0; b < nb; ++b)
        {
            const int r = (int) getParam (p, bxp::bandId (b, "route")), m = (int) getParam (p, bxp::bandId (b, "mode"));
            if (b == 0) { r0 = r; m0 = m; } else { sameR = sameR && r == r0; sameM = sameM && m == m0; }
        }
        route.setSelected (sameR ? r0 : -1); mode.setSelected (sameM ? m0 : -1); repaint();
    }
    void resized() override
    {
        auto r = getLocalBounds();
        r.removeFromTop (82); heat.setBounds (r.removeFromTop (26)); r.removeFromTop (78);
        for (auto* row : { &density, &color, &air, &input, &mix, &output }) { row->setBounds (r.removeFromTop (26)); r.removeFromTop (6); }
        r.removeFromTop (22); route.setBounds (r.removeFromTop (30)); r.removeFromTop (24); mode.setBounds (r.removeFromTop (30).removeFromLeft (r.getWidth() / 2 + 40));
        r.removeFromTop (12); auto t = r.removeFromTop (30); autoG.setBounds (t.removeFromLeft (t.getWidth() / 2 - 4)); t.removeFromLeft (8); delta.setBounds (t);
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (kDim); g.setFont (font (10.f, true)); g.drawText ("HEAT", 0, 0, 200, 14, juce::Justification::left);
        g.setColour (kText); g.setFont (font (60.f)); g.drawText (juce::String (getParam (p, "heat"), 1), 0, 12, 200, 64, juce::Justification::left);
        g.setColour (kDim); g.setFont (font (11.f));
        g.drawFittedText (L ("Escala o drive de todas as bandas (5.0 = valores das bandas tal como estão)", "Scales the drive of every band (5.0 = bands as set)"), getWidth() - 170, 26, 170, 40, juce::Justification::topRight, 3);
        // barra de caracter (pesos reais do morph)
        const auto e = p.makeEngineParams(); const auto w = bx::morphWeights (e.morphX, e.morphY);
        const juce::Colour cc[4] { kCoral, kCyan, kAmber, kViolet };
        auto bar = juce::Rectangle<float> (0, 128, (float) getWidth(), 8);
        g.setColour (kDim); g.setFont (font (10.f, true)); g.drawText (L ("CARÁCTER (MORPH)", "CHARACTER (MORPH)"), 0, 112, 200, 14, juce::Justification::left);
        float x = 0; juce::String txt;
        for (int i = 0; i < 4; ++i)
        {
            const float ww = (float) w[(size_t) i] * bar.getWidth(); if (ww < 0.5f) continue;
            g.setColour (cc[i]); g.fillRect (bar.getX() + x, bar.getY(), ww, bar.getHeight()); x += ww;
            txt << juce::String (juce::roundToInt (w[(size_t) i] * 100)) << " " << juce::String (bx::voicingName (e.corner[(size_t) i])).toLowerCase() << "   ";
        }
        g.setColour (kDim); g.setFont (font (11.f)); g.drawText (txt.trim(), 0, 140, getWidth(), 16, juce::Justification::left);
        g.setColour (kDim); g.setFont (font (10.f, true));
        g.drawText (L ("ROTA (todas as bandas)", "ROUTE (all bands)"), 0, route.getY() - 16, 240, 14, juce::Justification::left);
        g.drawText (L ("MODO (todas as bandas)", "MODE (all bands)"), 0, mode.getY() - 16, 240, 14, juce::Justification::left);
        g.setColour (kFaint); g.setFont (font (10.f));
        g.drawFittedText (L ("Arraste o ponto do centro para misturar carácteres · arraste um arco para fora = mais drive nessa banda · Shift = ajuste fino", "Drag the centre dot to blend characters · drag an arc outwards = more drive on that band · Shift = fine"),
                          0, getHeight() - 30, getWidth(), 30, juce::Justification::bottomLeft, 2);
    }
private:
    BurnixProcessor& p;
    ParamRow heat { "HEAT", kCoral }, density { "DENSITY", kCoral }, color { "COLOR", kCyan, true }, air { "AIR", kViolet },
             input { "INPUT", kText, true }, mix { "MIX", kCoral }, output { "OUTPUT", kText, true };
    Segments route { { "L/R", "MID", "SIDE", "M+S" } }, mode { { "INSERT", "PARALLEL" } };
    ToggleParam autoG { "AUTO GAIN", kCyan }, delta { "DELTA", kAmber };
};

// ------------------------------------------------------------------------------------------- SCULPT
class SculptPanel : public juce::Component
{
public:
    explicit SculptPanel (BurnixProcessor& pr) : p (pr)
    {
        for (int b = 0; b < bx::kMaxBands; ++b)
        {
            auto* c = chips.add (new juce::TextButton (juce::String (b + 1))); stylePill (*c, bandColour (b), kBg);
            c->onClick = [this, b] { if (onBand) onBand (b); }; addAndMakeVisible (c);
        }
        stylePill (minus); stylePill (plus); minus.onClick = [this] { setParam (p, "numBands", getParam (p, "numBands") - 1); };
        plus.onClick = [this] { setParam (p, "numBands", getParam (p, "numBands") + 1); };
        slope.bind (p.apvts, "slope");
        for (auto* c : std::initializer_list<juce::Component*> { &minus, &plus, &slope, &route, &filter, &mode, &on, &solo, &driveM, &driveS, &mix, &freq, &q }) addAndMakeVisible (c);
        driveM.setLabel ("DRIVE MID"); driveS.setLabel ("DRIVE SIDE"); freq.setSuffix (" Hz"); mix.setSuffix (" %");
        relabel();
    }
    void relabel()
    {
        minus.setTooltip (L ("Menos uma banda", "One band less")); plus.setTooltip (L ("Mais uma banda (até 6)", "One band more (up to 6)"));
        repaint();
    }
    std::function<void (int)> onBand;
    void setBand (int b)
    {
        band = b;
        route.bind (p.apvts, bxp::bandId (b, "route")); filter.bind (p.apvts, bxp::bandId (b, "filter")); mode.bind (p.apvts, bxp::bandId (b, "mode"));
        on.attach (p.apvts, bxp::bandId (b, "on")); solo.attach (p.apvts, bxp::bandId (b, "solo"));
        driveM.attach (p.apvts, bxp::bandId (b, "driveM")); driveS.attach (p.apvts, bxp::bandId (b, "driveS")); mix.attach (p.apvts, bxp::bandId (b, "mix"));
        freq.attach (p.apvts, bxp::bandId (b, "freq")); q.attach (p.apvts, bxp::bandId (b, "q"));
        refresh();
    }
    void refresh()
    {
        const int nb = (int) getParam (p, "numBands");
        for (int i = 0; i < chips.size(); ++i) { chips[i]->setVisible (i < nb); chips[i]->setToggleState (i == band, juce::dontSendNotification); }
        route.refresh(); filter.refresh(); mode.refresh(); slope.refresh();
        const int r = (int) getParam (p, bxp::bandId (band, "route"));
        driveM.setLabel (r == 0 ? "DRIVE" : "DRIVE MID"); driveS.setEnabled (r >= 2); driveS.setAlpha (r >= 2 ? 1.f : 0.35f);
        const bool f = (int) getParam (p, bxp::bandId (band, "filter")) != 0; freq.setAlpha (f ? 1.f : 0.35f); q.setAlpha (f ? 1.f : 0.35f);
        repaint();
    }
    void resized() override
    {
        auto r = getLocalBounds();
        auto top = r.removeFromTop (34);
        for (int i = 0; i < chips.size(); ++i) chips[i]->setBounds (top.getX() + i * 40, top.getY(), 34, 34);
        plus.setBounds (top.removeFromRight (34)); top.removeFromRight (6); minus.setBounds (top.removeFromRight (34));
        r.removeFromTop (64);
        auto t = r.removeFromTop (28); route.setBounds (t.removeFromLeft (t.getWidth() - 140)); t.removeFromLeft (8); on.setBounds (t.removeFromLeft (62)); t.removeFromLeft (6); solo.setBounds (t);
        r.removeFromTop (12);
        for (auto* row : { &driveM, &driveS, &mix }) { row->setBounds (r.removeFromTop (26)); r.removeFromTop (6); }
        r.removeFromTop (22); filter.setBounds (r.removeFromTop (28)); r.removeFromTop (8);
        freq.setBounds (r.removeFromTop (26)); r.removeFromTop (6); q.setBounds (r.removeFromTop (26));
        r.removeFromTop (22); mode.setBounds (r.removeFromTop (28)); r.removeFromTop (22); slope.setBounds (r.removeFromTop (28).removeFromLeft (220));
    }
    void paint (juce::Graphics& g) override
    {
        const auto e = p.makeEngineParams(); const int nb = e.numBands, b = juce::jlimit (0, nb - 1, band);
        double lo = 20, hi = 20000; { double x = 20; for (int k = 0; k < nb - 1; ++k) { x = juce::jlimit (x * 1.26, 20000.0 / 1.26, e.xo[k]); if (k == b - 1) lo = x; if (k == b) hi = x; } }
        auto hz = [] (double v) { return v >= 1000 ? juce::String (v / 1000.0, 1) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz"; };
        g.setColour (bandColour (b)); g.setFont (font (10.f, true));
        g.drawText (L ("BANDA ", "BAND ") + juce::String (b + 1) + L (" DE ", " OF ") + juce::String (nb) + "  ·  " + hz (lo) + T8 (" – ") + hz (hi), 0, 44, getWidth(), 14, juce::Justification::left);
        g.setColour (kText); g.setFont (font (26.f, true));
        const int v = e.band[(size_t) b].voice;
        g.drawText (v < 0 ? juce::String ("MORPH (global)") : juce::String (bx::voicingName (v)), 0, 60, getWidth(), 32, juce::Justification::left);
        g.setColour (kDim); g.setFont (font (10.f, true));
        g.drawText (L ("FILTRO SELETIVO (o que satura)", "SELECTIVE FILTER (what saturates)"), 0, filter.getY() - 16, 260, 14, juce::Justification::left);
        g.drawText (L ("MODO", "MODE"), 0, mode.getY() - 16, 200, 14, juce::Justification::left);
        g.drawText (L ("INCLINAÇÃO DOS CROSSOVERS", "CROSSOVER SLOPE"), 0, slope.getY() - 16, 260, 14, juce::Justification::left);
    }
private:
    BurnixProcessor& p; int band = 0;
    juce::OwnedArray<juce::TextButton> chips; juce::TextButton minus { T8 ("−") }, plus { "+" };
    Segments slope { { "24 dB", "48 dB" } }, route { { "L/R", "MID", "SIDE", "M+S" } }, filter { { "OFF", "HP", "LP", "BP", "NOTCH", "SHELF" } }, mode { { "INSERT", "PARALLEL" } };
    ToggleParam on { "ON", kCyan }, solo { "SOLO", kAmber };
    ParamRow driveM { "DRIVE MID", kCoral }, driveS { "DRIVE SIDE", kCyan }, mix { "MIX", kText }, freq { "FREQ", kViolet }, q { "Q", kViolet };
};

// ------------------------------------------------------------------------------------------- ENGINEER
class EngineerPanel : public juce::Component
{
public:
    explicit EngineerPanel (BurnixProcessor& pr) : p (pr)
    {
        phase.bind (p.apvts, "osPhase"); topo.bind (p.apvts, "topology");
        stage.attach (p.apvts, "stageBlend"); inter.attach (p.apvts, "interstage"); bias.attach (p.apvts, "bias"); trans.attach (p.apvts, "transient");
        body.attach (p.apvts, "body"); width.attach (p.apvts, "width"); monoF.attach (p.apvts, "monoFreq"); mono.attach (p.apvts, "monoBass");
        inter.setSuffix (" dB"); stage.setSuffix (" %"); trans.setSuffix (" %"); body.setSuffix (" %"); width.setSuffix (" %"); monoF.setSuffix (" Hz");
        stylePill (measure, kCyan, kBg); stylePill (keepRef);
        measure.onClick = [this] { if (onMeasure) onMeasure(); }; keepRef.onClick = [this] { if (onKeepRef) onKeepRef(); };
        for (auto* c : std::initializer_list<juce::Component*> { &phase, &topo, &stage, &inter, &bias, &trans, &body, &width, &mono, &monoF, &measure, &keepRef }) addAndMakeVisible (c);
        relabel();
    }
    void relabel()
    {
        for (auto* b : topo.btn) b->setTooltip (L ("Ordem dos estágios: voicing → pentode ou pentode → voicing", "Stage order: voicing → pentode or pentode → voicing"));
        measure.setButtonText (L ("MEDIR", "MEASURE")); keepRef.setButtonText (L ("GUARDAR REF.", "STORE REF."));
        measure.setTooltip (L ("Mede os harmónicos reais das regulações atuais (seno 1 kHz, −6 dBFS)", "Measures the real harmonics of the current settings (1 kHz sine, −6 dBFS)"));
        keepRef.setTooltip (L ("Guarda a medição atual como referência tracejada", "Keeps the current measurement as a dashed reference"));
        repaint();
    }
    std::function<void()> onMeasure, onKeepRef;
    const BurnixProcessor::HarmReport* report = nullptr;
    void refresh() { phase.refresh(); topo.refresh(); repaint(); }
    void resized() override
    {
        auto r = getLocalBounds();
        r.removeFromTop (18); phase.setBounds (r.removeFromTop (28)); r.removeFromTop (24); topo.setBounds (r.removeFromTop (28)); r.removeFromTop (12);
        for (auto* row : { &stage, &inter, &bias, &trans, &body, &width }) { row->setBounds (r.removeFromTop (24)); r.removeFromTop (5); }
        auto m = r.removeFromTop (26); mono.setBounds (m.removeFromLeft (96)); m.removeFromLeft (8); monoF.setBounds (m); monoF.labelW = 10;
        r.removeFromTop (14); auto b = r.removeFromTop (30); measure.setBounds (b.removeFromLeft (b.getWidth() / 2 - 4)); b.removeFromLeft (8); keepRef.setBounds (b);
        readout = r.withTrimmedTop (12).withHeight (juce::jmin (r.getHeight() - 12, 128));
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (kDim); g.setFont (font (10.f, true));
        g.drawText (L ("FILTRO DO OVERSAMPLING", "OVERSAMPLING FILTER"), 0, 0, 260, 14, juce::Justification::left);
        g.drawText (L ("TOPOLOGIA DAS VÁLVULAS", "TUBE TOPOLOGY"), 0, topo.getY() - 16, 260, 14, juce::Justification::left);
        // leituras reais
        const int lat = p.latency();
        const juce::String v[6] { report && report->valid ? juce::String (report->thd * 100.0, 2) + " %" : juce::String ("--"),
                                  juce::String (lat) + " smp", juce::String (p.meters.cpu.load(), 1) + " %",
                                  juce::String (p.meters.outTp.load(), 1) + " dBTP", juce::String (p.meters.autoGainDb.load(), 1) + " dB", juce::String (p.activeFactor()) + "x" };
        const juce::String k[6] { "THD", L ("LATÊNCIA", "LATENCY"), "CPU", "TRUE PEAK", "AUTO GAIN", "OVERSAMPLING" };
        const int cw = readout.getWidth() / 3, ch = juce::jmax (36, readout.getHeight() / 2 - 4);
        for (int i = 0; i < 6; ++i)
        {
            auto c = juce::Rectangle<int> (readout.getX() + (i % 3) * cw, readout.getY() + (i / 3) * (ch + 6), cw - 6, ch).toFloat();
            g.setColour (kLine2); g.drawRoundedRectangle (c, 10.f, 1.f);
            g.setColour (kDim); g.setFont (font (9.f, true)); g.drawText (k[i], c.reduced (8, 4).removeFromTop (12).toNearestInt(), juce::Justification::left);
            g.setColour (kText); g.setFont (font (17.f)); g.drawText (v[i], c.reduced (8, 4).withTrimmedTop (12).toNearestInt(), juce::Justification::left);
        }
    }
private:
    BurnixProcessor& p; juce::Rectangle<int> readout;
    Segments phase { { "MIN-PHASE", "LINEAR" } }, topo { { "VOICING > PENTODE", "PENTODE > VOICING" } };
    ParamRow stage { "STAGE BLEND", kViolet }, inter { "INTERSTAGE", kText, true }, bias { "BIAS", kCoral, true }, trans { "TRANSIENT", kCyan },
             body { "BODY", kAmber }, width { "WIDTH", kCyan }, monoF { "", kCyan };
    ToggleParam mono { "MONO BASS", kCyan };
    juce::TextButton measure { "MEDIR" }, keepRef { T8 ("GUARDAR REF.") };
};
} // namespace bxui
