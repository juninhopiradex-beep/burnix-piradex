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

    int view = 0;                                 // 0 PERFORM, 1 SCULPT, 2 ENGINEER
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
        // medidores na abertura
        auto meterArc = [&] (float r, float db, juce::Colour c)
        {
            juce::Path tr; tr.addCentredArc (CX, CY, r, r, 0, jr (122), jr (58), true); g.setColour (kLine); g.strokePath (tr, juce::PathStrokeType (6.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            const float f = juce::jlimit (0.f, 1.f, (db + 60.f) / 60.f); if (f <= 0.001f) return;
            juce::Path fl; fl.addCentredArc (CX, CY, r, r, 0, jr (122), jr (122 - 64 * f), true); g.setColour (c); g.strokePath (fl, juce::PathStrokeType (6.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        };
        meterArc (250, proc.meters.inPeak.load(), kCyan); meterArc (266, proc.meters.outPeak.load(), kCoral);

        // disco
        g.setColour (kDisc); g.fillEllipse (CX - 140, CY - 140, 280, 280); g.setColour (kLine); g.drawEllipse (CX - 140, CY - 140, 280, 280, 1.f);
        if (view == 0) paintMorph (g, e); else paintVoicePicker (g, e, sel);

        g.setColour (kDim); g.setFont (font (10.f, true));
        g.drawText (T8 ("MID · exterior   /   SIDE · interior"), juce::Rectangle<int> (0, 2, 780, 14), juce::Justification::centred);
        g.setColour (kFaint); g.setFont (mono (10.f));
        g.drawText ("20 Hz", juce::Rectangle<float> (50, 14).withCentre (pt (226, A0) + juce::Point<float> (-20, 26)).toNearestInt(), juce::Justification::centred);
        g.drawText ("20 kHz", juce::Rectangle<float> (50, 14).withCentre (pt (226, A0 + SW) + juce::Point<float> (20, 26)).toNearestInt(), juce::Justification::centred);
    }

    // ---------------------------------------------------------------------------------------- rato
    void mouseDown (const juce::MouseEvent& ev) override
    {
        drag = {}; const auto p = ev.position; const float r = p.getDistanceFrom ({ CX, CY }); const float a = angleAt (p);
        const auto e = proc.makeEngineParams();
        if (view == 2) { if (r > 255 && r < 300 && inGap (a)) pickQuality (a); return; }
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
            case Drag::Drive: set (drag.id, juce::jlimit (0.f, 10.f, drag.start + (p.getDistanceFrom ({ CX, CY }) - drag.r0) / (ev.mods.isShiftDown() ? 24.f : 6.f))); break;
            default: break;
        }
        repaint();
    }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (drag.kind == Drag::Morph) { end ("morphX"); end ("morphY"); }
        else if (drag.kind == Drag::Xo) end ("xo" + juce::String (drag.index + 1));
        else if (drag.kind == Drag::Drive) end (drag.id);
        drag = {};
    }
    void mouseDoubleClick (const juce::MouseEvent& ev) override
    {
        const auto p = ev.position; const float r = p.getDistanceFrom ({ CX, CY }), a = angleAt (p);
        const auto e = proc.makeEngineParams();
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
    struct Drag { enum Kind { None, Morph, Xo, Drive } kind = None; int index = -1; juce::String id; float start = 0, r0 = 0; };
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
        g.setColour (kFaint); g.setFont (font (9.f, true)); g.drawText ("MORPH", juce::Rectangle<int> ((int) CX - 40, (int) CY + 120, 80, 12), juce::Justification::centred);
    }

    // SCULPT: voicing da banda selecionada. Centro = MORPH (segue o morph global); seis voicings a volta.
    juce::Point<float> voicePos (int i) const { if (i == 0) return { CX, CY }; return pt (96, -90.f + (i - 1) * 60.f); }
    void paintVoicePicker (juce::Graphics& g, const bx::EngineParams& e, int sel)
    {
        const int cur = e.band[(size_t) juce::jlimit (0, e.numBands - 1, sel)].voice + 1;   // 0 = MORPH
        g.setColour (kFaint); g.setFont (font (9.f, true));
        g.drawText ("VOICING DA BANDA " + juce::String (sel + 1), juce::Rectangle<int> ((int) CX - 80, (int) CY - 132, 160, 12), juce::Justification::centred);
        for (int i = 0; i < 7; ++i)
        {
            const auto c = voicePos (i); const bool on = i == cur;
            auto r = juce::Rectangle<float> (i == 0 ? 74.f : 88.f, 24.f).withCentre (c);
            g.setColour (on ? kCoral : kLine2); if (on) g.fillRoundedRectangle (r, 12.f); else g.drawRoundedRectangle (r, 12.f, 1.f);
            g.setColour (on ? kBg : kDim); g.setFont (font (9.5f, true));
            g.drawText (bxp::voicingChoices()[i], r.toNearestInt(), juce::Justification::centred);
        }
    }
    void pickVoice (juce::Point<float> p)
    {
        const int b = getBand ? getBand() : 0;
        for (int i = 0; i < 7; ++i)
            if (juce::Rectangle<float> (i == 0 ? 74.f : 88.f, 24.f).withCentre (voicePos (i)).contains (p))
            {
                if (auto* pr = prm (bxp::bandId (b, "voice"))) { pr->beginChangeGesture(); pr->setValueNotifyingHost (pr->convertTo0to1 ((float) i)); pr->endChangeGesture(); }
                repaint(); return;
            }
    }

    // ENGINEER: radar real dos harmonicos (H2..H10) e seletor de oversampling na abertura
    static float spokeAngle (int k) { return 150.f + (k - 2) * 30.f; }
    void paintEngineer (juce::Graphics& g)
    {
        for (float r : { 250.f, 194.f, 139.f, 83.f }) { g.setColour (kLine); g.drawEllipse (CX - r, CY - r, 2 * r, 2 * r, 1.f); }
        for (int k = 2; k <= 10; ++k)
        {
            g.drawLine ({ pt (26, spokeAngle (k)), pt (250, spokeAngle (k)) }, 1.f);
            g.setColour (k % 2 == 0 ? kCoral : kViolet); g.setFont (mono (11.f));
            g.drawText ("H" + juce::String (k), juce::Rectangle<float> (30, 14).withCentre (pt (272, spokeAngle (k))).toNearestInt(), juce::Justification::centred);
            g.setColour (kLine);
        }
        auto poly = [&] (const BurnixProcessor::HarmReport& h)
        {
            juce::Path p; for (int k = 2; k <= 10; ++k) { const float rr = 250.f * juce::jlimit (0.f, 1.f, 1.f + (float) h.db[(size_t) k] / 90.f); const auto q = pt (juce::jmax (26.f, rr), spokeAngle (k)); if (k == 2) p.startNewSubPath (q); else p.lineTo (q); }
            p.closeSubPath(); return p;
        };
        if (ref.valid) { const auto p = poly (ref); g.setColour (kDim.withAlpha (0.07f)); g.fillPath (p); const float dash[] { 4.f, 4.f }; juce::Path d; juce::PathStrokeType (1.f).createDashedStroke (d, p, dash, 2); g.setColour (kDim.withAlpha (0.7f)); g.fillPath (d); }
        if (report.valid) { const auto p = poly (report); g.setColour (kCoral.withAlpha (0.18f)); g.fillPath (p); g.setColour (kCoral); g.strokePath (p, juce::PathStrokeType (2.f)); }
        g.setColour (kFaint); g.setFont (mono (9.f)); g.drawText ("0 dB", (int) CX + 6, (int) CY - 252, 40, 12, juce::Justification::left); g.drawText (T8 ("−40"), (int) CX + 6, (int) CY - 141, 40, 12, juce::Justification::left);
        // oversampling na abertura
        const int q = (int) std::lround (proc.apvts.getRawParameterValue ("quality")->load());
        for (int i = 0; i < 4; ++i)
        {
            const float a2 = 122.f - i * 16.f, a1 = a2 - 14.f;
            g.setColour (i == q ? kCyan : kLine); g.fillPath (seg (262, 278, a1, a2));
            g.setColour (i == q ? kCyan : kFaint); g.setFont (mono (11.f));
            g.drawText (juce::String (bxp::qualityFactor (i)) + "x", juce::Rectangle<float> (34, 14).withCentre (pt (296, 0.5f * (a1 + a2))).toNearestInt(), juce::Justification::centred);
        }
        g.setColour (kDim); g.setFont (font (10.f, true)); g.drawText ("OVERSAMPLING", juce::Rectangle<int> ((int) CX - 60, (int) CY + 214, 120, 14), juce::Justification::centred);
        g.setColour (kText); g.setFont (font (28.f));
        g.drawText (report.valid ? juce::String (report.thd * 100.0, 2) + " %" : juce::String ("--"), juce::Rectangle<int> ((int) CX - 80, (int) CY - 18, 160, 32), juce::Justification::centred);
        g.setColour (kDim); g.setFont (font (10.f, true));
        g.drawText (report.valid ? T8 ("THD medido · seno 1 kHz −6 dBFS") : T8 ("carregue em MEDIR"), juce::Rectangle<int> ((int) CX - 110, (int) CY + 16, 220, 14), juce::Justification::centred);
    }
    void pickQuality (float a)
    {
        if (a > 360.f) a -= 360.f;
        for (int i = 0; i < 4; ++i) { const float a2 = 122.f - i * 16.f, a1 = a2 - 14.f; if (a >= a1 - 1 && a <= a2 + 1) { if (auto* p = prm ("quality")) { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 ((float) i)); p->endChangeGesture(); } repaint(); return; } }
    }
};
} // namespace bxui
