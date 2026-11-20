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
// HEAT e MIX grandes; grupos CARACTER e NIVEL; rota e modo de todas as bandas; ganho compensado, auto gain, DELTA, CLIP
class PerformPanel : public juce::Component
{
public:
    explicit PerformPanel (BurnixProcessor& pr) : p (pr)
    {
        heat.attach (p.apvts, "heat"); mixK.attach (p.apvts, "mix");
        heat.format = [] (double v) { return juce::String (v, 1); }; mixK.format = [] (double v) { return juce::String (juce::roundToInt (v)) + " %"; };
        for (auto* r : std::initializer_list<juce::Component*> { &heat, &mixK, &density, &color, &air, &input, &output }) addAndMakeVisible (r);
        density.attach (p.apvts, "density"); color.attach (p.apvts, "color"); air.attach (p.apvts, "air");
        input.attach (p.apvts, "input"); output.attach (p.apvts, "output");
        color.setSuffix (" dB"); input.setSuffix (" dB"); output.setSuffix (" dB"); density.setSuffix (" %"); air.setSuffix (" %");
        for (auto* r : { &density, &color, &air, &input, &output }) r->labelW = 82;
        route.onPick = [this] (int i) { const int nb = (int) getParam (p, "numBands"); p.undo.beginNewTransaction ("Rota"); for (int b = 0; b < nb; ++b) setParam (p, bxp::bandId (b, "route"), (float) i); refresh(); };
        mode.onPick  = [this] (int i) { const int nb = (int) getParam (p, "numBands"); p.undo.beginNewTransaction ("Modo"); for (int b = 0; b < nb; ++b) setParam (p, bxp::bandId (b, "mode"), (float) i); refresh(); };
        comp.attach (p.apvts, "levelComp"); autoG.attach (p.apvts, "autoGain"); delta.attach (p.apvts, "delta"); clip.attach (p.apvts, "clip");
        for (auto* c : std::initializer_list<juce::Component*> { &route, &mode, &comp, &autoG, &delta, &clip }) addAndMakeVisible (c);
        relabel();
    }
    void relabel()
    {
        comp.setButtonText (L ("GANHO COMPENSADO", "GAIN COMPENSATION")); delta.setButtonText (L ("DELTA PURO", "PURE DELTA"));
        comp.setTooltip (L ("O drive muda o carácter e não o volume (comparação honesta). Ligado por omissão.", "Drive changes the character, not the level (honest comparison). On by default."));
        autoG.setTooltip (L ("Iguala o volume de saída ao da entrada, devagar (A/B justo).", "Slowly matches output loudness to the input (fair A/B)."));
        delta.setTooltip (L ("Ouvir só o que o BURNIX acrescenta, sem a diferença de volume.", "Hear only what BURNIX adds, without the level difference."));
        clip.setTooltip (L ("Segurança de saída: até −1 dBFS não toca no som; teto em 0 dBFS.", "Output safety: untouched up to −1 dBFS; ceiling at 0 dBFS."));
        heat.setTooltip (L ("HEAT escala o drive de todas as bandas (5.0 = bandas tal como estão)", "HEAT scales the drive of every band (5.0 = bands as set)"));
        repaint();
    }
    juce::Rectangle<int> deltaBounds() const { return delta.getBounds(); }
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
        auto k = r.removeFromTop (132); heat.setBounds (k.removeFromLeft (k.getWidth() / 2).reduced (6, 0)); mixK.setBounds (k.reduced (6, 0));
        r.removeFromTop (54);                                                      // barra do morph (desenhada)
        card1 = r.removeFromTop (28 + 3 * 28 + 6); auto c1 = card1.reduced (12, 0).withTrimmedTop (28);
        for (auto* row : { &density, &color, &air }) { row->setBounds (c1.removeFromTop (24)); c1.removeFromTop (4); }
        r.removeFromTop (8);
        card2 = r.removeFromTop (28 + 2 * 28 + 6); auto c2 = card2.reduced (12, 0).withTrimmedTop (28);
        for (auto* row : { &input, &output }) { row->setBounds (c2.removeFromTop (24)); c2.removeFromTop (4); }
        r.removeFromTop (10); route.setBounds (r.removeFromTop (28)); r.removeFromTop (6); mode.setBounds (r.removeFromTop (28));
        r.removeFromTop (10);
        auto t1 = r.removeFromTop (30); comp.setBounds (t1.removeFromLeft (t1.getWidth() / 2 - 3)); t1.removeFromLeft (6); autoG.setBounds (t1);
        r.removeFromTop (6);
        auto t2 = r.removeFromTop (30); delta.setBounds (t2.removeFromLeft (t2.getWidth() / 2 - 3)); t2.removeFromLeft (6); clip.setBounds (t2);
    }
    void paint (juce::Graphics& g) override
    {
        // barra de caracter (pesos reais do morph)
        const auto e = p.makeEngineParams(); const auto w = bx::morphWeights (e.morphX, e.morphY);
        const juce::Colour cc[4] { kCoral, kCyan, kAmber, kViolet };
        const int y0 = 134;
        auto bar = juce::Rectangle<float> (0, (float) y0 + 18, (float) getWidth(), 8);
        g.setColour (kDim); g.setFont (font (10.f, true)); g.drawText (L ("CARÁCTER (MORPH)", "CHARACTER (MORPH)"), 0, y0, 200, 14, juce::Justification::left);
        float x = 0; juce::String txt;
        for (int i = 0; i < 4; ++i)
        {
            const float ww = (float) w[(size_t) i] * bar.getWidth(); if (ww < 0.5f) continue;
            g.setColour (cc[i]); g.fillRect (bar.getX() + x, bar.getY(), ww, bar.getHeight()); x += ww;
            txt << juce::String (juce::roundToInt (w[(size_t) i] * 100)) << " " << juce::String (bx::voicingName (e.corner[(size_t) i])).toLowerCase() << "   ";
        }
        g.setColour (kDim); g.setFont (font (11.f)); g.drawText (txt.trim(), 0, y0 + 30, getWidth(), 16, juce::Justification::left);
        drawCard (g, card1, L ("CARÁCTER", "CHARACTER")); drawCard (g, card2, L ("NÍVEL", "LEVEL"));
        if (p.usesLegacyEngine())
        {
            g.setColour (kAmber); g.setFont (font (10.f, true));
            g.drawText (L ("SESSÃO DA 1.2 · motor antigo (⚙ › Som)", "1.2 SESSION · old engine (⚙ › Sound)"), 0, getHeight() - 14, getWidth(), 14, juce::Justification::left);
        }
    }
