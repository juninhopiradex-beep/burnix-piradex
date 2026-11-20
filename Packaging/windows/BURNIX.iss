; BURNIX by PIRADEX — instalador Windows (Inno Setup). Instala o VST3 em C:\Program Files\Common Files\VST3

#define MyName "BURNIX by PIRADEX"
#define MyVersion "1.2.0"
#define MyPublisher "Ricardo Amorim / Juninho Piradex"

; Caminhos passados pelo workflow (/D). Valores por omissao para build local.
#ifndef VST3SRC
  #define VST3SRC "out\BURNIX by PIRADEX.vst3"
#endif
#ifndef OUTDIR
  #define OUTDIR "."
#endif

[Setup]
AppId={{4C9A2E71-B3D8-4F05-A6E2-91D7C5B83F10}
AppName={#MyName}
AppVersion={#MyVersion}
AppPublisher={#MyPublisher}
AppPublisherURL=https://www.beatfreakstudio.com
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableWelcomePage=no
UninstallDisplayName={#MyName} (VST3)
OutputBaseFilename=BURNIX-{#MyVersion}-Windows-Setup
OutputDir={#OUTDIR}
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
PrivilegesRequired=admin
WizardStyle=modern

[Messages]
WelcomeLabel2=Este assistente vai instalar o {#MyName} (VST3, 64 bits) no teu sistema.%n%nProfessional Audio Plugin | Beatfreak Music%nMultiband Tube Saturator + Mid/Side%nCriador e Desenvolvedor: {#MyPublisher}%nVersao {#MyVersion}
FinishedLabel=O {#MyName} foi instalado na pasta VST3 do sistema. Reinicie o seu DAW e faca um novo scan de plugins.%n%nBEATFREAK MUSIC - Professional Audio Technology%nwww.beatfreakstudio.com

[Files]
Source: "{#VST3SRC}\*"; DestDir: "{commoncf64}\VST3\BURNIX by PIRADEX.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\BURNIX by PIRADEX.vst3"
