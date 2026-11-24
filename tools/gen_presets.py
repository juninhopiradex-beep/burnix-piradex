#!/usr/bin/env python3
"""
BURNIX by PIRADEX — gera Source/Plugin/FactoryPresets.h (160 presets de fabrica).

Cada preset parte do INIT (todos os parametros por omissao) e so lista o que muda.
Indices: voice 0 MORPH 1 EMBER 2 TRIODE 3 PENTODE 4 IRON 5 VELVET 6 FIRE
         route 0 L/R 1 MID 2 SIDE 3 M+S · filter 0 OFF 1 HP 2 LP 3 BP 4 NOTCH 5 SHELF · mode 0 INSERT 1 PARALLEL
         quality 0 ECO 1 HQ4 2 HQ8 3 ULTRA16 · osPhase 0 MIN 1 LINEAR
Uso:  python3 tools/gen_presets.py
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from preset_texts import TEXTS

EMBER, TRIODE, PENTODE, IRON, VELVET, FIRE, MORPH = 1, 2, 3, 4, 5, 6, 0
LR, MID, SIDE, MS = 0, 1, 2, 3
OFF, HP, LP, BP, NOTCH, SHELF = 0, 1, 2, 3, 4, 5
INS, PAR = 0, 1

P = []

def preset(name, cat, purpose, xo=None, bands=(), **g):
    v = {}
    if bands:
        v["numBands"] = len(bands)
    if xo:
        for i, f in enumerate(xo):
            v["xo%d" % (i + 1)] = f
    for i, b in enumerate(bands):
        k = "b%d_" % (i + 1)
        if b is None:
            v[k + "driveM"] = 0
            continue
        for key, pid in (("v", "voice"), ("d", "driveM"), ("s", "driveS"), ("r", "route"), ("f", "filter"),
                         ("fz", "freq"), ("q", "q"), ("m", "mode"), ("mx", "mix")):
            if key in b:
                v[k + pid] = b[key]
        if "d" not in b:
            v[k + "driveM"] = 3
    for key, val in g.items():
        v[key] = val
    P.append((name, cat, purpose, v))

def B(v=MORPH, d=3, **k):
    k.update(v=v, d=d)
    return k

# ---------------------------------------------------------------------------------------- 13 originais (ordem estavel)
preset("Init", "Base", "Ponto de partida: 4 bandas, drive 3, morph perto de TRIODE BLOOM.")
preset("Clean Analog Glue", "Mix Bus", "Cola suave numa banda, VELVET, ganho automatico.", bands=[B(VELVET, 2.5)], density=20, autoGain=1)
preset("Vocal Silk", "Voz", "Calor discreto na voz, agudos sedosos, ataques preservados.", [200, 4000],
       [B(EMBER, 1), B(EMBER, 3.5), B(VELVET, 2.5)], air=25, transient=30, autoGain=1)
preset("Kizomba Vocal Heat", "Voz", "Corpo da voz com 2.o harmonico (TRIODE) so no MID, brilho controlado.", None,
       [B(EMBER, 1), B(TRIODE, 5, r=MID), B(TRIODE, 3.5), B(VELVET, 2)], color=0.8, air=15, density=25, autoGain=1)
preset("G-Zouk Bass Presence", "Baixo", "Presenca do baixo em colunas pequenas: IRON CORE no sub, TRIODE nos medios-graves, graves em mono.",
       [100, 700], [B(IRON, 4.5, r=MID), B(TRIODE, 6, r=MID), None], monoBass=1, monoFreq=120, autoGain=1)
preset("Afro Kick Grit", "Kick", "Mordida no kick: PENTODE em paralelo filtrado a 900 Hz, FIRESTORM nos agudos.", [80, 2500],
       [B(IRON, 3), B(PENTODE, 6.5, m=PAR, mx=40, f=BP, fz=900, q=1), B(FIRE, 4, mx=50)], transient=50)
preset("Stereo Side Sparkle", "Estereo", "Brilho so nas laterais (SIDE), centro intacto, graves em mono.", [300, 5000],
       [None, B(VELVET, 0, r=SIDE, s=3.5), B(EMBER, 0, r=SIDE, s=5)], width=115, monoBass=1)
preset("Mixbus Velvet", "Mix Bus", "Saturacao de bus macia e densa, sem aspereza.", bands=[B(VELVET, 3.5)], density=30, autoGain=1)
preset("Master Subtle Color", "Master", "Cor minima para master: 3 bandas, HQ 8x, filtro linear.", [150, 5000],
       [B(MORPH, 1.5), B(MORPH, 2), B(MORPH, 1.5)], morphX=0.35, morphY=0.4, quality=2, osPhase=1, autoGain=1)
preset("Drum Parallel Fire", "Bateria", "Saturacao paralela agressiva em duas bandas com 2.o estagio.", [200],
       [B(FIRE, 5, m=PAR, mx=35), B(FIRE, 7, m=PAR, mx=45)], stageBlend=30, autoGain=1)
preset("Low-Mid Sculpt", "Mistura", "So os medios-graves (120-900 Hz) ganham corpo, focado a 300 Hz.", None,
       [None, B(TRIODE, 5, f=BP, fz=300, q=1.2), None, None])
preset("Air Without Harshness", "Mistura", "Harmonicos so acima de 10 kHz com VELVET, ataques protegidos.", [300, 6000],
       [None, None, B(VELVET, 4, f=SHELF, fz=10000)], transient=40)
preset("Mono-Safe Master", "Master", "Master com M+S: lado mais suave, graves em mono abaixo de 100 Hz, HQ 8x.", [120, 4000],
       [B(EMBER, 2, r=MS, s=0), B(EMBER, 2.5, r=MS, s=1.5), B(EMBER, 1.5, r=MS, s=1.5)], monoBass=1, monoFreq=100, quality=2, autoGain=1)

# ---------------------------------------------------------------------------------------- VOZ (+12)
for lvl, (dd, nm) in enumerate([(2, "Light"), (4, "Medium"), (6, "Hot")]):
    preset("Vocal Warmth %s" % nm, "Voz", "Calor de TRIODE nos medios da voz, intensidade %s." % nm.lower(), [180, 3500],
           [B(EMBER, 0.5), B(TRIODE, dd), B(VELVET, dd * 0.5)], density=15 + 10 * lvl, autoGain=1)
preset("Vocal Presence Lift", "Voz", "Harmonicos focados em 2-4 kHz para a voz passar na mistura.", [250, 2000, 6000],
       [None, B(EMBER, 1.5), B(PENTODE, 4, f=BP, fz=3000, q=0.9), B(VELVET, 2)], autoGain=1)
preset("Vocal Air Glow", "Voz", "Brilho de ar suave acima de 8 kHz, sem sibilancia agressiva.", [300, 8000],
       [None, B(EMBER, 1), B(VELVET, 4.5, f=SHELF, fz=9000)], air=20, transient=30, autoGain=1)
preset("Vocal Radio Edge", "Voz", "Voz de radio: banda media apertada e PENTODE com mordida.", [400, 3500],
       [None, B(PENTODE, 6.5, f=BP, fz=1500, q=1.4), B(FIRE, 3)], color=1.5, autoGain=1)
preset("Vocal Intimate", "Voz", "Voz proxima e quente: graves-medios cheios, agudos macios.", [150, 2500],
       [B(IRON, 2), B(TRIODE, 3.5), B(VELVET, 1.5)], body=30, density=20, autoGain=1)
preset("Vocal Belt Control", "Voz", "Notas fortes mais densas sem ficarem duras (density e transient).", [200, 3000],
       [B(EMBER, 1), B(TRIODE, 4.5), B(VELVET, 2.5)], density=45, transient=35, autoGain=1)
preset("Vocal Whisper Glow", "Voz", "Voz suave/sussurrada com mais presenca e textura.", [300, 5000],
       [None, B(EMBER, 5), B(VELVET, 5)], air=30, autoGain=1)
preset("Vocal Velvet Smooth", "Voz", "Saturacao VELVET em toda a voz para suavizar asperezas.", bands=[B(VELVET, 4)], density=20, autoGain=1)
preset("Vocal Tube Lead", "Voz", "Voz principal com caracter de valvula e 2.o estagio PENTODE ligeiro.", [200, 4000],
       [B(EMBER, 1), B(TRIODE, 4.5), B(EMBER, 2.5)], stageBlend=20, autoGain=1)
preset("Vocal Parallel Thicken", "Voz", "Ramo saturado em paralelo para engrossar sem perder clareza.", [250, 4000],
       [None, B(TRIODE, 6, m=PAR, mx=35), B(VELVET, 3, m=PAR, mx=25)], autoGain=1)

# ---------------------------------------------------------------------------------------- RAP / TRAP (+10)
preset("Rap Vocal Punch", "Rap & Trap", "Voz de rap com ataque e medios a frente.", [200, 3000],
       [B(EMBER, 1), B(PENTODE, 5), B(EMBER, 2.5)], transient=20, autoGain=1)
preset("Trap Vocal Grit", "Rap & Trap", "Grao agressivo na voz de trap, FIRESTORM em paralelo.", [300, 4000],
       [None, B(FIRE, 6, m=PAR, mx=35), B(PENTODE, 3)], autoGain=1)
preset("Drill Vocal Dark", "Rap & Trap", "Voz escura de drill: corpo IRON, agudos contidos.", [180, 2500],
       [B(IRON, 3), B(TRIODE, 4), B(VELVET, 1)], color=-1.5, autoGain=1)
preset("Ad-Lib Crunch", "Rap & Trap", "Ad-libs sujos e estreitos para ficarem atras da voz principal.", [500, 4000],
       [None, B(FIRE, 7, f=BP, fz=1500, q=1.2), None], width=70, autoGain=1)
preset("Melodic Rap Warm", "Rap & Trap", "Rap melodico com calor TRIODE e ar suave.", [200, 5000],
       [B(EMBER, 1), B(TRIODE, 4), B(VELVET, 3)], air=20, autoGain=1)
preset("Rap Double Wide", "Rap & Trap", "Dobras de voz mais largas: saturacao so no SIDE.", [300, 6000],
       [None, B(EMBER, 0, r=SIDE, s=4), B(VELVET, 0, r=SIDE, s=4)], width=125, autoGain=1)
preset("Trap Hook Shine", "Rap & Trap", "Refrao com brilho e presenca extra.", [250, 3000, 8000],
       [None, B(TRIODE, 3), B(PENTODE, 3.5), B(VELVET, 4)], air=25, autoGain=1)
preset("Phone Mid Grit", "Rap & Trap", "Grao tipo telefone so nos medios (500 Hz-3 kHz); graves e agudos passam limpos.", [500, 3000],
       [None, B(FIRE, 7), None], autoGain=1)
preset("Rap Vocal Clean Glue", "Rap & Trap", "Cola subtil para vozes de rap ja boas.", bands=[B(EMBER, 2)], density=15, autoGain=1)
preset("Trap Vocal Saturated Air", "Rap & Trap", "Agudos saturados e cintilantes para trap moderno.", [400, 7000],
       [None, B(EMBER, 1.5), B(FIRE, 4, f=SHELF, fz=9000)], transient=30, autoGain=1)

# ---------------------------------------------------------------------------------------- COROS (+6)
preset("Backing Vocal Soft", "Coros", "Coros macios que ficam atras da voz principal.", [250, 5000],
       [None, B(VELVET, 3), B(VELVET, 2)], color=-1, autoGain=1)
preset("Choir Wide Glow", "Coros", "Coros largos com brilho so nas laterais.", [300, 5000],
       [None, B(EMBER, 2, r=MS, s=4), B(VELVET, 1, r=MS, s=4)], width=125, autoGain=1)
preset("Harmony Warm Stack", "Coros", "Pilhas de harmonias com calor TRIODE.", [200, 3500],
       [B(EMBER, 1), B(TRIODE, 4), B(EMBER, 2)], density=25, autoGain=1)
preset("Gang Vocal Grit", "Coros", "Gritos de grupo com grao PENTODE.", [300, 3000],
       [None, B(PENTODE, 5.5), B(FIRE, 3)], autoGain=1)
preset("Backing Air Lift", "Coros", "So ar nos coros, acima de 7 kHz.", [300, 7000], [None, None, B(VELVET, 5)], air=20, autoGain=1)

# ---------------------------------------------------------------------------------------- BAIXO (+11)
for nm, v, d in [("Bass Warm Tube", TRIODE, 4), ("Bass Iron Weight", IRON, 5), ("Bass Pentode Bite", PENTODE, 5.5)]:
    preset(nm, "Baixo", "Baixo com %s nos medios-graves, sub limpo." % ("TRIODE" if v == TRIODE else ("IRON CORE" if v == IRON else "PENTODE")),
           [90, 800], [B(EMBER, 1, r=MID), B(v, d, r=MID), B(EMBER, 1.5)], monoBass=1, monoFreq=120, autoGain=1)
preset("Bass Small Speaker", "Baixo", "Harmonicos de 200-800 Hz para ouvir o baixo em telemovel.", [120, 900],
       [None, B(TRIODE, 6.5, r=MID), None], monoBass=1, autoGain=1)
preset("Bass Growl", "Baixo", "Rosnado nos medios do baixo, FIRESTORM em paralelo.", [150, 1500],
       [None, B(FIRE, 6, m=PAR, mx=40), B(PENTODE, 3)], monoBass=1, autoGain=1)
preset("Bass DI Warmer", "Baixo", "Baixo DI mais quente e redondo.", bands=[B(TRIODE, 3.5)], body=25, density=20, autoGain=1)
preset("Bass Finger Round", "Baixo", "Baixo de dedo: redondo, ataque preservado.", [120, 1200],
       [B(IRON, 2), B(EMBER, 3), B(VELVET, 1)], transient=40, autoGain=1)
preset("Bass Slap Edge", "Baixo", "Slap com brilho e ataque controlado.", [150, 2000],
       [B(EMBER, 1), B(PENTODE, 3.5), B(FIRE, 3)], transient=30, autoGain=1)
preset("Synth Bass Fat", "Baixo", "Synth bass gordo: IRON no grave, TRIODE em cima.", [100, 1000],
       [B(IRON, 4, r=MID), B(TRIODE, 5, r=MID), B(EMBER, 2)], monoBass=1, autoGain=1)
preset("Bass Mono Anchor", "Baixo", "Graves em mono abaixo de 150 Hz com calor discreto.", [150], [B(IRON, 3, r=MID), B(EMBER, 2)],
       monoBass=1, monoFreq=150, autoGain=1)
preset("Bass Velvet Smooth", "Baixo", "Baixo suave e controlado com VELVET.", bands=[B(VELVET, 4.5)], density=25, monoBass=1, autoGain=1)

# ---------------------------------------------------------------------------------------- 808 & SUB (+8)
for nm, d, extra in [("808 Audible Harmonics", 6, {}), ("808 Heavy Distort", 8.5, {"stageBlend": 25}), ("808 Clean Weight", 3, {})]:
    preset(nm, "808 & Sub", "808 com harmonicos que se ouvem em colunas pequenas.", [70, 400],
           [B(IRON, d * 0.5, r=MID), B(TRIODE, d, r=MID), B(EMBER, 1)], monoBass=1, monoFreq=150, autoGain=1, **extra)
preset("808 Knock", "808 & Sub", "808 com ataque e knock nos 150-400 Hz.", [120, 500],
       [B(IRON, 3, r=MID), B(PENTODE, 6, r=MID), None], transient=40, monoBass=1, autoGain=1)
preset("Sub Octave Glow", "808 & Sub", "Sub com brilho suave de 2.o harmonico.", [100], [B(TRIODE, 5, r=MID), None], monoBass=1, autoGain=1)
preset("808 Fire Clip", "808 & Sub", "808 sujo e agressivo (FIRESTORM).", bands=[B(FIRE, 7)], monoBass=1, monoFreq=150, autoGain=1)
preset("808 Velvet Round", "808 & Sub", "808 redondo e macio.", bands=[B(VELVET, 5)], density=30, monoBass=1, autoGain=1)
preset("Sub Mono Safe", "808 & Sub", "Sub controlado e em mono, cor minima.", [120], [B(EMBER, 2, r=MID), None], monoBass=1, monoFreq=140, autoGain=1)

# ---------------------------------------------------------------------------------------- KICK (+9)
preset("Kick Punch Tube", "Kick", "Kick com punch: TRIODE nos 60-200 Hz, ataque preservado.", [200, 3000],
       [B(TRIODE, 4.5), B(EMBER, 2), B(PENTODE, 3)], transient=30)
preset("Kick Click Edge", "Kick", "Mais clique no ataque (3-5 kHz).", [200, 2500], [None, B(EMBER, 1), B(PENTODE, 5, f=BP, fz=4000, q=1)])
preset("Kick Deep Iron", "Kick", "Kick profundo com IRON CORE no grave.", [120], [B(IRON, 5), B(EMBER, 1.5)], monoBass=1)
preset("Kick Parallel Smash", "Kick", "Kick esmagado em paralelo, FIRESTORM.", bands=[B(FIRE, 7.5, m=PAR, mx=40)], autoGain=1)
preset("Kick Kuduro Thump", "Kick", "Kick de kuduro: grave cheio e ataque seco.", [100, 2000],
       [B(IRON, 4, r=MID), B(PENTODE, 4), B(FIRE, 2)], transient=35, monoBass=1)
preset("Kick Amapiano Log", "Kick", "Kick/log drum de amapiano: medios-graves com tom.", [90, 600],
       [B(EMBER, 2), B(TRIODE, 6, f=BP, fz=250, q=1), None], monoBass=1, autoGain=1)
preset("Kick Soft Round", "Kick", "Kick suave e redondo para baladas.", bands=[B(VELVET, 3.5)], transient=20)
preset("Kick House Drive", "Kick", "Kick de house com densidade e grave firme.", [150], [B(TRIODE, 5), B(EMBER, 2)], density=30, monoBass=1)
preset("Kick Beater Bite", "Kick", "Mordida do batente, ataque realcado.", [300, 3000], [None, B(PENTODE, 4), B(FIRE, 4)], transient=10)

# ---------------------------------------------------------------------------------------- SNARE & CLAP (+6)
preset("Snare Crack", "Snare & Clap", "Estalo da tarola com PENTODE nos 1-4 kHz.", [250, 4000], [B(EMBER, 2), B(PENTODE, 5.5), B(FIRE, 3)], transient=25)
preset("Snare Body Tube", "Snare & Clap", "Corpo da tarola (200 Hz) com TRIODE.", [400, 4000], [B(TRIODE, 5), B(EMBER, 2), None])
preset("Snare Parallel Fire", "Snare & Clap", "Tarola agressiva em paralelo.", bands=[B(FIRE, 8, m=PAR, mx=35)], autoGain=1)
preset("Clap Wide Grit", "Snare & Clap", "Palmas largas e com grao nas laterais.", [500, 5000],
       [None, B(PENTODE, 3, r=MS, s=5), B(FIRE, 2, r=MS, s=4)], width=120)
preset("Rimshot Snap", "Snare & Clap", "Rimshot com mais estalo.", [800, 5000], [None, B(PENTODE, 5), B(EMBER, 3)], transient=15)

# ---------------------------------------------------------------------------------------- BATERIA BUS (+9)
preset("Drum Bus Glue", "Bateria", "Cola de bus de bateria com TRIODE e density.", [150, 4000], [B(IRON, 2), B(TRIODE, 3.5), B(EMBER, 2)], density=30, autoGain=1)
preset("Drum Bus Smash", "Bateria", "Esmagar o bus em paralelo (estilo NY).", bands=[B(FIRE, 8, m=PAR, mx=30)], stageBlend=20, autoGain=1)
preset("Drum Room Crunch", "Bateria", "Ambiente da bateria com crocancia so no SIDE.", [400, 5000],
       [None, B(PENTODE, 0, r=SIDE, s=6), B(FIRE, 0, r=SIDE, s=4)], width=115, autoGain=1)
preset("Drum Bus Warm", "Bateria", "Bus de bateria mais quente e redondo.", bands=[B(VELVET, 3.5)], body=20, autoGain=1)
preset("Drum Transient Safe Heat", "Bateria", "Calor sem matar os ataques (transient preserve alto).", [200, 3000],
       [B(TRIODE, 4), B(EMBER, 3), B(VELVET, 2)], transient=70, autoGain=1)
preset("Breakbeat Dirt", "Bateria", "Breaks sujos e vintage.", [200, 3000], [B(IRON, 4), B(FIRE, 5), B(VELVET, 3)], color=-1.5, autoGain=1)
preset("Overheads Shine", "Bateria", "Overheads com brilho controlado.", [500, 7000], [None, B(EMBER, 1.5), B(VELVET, 4)], air=25, autoGain=1)
preset("Drum Bus Punch MS", "Bateria", "Punch no centro (MID), laterais intactas.", [150, 4000],
       [B(TRIODE, 4, r=MID), B(PENTODE, 4, r=MID), B(EMBER, 2, r=MID)], autoGain=1)
preset("Lo-Fi Drum Bus", "Bateria", "Bateria lo-fi: agudos cortados, grao alto.", [200, 3500], [B(IRON, 3), B(FIRE, 5), None], color=-3, autoGain=1)

# ---------------------------------------------------------------------------------------- PERCUSSAO (+5)
preset("Perc Snap", "Percussao", "Percussao com mais ataque e estalo.", [500, 5000], [None, B(PENTODE, 4), B(EMBER, 3)], transient=20)
preset("Shaker Silk", "Percussao", "Shakers e hi-hats sedosos.", [3000], [None, B(VELVET, 4)], transient=30)
preset("Conga Warm Tone", "Percussao", "Congas e bongos com tom quente.", [150, 2500], [B(EMBER, 1), B(TRIODE, 4.5), B(VELVET, 1.5)])
preset("Hi-Hat Grit", "Percussao", "Hi-hats com grao e brilho.", [5000], [None, B(FIRE, 4)], transient=15)
preset("Perc Wide Sparkle", "Percussao", "Percussao mais larga e brilhante nas laterais.", [600, 6000],
       [None, B(EMBER, 1, r=MS, s=4), B(VELVET, 1, r=MS, s=5)], width=120)

# ---------------------------------------------------------------------------------------- GUITARRA (+8)
preset("Clean Guitar Sheen", "Guitarra", "Guitarra limpa com brilho e calor.", [250, 4000], [None, B(TRIODE, 3), B(VELVET, 3.5)], air=15, autoGain=1)
preset("Acoustic Body", "Guitarra", "Corpo de viola acustica sem embolar.", [150, 3000], [B(EMBER, 1), B(TRIODE, 3.5), B(VELVET, 2)], autoGain=1)
preset("Guitar Crunch Amp", "Guitarra", "Crunch de amplificador (PENTODE + 2.o estagio).", [200, 3500],
       [B(EMBER, 2), B(PENTODE, 6.5), B(FIRE, 3)], stageBlend=30, autoGain=1)
preset("Guitar Fuzz Fire", "Guitarra", "Fuzz agressivo FIRESTORM.", bands=[B(FIRE, 9)], stageBlend=40, autoGain=1)
preset("Semba Guitar Shine", "Guitarra", "Guitarra de semba com brilho e ataque.", [300, 3000], [None, B(EMBER, 3), B(PENTODE, 3.5)], transient=30, autoGain=1)
preset("Guitar Double Wide", "Guitarra", "Guitarras dobradas mais largas (SIDE).", [250, 4000],
       [None, B(TRIODE, 1, r=MS, s=4.5), B(VELVET, 1, r=MS, s=3.5)], width=125, autoGain=1)
preset("Guitar DI Tube", "Guitarra", "Guitarra DI com cor de valvula.", bands=[B(TRIODE, 5)], density=25, autoGain=1)
preset("Electric Lead Sing", "Guitarra", "Solo de guitarra que canta, medios densos.", [400, 3000],
       [None, B(TRIODE, 6), B(VELVET, 3)], density=35, autoGain=1)

# ---------------------------------------------------------------------------------------- TECLAS (+8)
preset("Piano Warm Hall", "Teclas", "Piano quente e cheio.", [200, 4000], [B(EMBER, 1), B(TRIODE, 3), B(VELVET, 2)], autoGain=1)
preset("Piano Pop Bright", "Teclas", "Piano pop brilhante, ataque claro.", [300, 3000], [None, B(EMBER, 2), B(PENTODE, 3.5)], air=20, autoGain=1)
preset("Rhodes Tube Bark", "Teclas", "Rhodes com 'bark' nos ataques.", [200, 2000], [B(EMBER, 1), B(TRIODE, 5.5), B(VELVET, 2)], autoGain=1)
preset("Organ Drive", "Teclas", "Orgao com drive de valvula.", bands=[B(PENTODE, 6)], stageBlend=20, autoGain=1)
preset("Keys Velvet Pad", "Teclas", "Teclados macios para tapetes.", bands=[B(VELVET, 3.5)], width=110, autoGain=1)
preset("Upright Piano Lo-Fi", "Teclas", "Piano de parede lo-fi.", [250, 3000], [B(IRON, 3), B(FIRE, 3.5), None], color=-2.5, autoGain=1)
preset("Clav Funk Edge", "Teclas", "Clavinet funky com mordida.", [300, 3000], [None, B(PENTODE, 5), B(FIRE, 3)], transient=20, autoGain=1)
preset("Keys Wide Shimmer", "Teclas", "Brilho largo nas teclas.", [400, 6000], [None, B(EMBER, 1, r=MS, s=3.5), B(VELVET, 1, r=MS, s=5)], width=120, autoGain=1)

# ---------------------------------------------------------------------------------------- SYNTHS & PADS (+8)
preset("Synth Lead Bite", "Synths", "Lead de synth com mordida PENTODE.", [300, 3000], [None, B(PENTODE, 5.5), B(FIRE, 3)], autoGain=1)
preset("Pad Warm Analog", "Synths", "Pad com calor analogico suave.", bands=[B(TRIODE, 3)], density=25, width=110, autoGain=1)
preset("Pad Wide Velvet", "Synths", "Pad largo e macio.", [300, 5000], [None, B(VELVET, 1, r=MS, s=4), B(VELVET, 1, r=MS, s=4)], width=130, autoGain=1)
preset("Pluck Snap", "Synths", "Plucks com ataque realcado.", [400, 4000], [None, B(EMBER, 3), B(PENTODE, 3)], transient=10, autoGain=1)
preset("Supersaw Tame", "Synths", "Supersaws mais suaves (VELVET nos agudos).", [500, 5000], [None, B(EMBER, 2), B(VELVET, 5)], color=-1, autoGain=1)
preset("Arp Grit", "Synths", "Arpejos com grao.", [400, 4000], [None, B(FIRE, 4.5), B(PENTODE, 2)], autoGain=1)
preset("Synth Brass Heat", "Synths", "Metais de synth com calor TRIODE.", [300, 3000], [None, B(TRIODE, 5), B(EMBER, 2.5)], autoGain=1)
preset("Ambient Texture Glow", "Synths", "Texturas ambient com brilho harmonico.", [400, 6000], [None, B(VELVET, 3), B(EMBER, 4, f=SHELF, fz=8000)], width=120, autoGain=1)

# ---------------------------------------------------------------------------------------- MIX BUS (+8)
preset("Mix Bus Tape Glue", "Mix Bus", "Cola tipo fita: IRON no grave, VELVET em cima.", [120, 5000], [B(IRON, 2.5), B(EMBER, 2), B(VELVET, 2.5)], density=25, autoGain=1)
preset("Mix Bus Tube Warmth", "Mix Bus", "Calor de valvula em toda a mistura.", bands=[B(TRIODE, 2.5)], density=20, autoGain=1)
preset("Mix Bus Punch", "Mix Bus", "Punch no centro da mistura.", [150, 3500], [B(TRIODE, 3, r=MID), B(PENTODE, 2.5, r=MID), B(EMBER, 1.5)], transient=30, autoGain=1)
preset("Mix Bus Wide Air", "Mix Bus", "Abrir e arejar a mistura (SIDE e ar).", [300, 6000],
       [None, B(EMBER, 1, r=MS, s=2.5), B(VELVET, 1, r=MS, s=3.5)], width=112, air=15, autoGain=1)
preset("Mix Bus Vintage Console", "Mix Bus", "Cor de mesa vintage: TRIODE + 2.o estagio leve.", [150, 5000],
       [B(EMBER, 2), B(TRIODE, 3), B(EMBER, 2)], stageBlend=15, color=-0.5, autoGain=1)
preset("Mix Bus Low-End Firm", "Mix Bus", "Grave mais firme e em mono.", [120], [B(IRON, 3, r=MID), B(EMBER, 1.5)], monoBass=1, autoGain=1)
preset("Mix Bus Parallel Color", "Mix Bus", "Cor em paralelo, sinal original intacto.", bands=[B(TRIODE, 5, m=PAR, mx=20)], autoGain=1)
preset("Mix Bus Gentle Density", "Mix Bus", "Densidade suave sem cor aparente.", bands=[B(VELVET, 2)], density=40, autoGain=1)

# ---------------------------------------------------------------------------------------- MASTER (+8)
preset("Master Warm Glue", "Master", "Calor e cola subtis para master, HQ 8x.", [150, 5000], [B(EMBER, 1.5), B(TRIODE, 1.5), B(VELVET, 1.5)], quality=2, autoGain=1)
preset("Master Loud Density", "Master", "Mais densidade e percecao de volume.", [120, 4000], [B(IRON, 2.5, r=MID), B(TRIODE, 3), B(EMBER, 2.5)],
       density=35, monoBass=1, quality=2, autoGain=1)
preset("Master Air Polish", "Master", "Polimento de agudos acima de 10 kHz.", [10000], [None, B(VELVET, 3)], quality=2, osPhase=1, autoGain=1)
preset("Master Afro Club", "Master", "Master de clube para afrohouse/kuduro: grave firme, medios presentes.", [100, 3000],
       [B(IRON, 3, r=MID), B(TRIODE, 2.5), B(EMBER, 2)], monoBass=1, monoFreq=110, quality=2, autoGain=1)
preset("Master Streaming Safe", "Master", "Cor minima, M+S suave, sem picos extra.", [150, 5000],
       [B(EMBER, 1, r=MS, s=0.5), B(EMBER, 1.5, r=MS, s=1), B(VELVET, 1, r=MS, s=1)], quality=2, osPhase=1, autoGain=1)
preset("Master Vinyl Warmth", "Master", "Calor tipo vinil e graves em mono.", [150, 6000], [B(IRON, 2), B(TRIODE, 2), B(VELVET, 1)], color=-1, monoBass=1, monoFreq=150, quality=2, autoGain=1)
preset("Master Wide Sheen", "Master", "Brilho e largura no master.", [300, 7000], [None, B(EMBER, 1, r=MS, s=2), B(VELVET, 1, r=MS, s=3)], width=108, quality=2, autoGain=1)
preset("Master Ultra Transparent", "Master", "Quase invisivel: ULTRA 16x, drive minimo.", bands=[B(EMBER, 1)], quality=3, osPhase=1, autoGain=1)

# ---------------------------------------------------------------------------------------- ESTEREO (+5)
preset("Side Only Warmth", "Estereo", "Calor so nas laterais; centro intacto.", bands=[B(TRIODE, 0, r=SIDE, s=4)], autoGain=1)
preset("Mid Focus Heat", "Estereo", "Calor so no centro (voz, kick, baixo).", bands=[B(TRIODE, 4, r=MID)], autoGain=1)
preset("Widen Highs Only", "Estereo", "Alarga so os agudos.", [4000], [None, B(VELVET, 0, r=SIDE, s=4)], width=125)
preset("Narrow Lows Mono", "Estereo", "Graves em mono e cor discreta.", [200], [B(EMBER, 1.5, r=MID), None], monoBass=1, monoFreq=200)
preset("Stereo Balance Color", "Estereo", "Cor equilibrada M+S com lado mais suave.", [250, 5000],
       [B(EMBER, 2, r=MS, s=1), B(TRIODE, 2.5, r=MS, s=2), B(VELVET, 1.5, r=MS, s=2.5)], autoGain=1)

# ---------------------------------------------------------------------------------------- AFRO & ANGOLA (+10)
preset("Kuduro Energy", "Afro & Angola", "Kuduro: grave seco, medios agressivos, agudos com grao.", [100, 2500],
       [B(IRON, 3.5, r=MID), B(PENTODE, 5), B(FIRE, 3)], transient=30, monoBass=1, autoGain=1)
preset("Semba Warm Classic", "Afro & Angola", "Semba com calor classico de valvula.", [150, 4000], [B(EMBER, 1.5), B(TRIODE, 3.5), B(VELVET, 2)], autoGain=1)
preset("Afrohouse Groove Bus", "Afro & Angola", "Bus de afrohouse com groove e grave firme.", [100, 3000],
       [B(IRON, 3, r=MID), B(TRIODE, 3), B(EMBER, 2.5)], density=25, monoBass=1, autoGain=1)
preset("Amapiano Log Drum", "Afro & Angola", "Log drum de amapiano com tom e mordida.", [80, 800],
       [B(EMBER, 1, r=MID), B(TRIODE, 6.5, f=BP, fz=200, q=0.9), None], monoBass=1, autoGain=1)
preset("Tarraxinha Smooth Low", "Afro & Angola", "Tarraxinha: grave macio e envolvente.", [120, 2000], [B(VELVET, 4, r=MID), B(EMBER, 2), B(VELVET, 1.5)], monoBass=1, autoGain=1)
preset("Afrobeat Bus Color", "Afro & Angola", "Afrobeat: cor viva nos medios.", [200, 4000], [B(EMBER, 1.5), B(TRIODE, 4), B(EMBER, 2.5)], autoGain=1)
preset("Kizomba Master Warm", "Afro & Angola", "Master de kizomba: quente, intimo, graves em mono.", [120, 5000],
       [B(IRON, 2, r=MID), B(TRIODE, 2.5), B(VELVET, 2)], monoBass=1, quality=2, autoGain=1)
preset("Ghetto Zouk Pad Glow", "Afro & Angola", "Pads de ghetto zouk com brilho largo.", [400, 6000], [None, B(VELVET, 2, r=MS, s=4), B(EMBER, 1, r=MS, s=4)], width=120, autoGain=1)
preset("Afro Perc Shine", "Afro & Angola", "Percussao afro com estalo e brilho.", [500, 5000], [None, B(PENTODE, 3.5), B(EMBER, 3.5)], transient=25)
preset("Kuduro Vocal Shout", "Afro & Angola", "Voz de kuduro gritada com grao e presenca.", [300, 3000], [None, B(FIRE, 5), B(PENTODE, 3)], autoGain=1)

# ---------------------------------------------------------------------------------------- CRIATIVO (+8)
preset("Transistor Mid Crunch", "Criativo", "Crocancia de radio antigo so nos medios (600 Hz-3 kHz).", [600, 3000], [None, B(FIRE, 8), None], autoGain=1)
preset("Broken Speaker", "Criativo", "Coluna partida: clip assimetrico extremo.", bands=[B(FIRE, 10)], bias=60, stageBlend=60, autoGain=1)
preset("Warm Lo-Fi Tape", "Criativo", "Lo-fi quente: agudos escuros, densidade alta.", [150, 3000], [B(IRON, 4), B(TRIODE, 4), B(VELVET, 2)], density=60, color=-4, autoGain=1)
preset("Fire Morph Sweep", "Criativo", "Morph no canto FIRESTORM para automatizar.", bands=[B(MORPH, 6)], morphX=0.5, morphY=0.9, autoGain=1)
preset("Side Chaos", "Criativo", "Laterais destruidas, centro limpo.", bands=[B(FIRE, 0, r=SIDE, s=9)], width=140, autoGain=1)
preset("Octave Fuzz Bias", "Criativo", "Fuzz com bias alto (harmonicos pares fortes).", bands=[B(TRIODE, 8)], bias=90, autoGain=1)
preset("Delta Harmonics Only", "Criativo", "So os harmonicos acrescentados (DELTA ligado).", bands=[B(TRIODE, 6)], delta=1)
preset("Megaphone Mid Bite", "Criativo", "Mordida de megafone so nos medios (800 Hz-2,5 kHz).", [800, 2500], [None, B(PENTODE, 9), None], autoGain=1)

# ---------------------------------------------------------------------------------------- MASTER 2.1 (+10, fase linear)
preset("LP Master Clean Glue", "Master", "Master com crossovers de fase linear: cola subtil em 3 bandas, HQ 4x.", [150, 5000],
       [B(EMBER, 1.2), B(TRIODE, 1.5), B(VELVET, 1.2)], xoLinear=1, quality=1, autoGain=1)
preset("LP Master Low-End Weight", "Master", "Peso no grave sem rodar a fase do kick e do baixo (fase linear).", [90, 400, 5000],
       [B(IRON, 2.2, r=MID), B(TRIODE, 1.8), B(EMBER, 1), B(VELVET, 1)], xoLinear=1, quality=1, monoBass=1, monoFreq=90, autoGain=1)
preset("LP Master Vocal Forward", "Master", "Voz mais à frente no master: médios com TRIODE, resto quase limpo.", [250, 4000],
       [B(EMBER, 0.8), B(TRIODE, 2.2, r=MID), B(VELVET, 1)], xoLinear=1, quality=1, autoGain=1)
preset("LP Master Air & Silk", "Master", "Brilho sedoso acima de 8 kHz, graves e médios intactos.", [8000],
       [None, B(VELVET, 2.5)], xoLinear=1, quality=1, air=10, autoGain=1)
preset("LP Master Afro Club", "Master", "Clube afro/kuduro: sub firme em mono, médios presentes, fase linear.", [100, 2500],
       [B(IRON, 2.5, r=MID), B(PENTODE, 2), B(EMBER, 1.5)], xoLinear=1, quality=1, monoBass=1, monoFreq=110, density=15, autoGain=1)
preset("LP Master Kizomba Warmth", "Master", "Calor de kizomba/zouk: médios-graves cheios, agudos suaves.", [180, 3000],
       [B(TRIODE, 1.5), B(TRIODE, 2.5), B(VELVET, 1.5)], xoLinear=1, quality=1, color=-0.5, autoGain=1)
preset("LP Master Wide Top", "Master", "Laterais com brilho (SIDE) acima de 4 kHz, centro e graves intactos.", [300, 4000],
       [None, B(EMBER, 0.5), B(VELVET, 0.5, r=MS, s=2.5)], xoLinear=1, quality=1, width=106, monoBass=1, autoGain=1)
preset("LP Master Dense Loud", "Master", "Mais densidade e volume percebido com 4 bandas de fase linear.", [120, 1000, 6000],
       [B(IRON, 2), B(TRIODE, 2.5), B(PENTODE, 2), B(EMBER, 1.5)], xoLinear=1, quality=1, density=30, autoGain=1)
preset("LP Master Transparent", "Master", "Quase invisível: drive mínimo, só para colar, fase linear.", [200, 6000],
       [B(EMBER, 0.6), B(EMBER, 0.8), B(EMBER, 0.6)], xoLinear=1, quality=1, autoGain=1)
preset("LP Master Streaming Safe", "Master", "Para plataformas: cor mínima, M+S suave, CLIP de segurança a 0 dBFS.", [150, 5000],
       [B(EMBER, 1, r=MS, s=0.5), B(TRIODE, 1.2, r=MS, s=0.8), B(VELVET, 1, r=MS, s=1)], xoLinear=1, quality=1, clip=1, autoGain=1)

assert len(P) == 160, len(P)
names = [p[0] for p in P]
assert len(names) == len(set(names)), "nomes repetidos"

missing = [p[0] for p in P if p[0] not in TEXTS]
assert not missing, "sem texto: %s" % missing

def cstr(t):
    # literal C++ so com ASCII: os acentos vao como bytes UTF-8 escapados (a GUI le com CharPointer_UTF8)
    o = ""
    for ch in t:
        if ch == '"': o += '\\"'
        elif ch == "\\": o += "\\\\"
        elif ord(ch) < 128: o += ch
        else: o += "".join("\\x%02x" % b for b in ch.encode("utf-8")) + '""'
    return '"' + o + '"'

out = []
out.append("// =============================================================================")
out.append("//  BURNIX by PIRADEX — 160 presets de fabrica. GERADO por tools/gen_presets.py (editar la, nao aqui).")
out.append("//  Cada preset parte do INIT (todos os parametros por omissao) e altera so o que lista. Finalidade em \"purpose\".")
out.append("// =============================================================================")
out.append("#pragma once\n#include <utility>\n#include <vector>\n\nnamespace bxp\n{\nstruct FactoryPreset\n{\n    const char* name; const char* category; const char* purpose; const char* purposeEn;   // textos em UTF-8\n    std::vector<std::pair<const char*, float>> values;\n};\n")
out.append("inline const std::vector<FactoryPreset>& factoryPresets()\n{\n    static const std::vector<FactoryPreset> p {")
for name, cat, purpose, v in P:
    vals = ", ".join('{ "%s", %sf }' % (k, repr(float(val))) for k, val in v.items())
    pt, en = TEXTS[name]
    out.append('        { "%s", "%s", %s,\n          %s,\n          { %s } },' % (name, cat, cstr(pt), cstr(en), vals))
out.append("    };\n    return p;\n}\n} // namespace bxp\n")
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
with open(os.path.join(root, "Source", "Plugin", "FactoryPresets.h"), "w") as f:
    f.write("\n".join(out))
cats = {}
for p in P:
    cats[p[1]] = cats.get(p[1], 0) + 1
print("160 presets gerados:", cats)
