#include "PluginEditor.h"
#include "../Plugin/AppPaths.h"

using namespace bxui;

// ------------------------------------------------------------------------------------------------ login
LoginOverlay::LoginOverlay (std::function<void()> ok) : onOk (std::move (ok))
{
    pw.setPasswordCharacter ((juce::juce_wchar) 0x2022); pw.setTextToShowWhenEmpty (T8 ("Senha de acesso"), kFaint); pw.setJustification (juce::Justification::centred);
    pw.setFont (font (16.f)); pw.onReturnKey = [this] { submit(); };
    remember.setToggleState (true, juce::dontSendNotification); remember.setButtonText (T8 ("Lembrar neste computador"));
    enter.setButtonText ("ENTRAR"); stylePill (enter, kCoral, kBg); enter.setToggleState (true, juce::dontSendNotification); enter.onClick = [this] { submit(); };
    msg.setJustificationType (juce::Justification::centred); msg.setFont (font (12.f)); msg.setColour (juce::Label::textColourId, kCoral);
    addAndMakeVisible (pw); addAndMakeVisible (remember); addAndMakeVisible (enter); addAndMakeVisible (msg);
}
void LoginOverlay::resized()
{
    auto r = getLocalBounds().withSizeKeepingCentre (400, 350).reduced (34, 0);
    pw.setBounds (r.getX(), r.getY() + 150, r.getWidth(), 40); remember.setBounds (r.getX(), r.getY() + 198, r.getWidth(), 26);
    enter.setBounds (r.getX(), r.getY() + 234, r.getWidth(), 40); msg.setBounds (r.getX() - 20, r.getY() + 280, r.getWidth() + 40, 34);
}
void LoginOverlay::paint (juce::Graphics& g)
{
    g.fillAll (kBg.withAlpha (0.96f));
    auto card = getLocalBounds().withSizeKeepingCentre (400, 350).toFloat();
    g.setColour (kDisc); g.fillRoundedRectangle (card, 20.f); g.setColour (kLine2); g.drawRoundedRectangle (card.reduced (0.5f), 20.f, 1.f);
    // anel de assinatura
    const auto c = card.getCentre().withY (card.getY() + 58);
    juce::Path ring; ring.addCentredArc (c.x, c.y, 26, 26, 0, juce::degreesToRadians (-135.f), juce::degreesToRadians (135.f), true);
    g.setGradientFill (juce::ColourGradient (kCyan, c.x - 26, c.y, kViolet, c.x + 26, c.y, false)); g.strokePath (ring, juce::PathStrokeType (5.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (kCoral); g.fillEllipse (c.x - 6, c.y - 6, 12, 12);
    g.setColour (kText); g.setFont (font (26.f, true)); g.drawText ("BURNIX", card.withTrimmedTop (90).withHeight (30).toNearestInt(), juce::Justification::centred);
    g.setColour (kDim); g.setFont (font (11.f)); g.drawText (T8 ("by PIRADEX  ·  Harmonic Color Engine"), card.withTrimmedTop (118).withHeight (16).toNearestInt(), juce::Justification::centred);
}
void LoginOverlay::submit()
{
    if (busy) return;
    const juce::String pass = pw.getText();
    if (pass.trim().isEmpty()) { msg.setText (T8 ("Escreva a senha."), juce::dontSendNotification); return; }
    busy = true; enter.setEnabled (false); msg.setColour (juce::Label::textColourId, kDim); msg.setText (T8 ("A verificar..."), juce::dontSendNotification);
    const bool rem = remember.getToggleState(); juce::Component::SafePointer<LoginOverlay> safe (this);
    juce::Thread::launch ([safe, pass, rem]
    {
        const auto m = AccessManager::matchPassword (pass);                 // PBKDF2 fora da thread de mensagens
        AccessManager::PccOutcome po = AccessManager::PccOutcome::Disabled; juce::String pmsg;
        if (m.index < 0 && ! m.expired && AccessManager::get().pccConfigured()) po = AccessManager::get().loginPcc (pass, rem, pmsg);
        juce::MessageManager::callAsync ([safe, m, rem, po, pmsg]
        {
            if (safe == nullptr) return;
            safe->busy = false; safe->enter.setEnabled (true); safe->msg.setColour (juce::Label::textColourId, kCoral);
            if (po == AccessManager::PccOutcome::Ok) { safe->pw.clear(); safe->msg.setText ({}, juce::dontSendNotification); if (safe->onOk) safe->onOk(); return; }
            if (po == AccessManager::PccOutcome::Unreachable) { safe->msg.setText (T8 ("Sem ligação ao servidor de licenças. Verifique a internet."), juce::dontSendNotification); return; }
            if (po == AccessManager::PccOutcome::BadReply)    { safe->msg.setText (T8 ("Resposta do servidor não reconhecida."), juce::dontSendNotification); return; }
            if (po == AccessManager::PccOutcome::Invalid)     { safe->msg.setText (pmsg.isNotEmpty() ? pmsg : T8 ("Licença inválida, expirada ou desativada."), juce::dontSendNotification); return; }
            const auto res = AccessManager::get().applyLogin (m, rem);
            if (res == AccessManager::LoginResult::Ok) { safe->pw.clear(); safe->msg.setText ({}, juce::dontSendNotification); if (safe->onOk) safe->onOk(); }
            else if (res == AccessManager::LoginResult::Expired) safe->msg.setText (T8 ("Esta senha beta já expirou."), juce::dontSendNotification);
            else safe->msg.setText (T8 ("Senha incorreta."), juce::dontSendNotification);
        });
    });
}

// ------------------------------------------------------------------------------------------------ editor
BurnixEditor::BurnixEditor (BurnixProcessor& p)
    : AudioProcessorEditor (p), proc (p), orbitView (p), perform (p), sculpt (p), engineer (p),
      login ([this] { login.setVisible (false); })
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);
    for (auto* c : std::initializer_list<juce::Component*> { &tabs, &prev, &presetB, &next, &abB, &undoB, &redoB, &qualityB, &bypass, &gear, &orbitView, &perform, &sculpt, &engineer, &login })
        content.addAndMakeVisible (c);
    tabs.onPick = [this] (int v) { setView (v); };
    stylePill (presetB); stylePill (abB, kText, kBg); stylePill (qualityB, kCyan, kBg); qualityB.setToggleState (true, juce::dontSendNotification);
    presetB.onClick = [this] { presetMenu(); };
    prev.onClick = [this] { const int n = proc.getNumPrograms(); proc.applyPreset ((proc.getCurrentProgram() + n - 1) % n); updateHeader(); };
    next.onClick = [this] { const int n = proc.getNumPrograms(); proc.applyPreset ((proc.getCurrentProgram() + 1) % n); updateHeader(); };
    abB.onClick = [this] { proc.toggleAB(); updateHeader(); };
    abB.setTooltip (T8 ("Comparar duas regulações (A/B). Ligue o AUTO GAIN para comparar ao mesmo volume."));
    undoB.onClick = [this] { proc.undo.undo(); }; redoB.onClick = [this] { proc.undo.redo(); };
    qualityB.onClick = [this] { qualityMenu(); };
    gear.onClick = [this] { settingsMenu(); };
    bypass.attach (proc.apvts, "bypass");
    bypass.setTooltip (T8 ("Bypass real: a entrada passa intacta, alinhada pela latência."));
    orbitView.getBand = [this] { return band; };
    orbitView.setBand = [this] (int b) { setBand (b); };
    sculpt.onBand = [this] (int b) { setBand (b); };
    engineer.onMeasure = [this] { runMeasure (false); };
    engineer.onKeepRef = [this] { runMeasure (true); };
    engineer.report = &report;

    for (int i = 0; i < 2048; ++i) window[(size_t) i] = 0.5f - 0.5f * std::cos (2.f * juce::MathConstants<float>::pi * i / 2047.f);

    login.setVisible (! AccessManager::get().isUnlocked());
    juce::Thread::launch ([] { AccessManager::get().revalidatePcc(); });

    constrainer.setFixedAspectRatio (1200.0 / 740.0); constrainer.setSizeLimits (900, 555, 2400, 1480);
    setConstrainer (&constrainer); setResizable (true, true);
    view = juce::jlimit (0, 2, proc.uiView); band = juce::jlimit (0, bx::kMaxBands - 1, proc.uiBand - 1);
    setView (view); setBand (band);
    setScale (juce::jlimit (0.75f, 2.0f, proc.uiScale));
    updateHeader();
    startTimerHz (30);
}

