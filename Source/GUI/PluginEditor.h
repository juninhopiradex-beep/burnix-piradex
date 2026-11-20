// =============================================================================
//  BURNIX by PIRADEX — editor "Orbita v2": cabecalho, anel (OrbitView), painel da direita e login.
//  Tamanho de base 1200 x 740; escala 75–200 % (menu de definicoes ou arrastar o canto), mantendo a proporcao.
// =============================================================================
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Theme.h"
#include "OrbitView.h"
#include "Panels.h"
#include "../Plugin/PluginProcessor.h"
#include "../Plugin/Access.h"

namespace bxui
{
class IconButton : public juce::Button
{
public:
    enum Kind { Prev, Next, Undo, Redo, Gear };
    IconButton (Kind k, const juce::String& tip) : juce::Button (tip), kind (k) { setTooltip (tip); setWantsKeyboardFocus (true); }
    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (hasKeyboardFocus (false) ? kCyan : (over ? kDim : kLine2)); g.drawRoundedRectangle (r, r.getHeight() / 2, 1.f);
        const auto c = r.getCentre(); juce::Path p; g.setColour (kDim);
        switch (kind)
        {
            case Prev: p.startNewSubPath (c.x + 3, c.y - 5); p.lineTo (c.x - 2, c.y); p.lineTo (c.x + 3, c.y + 5); break;
            case Next: p.startNewSubPath (c.x - 3, c.y - 5); p.lineTo (c.x + 2, c.y); p.lineTo (c.x - 3, c.y + 5); break;
            case Undo: p.addCentredArc (c.x + 1, c.y + 1, 5, 5, 0, -2.6f, 1.3f, true); p.startNewSubPath (c.x - 6, c.y - 3); p.lineTo (c.x - 4, c.y + 1); p.lineTo (c.x - 1, c.y - 2); break;
            case Redo: p.addCentredArc (c.x - 1, c.y + 1, 5, 5, 0, 2.6f, -1.3f, true); p.startNewSubPath (c.x + 6, c.y - 3); p.lineTo (c.x + 4, c.y + 1); p.lineTo (c.x + 1, c.y - 2); break;
            case Gear: p.addEllipse (c.x - 3.5f, c.y - 3.5f, 7, 7); for (int i = 0; i < 8; ++i) { const float a = i * juce::MathConstants<float>::pi / 4; p.startNewSubPath (c.x + 5.5f * std::cos (a), c.y + 5.5f * std::sin (a)); p.lineTo (c.x + 7.5f * std::cos (a), c.y + 7.5f * std::sin (a)); } break;
        }
        g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
private:
    Kind kind;
};

// aplica o esquema de cores e o idioma guardados ANTES de os widgets do editor serem construidos
struct UiInit { UiInit() { applySkin (AccessManager::get().skin()); gLang = AccessManager::get().language(); } };

class LoginOverlay : public juce::Component
{
public:
    explicit LoginOverlay (std::function<void()> ok);
    void relabel();
    void resized() override;
    void paint (juce::Graphics&) override;
private:
    void submit();
    std::function<void()> onOk; juce::TextEditor pw; juce::ToggleButton remember; juce::TextButton enter; juce::Label msg; bool busy = false;
};

class Content : public juce::Component
{
public:
    void paint (juce::Graphics& g) override
    {
        g.fillAll (kBg); g.setColour (kHead); g.drawHorizontalLine (63, 0.f, (float) getWidth());
        drawLogo (g, 24.f, 40.f, 21.f, false, true);
    }
};
} // namespace bxui

class BurnixEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BurnixEditor (BurnixProcessor&);
    ~BurnixEditor() override;
    void paint (juce::Graphics&) override {}
    void resized() override;
    void setView (int v);
    void setBand (int b);
    void setScale (float s);
    // testes
    bxui::OrbitView& orbit() { return orbitView; }
    void pump() { timerCallback(); }
    bool loginVisible() const { return login.isVisible(); }
    void showLoginForTest() { login.setVisible (true); }
    void testSkin (int i) { applySkinChoice (i); }
    void testLang (int l) { applyLanguage (l); }

private:
    void timerCallback() override;
    void presetMenu();
    void settingsMenu();
    void qualityMenu();
    void updateHeader();
    void runMeasure (bool asRef);
    void applyLanguage (int lang);
    void applySkinChoice (int skin);
    void setFps (int fps);
    void setTips (bool on);

    BurnixProcessor& proc;
    bxui::UiInit uiInit;
    bxui::LnF lnf;
    bxui::Content content;
    std::unique_ptr<juce::TooltipWindow> tips;
    bxui::Segments tabs { { "PERFORM", "SCULPT", "ENGINEER" } };
    bxui::IconButton prev { bxui::IconButton::Prev, "Preset anterior" }, next { bxui::IconButton::Next, "Preset seguinte" },
                     undoB { bxui::IconButton::Undo, "Desfazer" }, redoB { bxui::IconButton::Redo, "Refazer" }, gear { bxui::IconButton::Gear, T8 ("Definições: escala da janela, sessão, sobre") };
    juce::TextButton presetB, abB { "A" }, qualityB;
    bxui::ToggleParam bypass { "BYPASS", bxui::kAmber };
    bxui::OrbitView orbitView;
    bxui::PerformPanel perform; bxui::SculptPanel sculpt; bxui::EngineerPanel engineer;
    bxui::LoginOverlay login;
    juce::dsp::FFT fft { 11 }; std::array<float, 4096> fftBuf {}; std::array<float, 2048> window {};
    BurnixProcessor::HarmReport report, ref; std::atomic<bool> measuring { false };
    int view = 0, band = 0, ticks = 0, fps = 30; float scale = 1.f; bool showSpectrum = true;
    juce::ComponentBoundsConstrainer constrainer;
    static juce::String T8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BurnixEditor)
};
