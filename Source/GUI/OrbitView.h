// =============================================================================
//  BURNIX by PIRADEX — o anel "Orbita" (780 x 676 px, coordenadas de base; o editor escala 75–200 %)
//
//  Geometria: centro (390, 340). Frequencia -> angulo em escala logaritmica, de 135 graus (20 Hz, baixo-esquerda)
//  a 405 graus (20 kHz, baixo-direita), no sentido dos ponteiros. A abertura em baixo (58..122 graus) mostra os
//  medidores IN/OUT (PERFORM, SCULPT) ou o seletor de oversampling (ENGINEER).
//    SIDE  anel interior  r 150..(150 + 4 x drive)      MID  anel exterior  r 206..(206 + 5 x drive)
//    espectro de saida  r 262..300                     disco central r 140: morph (PERFORM) / voicing da banda (SCULPT)
//  Interacao: arrastar um arco para fora/dentro = drive dessa banda (MID por fora, SIDE por dentro); duplo-clique
//  repoe; arrastar o ponto do disco = morph; SCULPT: pegas brancas movem crossovers, clique num arco escolhe a banda.
// =============================================================================
#pragma once
#include "Theme.h"
#include "../Plugin/PluginProcessor.h"

namespace bxui
{
class OrbitView : public juce::Component, public juce::SettableTooltipClient
{
public:
    static constexpr float CX = 390.f, CY = 340.f, A0 = 135.f, SW = 270.f;
    explicit OrbitView (BurnixProcessor& p) : proc (p) { setWantsKeyboardFocus (false); spec.fill (0.f); }

    int view = 0;                                 // 0 PERFORM (e MOTION), 1 SCULPT, 2 ENGINEER
    bool drawMode = false;                        // ENGINEER: DESENHAR (H2..H7 da voz DRAW) em vez de MEDIDO
    std::function<int()> getBand;                 // banda selecionada (0..5)
    std::function<void (int)> setBand;
    std::array<float, 130> spec;                  // 0..1, preenchido pelo editor
    BurnixProcessor::HarmReport report, ref;      // vista ENGINEER

    static float angleOf (double hz) { return A0 + SW * (float) (std::log10 (juce::jlimit (20.0, 20000.0, hz) / 20.0) / 3.0); }
    static double hzOf (float a) { return 20.0 * std::pow (10.0, 3.0 * juce::jlimit (0.f, 1.f, (a - A0) / SW)); }
    static juce::Point<float> pt (float r, float aDeg) { const float a = juce::degreesToRadians (aDeg); return { CX + r * std::cos (a), CY + r * std::sin (a) }; }
    static float jr (float aDeg) { return juce::degreesToRadians (aDeg + 90.f); }
    static juce::Path seg (float ri, float ro, float a1, float a2)
    {
        juce::Path p; p.addCentredArc (CX, CY, ro, ro, 0, jr (a1), jr (a2), true); p.addCentredArc (CX, CY, ri, ri, 0, jr (a2), jr (a1)); p.closeSubPath(); return p;
    }