BurnixEditor::~BurnixEditor() { stopTimer(); setLookAndFeel (nullptr); }

void BurnixEditor::setScale (float s)
{
    scale = juce::jlimit (0.75f, 2.0f, s); proc.uiScale = scale;
    setSize (juce::roundToInt (1200 * scale), juce::roundToInt (740 * scale));
}

void BurnixEditor::resized()
{
    scale = getWidth() / 1200.f; proc.uiScale = scale;
    content.setBounds (0, 0, 1200, 740); content.setTransform (juce::AffineTransform::scale (scale));
    // cabecalho
    tabs.setBounds (300, 17, 330, 30);
    prev.setBounds (650, 17, 30, 30); presetB.setBounds (684, 17, 180, 30); next.setBounds (868, 17, 30, 30);
    abB.setBounds (906, 17, 40, 30); undoB.setBounds (952, 17, 30, 30); redoB.setBounds (986, 17, 30, 30);
    qualityB.setBounds (1022, 17, 64, 30); bypass.setBounds (1092, 17, 66, 30); gear.setBounds (1162, 17, 30, 30);
    orbitView.setBounds (0, 64, 780, 676);
    for (auto* pnl : std::initializer_list<juce::Component*> { &perform, &sculpt, &engineer }) pnl->setBounds (800, 86, 372, 632);
    login.setBounds (0, 0, 1200, 740);
}

