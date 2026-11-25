# BURNIX by PIRADEX — Manual (English)

**Harmonic Color Engine** · multiband tube saturation with Mid/Side · version 2.0.0
Created and developed by Ricardo Amorim / Juninho Piradex · Beatfreak Music · www.beatfreakstudio.com · juninhopiradex@hotmail.com

## Installation
* **macOS:** open `BURNIX-1.0.1-macOS-AppleSilicon.pkg` ou `BURNIX-1.0.1-macOS-Intel-macOS10.13.pkg` and pick VST3, Audio Unit and/or CLAP. Intel (10.13+) and Apple Silicon
  (or the single-architecture `...-Intel-macOS10.13.pkg` / `...-AppleSilicon.pkg`).
* **Windows:** run `BURNIX-1.0.1-Windows-Setup.exe` (64-bit VST3 and CLAP in `C:\Program Files\Common Files\VST3` and `\CLAP`).
* Restart your DAW and rescan. Since 1.0.1 BURNIX asks right away for the master password, the beta password (valid until 31 Dec 2026) or
  a licence key (validated online by PCC; works 14 days offline). Choose English in Settings ⚙ → Language.

## The idea
BURNIX splits the signal into 1–6 bands and saturates each one with its own tube character, on the centre (MID), the sides
(SIDE) or both. The ring shows it: **MID outside, SIDE inside**; thicker arc = more drive; brighter arc = more harmonics
actually being added.

## PERFORM
* **Drag an arc outwards** for more drive on that band, inwards for less. Double-click resets. Shift = fine.
* **Morph (centre):** drag the dot between four characters (TRIODE BLOOM, PENTODE EDGE, FIRESTORM, VELVET by default).
* **HEAT** scales every band's drive (5.0 = as set; 10 = double; 0 = clean). **DENSITY** adds sag. **COLOR** tilts the saturated
  signal, **AIR** adds 12 kHz sheen. **INPUT / OUTPUT / MIX**. **AUTO GAIN** (opt-in) matches output loudness to the input.
  **DELTA** plays only what BURNIX adds. Input (cyan) and output (coral) meters sit in the gap at the bottom of the ring.

## SCULPT
* White handles move the crossovers; click an arc (or 1–6) to select a band; − / + remove or add bands.
* Centre: the band's voicing (MORPH follows the global morph, or one of six fixed voicings).
* Route L/R, MID, SIDE or M+S; ON / SOLO; DRIVE MID / SIDE; MIX; selective filter (HP, LP, BP, NOTCH, SHELF) with FREQ and Q;
  INSERT (replaces the filtered part) or PARALLEL (adds the saturated branch, level goes up); 24 or 48 dB/oct crossovers.

## ENGINEER
* **MEDIR** (measure) analyses the current settings with a 1 kHz sine at −6 dBFS and plots the real H2–H10 on the radar;
  **GUARDAR REF.** keeps a dashed reference.
* Oversampling ECO 1x / HQ 4x / HQ 8x / ULTRA 16x, MIN-PHASE or LINEAR filters.
* Topology, STAGE BLEND, INTERSTAGE (second PENTODE stage), BIAS, TRANSIENT, BODY, WIDTH, MONO BASS.
* Live readouts: THD, latency, CPU, true peak, auto gain, oversampling factor.

## Header
Presets (160 factory presets in 19 categories, user presets, save), A/B, undo/redo, quality, true **BYPASS** (input passes untouched, latency-aligned).

## Settings (gear ⚙)
Language (Português / English) · Colours (Orbit, Ember, Ice, Indigo colour-blind safe, Light) · Window scale 75–200 % ·
Animation 60 / 30 / 15 fps · Spectrum on the ring · Hover tips · Use current settings when BURNIX opens / back to factory ·
Copy / paste settings between instances · Reset everything · Session · About.

**Sound** (1.3 engine): Gain compensation (on by default: drive changes the character, not the level) · Drive curve Progressive (1.3)
or Classic (1.2) · Analogue drift 0–100 % (level-dependent bias and fixed small channel differences) · Render quality (play in HQ 4x,
bounce in HQ 8x or ULTRA 16x; MIN-PHASE only, same latency) · Output CLIP 0 dBFS · "1.2 session: upgrade to the 1.3 engine".

## 1.3 engine
ADAA antialiasing on every curve (measured aliasing at HQ 4x went from −35 dB to −62 dB with less CPU) · IRON CORE saturates the lows
more, with light hysteresis · FIRESTORM starts earlier with no step · pure DELTA (only what BURNIX adds, no level difference) ·
crossover slopes 6/12/24/48 dB · sessions saved in 1.2 open with the old engine and sound identical (verified sample by sample).

## MOTION tab (2.0)
* **Modulation routings 1–3:** source **ENV** (input envelope), **LFO** or **SC** (sidechain); target **HEAT**, **MORPH X**,
  **MORPH Y**, **MIX** or **DENS**; **AMOUNT** ±100 %. Live source bars sit in the card header.
* **LFO:** RATE 0.05–20 Hz, shape SINE, TRI, SQR or RND (smoothed random). **ENVELOPE:** ATTACK and RELEASE (ENV and sidechain).
* **Sidechain:** route a key track to BURNIX's side input. Example: SC → MIX −100 % makes BURNIX back off when the kick hits.
* **A/B MORPH:** moves the current sound towards the other A/B side; set the verse on A, the chorus on B and automate A → B.
* A violet ring on the morph pad shows where the morph is right now when modulation is running.

## Per-band dynamics and attack (SCULPT, 2.0)
**DYNAMICS** (±100 %) acts on the band's attacks only: + adds punch, − softens (more sustain). **ATTACK** (±12 dB) is extra drive
on the attacks only: + bites the attack, − saturates the body and leaves the attack clean.

## Harmonic Draw and captures (ENGINEER, 2.0)
Pick **DRAW** as a band voicing (SCULPT). In ENGINEER press **DRAW** and drag the H2–H7 bars (double-click resets, Shift = 0.1 dB);
the light line is the last measurement. **IMPORT** reads a `.bnxcap` capture (or CSV/text with six numbers H2..H7 in dB), e.g. a
preamp, console or tape measured with ANALYSER. **EXPORT** saves the current drawing.

## Meters: AUDIBLE BASS and PHONE (2.0)
**AUDIBLE BASS** shows how many dB of 150–800 Hz harmonics BURNIX created from the sub (what makes bass audible on small speakers).
**PHONE** monitors like a phone speaker (mono, no lows, no top). Monitoring only: switch it off before bouncing.

## Tools (⚙ › Tools, 2.0)
* **Blind A/B test:** BURNIX hides which side is A or B (X and Y buttons, level-matched). Pick by ear, then **REVEAL**.
* **BURNIX Link:** every BURNIX instance in the session with the saturation each one adds, and the total.
* **Assistant:** play the track and press LISTEN: in 10 s it says what the source sounds like, why, and suggests three presets.
* **Reference match:** pick a reference track and play yours: it compares density (crest factor) band by band and proposes each
  band's drive (APPLY DRIVES, undoable).