    void paint (juce::Graphics& g) override
    {
        const auto e = proc.makeEngineParams();
        const int nb = e.numBands;
        float edges[bx::kMaxBands + 1]; edges[0] = A0; edges[nb] = A0 + SW;
        { double lo = 20; for (int k = 0; k < nb - 1; ++k) { lo = juce::jlimit (lo * 1.26, 20000.0 / 1.26, e.xo[k]); edges[k + 1] = angleOf (lo); } }

        g.setColour (kHead); for (float a = 0; a < 360; a += 3) { auto q = pt (304, a); g.fillEllipse (q.x - 0.6f, q.y - 0.6f, 1.2f, 1.2f); }

        if (view == 2) { paintEngineer (g); return; }

        // espectro
        { juce::Path sp; for (int i = 0; i < (int) spec.size(); ++i) { const float a = A0 + SW * i / (float) (spec.size() - 1); const auto p1 = pt (264, a), p2 = pt (264 + 3 + spec[(size_t) i] * 34, a); sp.startNewSubPath (p1); sp.lineTo (p2); }
          g.setColour (kCyan.withAlpha (view == 1 ? 0.3f : 0.5f)); g.strokePath (sp, juce::PathStrokeType (2.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }

        // arcos das bandas: MID por fora, SIDE por dentro
        const int sel = getBand ? getBand() : 0;
        for (int b = 0; b < nb; ++b)
        {
            const auto& bp = e.band[(size_t) b];
            const float hs = (float) e.heat / 5.f;
            const float dM = (float) juce::jlimit (0.0, 10.0, bp.driveM * hs), dS = (float) juce::jlimit (0.0, 10.0, (bp.route == bx::Route::LR ? bp.driveM : bp.driveS) * hs);
            const auto col = bandColour (b);
            const auto& act = proc.activeEngine().activity[(size_t) b];
            auto alphaOf = [] (float db) { return 0.35f + 0.65f * juce::jlimit (0.f, 1.f, (db + 55.f) / 40.f); };
            const float a1 = edges[b] + 0.7f, a2 = edges[b + 1] - 0.7f;
            const bool mOn = bp.on && bp.route != bx::Route::Side, sOn = bp.on && bp.route != bx::Route::Mid;
            g.setColour (col.withAlpha (mOn ? alphaOf (act.mid.load()) : 0.08f)); g.fillPath (seg (206, 206 + 3 + 5 * (mOn ? dM : 0), a1, a2));
            g.setColour (kCyan.withAlpha (sOn ? 0.15f + 0.8f * alphaOf (act.side.load()) * dS / 10.f : 0.05f)); g.fillPath (seg (150, 150 + 3 + 4 * (sOn ? dS : 0), a1, a2));
            if (bp.solo) { g.setColour (kAmber); g.drawText ("S", juce::Rectangle<float> (0, 0, 14, 14).withCentre (pt (196, 0.5f * (a1 + a2))).toNearestInt(), juce::Justification::centred); }
            if (view == 1 && b == sel) { g.setColour (kText); g.strokePath (seg (146, 258, edges[b] + 0.3f, edges[b + 1] - 0.3f), juce::PathStrokeType (2.f)); }
        }
        // crossovers
        for (int k = 1; k < nb; ++k)
        {
            juce::Path t; t.startNewSubPath (pt (146, edges[k])); t.lineTo (pt (view == 1 ? 290.f : 258.f, edges[k]));
            g.setColour (view == 1 ? kText.withAlpha (0.55f) : kBg); g.strokePath (t, juce::PathStrokeType (view == 1 ? 1.5f : 4.f));
            if (view == 1)
            {
                const auto h = pt (286, edges[k]); g.setColour (kBg); g.fillEllipse (h.x - 7, h.y - 7, 14, 14); g.setColour (kText); g.drawEllipse (h.x - 7, h.y - 7, 14, 14, 2.f);
                const double hz = hzOf (edges[k]); g.setFont (mono (10.f)); g.setColour (kText);
                g.drawText (hz >= 1000 ? juce::String (hz / 1000.0, hz >= 10000 ? 0 : 1) + "k" : juce::String (juce::roundToInt (hz / (hz < 100 ? 1.0 : 10.0)) * (hz < 100 ? 1 : 10)), juce::Rectangle<float> (44, 14).withCentre (pt (306, edges[k])).toNearestInt(), juce::Justification::centred);
            }
        }
        // disco
        g.setColour (kDisc); g.fillEllipse (CX - 140, CY - 140, 280, 280); g.setColour (kLine); g.drawEllipse (CX - 140, CY - 140, 280, 280, 1.f);
        if (view == 0) paintMorph (g, e); else paintVoicePicker (g, e, sel);

        // legenda dos aneis e dica (em baixo, legivel)
        {
            const int y = 586; g.setFont (font (11.f));
            const juce::String m = L ("MID · anel exterior", "MID · outer ring"), sd = L ("SIDE · anel interior", "SIDE · inner ring");
            const int wm = 14 + 6 + (int) juce::GlyphArrangement::getStringWidth (font (11.f), m), ws = 14 + 6 + (int) juce::GlyphArrangement::getStringWidth (font (11.f), sd);
            int x = (int) CX - (wm + 20 + ws) / 2;
            g.setColour (kCoral); g.fillRoundedRectangle ((float) x, (float) y + 4, 14, 6, 3); g.setColour (kDim); g.drawText (m, x + 20, y, wm, 14, juce::Justification::left); x += wm + 20;
            g.setColour (kCyan); g.fillRoundedRectangle ((float) x, (float) y + 4, 14, 6, 3); g.setColour (kDim); g.drawText (sd, x + 20, y, ws, 14, juce::Justification::left);
            g.setColour (kDim); g.setFont (font (12.f));
            g.drawText (view == 0 ? L ("Cada arco é uma banda · puxa-o para fora para dar mais drive · arrasta o ponto para mudar o carácter", "Each arc is a band · drag it outwards for more drive · drag the dot to change the character")
                                  : L ("Clica num arco para escolher a banda · arrasta as pegas brancas para mover os crossovers", "Click an arc to pick the band · drag the white handles to move the crossovers"),
                        juce::Rectangle<int> (0, 612, 780, 16), juce::Justification::centred);
        }
        g.setColour (kFaint); g.setFont (mono (10.f));
        g.drawText ("20 Hz", juce::Rectangle<float> (50, 14).withCentre (pt (226, A0) + juce::Point<float> (-20, 26)).toNearestInt(), juce::Justification::centred);
        g.drawText ("20 kHz", juce::Rectangle<float> (50, 14).withCentre (pt (226, A0 + SW) + juce::Point<float> (20, 26)).toNearestInt(), juce::Justification::centred);
    }

    // ---------------------------------------------------------------------------------------- rato
    void mouseDown (const juce::MouseEvent& ev) override
    {
        drag = {}; const auto p = ev.position; const float r = p.getDistanceFrom ({ CX, CY }); const float a = angleAt (p);
        const auto e = proc.makeEngineParams();
        if (view == 2) { if (drawMode) { const int k = harmAt (p.x); if (k > 0) { drag.kind = Drag::Harm; drag.id = "h" + juce::String (k); begin (drag.id); setHarm (p.y, ev.mods.isShiftDown()); } } return; }
        if (r < 140)
        {
            if (view == 0) { drag.kind = Drag::Morph; begin ("morphX"); begin ("morphY"); setMorph (p); }
            else pickVoice (p);
            return;
        }
        if (view == 1 && r > 274 && r < 300)
            for (int k = 0; k < e.numBands - 1; ++k)
                if (std::abs (angleOf (e.xo[k]) - a) < 4.f) { drag.kind = Drag::Xo; drag.index = k; begin ("xo" + juce::String (k + 1)); return; }
        if (r >= 146 && r <= 262 && ! inGap (a))
        {
            const int b = bandAt (e, a); if (b < 0) return;
            if (setBand) setBand (b);
            const auto& bp = e.band[(size_t) b];
            const bool side = r < 203 && bp.route != bx::Route::LR;
            drag.kind = Drag::Drive; drag.index = b; drag.id = bxp::bandId (b, side ? "driveS" : "driveM");
            drag.start = (float) (side ? bp.driveS : bp.driveM); drag.r0 = r; begin (drag.id);
            repaint();
        }
    }
    void mouseDrag (const juce::MouseEvent& ev) override
    {
        const auto p = ev.position;
        switch (drag.kind)
        {
            case Drag::Morph: setMorph (p); break;
            case Drag::Xo:    set ("xo" + juce::String (drag.index + 1), (float) hzOf (angleAt (p))); break;
            case Drag::Harm:  setHarm (p.y, ev.mods.isShiftDown()); break;
            case Drag::Drive: set (drag.id, juce::jlimit (0.f, 10.f, drag.start + (p.getDistanceFrom ({ CX, CY }) - drag.r0) / (ev.mods.isShiftDown() ? 24.f : 6.f))); break;
            default: break;
        }
        repaint();
    }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (drag.kind == Drag::Morph) { end ("morphX"); end ("morphY"); }
        else if (drag.kind == Drag::Xo) end ("xo" + juce::String (drag.index + 1));
        else if (drag.kind == Drag::Drive || drag.kind == Drag::Harm) end (drag.id);
        drag = {};
    }
    void mouseDoubleClick (const juce::MouseEvent& ev) override
    {
        const auto p = ev.position; const float r = p.getDistanceFrom ({ CX, CY }), a = angleAt (p);
        const auto e = proc.makeEngineParams();
        if (view == 2 && drawMode) { const int k = harmAt (p.x); if (k > 0) resetParam ("h" + juce::String (k)); repaint(); return; }
        if (view != 2 && r >= 146 && r <= 262 && ! inGap (a))
        {
            const int b = bandAt (e, a); if (b < 0) return;
            const bool side = r < 203 && e.band[(size_t) b].route != bx::Route::LR;
            resetParam (bxp::bandId (b, side ? "driveS" : "driveM"));
        }
        else if (view == 0 && r < 140) { resetParam ("morphX"); resetParam ("morphY"); }
        repaint();
    }

private:
    struct Drag { enum Kind { None, Morph, Xo, Drive, Harm } kind = None; int index = -1; juce::String id; float start = 0, r0 = 0; };
    Drag drag;
    BurnixProcessor& proc;

    static float angleAt (juce::Point<float> p) { float a = juce::radiansToDegrees (std::atan2 (p.y - CY, p.x - CX)); while (a < A0 - 45.f) a += 360.f; while (a >= A0 + 315.f) a -= 360.f; return a; }
    static bool inGap (float a) { return a > A0 + SW || a < A0; }
    static int bandAt (const bx::EngineParams& e, float a)
    {
        double lo = 20; for (int k = 0; k < e.numBands - 1; ++k) { lo = juce::jlimit (lo * 1.26, 20000.0 / 1.26, e.xo[k]); if (a < angleOf (lo)) return k; }
        return e.numBands - 1;
    }
    juce::RangedAudioParameter* prm (const juce::String& id) { return proc.apvts.getParameter (id); }
    void begin (const juce::String& id) { if (auto* p = prm (id)) p->beginChangeGesture(); }
    void end (const juce::String& id)   { if (auto* p = prm (id)) p->endChangeGesture(); }
    void set (const juce::String& id, float v) { if (auto* p = prm (id)) p->setValueNotifyingHost (p->convertTo0to1 (v)); }
    void resetParam (const juce::String& id) { if (auto* p = prm (id)) { p->beginChangeGesture(); p->setValueNotifyingHost (p->getDefaultValue()); p->endChangeGesture(); } }
    // ENGINEER / DESENHAR: barras H2..H7 (mesma geometria da vista medida)
    static constexpr float hx0 = 84, htop = 100, hbot = 418;
    static float harmX (int k) { return hx0 + 12 + (k - 2) * 72.f; }
    static int harmAt (float x) { for (int k = 2; k <= 7; ++k) if (x >= harmX (k) - 14 && x <= harmX (k) + 58) return k; return -1; }
    void setHarm (float y, bool fine)
    {
        float db = -100.f * (1.f - juce::jlimit (0.f, 1.f, (hbot - y) / (hbot - htop)));
        if (! fine) db = std::round (db);
        set (drag.id, juce::jlimit (-80.f, 0.f, db));
    }
    void setMorph (juce::Point<float> p)
    {
        auto d = p - juce::Point<float> (CX, CY); if (d.getDistanceFromOrigin() > 128.f) d = d * (128.f / d.getDistanceFromOrigin());
        set ("morphX", juce::jlimit (0.f, 1.f, 0.5f + d.x / 256.f)); set ("morphY", juce::jlimit (0.f, 1.f, 0.5f + d.y / 256.f));
    }

    void paintMorph (juce::Graphics& g, const bx::EngineParams& e)
    {
        g.setColour (kLine); g.drawLine (CX, CY - 128, CX, CY + 128); g.drawLine (CX - 128, CY, CX + 128, CY);
        { juce::Path c; c.addEllipse (CX - 70, CY - 70, 140, 140); const float dash[] { 2.f, 5.f }; juce::Path d; juce::PathStrokeType (1.f).createDashedStroke (d, c, dash, 2); g.setColour (kLine2); g.fillPath (d); }
        const juce::Point<float> pos[4] { { CX, CY - 112 }, { CX + 92, CY }, { CX, CY + 110 }, { CX - 92, CY } };
        const juce::Colour cc[4] { kCoral, kCyan, kAmber, kViolet };
        const auto w = bx::morphWeights (e.morphX, e.morphY);
        for (int i = 0; i < 4; ++i)
        {
            g.setColour (cc[i].withAlpha (0.45f + 0.55f * (float) w[(size_t) i])); g.setFont (font (10.f, true));
            juce::String n = bx::voicingName (e.corner[(size_t) i]); if (i == 1 || i == 3) n = n.upToFirstOccurrenceOf (" ", false, false);
            g.drawText (n, juce::Rectangle<float> (110, 14).withCentre (pos[i]).toNearestInt(), juce::Justification::centred);
        }
        const juce::Point<float> puck (CX + ((float) e.morphX - 0.5f) * 256.f, CY + ((float) e.morphY - 0.5f) * 256.f);
        g.setColour (kCoral.withAlpha (0.16f)); g.fillEllipse (puck.x - 24, puck.y - 24, 48, 48);
        g.setColour (kText); g.fillEllipse (puck.x - 9, puck.y - 9, 18, 18);
        if (proc.modLive.active.load())
        {
            const juce::Point<float> live (CX + (proc.modLive.mx.load() - 0.5f) * 256.f, CY + (proc.modLive.my.load() - 0.5f) * 256.f);
            if (live.getDistanceFrom (puck) > 1.5f) { g.setColour (kViolet.withAlpha (0.5f)); g.drawLine (juce::Line<float> (puck, live), 1.f); }
            g.setColour (kViolet); g.drawEllipse (live.x - 7, live.y - 7, 14, 14, 2.f);
        }
        g.setColour (kFaint); g.setFont (font (9.f, true)); g.drawText ("MORPH", juce::Rectangle<int> ((int) CX - 40, (int) CY + 120, 80, 12), juce::Justification::centred);
    }

    // SCULPT: voicing da banda selecionada. Centro = MORPH (segue o morph global); seis voicings a volta.
    juce::Point<float> voicePos (int i) const { if (i == 0) return { CX, CY }; return pt (104, -90.f + (i - 1) * (360.f / 7.f)); }
    static float voiceW (int i) { return i == 0 ? 74.f : 82.f; }
    void paintVoicePicker (juce::Graphics& g, const bx::EngineParams& e, int sel)
    {
        const int cur = e.band[(size_t) juce::jlimit (0, e.numBands - 1, sel)].voice + 1;   // 0 = MORPH
        g.setColour (kFaint); g.setFont (font (9.f, true));
        g.drawText (L ("VOICING DA BANDA ", "BAND VOICING ") + juce::String (sel + 1), juce::Rectangle<int> ((int) CX - 80, (int) CY - 132, 160, 12), juce::Justification::centred);
        for (int i = 0; i < 8; ++i)
        {
            const auto c = voicePos (i); const bool on = i == cur;
            auto r = juce::Rectangle<float> (voiceW (i), 24.f).withCentre (c);
            g.setColour (on ? kCoral : kLine2); if (on) g.fillRoundedRectangle (r, 12.f); else g.drawRoundedRectangle (r, 12.f, 1.f);
            g.setColour (on ? kBg : (i == 7 ? kAmber : kDim)); g.setFont (font (9.f, true));
            g.drawText (bxp::voicingChoices()[i], r.toNearestInt(), juce::Justification::centred);
        }
    }
    void pickVoice (juce::Point<float> p)
    {
        const int b = getBand ? getBand() : 0;
        for (int i = 0; i < 8; ++i)
            if (juce::Rectangle<float> (voiceW (i), 24.f).withCentre (voicePos (i)).contains (p))
            {
                if (auto* pr = prm (bxp::bandId (b, "voice"))) { pr->beginChangeGesture(); pr->setValueNotifyingHost (pr->convertTo0to1 ((float) i)); pr->endChangeGesture(); }
                repaint(); return;
            }
    }

    // ENGINEER: harmonicos H2..H10 MEDIDOS em barras (pares a coral, impares a ciano) e a referencia guardada
    void paintEngineer (juce::Graphics& g)
    {
        g.fillAll (kBg);
        const auto box = juce::Rectangle<float> (24, 30, 740, 440);
        g.setColour (kLine); g.drawRoundedRectangle (box, 14.f, 1.f);
        if (drawMode) { paintDraw (g, box); return; }
        g.setColour (kDim); g.setFont (font (10.f, true)); g.drawText (L ("HARMÓNICOS MEDIDOS", "MEASURED HARMONICS"), 44, 44, 260, 14, juce::Justification::left);
        g.setFont (font (11.f)); g.drawText (L ("seno 1 kHz a −6 dBFS · dB relativos ao fundamental", "1 kHz sine at −6 dBFS · dB relative to the fundamental"), 44, 60, 420, 14, juce::Justification::left);
        g.setColour (kText); g.setFont (font (30.f));
        g.drawText (report.valid ? juce::String (report.thd * 100.0, 2) + " %" : juce::String ("--"), 520, 40, 190, 34, juce::Justification::right);
        g.setColour (kDim); g.setFont (font (11.f)); g.drawText ("THD", 714, 54, 40, 16, juce::Justification::left);
        const float x0 = 84, top = 100, bot = 418, h = bot - top;
        auto yOf = [&] (double db) { return bot - h * (float) juce::jlimit (0.0, 1.0, 1.0 + db / 100.0); };
        g.setFont (mono (10.f));
        for (int t = 0; t >= -100; t -= 20) { const float y = yOf (t); g.setColour (kLine); g.drawHorizontalLine ((int) y, x0 - 4, 748); g.setColour (kFaint); g.drawText (juce::String (t), 40, (int) y - 7, 36, 14, juce::Justification::right); }
        for (int k = 2; k <= 10; ++k)
        {
            const float x = x0 + 12 + (k - 2) * 72.f, w = 44;
            const juce::Colour c = k % 2 == 0 ? kCoral : kCyan;
            if (report.valid)
            {
                const float y = yOf (report.db[(size_t) k]); g.setColour (c); g.fillRoundedRectangle (x, y, w, bot - y, 4.f);
                g.setColour (kText); g.setFont (mono (10.f)); g.drawText (report.db[(size_t) k] < -99 ? juce::String ("<-99") : juce::String (juce::roundToInt (report.db[(size_t) k])), (int) x - 6, (int) y - 16, (int) w + 12, 14, juce::Justification::centred);
            }
            if (ref.valid) { const float y = yOf (ref.db[(size_t) k]); g.setColour (kText.withAlpha (0.8f)); g.fillRect (x - 4, y - 1, w + 8, 2.f); }
            g.setColour (kDim); g.setFont (font (12.f, true)); g.drawText ("H" + juce::String (k), (int) x - 6, (int) bot + 8, (int) w + 12, 16, juce::Justification::centred);
        }
        // legenda
        g.setFont (font (11.f)); int lx = 44; const int ly = 448;
        auto key = [&] (juce::Colour c, const juce::String& t, bool line)
        {
            g.setColour (c); if (line) g.fillRect ((float) lx, (float) ly + 6, 14.f, 2.f); else g.fillRoundedRectangle ((float) lx, (float) ly + 3, 14, 8, 2);
            g.setColour (kDim); const int tw = (int) juce::GlyphArrangement::getStringWidth (font (11.f), t) + 4; g.drawText (t, lx + 20, ly, tw, 14, juce::Justification::left); lx += 20 + tw + 18;
        };
        key (kCoral, L ("pares (calor)", "even (warmth)"), false); key (kCyan, L ("ímpares (mordida)", "odd (bite)"), false); if (ref.valid) key (kText, L ("referência guardada", "stored reference"), true);
        if (! report.valid) { g.setColour (kDim); g.setFont (font (13.f)); g.drawText (L ("carrega em MEDIR", "press MEASURE"), box.toNearestInt(), juce::Justification::centred); }
    }
    void paintDraw (juce::Graphics& g, juce::Rectangle<float> box)
    {
        g.setColour (kAmber); g.setFont (font (10.f, true)); g.drawText (L ("DESENHAR HARMÓNICOS · VOZ DRAW", "DRAW HARMONICS · DRAW VOICE"), 44, 44, 360, 14, juce::Justification::left);
        g.setColour (kDim); g.setFont (font (11.f));
        g.drawText (L ("arrasta cada barra · duplo-clique repõe · Shift = 0,1 dB", "drag each bar · double-click resets · Shift = 0.1 dB"), 44, 60, 420, 14, juce::Justification::left);
        const auto e = proc.makeEngineParams(); bool used = false;
        for (int b = 0; b < e.numBands; ++b) { const int v = e.band[(size_t) b].voice; if (v == bx::Draw || (v < 0 && std::find (e.corner.begin(), e.corner.end(), (int) bx::Draw) != e.corner.end())) used = true; }
        g.setColour (used ? kCyan : kAmber); g.setFont (font (11.f, true));
        g.drawFittedText (used ? L ("A soar: há bandas com a voz DRAW.", "Live: some bands use the DRAW voice.")
                               : L ("Ainda não soa: escolhe DRAW no voicing de uma banda (SCULPT).", "Not heard yet: pick DRAW as a band voicing (SCULPT)."),
                          430, 38, 320, 34, juce::Justification::topRight, 2);
        const float h = hbot - htop; auto yOf = [&] (double db) { return hbot - h * (float) juce::jlimit (0.0, 1.0, 1.0 + db / 100.0); };
        g.setFont (mono (10.f));
        for (int t = 0; t >= -100; t -= 20) { const float y = yOf (t); g.setColour (kLine); g.drawHorizontalLine ((int) y, hx0 - 4, 748); g.setColour (kFaint); g.drawText (juce::String (t), 40, (int) y - 7, 36, 14, juce::Justification::right); }
        for (int k = 2; k <= 10; ++k)
        {
            const float x = harmX (k), w = 44;
            if (k <= 7)
            {
                const float db = proc.apvts.getRawParameterValue ("h" + juce::String (k))->load(); const float y = yOf (db);
                const juce::Colour c = k % 2 == 0 ? kCoral : kCyan;
                g.setColour (c.withAlpha (db <= -79.5f ? 0.25f : 1.f)); g.fillRoundedRectangle (x, y, w, hbot - y, 4.f);
                g.setColour (kText); g.fillRoundedRectangle (x - 3, y - 3, w + 6, 6, 3);
                g.setFont (mono (10.f)); g.drawText (db <= -79.5f ? L ("desl.", "off") : juce::String (db, 1), (int) x - 10, (int) y - 20, (int) w + 20, 14, juce::Justification::centred);
                if (report.valid && report.db[(size_t) k] > -100) { const float ym = yOf (report.db[(size_t) k]); g.setColour (kText.withAlpha (0.5f)); g.fillRect (x - 4, ym - 1, w + 8, 2.f); }
            }
            else { g.setColour (kLine); g.fillRoundedRectangle (x, hbot - 4, w, 4, 2); }
            g.setColour (k <= 7 ? kDim : kFaint); g.setFont (font (12.f, true)); g.drawText ("H" + juce::String (k), (int) x - 6, (int) hbot + 8, (int) w + 12, 16, juce::Justification::centred);
        }
        g.setColour (kDim); g.setFont (font (11.f));
        g.drawText (L ("barras = nível desenhado (dB face ao fundamental) · linha clara = última medição", "bars = drawn level (dB re fundamental) · light line = last measurement"), 44, 448, 700, 14, juce::Justification::left);
        juce::ignoreUnused (box);
    }
    void pickQuality (float a)
    {
        if (a > 360.f) a -= 360.f;
        for (int i = 0; i < 4; ++i) { const float a2 = 122.f - i * 16.f, a1 = a2 - 14.f; if (a >= a1 - 1 && a <= a2 + 1) { if (auto* p = prm ("quality")) { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 ((float) i)); p->endChangeGesture(); } repaint(); return; } }
    }
};
} // namespace bxui