void BurnixEditor::setView (int v)
{
    view = juce::jlimit (0, 2, v); proc.uiView = view;
    tabs.setSelected (view); orbitView.view = view;
    perform.setVisible (view == 0); sculpt.setVisible (view == 1); engineer.setVisible (view == 2);
    if (view == 2 && ! report.valid) runMeasure (false);
    orbitView.repaint();
}

void BurnixEditor::setBand (int b)
{
    const int nb = (int) proc.apvts.getRawParameterValue ("numBands")->load();
    band = juce::jlimit (0, nb - 1, b); proc.uiBand = band + 1;
    sculpt.setBand (band); orbitView.repaint();
}

void BurnixEditor::updateHeader()
{
    presetB.setButtonText (proc.currentPresetName());
    abB.setButtonText (proc.abSlot() == 0 ? "A" : "B");
    qualityB.setButtonText (bxp::qualityChoices()[(int) std::lround (proc.apvts.getRawParameterValue ("quality")->load())].upToFirstOccurrenceOf (" ", false, false)
                            + " " + juce::String (bxp::qualityFactor ((int) std::lround (proc.apvts.getRawParameterValue ("quality")->load()))) + "x");
    content.repaint (0, 0, 300, 64);
}

void BurnixEditor::presetMenu()
{
    juce::PopupMenu m, fac; juce::StringArray cats;
    const auto& list = bxp::factoryPresets();
    for (auto& pr : list) cats.addIfNotAlreadyThere (pr.category);
    for (auto& c : cats)
    {
        juce::PopupMenu sub;
        for (int i = 0; i < (int) list.size(); ++i) if (c == list[(size_t) i].category) sub.addItem (1 + i, list[(size_t) i].name, true, i == proc.getCurrentProgram());
        fac.addSubMenu (c, sub);
    }
    m.addSectionHeader (T8 ("PRESETS DE FÁBRICA")); m.addSubMenu (T8 ("Fábrica"), fac);
    const auto users = proc.userPresets();
    juce::PopupMenu us; for (int i = 0; i < users.size(); ++i) us.addItem (1000 + i, users[i].getFileNameWithoutExtension());
    m.addSubMenu ("Meus presets", us, users.size() > 0);
    m.addSeparator(); m.addItem (2000, "Guardar preset...");
    m.addItem (2001, "Abrir a pasta dos presets");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetB), [this, users] (int r)
    {
        if (r >= 1 && r < 1000) proc.applyPreset (r - 1);
        else if (r >= 1000 && r < 2000) proc.loadUserPreset (users[r - 1000]);
        else if (r == 2001) bxpaths::userPresetDir().startAsProcess();
        else if (r == 2000)
        {
            auto* w = new juce::AlertWindow ("Guardar preset", "Nome do preset:", juce::MessageBoxIconType::NoIcon);
            w->addTextEditor ("name", proc.currentPresetName()); w->addButton ("Guardar", 1, juce::KeyPress (juce::KeyPress::returnKey)); w->addButton ("Cancelar", 0, juce::KeyPress (juce::KeyPress::escapeKey));
            juce::Component::SafePointer<BurnixEditor> safe (this);
            w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int res)
            {
                if (safe != nullptr && res == 1) { const auto n = w->getTextEditorContents ("name").trim(); if (n.isNotEmpty()) safe->proc.saveUserPreset (n); safe->updateHeader(); }
            }), true);
        }
        updateHeader();
    });
}

void BurnixEditor::qualityMenu()
{
    juce::PopupMenu m; const int q = (int) std::lround (proc.apvts.getRawParameterValue ("quality")->load());
    const char* desc[] { "ECO 1x  (sem oversampling, latência 0)", "HQ 4x  (recomendado)", "HQ 8x  (master)", "ULTRA 16x  (render / máxima rejeição de aliasing)" };
    for (int i = 0; i < 4; ++i) m.addItem (1 + i, T8 (desc[i]), true, i == q);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (qualityB), [this] (int r) { if (r > 0) setParam (proc, "quality", (float) (r - 1)); updateHeader(); });
}