private:
    BurnixProcessor& p; juce::Rectangle<int> card1, card2;
    BigKnob heat { "HEAT", kCoral }, mixK { "MIX", kText };
    ParamRow density { "DENSITY", kCoral }, color { "COLOR", kCyan, true }, air { "AIR", kViolet }, input { "INPUT", kText, true }, output { "OUTPUT", kText, true };
    Segments route { { "L/R", "MID", "SIDE", "M+S" } }, mode { { "INSERT", "PARALLEL" } };
    ToggleParam comp { "GANHO COMPENSADO", kCyan }, autoG { "AUTO GAIN", kCyan }, delta { "DELTA PURO", kAmber }, clip { "CLIP 0 dBFS", kCyan };
};

// ------------------------------------------------------------------------------------------- MEDIDORES (IN / OUT / HARM)
class MeterStrip : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit MeterStrip (BurnixProcessor& pr) : p (pr) {}
    void update()
    {
        auto follow = [] (float& v, float t) { v = t > v ? t : v + 0.25f * (t - v); };
        follow (vin, p.meters.inRms.load()); follow (vout, p.meters.outRms.load()); follow (vh, p.meters.harmRms.load());
        pin = juce::jmax (p.meters.inPeak.load(), pin - 0.6f); pout = juce::jmax (p.meters.outPeak.load(), pout - 0.6f);
        repaint();
    }
    void paint (juce::Graphics& g) override
    {
        const float top = 22, bot = (float) getHeight() - 92, h = bot - top;
        auto yOf = [&] (float db) { return bot - h * juce::jlimit (0.f, 1.f, (db + 60.f) / 60.f); };
        g.setColour (kDim); g.setFont (font (9.f, true)); g.drawText ("dBFS", 0, 0, getWidth(), 12, juce::Justification::centred);
        g.setFont (mono (9.f));
        for (int d : { 0, -6, -12, -24, -48 }) { g.setColour (kFaint); g.drawText (juce::String (d), 0, (int) yOf ((float) d) - 6, 20, 12, juce::Justification::right); }
        const float xs[3] { 22.f, 46.f, 70.f }; const juce::Colour cs[3] { kCyan, kCoral, kAmber };
        const float vals[3] { vin, vout, vh }, peaks[3] { pin, pout, -200.f };
        for (int i = 0; i < 3; ++i)
        {
            g.setColour (kLine); g.fillRoundedRectangle (xs[i], top, 12, h, 3);
            const float y = yOf (vals[i]); g.setColour (cs[i]); g.fillRoundedRectangle (xs[i], y, 12, bot - y, 3);
            if (peaks[i] > -60) { g.setColour (kText); g.fillRect (xs[i], yOf (peaks[i]) - 1, 12.f, 2.f); }
        }
        g.setColour (kDim); g.setFont (font (8.f, true));
        const char* n[3] { "IN", "OUT", "HARM" };
        for (int i = 0; i < 3; ++i) g.drawText (n[i], (int) xs[i] - 6, (int) bot + 4, 24, 12, juce::Justification::centred);
        g.setFont (mono (10.f));
        for (int i = 0; i < 3; ++i) { g.setColour (cs[i]); g.drawText (vals[i] <= -119 ? juce::String ("--") : juce::String (vals[i], 1), 0, (int) bot + 20 + i * 14, getWidth(), 14, juce::Justification::centred); }
        g.setColour (kDim); g.setFont (font (9.f));
        g.drawFittedText (L ("HARM = o que o BURNIX acrescenta", "HARM = what BURNIX adds"), 0, (int) bot + 64, getWidth(), 26, juce::Justification::centred, 2);
    }
