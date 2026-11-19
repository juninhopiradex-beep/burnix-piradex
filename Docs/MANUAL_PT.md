# BURNIX by PIRADEX — Manual (Português)

**Harmonic Color Engine** · saturação de válvulas multibanda com Mid/Side · versão 1.0.0
Criador e Desenvolvedor Ricardo Amorim / Juninho Piradex · Beatfreak Music · www.beatfreakstudio.com · juninhopiradex@hotmail.com

## Instalação
* **macOS:** abra `BURNIX-1.0.0-macOS.pkg` e escolha VST3 e/ou Audio Unit (`/Library/Audio/Plug-Ins/VST3` e `/Components`).
  Funciona em Intel (10.13+) e Apple Silicon.
* **Windows:** corra `BURNIX-1.0.0-Windows-Setup.exe` (VST3 64 bits em `C:\Program Files\Common Files\VST3`).
* Reinicie o DAW e faça um novo scan de plugins. Até 31/12/2026 o BURNIX abre sem senha; depois disso pede a senha master
  ou uma licença (validada online pelo PCC; funciona 14 dias sem internet).

## A ideia em 30 segundos
O BURNIX divide o som em 1 a 6 bandas e satura cada uma com um carácter de válvula diferente, no centro (MID), nas laterais
(SIDE) ou nos dois. O anel mostra isso: **MID por fora, SIDE por dentro**; quanto mais grosso o arco, mais drive; quanto mais
brilhante, mais harmónicos está realmente a acrescentar.

## Vista PERFORM (uso do dia a dia)
* **Arrastar um arco para fora** aumenta o drive dessa banda; para dentro diminui. Duplo-clique repõe. Shift = fino.
* **Morph (centro):** arraste o ponto entre os quatro carácteres (por omissão TRIODE BLOOM, PENTODE EDGE, FIRESTORM e VELVET).
  A barra "CARÁCTER" mostra a mistura real.
* **HEAT** escala o drive de todas as bandas (5.0 = as bandas como estão; 10 = o dobro; 0 = limpo).
* **DENSITY** dá "sag" (compressão e assimetria que acompanham o nível). **COLOR** inclina o som (graves ↔ agudos) só no
  sinal saturado. **AIR** acrescenta brilho a 12 kHz. **INPUT/OUTPUT** ganhos. **MIX** mistura global seco/saturado.
* **ROTA / MODO (todas as bandas):** atalhos para pôr todas as bandas em L/R, MID, SIDE ou M+S, e em INSERT ou PARALLEL.
* **AUTO GAIN** iguala o volume ao da entrada (para comparar sem ser enganado pelo volume). Desligado por omissão.
* **DELTA** deixa ouvir só o que o BURNIX acrescenta.
* Na base do anel: medidores de entrada (cyan) e saída (coral).

## Vista SCULPT (bandas)
* As **pegas brancas** no anel movem os crossovers. Clique num arco (ou nos números 1–6) para escolher a banda; **− / +**
  tiram ou põem bandas.
* **Centro:** voicing da banda (MORPH = segue o morph global, ou um dos seis fixos).
* **Rota:** L/R (o mesmo drive nos dois canais), MID, SIDE ou M+S (drives separados). **ON / SOLO.**
* **Filtro seletivo** decide o que satura dentro da banda: HP, LP, BP, NOTCH ou SHELF (agudos +6 dB), com FREQ e Q.
* **INSERT** substitui a parte filtrada pela versão saturada. **PARALLEL** soma o ramo saturado ao som (o volume sobe).
* **Inclinação** dos crossovers: 24 ou 48 dB/oitava (a soma das bandas é sempre plana).

## Vista ENGINEER (técnica)
* **MEDIR** analisa o som atual com um seno de 1 kHz a −6 dBFS e desenha os harmónicos H2–H10 reais no radar; **GUARDAR REF.**
  guarda uma referência tracejada para comparar.
* **Oversampling** (na base do anel ou no botão do topo): ECO 1x (latência 0), HQ 4x (recomendado), HQ 8x, ULTRA 16x.
  **MIN-PHASE** (latência baixa) ou **LINEAR** (fase linear, latência maior).
* **Topologia, STAGE BLEND, INTERSTAGE:** acrescenta um 2.º estágio PENTODE antes ou depois do voicing.
* **BIAS** (assimetria → mais harmónicos pares), **TRANSIENT** (protege os ataques), **BODY** (graves 120 Hz),
  **WIDTH**, **MONO BASS** (graves em mono abaixo da frequência escolhida).
* Leituras reais: THD, latência, CPU, true peak, ganho automático e fator de oversampling.

## Topo
Presets (de fábrica por categoria, os seus, "Guardar preset..."), **A/B**, desfazer/refazer, qualidade, **BYPASS** (real:
a entrada passa intacta e alinhada) e ⚙ (escala da janela 75–200 %, sessão, sobre).

## Presets de fábrica
Init · Clean Analog Glue · Vocal Silk · Kizomba Vocal Heat · G-Zouk Bass Presence · Afro Kick Grit · Stereo Side Sparkle ·
Mixbus Velvet · Master Subtle Color · Drum Parallel Fire · Low-Mid Sculpt · Air Without Harshness · Mono-Safe Master.

## Dicas
* Na mistura use **HQ 4x**; no master **HQ 8x LINEAR**; ULTRA 16x para render.
* Para comparar de forma justa ligue **AUTO GAIN** e use **A/B**.
* Use **DELTA** para afinar o filtro seletivo: ouve-se exatamente o que está a ser acrescentado.