void BurnixEditor::settingsMenu()
{
    juce::PopupMenu m, sc;
    for (int pct : { 75, 100, 125, 150, 175, 200 }) sc.addItem (10 + pct, juce::String (pct) + " %", true, std::abs (scale * 100 - pct) < 2);
    m.addSubMenu ("Escala da janela", sc);
    m.addSeparator();
    const auto role = AccessManager::get().roleText();
    m.addItem (1, T8 ("Sessão: ") + (role.isNotEmpty() ? role : (AccessManager::inTrial() ? T8 ("período aberto até 31/12/2026") : T8 ("sem sessão"))), false);
    m.addItem (2, T8 ("Terminar sessão"), role.isNotEmpty());
    m.addSeparator();
    m.addItem (3, T8 ("Sobre o BURNIX..."));
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (gear), [this] (int r)
    {
        if (r > 10) setScale ((r - 10) / 100.f);
        else if (r == 2) { AccessManager::get().logout(); login.setVisible (! AccessManager::get().isUnlocked()); }
        else if (r == 3)
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::NoIcon, "BURNIX by PIRADEX " + juce::String (JucePlugin_VersionString),
                T8 ("Harmonic Color Engine: saturação de válvulas multibanda com Mid/Side.\n\nCriador e Desenvolvedor Ricardo Amorim / Juninho Piradex\nBeatfreak Music · www.beatfreakstudio.com\nSuporte: juninhopiradex@hotmail.com"));
    });
}

void BurnixEditor::runMeasure (bool asRef)
{
    if (measuring.exchange (true)) return;
    const auto params = proc.makeEngineParams(); juce::Component::SafePointer<BurnixEditor> safe (this);
    juce::Thread::launch ([safe, params, asRef]
    {
        const auto r = BurnixProcessor::measureHarmonics (params);
        juce::MessageManager::callAsync ([safe, r, asRef]
        {
            if (safe == nullptr) return;
            safe->measuring = false;
            if (asRef) safe->ref = r; else safe->report = r;
            if (asRef && ! safe->report.valid) safe->report = r;
            safe->orbitView.report = safe->report; safe->orbitView.ref = safe->ref;
            safe->orbitView.repaint(); safe->engineer.repaint();
        });
    });
}

void BurnixEditor::timerCallback()
{
    ++ticks;
    // espectro de saida -> 130 barras logaritmicas no anel
    if (view != 2 && proc.popSpectrum (fftBuf.data(), 2048))
    {
        for (int i = 0; i < 2048; ++i) fftBuf[(size_t) i] *= window[(size_t) i];
        std::fill (fftBuf.begin() + 2048, fftBuf.end(), 0.f);
        fft.performFrequencyOnlyForwardTransform (fftBuf.data());
        const double sr = juce::jmax (8000.0, proc.getSampleRate());
        for (int k = 0; k < (int) orbitView.spec.size(); ++k)
        {
            const double f1 = 20 * std::pow (1000.0, k / 130.0), f2 = 20 * std::pow (1000.0, (k + 1) / 130.0);
            int b1 = juce::jlimit (1, 1023, (int) (f1 / sr * 2048)), b2 = juce::jlimit (b1, 1023, (int) (f2 / sr * 2048));
            float m = 0; for (int b = b1; b <= b2; ++b) m = juce::jmax (m, fftBuf[(size_t) b]);
            const float db = juce::Decibels::gainToDecibels (m / 512.f, -100.f), v = juce::jlimit (0.f, 1.f, (db + 84.f) / 78.f);
            auto& s = orbitView.spec[(size_t) k]; s = v > s ? v : s * 0.86f + v * 0.14f;
        }
    }
    orbitView.repaint();
    if (ticks % 3 == 0)
    {
        perform.refresh(); sculpt.refresh(); engineer.refresh(); updateHeader();
        undoB.setEnabled (proc.undo.canUndo()); redoB.setEnabled (proc.undo.canRedo());
        const int nb = (int) proc.apvts.getRawParameterValue ("numBands")->load(); if (band >= nb) setBand (nb - 1);
    }
    if (ticks % 300 == 0) { AccessManager::get().checkExpiry(); login.setVisible (! AccessManager::get().isUnlocked()); }
    if (ticks % 30 == 0) proc.undo.beginNewTransaction();
}