private:
    BurnixProcessor& p; float vin = -120, vout = -120, vh = -120, pin = -120, pout = -120;
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
        slope.setValueMap ({ 2, 3, 0, 1 }); slope.bind (p.apvts, "slope");
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
        r.removeFromTop (22); mode.setBounds (r.removeFromTop (28)); r.removeFromTop (22); slope.setBounds (r.removeFromTop (28).removeFromLeft (300));
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
    Segments slope { { "6 dB", "12 dB", "24 dB", "48 dB" } }, route { { "L/R", "MID", "SIDE", "M+S" } }, filter { { "OFF", "HP", "LP", "BP", "NOTCH", "SHELF" } }, mode { { "INSERT", "PARALLEL" } };
    ToggleParam on { "ON", kCyan }, solo { "SOLO", kAmber };
    ParamRow driveM { "DRIVE MID", kCoral }, driveS { "DRIVE SIDE", kCyan }, mix { "MIX", kText }, freq { "FREQ", kViolet }, q { "Q", kViolet };
};

// ------------------------------------------------------------------------------------------- ENGINEER
// painel da direita: VALVULAS, ANALOGICO, SAIDA E ESTEREO e leituras reais
class EngineerPanel : public juce::Component
{
public:
    explicit EngineerPanel (BurnixProcessor& pr) : p (pr)
    {
        topo.bind (p.apvts, "topology"); curve.bind (p.apvts, "driveCurve");
        stage.attach (p.apvts, "stageBlend"); inter.attach (p.apvts, "interstage"); bias.attach (p.apvts, "bias"); trans.attach (p.apvts, "transient");
        body.attach (p.apvts, "body"); width.attach (p.apvts, "width"); monoF.attach (p.apvts, "monoFreq"); mono.attach (p.apvts, "monoBass");
        drift.attach (p.apvts, "drift"); comp.attach (p.apvts, "levelComp"); clip.attach (p.apvts, "clip");
        inter.setSuffix (" dB"); stage.setSuffix (" %"); trans.setSuffix (" %"); body.setSuffix (" %"); width.setSuffix (" %"); monoF.setSuffix (" Hz"); drift.setSuffix (" %");
        for (auto* row : { &stage, &inter, &bias, &trans, &body, &width, &drift }) { row->labelW = 92; row->valueW = 58; }
        for (auto* c : std::initializer_list<juce::Component*> { &topo, &curve, &stage, &inter, &bias, &trans, &body, &width, &mono, &monoF, &drift, &comp, &clip }) addAndMakeVisible (c);
        relabel();
    }
    void relabel()
    {
        for (auto* b : topo.btn) b->setTooltip (L ("Ordem dos estágios: voicing → pentode ou pentode → voicing", "Stage order: voicing → pentode or pentode → voicing"));
        curve.btn[0]->setButtonText (L ("PROGRESSIVA", "PROGRESSIVE")); curve.btn[1]->setButtonText (L ("CLÁSSICA (1.2)", "CLASSIC (1.2)"));
        comp.setButtonText (L ("GANHO COMPENSADO", "GAIN COMPENSATION"));
        drift.slider.setTooltip (L ("Bias que respira com o nível e pequenas diferenças fixas entre canais, como numa mesa real. Desligue em master.",
                             "Level-dependent bias and small fixed channel differences, like a real console. Turn off for mastering."));
        repaint();
    }
    void refresh() { topo.refresh(); curve.refresh(); repaint(); }
    void resized() override
    {
        auto r = getLocalBounds();
        cardT = r.removeFromTop (28 + 28 + 6 + 5 * 26 + 8); auto a = cardT.reduced (10, 0).withTrimmedTop (28);
        topo.setBounds (a.removeFromTop (26)); a.removeFromTop (6);
        for (auto* row : { &stage, &inter, &bias, &trans, &body }) { row->setBounds (a.removeFromTop (24)); a.removeFromTop (2); }
        r.removeFromTop (8);
        cardA = r.removeFromTop (28 + 26 + 6 + 26 + 6 + 28 + 8); auto b = cardA.reduced (10, 0).withTrimmedTop (28);
        drift.setBounds (b.removeFromTop (24)); b.removeFromTop (6); curve.setBounds (b.removeFromTop (26)); b.removeFromTop (6); comp.setBounds (b.removeFromTop (28));
        r.removeFromTop (8);
        cardS = r.removeFromTop (28 + 26 + 6 + 28 + 8); auto c = cardS.reduced (10, 0).withTrimmedTop (28);
        width.setBounds (c.removeFromTop (24)); c.removeFromTop (6);
        auto m = c.removeFromTop (28); mono.setBounds (m.removeFromLeft (96)); m.removeFromLeft (6); clip.setBounds (m.removeFromRight (96)); m.removeFromRight (6); monoF.setBounds (m); monoF.labelW = 4; monoF.valueW = 52;
        readout = r.withTrimmedTop (10);
    }
    void paint (juce::Graphics& g) override
    {
        drawCard (g, cardT, L ("VÁLVULAS", "TUBES")); drawCard (g, cardA, L ("ANALÓGICO", "ANALOGUE")); drawCard (g, cardS, L ("SAÍDA E ESTÉREO", "OUTPUT AND STEREO"));
        const int lat = p.latency();
        const juce::String v[3] { juce::String (lat) + " smp", juce::String (p.meters.cpu.load(), 1) + " %", juce::String (p.meters.outTp.load(), 1) + " dBTP" };
        const juce::String k[3] { L ("LATÊNCIA", "LATENCY"), "CPU", "TRUE PEAK" };
        const int cw = readout.getWidth() / 3, ch = juce::jmin (54, readout.getHeight());
        for (int i = 0; i < 3; ++i)
        {
            auto c = juce::Rectangle<int> (readout.getX() + i * cw, readout.getY(), cw - 6, ch).toFloat();
            g.setColour (kLine2); g.drawRoundedRectangle (c, 10.f, 1.f);
            g.setColour (kDim); g.setFont (font (9.f, true)); g.drawText (k[i], c.reduced (8, 4).removeFromTop (12).toNearestInt(), juce::Justification::left);
            g.setColour (kText); g.setFont (font (16.f)); g.drawText (v[i], c.reduced (8, 4).withTrimmedTop (12).toNearestInt(), juce::Justification::left);
        }
    }
private:
    BurnixProcessor& p; juce::Rectangle<int> readout, cardT, cardA, cardS;
    Segments topo { { "VOICING > PENTODE", "PENTODE > VOICING" } }, curve { { "PROGRESSIVA", "CLASSICA" } };
    ParamRow stage { "STAGE BLEND", kViolet }, inter { "INTERSTAGE", kText, true }, bias { "BIAS", kCoral, true }, trans { "TRANSIENT", kCyan },
             body { "BODY", kAmber }, width { "WIDTH", kCyan }, monoF { "", kCyan }, drift { "DRIFT", kAmber };
    ToggleParam mono { "MONO BASS", kCyan }, comp { "GANHO COMPENSADO", kCyan }, clip { "CLIP 0 dBFS", kCyan };
};

