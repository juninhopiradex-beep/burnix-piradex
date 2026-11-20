// =============================================================================
//  BURNIX by PIRADEX — navegador de presets (1.3): pesquisa, favoritos, categorias, intensidade, descricao,
//  carregar ou carregar no B para comparar. Os favoritos ficam em <pasta do utilizador>/favoritos.txt.
// =============================================================================
#pragma once
#include "Theme.h"
#include "Panels.h"
#include "../Plugin/AppPaths.h"

namespace bxui
{
class PresetBrowser : public juce::Component, private juce::ListBoxModel, private juce::TextEditor::Listener
{
public:
    explicit PresetBrowser (BurnixProcessor& pr) : p (pr)
    {
        search.setTextToShowWhenEmpty (L ("Pesquisar presets", "Search presets"), kDim); search.addListener (this);
        search.setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack); search.setColour (juce::TextEditor::outlineColourId, kLine2);
        search.setColour (juce::TextEditor::focusedOutlineColourId, kCyan); search.setColour (juce::TextEditor::textColourId, kText); search.setFont (font (14.f)); search.setJustification (juce::Justification::centredLeft); search.setIndents (14, 0);
        list.setModel (this); list.setRowHeight (38); list.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack); list.setOutlineThickness (0);
        stylePill (loadB, kAmber, kBg); stylePill (loadToB); stylePill (favB); stylePill (closeB); stylePill (menuB);
        menuB.onClick = [this] { if (onMenu) onMenu(); };
        loadB.onClick = [this] { load (false); }; loadToB.onClick = [this] { load (true); }; favB.onClick = [this] { toggleFav(); };
        closeB.onClick = [this] { if (onClose) onClose(); };
        for (auto* c : std::initializer_list<juce::Component*> { &search, &list, &loadB, &loadToB, &favB, &closeB, &menuB }) addAndMakeVisible (c);
        setWantsKeyboardFocus (true);
    }
    std::function<void()> onClose, onLoaded, onMenu;

    void open()
    {
        loadFavs(); build(); relabel(); filter(); search.grabKeyboardFocus();
        const int cur = p.currentFactoryIndex();
        for (int i = 0; i < (int) shown.size(); ++i) if (shown[(size_t) i]->factory == cur && cur >= 0) { list.selectRow (i); list.scrollToEnsureRowIsOnscreen (i); }
    }
    void relabel()
    {
        loadB.setButtonText (L ("CARREGAR", "LOAD")); loadToB.setButtonText (L ("CARREGAR NO B (comparar)", "LOAD INTO B (compare)"));
        closeB.setButtonText (T8 ("✕")); closeB.setTooltip (L ("Fechar (Esc)", "Close (Esc)"));
        menuB.setButtonText (L ("GUARDAR · PASTA", "SAVE · FOLDER")); menuB.setTooltip (L ("Guardar o preset atual ou abrir a pasta dos presets", "Save the current preset or open the presets folder"));
        buildChips(); resized(); repaint();
    }

    void resized() override
    {
        panel = getLocalBounds().reduced (50, 40);
        auto r = panel.reduced (24, 22);
        auto top = r.removeFromTop (38); closeB.setBounds (top.removeFromRight (38)); top.removeFromRight (10); search.setBounds (top.removeFromRight (380)); top.removeFromRight (10); menuB.setBounds (top.removeFromRight (150));
        r.removeFromTop (12);
        // fichas de categoria em linhas (quebra automatica)
        int x = r.getX(), y = r.getY(); const int h = 28;
        for (auto* c : catChips)
        {
            const int w = (int) juce::GlyphArrangement::getStringWidth (font (12.f, true), c->getButtonText()) + 26;
            if (x + w > r.getRight()) { x = r.getX(); y += h + 6; }
            c->setBounds (x, y, w, h); x += w + 6;
        }
        y += h + 10; x = r.getX() + 96;
        for (auto* c : intChips) { const int w = (int) juce::GlyphArrangement::getStringWidth (font (12.f, true), c->getButtonText()) + 26; c->setBounds (x, y, w, h); x += w + 6; }
        intY = y; r.setTop (y + h + 14);
        auto right = r.removeFromRight (340); r.removeFromRight (20);
        listArea = r; list.setBounds (r.withTrimmedTop (20));
        detail = right;
        auto b = right.reduced (16).removeFromBottom (40 * 3 + 16);
        loadB.setBounds (b.removeFromTop (40)); b.removeFromTop (8); loadToB.setBounds (b.removeFromTop (40)); b.removeFromTop (8); favB.setBounds (b.removeFromTop (40));
    }
    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff06070f).withAlpha (0.72f));
        g.setColour (kDisc); g.fillRoundedRectangle (panel.toFloat(), 16.f); g.setColour (kLine2); g.drawRoundedRectangle (panel.toFloat().reduced (0.5f), 16.f, 1.f);
        auto r = panel.reduced (24, 22);
        g.setColour (kText); g.setFont (font (18.f, true)); g.drawText ("PRESETS", r.getX(), r.getY() + 8, 120, 22, juce::Justification::left);
        g.setColour (kDim); g.setFont (font (12.f));
        g.drawText (juce::String ((int) shown.size()) + L (" de ", " of ") + juce::String ((int) all.size()), r.getX() + 120, r.getY() + 10, 200, 20, juce::Justification::left);
        g.setColour (kDim); g.setFont (font (10.f, true)); g.drawText (L ("INTENSIDADE", "INTENSITY"), r.getX(), intY + 7, 92, 14, juce::Justification::left);
        g.drawText (L ("NOME", "NAME"), listArea.getX() + 50, listArea.getY(), 200, 14, juce::Justification::left);
        g.drawText (L ("CATEGORIA", "CATEGORY"), listArea.getRight() - 210, listArea.getY(), 120, 14, juce::Justification::left);
        g.drawText (L ("NÍVEL", "LEVEL"), listArea.getRight() - 70, listArea.getY(), 60, 14, juce::Justification::left);
        // detalhe
        g.setColour (kLine2); g.drawRoundedRectangle (detail.toFloat().reduced (0.5f), 12.f, 1.f);
        if (auto* it = current())
        {
            auto d = detail.reduced (18);
            g.setColour (kDim); g.setFont (font (10.f, true)); g.drawText ((it->catLabel + (it->factory >= 0 ? L (" · FÁBRICA", " · FACTORY") : L (" · MEU", " · MINE"))).toUpperCase(), d.removeFromTop (16), juce::Justification::left);
            g.setColour (kText); g.setFont (font (22.f, true)); g.drawFittedText (it->name, d.removeFromTop (56), juce::Justification::topLeft, 2);
            g.setColour (kText.withAlpha (0.86f)); g.setFont (font (13.f));
            g.drawFittedText (it->desc.isNotEmpty() ? it->desc : L ("Preset guardado por ti.", "Preset you saved."), d.removeFromTop (60), juce::Justification::topLeft, 4);
            d.removeFromTop (6);
            int x = d.getX(), y = d.getY();
            for (auto& t : it->tags)
            {
                const int w = (int) juce::GlyphArrangement::getStringWidth (font (11.f, true), t) + 18;
                if (x + w > d.getRight()) { x = d.getX(); y += 26; }
                g.setColour (kLine2); g.drawRoundedRectangle ((float) x, (float) y, (float) w, 20.f, 10.f, 1.f);
                g.setColour (kDim); g.setFont (font (11.f, true)); g.drawText (t, x, y, w, 20, juce::Justification::centred); x += w + 6;
            }
            g.setColour (kDim); g.setFont (font (12.f));
            const juce::String lv[3] { L ("Subtil", "Subtle"), L ("Média", "Medium"), L ("Forte", "Strong") };
            g.drawText (L ("Intensidade: ", "Intensity: ") + lv[juce::jlimit (0, 2, it->intensity - 1)], d.getX(), y + 32, d.getWidth(), 18, juce::Justification::left);
        }
    }
    bool keyPressed (const juce::KeyPress& k) override
    {
        if (k == juce::KeyPress::escapeKey) { if (onClose) onClose(); return true; }
        if (k == juce::KeyPress::returnKey) { load (false); return true; }
        if (k == juce::KeyPress::downKey) { list.selectRow (juce::jmin ((int) shown.size() - 1, list.getSelectedRow() + 1)); return true; }
        if (k == juce::KeyPress::upKey)   { list.selectRow (juce::jmax (0, list.getSelectedRow() - 1)); return true; }
        return false;
    }

