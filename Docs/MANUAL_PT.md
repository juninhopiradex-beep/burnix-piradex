# BURNIX by PIRADEX — Manual (Português)

**Harmonic Color Engine** · saturação de válvulas multibanda com Mid/Side · versão 2.0.0
Criador e Desenvolvedor Ricardo Amorim / Juninho Piradex · Beatfreak Music · www.beatfreakstudio.com · juninhopiradex@hotmail.com

## Instalação
* **macOS:** abra `BURNIX-2.0.0-macOS.pkg` e escolha VST3, Audio Unit e/ou CLAP (`/Library/Audio/Plug-Ins/VST3`, `/Components` e `/CLAP`).
  No Mac Intel antigo use `BURNIX-2.0.0-macOS-Intel-macOS10.13.pkg`; nos Mac M1/M2/M3/M4 `...-AppleSilicon.pkg`.
  Funciona em Intel (10.13+) e Apple Silicon.
* **Windows:** corra `BURNIX-2.0.0-Windows-Setup.exe` (VST3 e CLAP 64 bits em `C:\Program Files\Common Files\VST3` e `\CLAP`).
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
* **Centro:** voicing da banda (MORPH = segue o morph global, um dos seis fixos, ou DRAW = os harmónicos desenhados por si).
* **Rota:** L/R (o mesmo drive nos dois canais), MID, SIDE ou M+S (drives separados). **ON / SOLO.**
* **Filtro seletivo** decide o que satura dentro da banda: HP, LP, BP, NOTCH ou SHELF (agudos +6 dB), com FREQ e Q.
* **INSERT** substitui a parte filtrada pela versão saturada. **PARALLEL** soma o ramo saturado ao som (o volume sobe).
* **Inclinação** dos crossovers: 6, 12, 24 ou 48 dB/oitava (a soma das bandas é sempre plana; 6 e 12 dB dão transições mais naturais em master).

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
Presets (150 de fábrica em 19 categorias, os seus, "Guardar preset..."), **A/B**, desfazer/refazer, qualidade e **BYPASS** (real:
a entrada passa intacta e alinhada).

## Definições (roda dentada ⚙)
* **Idioma:** Português ou English.
* **Cores:** Órbita (original), Brasa, Gelo, Índigo (seguro para daltonismo: azul/laranja) e Clara.
* **Escala da janela:** 75 a 200 % (também pode arrastar o canto).
* **Animação:** Suave 60 fps, Normal 30 fps ou Económica 15 fps (poupa CPU em Macs antigos).
* **Espectro no anel** e **dicas ao passar o rato**: ligar/desligar.
* **Regulações:** usar as atuais sempre que abrir um BURNIX novo (ou voltar à de fábrica), copiar e colar regulações entre
  instâncias, repor tudo (Init).
* **Som** (motor 1.3):
  * **Ganho compensado** (ligado por omissão): o drive muda o carácter e não o volume, para comparar de ouvido sem enganos.
  * **Curva do drive:** Progressiva (1.3, o meio do botão já se ouve) ou Clássica (igual à 1.2).
  * **Drift analógico** (0–100 %): o bias "respira" com o nível e há pequenas diferenças fixas entre canais, como numa mesa real. Desligue em master.
  * **Qualidade ao exportar:** toca em HQ 4x e o bounce sai em HQ 8x ou ULTRA 16x (só com MIN-PHASE, latência igual). Ideal no Mac antigo.
  * **CLIP 0 dBFS:** segurança de saída; até −1 dBFS não toca no som.
  * **Sessão da 1.2: atualizar para o motor 1.3** (só aparece em sessões antigas, que abrem sempre com o som da 1.2).
* **Sessão** (terminar sessão) e **Sobre**.
As preferências da interface ficam guardadas e valem para todas as instâncias.

## Presets de fábrica
160 presets em 19 categorias: Voz, Rap & Trap, Coros, Baixo, 808 & Sub, Kick, Snare & Clap, Bateria, Percussão, Guitarra, Teclas,
Synths, Mix Bus, Mistura, Master, Estéreo, Afro & Angola, Criativo e Base. Todos saem a menos de 0,7 dB do volume de entrada.

