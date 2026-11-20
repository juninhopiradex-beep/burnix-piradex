# BURNIX by PIRADEX — Manual (English)

**Harmonic Color Engine** · multiband tube saturation with Mid/Side · version 1.0.0
Created and developed by Ricardo Amorim / Juninho Piradex · Beatfreak Music · www.beatfreakstudio.com · juninhopiradex@hotmail.com

## Installation
* **macOS:** open `BURNIX-1.0.0-macOS.pkg` and pick VST3 and/or Audio Unit. Intel (10.13+) and Apple Silicon.
* **Windows:** run `BURNIX-1.0.0-Windows-Setup.exe` (64-bit VST3 in `C:\Program Files\Common Files\VST3`).
* Restart your DAW and rescan. Until 31 Dec 2026 BURNIX opens without a password; after that it asks for the master password or
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
Presets (150 factory presets in 19 categories, user presets, save), A/B, undo/redo, quality, true **BYPASS** (input passes untouched, latency-aligned).

## Settings (gear ⚙)
Language (Português / English) · Colours (Orbit, Ember, Ice, Indigo colour-blind safe, Light) · Window scale 75–200 % ·
Animation 60 / 30 / 15 fps · Spectrum on the ring · Hover tips · Use current settings when BURNIX opens / back to factory ·
Copy / paste settings between instances · Reset everything · Session · About.