private:
    struct Item { int factory = -1; juce::File file; juce::String name, cat, catLabel, desc; int intensity = 1; bool fav = false; juce::StringArray tags; };
    BurnixProcessor& p;
    juce::TextEditor search; juce::ListBox list;
    juce::TextButton loadB, loadToB, favB, closeB, menuB;
    juce::OwnedArray<juce::TextButton> catChips, intChips;
    std::vector<std::unique_ptr<Item>> all; std::vector<Item*> shown;
    juce::String catFilter = "*"; int intFilter = 0, intY = 0;
    juce::Rectangle<int> panel, listArea, detail;
    juce::StringArray favs;

    static juce::String catName (const juce::String& c)
    {
        static const char* cat[][3] { { "Base", "Base", "Base" }, { "Voz", "Voz", "Vocals" }, { "Rap & Trap", "Rap & Trap", "Rap & Trap" }, { "Coros", "Coros", "Backing vocals" },
            { "Baixo", "Baixo", "Bass" }, { "808 & Sub", "808 & Sub", "808 & Sub" }, { "Kick", "Kick", "Kick" }, { "Snare & Clap", "Snare & Clap", "Snare & Clap" },
            { "Bateria", "Bateria", "Drums" }, { "Percussao", "Percuss\xc3\xa3o", "Percussion" }, { "Guitarra", "Guitarra", "Guitar" }, { "Teclas", "Teclas", "Keys" },
            { "Synths", "Synths", "Synths" }, { "Mix Bus", "Mix Bus", "Mix Bus" }, { "Mistura", "Mistura", "Mixing" }, { "Master", "Master", "Master" },
            { "Estereo", "Est\xc3\xa9reo", "Stereo" }, { "Afro & Angola", "Afro & Angola", "Afro & Angola" }, { "Criativo", "Criativo", "Creative" } };
        for (auto& e : cat) if (c == e[0]) return L (e[1], e[2]);
        return c;
    }
    juce::File favFile() const { return bxpaths::userDir().getChildFile ("favoritos.txt"); }
    void loadFavs() { favs.clear(); favs.addLines (favFile().loadFileAsString()); favs.removeEmptyStrings(); }
    void saveFavs() { favFile().replaceWithText (favs.joinIntoString ("\n")); }
    Item* current() { const int r = list.getSelectedRow(); return r >= 0 && r < (int) shown.size() ? shown[(size_t) r] : nullptr; }

    void build()
    {
        all.clear();
        const auto& fl = bxp::factoryPresets();
        for (int i = 0; i < (int) fl.size(); ++i)
        {
            const auto& pr = fl[(size_t) i]; auto it = std::make_unique<Item>();
            it->factory = i; it->name = pr.name; it->cat = pr.category; it->catLabel = catName (pr.category);
            it->desc = juce::String (juce::CharPointer_UTF8 (gLang == 1 ? pr.purposeEn : pr.purpose));
            // intensidade e etiquetas a partir dos valores (o que nao esta listado fica no valor por omissao)
            auto val = [&pr] (const juce::String& id, float def) { for (auto& [k, v] : pr.values) if (id == k) return v; return def; };
            const int nb = (int) val ("numBands", 4); const float heat = val ("heat", 5.f) / 5.f; float mx = 0; bool ms = false, par = false, fire = false;
            juce::StringArray voices;
            for (int b = 0; b < nb; ++b)
            {
                const float dm = val (bxp::bandId (b, "driveM"), 3.f), ds = val (bxp::bandId (b, "driveS"), 3.f); const int rt = (int) val (bxp::bandId (b, "route"), 0.f);
                mx = juce::jmax (mx, (rt == 2 ? ds : juce::jmax (dm, rt >= 2 ? ds : 0.f)) * heat);
                ms = ms || rt != 0; par = par || val (bxp::bandId (b, "mode"), 0.f) > 0.5f;
                const int v = (int) val (bxp::bandId (b, "voice"), 0.f); if (v > 0 && (dm > 0 || ds > 0)) { voices.addIfNotAlreadyThere (bx::voicingName (v - 1)); fire = fire || (v == 6 && dm >= 4); }
            }
            it->intensity = (fire || mx >= 6.5f || val ("bias", 0) > 50) ? 3 : (mx >= 3.6f ? 2 : 1);
            it->tags.add (it->catLabel); it->tags.addArray (voices); if (ms) it->tags.add ("M/S"); if (par) it->tags.add ("PARALLEL");
            if (val ("quality", 1) >= 2) it->tags.add (val ("quality", 1) >= 3 ? "ULTRA 16x" : "HQ 8x");
            if (val ("delta", 0) > 0.5f) it->tags.add ("DELTA");
            it->fav = favs.contains (it->name);
            all.push_back (std::move (it));
        }
        for (auto& f : p.userPresets())
        {
            auto it = std::make_unique<Item>(); it->file = f; it->name = f.getFileNameWithoutExtension(); it->cat = "#mine"; it->catLabel = L ("Meus", "Mine");
            it->tags.add (it->catLabel); it->intensity = 2; it->fav = favs.contains (it->name); all.push_back (std::move (it));
        }
    }
    void buildChips()
    {
        catChips.clear(); intChips.clear();
        juce::StringArray keys { "*", "#fav", "#mine" }, labels { L ("Todos", "All"), T8 ("★ ") + L ("Favoritos", "Favourites"), L ("Meus", "Mine") };
        for (auto& pr : bxp::factoryPresets()) if (! keys.contains (pr.category)) { keys.add (pr.category); labels.add (catName (pr.category)); }
        for (int i = 0; i < keys.size(); ++i)
        {
            auto* c = catChips.add (new juce::TextButton (labels[i])); stylePill (*c); const auto k = keys[i];
            c->setToggleState (k == catFilter, juce::dontSendNotification);
            c->onClick = [this, k] { catFilter = k; for (auto* b : catChips) b->setToggleState (b == findChip (k), juce::dontSendNotification); filter(); };
            c->getProperties().set ("key", k); addAndMakeVisible (c);
        }
        const juce::String il[4] { L ("Qualquer", "Any"), L ("Subtil", "Subtle"), L ("Média", "Medium"), L ("Forte", "Strong") };
        for (int i = 0; i < 4; ++i)
        {
            auto* c = intChips.add (new juce::TextButton (il[i])); stylePill (*c); c->setToggleState (i == intFilter, juce::dontSendNotification);
            c->onClick = [this, i] { intFilter = i; for (int j = 0; j < intChips.size(); ++j) intChips[j]->setToggleState (j == i, juce::dontSendNotification); filter(); };
            addAndMakeVisible (c);
        }
    }
    juce::TextButton* findChip (const juce::String& k) { for (auto* c : catChips) if (c->getProperties()["key"].toString() == k) return c; return nullptr; }
    void filter()
    {
        const auto q = search.getText().trim().toLowerCase();
        shown.clear();
        for (auto& it : all)
        {
            if (catFilter == "#fav" && ! it->fav) continue;
            if (catFilter == "#mine" && it->factory >= 0) continue;
            if (catFilter != "*" && ! catFilter.startsWithChar ('#') && it->cat != catFilter) continue;
            if (intFilter > 0 && it->intensity != intFilter) continue;
            if (q.isNotEmpty() && ! (it->name.toLowerCase().contains (q) || it->desc.toLowerCase().contains (q) || it->catLabel.toLowerCase().contains (q) || it->tags.joinIntoString (" ").toLowerCase().contains (q))) continue;
            shown.push_back (it.get());
        }
        list.updateContent(); if (! shown.empty() && list.getSelectedRow() < 0) list.selectRow (0);
        updateFavButton(); repaint();
    }
    void updateFavButton()
    {
        auto* it = current(); favB.setEnabled (it != nullptr); loadB.setEnabled (it != nullptr); loadToB.setEnabled (it != nullptr);
        favB.setButtonText (it != nullptr && it->fav ? T8 ("★ ") + L ("TIRAR DOS FAVORITOS", "REMOVE FAVOURITE") : T8 ("☆ ") + L ("FAVORITO", "FAVOURITE"));
    }
    void toggleFav()
    {
        if (auto* it = current()) { it->fav = ! it->fav; if (it->fav) favs.addIfNotAlreadyThere (it->name); else favs.removeString (it->name); saveFavs(); updateFavButton(); list.repaint(); }
    }
    void load (bool intoB)
    {
        auto* it = current(); if (it == nullptr) return;
        if (it->factory >= 0) { if (intoB) p.loadPresetIntoB (it->factory); else p.applyPreset (it->factory); }
        else { if (intoB && p.abSlot() == 0) { p.storeAB (0); p.toggleAB(); } p.loadUserPreset (it->file); }
        if (onLoaded) onLoaded();
        if (onClose) onClose();
    }

    // ListBoxModel
    int getNumRows() override { return (int) shown.size(); }
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool sel) override
    {
        if (row < 0 || row >= (int) shown.size()) return;
        const auto* it = shown[(size_t) row];
        if (sel) { g.setColour (kLine); g.fillRoundedRectangle (0.f, 2.f, (float) w, (float) h - 4, 8.f); }
        g.setColour (it->fav ? kAmber : kLine2); g.setFont (font (15.f)); g.drawText (it->fav ? T8 ("★") : T8 ("☆"), 10, 0, 26, h, juce::Justification::centred);
        g.setColour (kText); g.setFont (font (13.5f, sel)); g.drawText (it->name, 50, 0, w - 300, h, juce::Justification::centredLeft);
        const int tw = (int) juce::GlyphArrangement::getStringWidth (font (11.f, true), it->catLabel) + 18;
        g.setColour (kLine2); g.drawRoundedRectangle ((float) w - 210, h * 0.5f - 10, (float) tw, 20.f, 10.f, 1.f);
        g.setColour (kDim); g.setFont (font (11.f, true)); g.drawText (it->catLabel, w - 210, (int) (h * 0.5f) - 10, tw, 20, juce::Justification::centred);
        for (int i = 0; i < 3; ++i) { g.setColour (i < it->intensity ? kCoral : kLine2); g.fillEllipse ((float) w - 66 + i * 12, h * 0.5f - 4, 8, 8); }
    }
    void listBoxItemClicked (int, const juce::MouseEvent& e) override { if (e.x < 40) toggleFav(); updateFavButton(); repaint(); }
    void listBoxItemDoubleClicked (int, const juce::MouseEvent&) override { load (false); }
    void selectedRowsChanged (int) override { updateFavButton(); repaint(); }
    void textEditorTextChanged (juce::TextEditor&) override { filter(); }
    void textEditorReturnKeyPressed (juce::TextEditor&) override { load (false); }
    void textEditorEscapeKeyPressed (juce::TextEditor&) override { if (onClose) onClose(); }
};
} // namespace bxui