// ENGINEER, em baixo a esquerda: oversampling a tocar, qualidade ao exportar, MEDIR e GUARDAR REF.
class EngineerLeft : public juce::Component
{
public:
    explicit EngineerLeft (BurnixProcessor& pr) : p (pr)
    {
        quality.bind (p.apvts, "quality"); phase.bind (p.apvts, "osPhase"); render.bind (p.apvts, "renderQuality");
        stylePill (measure, kCyan, kBg); stylePill (keepRef);
        measure.onClick = [this] { if (onMeasure) onMeasure(); }; keepRef.onClick = [this] { if (onKeepRef) onKeepRef(); };
        for (auto* c : std::initializer_list<juce::Component*> { &quality, &phase, &render, &measure, &keepRef }) addAndMakeVisible (c);
        relabel();
    }
    std::function<void()> onMeasure, onKeepRef;
    void relabel()
    {
        measure.setButtonText (L ("MEDIR", "MEASURE")); keepRef.setButtonText (L ("GUARDAR REF.", "STORE REF."));
        render.btn[0]->setButtonText (L ("IGUAL", "SAME"));
        measure.setTooltip (L ("Mede os harmónicos reais das regulações atuais (seno 1 kHz, −6 dBFS)", "Measures the real harmonics of the current settings (1 kHz sine, −6 dBFS)"));
        keepRef.setTooltip (L ("Guarda a medição atual como referência", "Keeps the current measurement as a reference"));
        for (auto* b : render.btn) b->setTooltip (L ("Ao exportar (bounce) usa esta qualidade. Só com MIN-PHASE e oversampling ligado: a latência fica igual.", "Used when bouncing/exporting. MIN-PHASE with oversampling only: latency stays the same."));
        repaint();
    }
    void refresh() { quality.refresh(); phase.refresh(); render.refresh(); }
    void resized() override
    {
        auto r = getLocalBounds();
        auto left = r.removeFromLeft (r.getWidth() / 2 - 8); r.removeFromLeft (16);
        cardQ = left.removeFromTop (110); auto a = cardQ.reduced (12, 0).withTrimmedTop (28);
        quality.setBounds (a.removeFromTop (28)); a.removeFromTop (8); phase.setBounds (a.removeFromTop (28));
        cardR = r.removeFromTop (110); auto b = cardR.reduced (12, 0).withTrimmedTop (28);
        render.setBounds (b.removeFromTop (28));
        auto bb = getLocalBounds().removeFromBottom (32); measure.setBounds (bb.removeFromLeft (120)); bb.removeFromLeft (8); keepRef.setBounds (bb.removeFromLeft (140));
    }
    void paint (juce::Graphics& g) override
    {
        drawCard (g, cardQ, L ("OVERSAMPLING (A TOCAR)", "OVERSAMPLING (PLAYBACK)")); drawCard (g, cardR, L ("QUALIDADE AO EXPORTAR", "RENDER QUALITY"));
        g.setColour (kDim); g.setFont (font (11.f));
        g.drawFittedText (L ("Toca leve no Mac antigo; o bounce sai na qualidade escolhida.", "Play light on older Macs; the bounce uses the chosen quality."),
                          cardR.reduced (12, 0).withTrimmedTop (64).withHeight (36), juce::Justification::topLeft, 2);
    }
private:
    BurnixProcessor& p; juce::Rectangle<int> cardQ, cardR;
    Segments quality { { "1x", "4x", "8x", "16x" } }, phase { { "MIN-PHASE", "LINEAR" } }, render { { "IGUAL", "8x", "16x" } };
    juce::TextButton measure { "MEDIR" }, keepRef { "GUARDAR REF." };
};
} // namespace bxui