## Motor 1.3 (o que mudou no som)
* **ADAA** (antialiasing por antiderivada) em todas as curvas: em HQ 4x o aliasing medido passou de −35 dB para −62 dB, com menos CPU.
* **IRON CORE** passou a saturar mais nos graves, com histerese leve, como um transformador.
* **FIRESTORM** começa a saturar mais cedo, sem degrau.
* **DELTA** puro: ouve-se só o que o BURNIX acrescenta, sem a diferença de volume.
* Sessões gravadas na 1.2 abrem com o motor antigo e soam exatamente igual (verificado amostra a amostra).

## Separador MOTION (2.0)
* **Ligações de modulação 1–3:** escolha a fonte (**ENV** = envolvente da entrada, **LFO**, **SC** = sidechain) e o destino
  (**HEAT**, **MORPH X**, **MORPH Y**, **MIX**, **DENS**), e a **QUANTIDADE** (±100 %). No cabeçalho vê as três fontes a mexer.
* **LFO:** RATE 0,05–20 Hz e forma SINE, TRI, SQR ou RND (aleatório suave).
* **ENVOLVENTE:** ATTACK e RELEASE de quem segue o nível (vale para ENV e para o sidechain).
* **Sidechain:** na DAW, ligue uma pista à entrada lateral do BURNIX. Exemplo: SC → MIX −100 % faz o BURNIX recuar quando o kick bate.
* **A/B MORPH:** leva o som atual até ao outro lado do A/B. Prepare o verso no A e o refrão no B e automatize o A → B.
* No anel, o **anel violeta** mostra onde o morph está naquele instante quando há modulação.

## Dinâmica e ataque por banda (SCULPT, 2.0)
* **DINÂMICA (±100 %):** mexe só nos ataques da banda: + dá punch, − amacia e deixa mais sustain.
* **ATAQUE (±12 dB):** drive extra só nos ataques: + "morde" o ataque, − satura só o corpo e deixa o ataque limpo.

## Harmonic Draw e capturas (ENGINEER, 2.0)
* Escolha **DRAW** no voicing de uma banda (SCULPT). No ENGINEER carregue em **DESENHAR** e arraste as barras H2–H7
  (duplo-clique repõe, Shift = 0,1 dB). A linha clara é a última medição, para confirmar.
* **IMPORTAR** lê uma captura `.bnxcap` (ou um CSV/texto com seis números H2..H7 em dB), por exemplo um pré, consola ou fita
  medidos no ANALYSER. **EXPORTAR** guarda o desenho atual.

## Medidores: GRAVE AUDÍVEL e TELEMÓVEL (2.0)
* **GRAVE AUDÍVEL:** quantos dB de harmónicos (150–800 Hz) o BURNIX criou a partir do sub, o que faz o grave ouvir-se em colunas pequenas.
* **TELEMÓVEL:** ouvir como num telemóvel (mono, sem graves, sem topo). É só monitorização: desligue antes do bounce.

## Ferramentas (⚙ › Ferramentas, 2.0)
* **Teste A/B cego:** o BURNIX esconde qual é o A e o B (botões X e Y), ao mesmo volume. Escolha de ouvido e depois **REVELAR**.
* **BURNIX Link:** lista todas as instâncias do BURNIX na sessão com a saturação que cada uma acrescenta e a soma, para não empastar.
* **Assistente:** toque a pista e carregue em OUVIR: em 10 s diz o que a fonte parece (voz, baixo, 808, kick, bateria, master...),
  porquê, e sugere três presets.
* **Match de referência:** escolha uma faixa de referência e toque a sua: compara a densidade (fator de crista) banda a banda e
  propõe o drive de cada banda (APLICAR DRIVES, com desfazer).

## Dicas
* Na mistura use **HQ 4x**; no master **HQ 8x LINEAR**; ULTRA 16x para render.
* Para comparar de forma justa deixe o **ganho compensado** ligado e use **A/B**; o **AUTO GAIN** acerta o resto.
* Use **DELTA** para afinar o filtro seletivo: ouve-se exatamente o que está a ser acrescentado.
