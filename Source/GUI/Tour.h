// =============================================================================
//  BURNIX by PIRADEX — visita guiada (1.3): 3 passos na primeira vez que se abre o plugin; repete-se no menu ⚙.
//  Escurece tudo menos o elemento do passo (arco, ponto do morph, DELTA PURO) e mostra um cartao com o texto.
// =============================================================================
#pragma once
#include "Theme.h"

namespace bxui
{
class Tour : public juce::Component
{
public:
    Tour()
    {
        stylePill (next, kAmber, kBg); stylePill (skip); next.setToggleState (true, juce::dontSendNotification);
        next.onClick = [this] { if (step < 2) { ++step; layout(); } else finish(); };
        skip.onClick = [this] { finish(); };
        addAndMakeVisible (next); addAndMakeVisible (skip);
    }
    std::function<void()> onFinish;
    std::array<juce::Rectangle<float>, 3> targets;      // em coordenadas do editor (1200 x 740)
    void start() { step = 0; setVisible (true); layout(); toFront (true); }
    void resized() override { layout(); }
    void paint (juce::Graphics& g) override
    {
        juce::Path shade; shade.addRectangle (getLocalBounds().toFloat());
        const auto t = targets[(size_t) step];
        juce::Path hole; if (step == 2) hole.addRoundedRectangle (t, 14.f); else hole.addEllipse (t);
        shade.addPath (hole); shade.setUsingNonZeroWinding (false);
        g.setColour (juce::Colour (0xff06070f).withAlpha (0.74f)); g.fillPath (shade);
        g.setColour (kAmber); g.strokePath (hole, juce::PathStrokeType (2.f));
        g.setColour (kDisc); g.fillRoundedRectangle (card.toFloat(), 14.f); g.setColour (kAmber); g.drawRoundedRectangle (card.toFloat().reduced (0.5f), 14.f, 1.f);
        auto c = card.reduced (18, 16);
        g.setColour (kAmber); g.setFont (font (10.f, true)); g.drawText (L ("PASSO ", "STEP ") + juce::String (step + 1) + L (" DE 3", " OF 3"), c.removeFromTop (14), juce::Justification::left);
        c.removeFromTop (4);
        const juce::String title[3] { L ("Puxa um arco para fora", "Drag an arc outwards"), L ("Arrasta o ponto do centro", "Drag the centre dot"), L ("Liga o DELTA PURO", "Turn on PURE DELTA") };
        const juce::String body[3] {
            L ("Cada arco é uma banda de frequências. Puxar para fora dá mais drive a essa banda: o anel de fora é o MID, o de dentro é o SIDE.",
               "Each arc is a frequency band. Dragging it outwards adds drive to that band: the outer ring is MID, the inner ring is SIDE."),
            L ("O ponto mistura os quatro carácteres (TRIODE, PENTODE, FIRESTORM, VELVET). Duplo-clique volta ao início.",
               "The dot blends the four characters (TRIODE, PENTODE, FIRESTORM, VELVET). Double-click resets it."),
            L ("Ouve só o que o BURNIX acrescenta, sem a diferença de volume. É a forma mais rápida de perceber o que cada regulação faz.",
               "Hear only what BURNIX adds, without the level difference. The quickest way to learn what each setting does.") };
        g.setColour (kText); g.setFont (font (19.f, true)); g.drawText (title[step], c.removeFromTop (26), juce::Justification::left);
        g.setColour (kText.withAlpha (0.86f)); g.setFont (font (13.f)); g.drawFittedText (body[step], c.removeFromTop (60), juce::Justification::topLeft, 4);
        for (int i = 0; i < 3; ++i) { g.setColour (i == step ? kAmber : kLine2); g.fillRoundedRectangle ((float) card.getX() + 18 + i * 12 + (i > step ? 10 : 0), (float) card.getBottom() - 30, i == step ? 18.f : 6.f, 6.f, 3.f); }
    }
    void mouseDown (const juce::MouseEvent&) override {}      // bloqueia o resto enquanto a visita esta aberta
private:
    void layout()
    {
        const auto t = targets[(size_t) step];
        const int cw = 340, ch = 186;
        int x = (int) t.getRight() + 24, y = (int) t.getY();
        if (x + cw > getWidth() - 16) x = (int) t.getX() - cw - 24;
        y = juce::jlimit (16, getHeight() - ch - 16, y);
        card = { x, y, cw, ch };
        next.setButtonText (step < 2 ? L ("Seguinte", "Next") : L ("Começar", "Start")); skip.setButtonText (L ("Saltar", "Skip"));
        next.setBounds (card.getRight() - 18 - 110, card.getBottom() - 42, 110, 30); skip.setBounds (next.getX() - 90, next.getY(), 82, 30);
        repaint();
    }
    void finish() { setVisible (false); if (onFinish) onFinish(); }
    int step = 0; juce::Rectangle<int> card; juce::TextButton next, skip;
};
} // namespace bxui
