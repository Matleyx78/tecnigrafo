# Ambiente di sviluppo (Debian, WSL, Windows con Visual Studio)

Procedura per Debian 13 (trixie), che ha Qt 6.8 nei pacchetti (minimo del progetto,
ADR 0028). Debian 12 (bookworm) ha Qt 6.4: serve Qt 6.8 installato a parte, vedi
il [README](../README.md). Su Windows 11 ci sono **due modi distinti** di lavorare:
la stessa procedura di Debian dentro WSL, sezione
[5. Windows con WSL](#5-windows-con-wsl), oppure la compilazione nativa con
Visual Studio, sezione
[8. Windows nativo con Visual Studio](#8-windows-nativo-con-visual-studio).
Per macOS vedi il [README](../README.md).

## Windows o WSL: dove sono?

Su un PC Windows si può lavorare in due ambienti che **non condividono nulla**:
due copie del progetto, due compilatori, due installazioni di Qt, due cartelle
`build`, e anche `git` e `gh` (con i loro accessi) sono separati.

| | Windows nativo (sezione 8) | Windows con WSL (sezioni 1-5) |
|---|---|---|
| Dove scrivo i comandi | PowerShell, prompt `PS C:\...>` | terminale Debian, prompt `utente@PC:~$` |
| Dove sta il progetto | `C:\progetti\tecnigrafo` | `~/progetti/tecnigrafo`, dentro Debian |
| Compilatore | MSVC, quello di Visual Studio | GCC di Debian |
| Qt | installer ufficiale, `C:\Qt\6.8.3\msvc2022_64` | `apt install qt6-base-dev` |
| Il programma si chiama | `build\release\app\tecnigrafo.exe` | `build/ci/app/tecnigrafo` |
| Separatore dei percorsi | `\` | `/` |
| VS Code, in basso a sinistra | niente | **WSL: Debian** |
| La build vale come prova su | Windows | Linux |

Regole per non confondersi:

- un file con `.exe` è Windows; senza estensione è Linux. Non si lanciano uno
  dall'altro ambiente;
- la cartella `build` di un ambiente non si riusa nell'altro: la configurazione
  ricorda il compilatore e non va d'accordo con quello sbagliato;
- si lavora nel proprio ambiente: non compilare dentro `\\wsl.localhost\Debian\...`
  da Windows, né dentro `/mnt/c/...` da Debian (lentissimo). Per **guardare** i file
  di Debian da Windows, da Debian: `explorer.exe .`;
- se hai dubbi su dove sei, guarda il prompt: `PS C:\` è Windows, `utente@...:~$`
  è Debian.

## 1. Pacchetti di sistema

```sh
sudo apt update
sudo apt install build-essential cmake ninja-build git gdb pipx \
    qt6-base-dev qt6-wayland libgl-dev libgtest-dev clangd
```

Su Debian 12 la GoogleTest dei repository è troppo vecchia: CMake scarica da sola
la versione giusta alla prima configurazione (serve la rete).

## 2. clang-format e clang-tidy versione 18

Il progetto fissa la versione 18 (ADR 0002), che i repository Debian non hanno.
Si installa dai pacchetti ufficiali pubblicati su PyPI:

```sh
pipx install clang-format==18.1.8
pipx install clang-tidy==18.1.8
pipx ensurepath
```

Chiudi la sessione ed entra di nuovo, poi verifica:

```sh
clang-format --version   # deve dire 18.1.8
clang-tidy --version     # deve dire 18.1.8
```

Non installare il pacchetto Debian `clang-format`: una versione diversa
formatterebbe il codice in modo diverso da quello che la CI accetta.

## 3. Prima compilazione dal terminale

```sh
cd ~/progetti/tecnigrafo
cmake --preset ci && cmake --build --preset ci && ctest --preset ci
./build/ci/app/tecnigrafo
```

Se sullo stesso PC lavorano anche gli agenti, ognuno usa la sua cartella di
build con `TG_BUILD_SUFFIX` (per esempio `build-codex/ci`, vedi AGENTS.md):
la tua resta `build/`.

## 4. VS Code

1. Apri la cartella del progetto: `code ~/progetti/tecnigrafo`.
2. Accetta "Installa" sulle estensioni consigliate (`.vscode/extensions.json`):
   CMake Tools, clangd, C/C++, Clang-Format, EditorConfig, GitHub Actions.
3. Se compare l'avviso di conflitto tra clangd e IntelliSense di C/C++, scegli di
   disattivare IntelliSense (le impostazioni del progetto lo fanno già).
4. Nella barra di stato in basso scegli il preset **Debug**.
5. `F7` compila; la provetta nella barra laterale mostra ed esegue i test.
6. Come destinazione di avvio scegli `tecnigrafo`, poi `F5` avvia il programma
   nel debugger.

Le impostazioni condivise (`.vscode/settings.json`) attivano la formattazione al
salvataggio con clang-format 18 e i controlli di clang-tidy direttamente
nell'editor.

## 5. Windows con WSL

Su Windows 11 si può lavorare esattamente come su Debian grazie a WSL (Windows
Subsystem for Linux): Debian gira dentro Windows, VS Code resta su Windows ma
lavora sui file e con gli strumenti di Debian, e la finestra di Tecnigrafo si
apre sul desktop di Windows.

Una build fatta in WSL è una build **Linux**. Vale come prova su Linux, non su
Windows: la prova manuale e il benchmark su Windows richiesti per chiudere una
tappa si fanno con la compilazione nativa descritta nella
[sezione 8](#8-windows-nativo-con-visual-studio).

### 5.1 Installare Debian

Da PowerShell aperto come amministratore:

```powershell
wsl --install -d Debian
```

Riavvia il PC. Al primo avvio Debian chiede nome utente e password, che non
dipendono da quelli di Windows. Controlla poi l'installazione:

```powershell
wsl -l -v   # deve comparire Debian con VERSION 2
```

Se dopo il riavvio non compare nessuna distribuzione, il primo comando ha
attivato solo WSL: esegui `wsl --update` e poi di nuovo
`wsl --install -d Debian`. Se l'errore parla di virtualizzazione (per esempio
`0x80370102`), attiva Intel VT-x o AMD-V/SVM nel BIOS/UEFI.

### 5.2 Pacchetti e prima compilazione

Dal terminale Debian, prima aggiorna il sistema e installa `wget`, che manca
nell'immagine Debian di WSL e serve a VS Code per scaricare la sua parte server:

```sh
sudo apt update && sudo apt full-upgrade
sudo apt install wget ca-certificates
```

Poi segui le sezioni 1, 2 e 3 senza cambiare nulla. Due attenzioni:

- tieni il progetto **dentro Debian** (`~/progetti/tecnigrafo`), non in una
  cartella di Windows sotto `/mnt/c`: la compilazione sarebbe molto più lenta;
- clona e fai i commit con il `git` di Debian, non con Git per Windows sulla
  stessa cartella, per non mescolare le terminazioni di riga.

### 5.3 VS Code

1. Installa VS Code su **Windows** e aggiungi l'estensione **WSL** di Microsoft
   (`ms-vscode-remote.remote-wsl`).
2. Dal terminale Debian apri il progetto: `code ~/progetti/tecnigrafo`. La prima
   volta VS Code installa in Debian la sua parte server.
3. Nella barra di stato, in basso a sinistra, deve comparire **WSL: Debian**.
4. Accetta "Installa" sulle estensioni consigliate: vengono installate dentro
   Debian, non su Windows.
5. Da qui in avanti vale la sezione 4, dal punto 3.

Per avere sempre lo stesso terminale, in Windows Terminal puoi impostare Debian
come profilo predefinito.

### 5.4 Aprire i link nel browser di Windows

Debian in WSL non ha un browser, quindi i programmi che ne aprono uno (per
esempio `gh auth login`) si fermano. Indica loro il browser predefinito di
Windows:

```sh
echo 'export BROWSER="explorer.exe"' >> ~/.bashrc
source ~/.bashrc
```

## 6. Antigravity CLI (Gemini) senza conferme continue

Con le impostazioni predefinite Antigravity CLI (`agy`) chiede conferma prima di
ogni scrittura e di ogni comando, e il responsabile deve restare davanti allo
schermo. Si configura una volta, in Debian (anche in WSL), con due accorgimenti.

**1. Regole dei permessi.** In `~/.gemini/antigravity-cli/settings.json` si
autorizzano i comandi che servono al lavoro su Tecnigrafo e si vietano quelli
che spettano solo al responsabile. Il divieto vince sempre sull'autorizzazione:

```json
{
  "permissions": {
    "allow": [
      "command(git)",
      "command(gh)",
      "command(cmake)",
      "command(ctest)",
      "command(python3)",
      "command(scripts/controlla.sh)",
      "command(scripts/ci-attendi.sh)",
      "command(scripts/ci-errori.sh)"
    ],
    "deny": [
      "command(regex:^gh pr merge)",
      "command(regex:^gh repo delete)",
      "command(regex:^git push.*(-f|--force))",
      "command(regex:^git push.* main$)"
    ]
  }
}
```

Le stesse regole si possono vedere e modificare dentro `agy` con `/permissions`.
La sintassi viene da guide di terze parti e può cambiare tra le versioni: dopo
averle scritte, controlla con `/permissions` che `agy` le legga.

**2. Modalità automatica e artifact.** Si avvia con:

```sh
agy --mode auto
```

In questa modalità (da `agy` 1.3.5, su Linux e macOS) le modifiche ai file e i
comandi sicuri si approvano da soli, mentre le azioni irreversibili chiedono
ancora conferma. In `/config` si imposta anche la revisione degli artifact
(piani e documenti) su *agent-decides*: altrimenti l'agente si ferma ad
aspettare che il responsabile approvi il piano.

Non usare `--dangerously-skip-permissions` né `toolPermission` su
`always-proceed`: approvano tutto senza chiedere, comprese le azioni
irreversibili, e un testo malevolo letto in una issue o in una pagina web
potrebbe far eseguire comandi qualsiasi.

Nell'IDE Antigravity la lista dei comandi autorizzati è segnalata come
inaffidabile (comandi richiesti anche se autorizzati, voci spostate dopo il
riavvio): per lavorare a lotti si usa la CLI.

## 7. `tecnigrafo-mcp` per gli agenti

`tecnigrafo-mcp` è un server MCP di sviluppo ([ADR 0027](adr/0027-server-mcp-di-sviluppo.md)):
un agente lo collega al suo strumento e **esegue comandi di Tecnigrafo dal
vivo**, leggendo subito il rapporto, invece di scrivere uno script e lanciarlo.
Parla MCP su stdio, senza porte di rete e senza Qt, e **non si installa**: è
un eseguibile di sviluppo, come `tecnigrafo-script`. Non sostituisce la CI, che
resta la sola a fare fede. Gli strumenti che offre, con argomenti ed esempi,
sono in [docs/verifica/mcp.md](verifica/mcp.md).

Chi scrive i riferimenti della modalità di controllo **non lo usa** per
ricavare un valore atteso (AGENTS.md, "Modalità di controllo e riferimenti").

### 7.1 Dove si trova e come si avvia

Si compila insieme al resto, con gli stessi preset della sezione 3:

```sh
cmake --preset ci && cmake --build --preset ci
```

Senza Qt basta `cmake --preset ci -DTG_BUILD_APP=OFF`: `tecnigrafo-mcp` non lo
usa. L'eseguibile è in

```
build<TG_BUILD_SUFFIX>/<preset>/mcp/tecnigrafo-mcp
```

quindi `build/ci/mcp/tecnigrafo-mcp` per la tua cartella, e per esempio
`build-codex/ci/mcp/tecnigrafo-mcp` per quella di un agente (AGENTS.md,
"Comandi"). Per vedere che funziona: `tecnigrafo-mcp --version`.

Le opzioni:

| Opzione | Effetto |
|---|---|
| `--root <cartella>` | la sola cartella in cui i comandi dei file leggono e scrivono (`OPEN`, `SAVEAS`, i casi di `check_case`). Senza, nessun accesso ai file |
| `--sources <cartella>` | i file pubblici di `sorgenti.json` già scaricati, in sola lettura come `@sources/<nome>` (di solito `build/verifica-sorgenti`) |
| `--questions` | offre anche lo strumento `ask`, per le domande al modello |

Il server aspetta messaggi su stdin e finisce alla fine dell'input: non lo si
avvia a mano, lo avvia lo strumento dell'agente. Le configurazioni qui sotto
usano una cartella di lavoro dentro `build/`, che Git ignora; crea prima la
cartella, perché `--root` deve esistere:

```sh
mkdir -p ~/progetti/tecnigrafo/build/mcp-lavoro
```

Nelle configurazioni scrivi **percorsi assoluti** (`/home/<utente>/progetti/…`):
`~` e le variabili non si espandono dappertutto.

**Non puntare `--root` a `tests/verifica/casi`** per lavorare: `save`
scriverebbe dentro i casi, che nessun pacchetto di lavoro può modificare. Per
provare un caso con `check_case`, che non scrive mai dentro `--root`, avvia un
secondo server con `--root tests/verifica/casi` e `--sources
build/verifica-sorgenti`.

### 7.2 Claude Code

```sh
claude mcp add --transport stdio tecnigrafo -- \
    /home/<utente>/progetti/tecnigrafo/build/ci/mcp/tecnigrafo-mcp \
    --root /home/<utente>/progetti/tecnigrafo/build/mcp-lavoro
```

Le opzioni di `claude` vengono **prima** del nome, e `--` separa quelle dal
comando che avvia il server: senza, Claude Code leggerebbe `--root` come una
sua opzione. La configurazione resta nel tuo `~/.claude.json`, per questo
progetto soltanto (`--scope local`, quello predefinito). Non usare `--scope
project`: scriverebbe un `.mcp.json` nella radice del repository con percorsi
del tuo computer.

Controllo: `claude mcp list` deve mostrare `tecnigrafo` come connesso, e `/mcp`
dentro una sessione ne elenca gli strumenti. Per togliere il server: `claude
mcp remove tecnigrafo`.

Fonte: [Connect Claude Code to tools via MCP](https://code.claude.com/docs/en/mcp).

### 7.3 Codex

```sh
codex mcp add tecnigrafo -- \
    /home/<utente>/progetti/tecnigrafo/build/ci/mcp/tecnigrafo-mcp \
    --root /home/<utente>/progetti/tecnigrafo/build/mcp-lavoro
```

Il comando scrive in `~/.codex/config.toml`, dove si può anche scrivere a mano
la stessa voce:

```toml
[mcp_servers.tecnigrafo]
command = "/home/<utente>/progetti/tecnigrafo/build/ci/mcp/tecnigrafo-mcp"
args = ["--root", "/home/<utente>/progetti/tecnigrafo/build/mcp-lavoro"]
```

Come per Claude Code, `--` separa il comando dalle opzioni di `codex`. Dopo
averlo aggiunto, avvia `codex` in una sessione nuova; `codex mcp --help`
elenca i sottocomandi disponibili.

Fonte: [Codex: Model Context Protocol](https://developers.openai.com/codex/mcp).
La sintassi di `codex mcp add` e della tabella `[mcp_servers.<nome>]` è
quella della documentazione di Codex nel repository
[openai/codex](https://github.com/openai/codex); se il comando risponde con un
errore, fa fede la pagina.

### 7.4 Antigravity

Antigravity non ha, nella sua documentazione, un comando per aggiungere un
server: si modifica il file `mcp_config.json`. Per Tecnigrafo usa quello
**globale**, `~/.gemini/config/mcp_config.json`, non quello del progetto
(`<progetto>/.agents/mcp_config.json`), che finirebbe nel repository con i
percorsi del tuo computer:

```json
{
  "mcpServers": {
    "tecnigrafo": {
      "command": "/home/<utente>/progetti/tecnigrafo/build/ci/mcp/tecnigrafo-mcp",
      "args": ["--root", "/home/<utente>/progetti/tecnigrafo/build/mcp-lavoro"]
    }
  }
}
```

Se il file esiste già, aggiungi solo la voce `tecnigrafo` dentro `mcpServers`.
Non confonderlo con `~/.gemini/antigravity-cli/settings.json` della sezione 6,
che contiene i permessi. Nell'IDE lo stesso file si apre dal pannello
dell'agente: **…**, **MCP Servers**, **Manage MCP Servers**, **View raw
config**.

Controllo: nella CLI scrivi `/mcp` e premi `Invio`; il gestore mostra lo stato
del server (attivo, disconnesso, in caricamento) e si può ricaricare la
configurazione.

Fonte: [Antigravity: MCP](https://antigravity.google/docs/mcp).

### 7.5 La prova: seguire la guida senza altre istruzioni

Con il server collegato, in una sessione nuova dell'agente chiedi, senza dare
altre istruzioni:

> Usa `tecnigrafo-mcp`: elenca i comandi, descrivi `LINE`, disegna una linea da
> 0,0 a 10,0, leggi il rapporto e salvala come `prova.dxf`.

Deve usare `list_commands`, `describe_command`, `run` (con le righe `LINE`,
`0,0`, `10,0` e una stringa vuota), `report` e `save`. Il rapporto deve avere
`257 LINE start=0,0 end=10,0` tra le entità, e `build/mcp-lavoro/prova.dxf`
deve esistere. Poi `reset`, `open` con lo stesso nome e `report`: la linea è
di nuovo là.

Se l'agente non trova gli strumenti: il server non è connesso (controlla con i
comandi di ogni sezione), il percorso dell'eseguibile non è assoluto o la
compilazione non è stata fatta. Gli errori del server arrivano su stderr, che
lo strumento dell'agente di solito mostra nei suoi log.

## 8. Windows nativo con Visual Studio

Qui si lavora **senza WSL**: tutto dentro Windows, con il compilatore di Visual
Studio (MSVC) e Qt per Windows. Il risultato è `tecnigrafo.exe`, la build che vale
come prova su Windows: la prova manuale e il benchmark richiesti per chiudere una
tappa si fanno qui. Non si mescola con la sezione 5, che è un ambiente a parte
(vedi la tabella "Windows o WSL: dove sono?" in testa al file).

**Tutti i comandi di questa sezione si scrivono in PowerShell, non nel terminale
Debian.**

### 8.1 Cosa installare (una volta sola)

1. **Visual Studio 2022 Community**, con il carico di lavoro "Sviluppo di
   applicazioni desktop con C++": porta compilatore, CMake e Ninja.
2. **Qt 6.8** per MSVC 2022 64-bit, con l'installer ufficiale (il componente
   "MSVC 2022 64-bit"; il progetto non chiede moduli aggiuntivi). Negli esempi la
   cartella è `C:\Qt\6.8.3\msvc2022_64`: se hai un'altra versione, cambia il
   percorso.
3. **Git per Windows**, per scaricare il progetto.

### 8.2 Scaricare il progetto

In PowerShell, in una cartella corta (i percorsi lunghi danno problemi a MSVC):

```powershell
mkdir C:\progetti
cd C:\progetti
git clone https://github.com/Matleyx78/tecnigrafo
cd tecnigrafo
```

È una copia **diversa** da quella che hai in Debian: le modifiche non passano
dall'una all'altra se non con `git push` e `git pull`.

### 8.3 Aprire il terminale a 64 bit (ogni volta)

Per compilare serve un terminale **a 64 bit** con l'ambiente di Visual Studio.
Il collegamento "Developer PowerShell for VS 2022" del menu Start apre un
ambiente a 32 bit, che non si collega a Qt `msvc2022_64`: non usarlo. Due modi
giusti:

- apri dal menu Start **"x64 Native Tools Command Prompt for VS 2022"** (è un
  prompt `cmd`: per usare i comandi PowerShell di questa sezione scrivi
  `powershell`); oppure
- in una PowerShell normale:

  ```powershell
  Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
  & "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
  ```

Controlla di essere nel posto giusto:

```powershell
where.exe cl      # la prima riga deve contenere Hostx64\x64
cmake --version   # almeno 3.24
```

Se `cmake` o `cl` non vengono riconosciuti, hai aperto un terminale normale: torna
qui sopra.

### 8.4 Compilare

```powershell
cd C:\progetti\tecnigrafo
cmake --preset release -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64
cmake --build --preset release
ctest --preset release
```

Nel percorso di Qt si scrive `/` anche su Windows. Il primo comando ricorda il
percorso di Qt: le volte dopo bastano `cmake --build --preset release` e
`ctest --preset release`.

Il preset `release` serve per usare il programma; per il debugger usa `debug` (gli
stessi comandi con `debug` al posto di `release`; il programma finisce in
`build\debug\app`). Le cartelle sono separate, quindi puoi tenerli entrambi.

Se hai configurato da un terminale sbagliato (a 32 bit, o non di Visual Studio),
cancella la cartella e ricomincia da quello giusto:

```powershell
Remove-Item -Recurse -Force build\release
```

### 8.5 Lanciare tecnigrafo.exe

Dopo la compilazione il programma è in `build\release\app\tecnigrafo.exe`, ma da
solo non parte: Windows non trova le DLL di Qt. Si risolve in uno di due modi.

**A. Aggiungere Qt al `PATH` (il più comodo per lavorare).** Vale solo per la
finestra di terminale in cui lo scrivi: in una finestra nuova va ripetuto.

```powershell
$env:PATH = "C:\Qt\6.8.3\msvc2022_64\bin;$env:PATH"
.\build\release\app\tecnigrafo.exe
```

**B. Copiare le DLL accanto al programma con `windeployqt`.** Si fa una volta per
cartella di build, e va rifatto solo se la cancelli.

```powershell
& C:\Qt\6.8.3\msvc2022_64\bin\windeployqt.exe build\release\app\tecnigrafo.exe
.\build\release\app\tecnigrafo.exe
```

Per la build di debug sostituisci `release` con `debug` e aggiungi `--debug` a
`windeployqt`.

La finestra di Tecnigrafo si apre sul desktop e il terminale torna subito libero:
è normale, l'applicazione non ha una console.

Se qualcosa non va:

| Sintomo | Causa | Rimedio |
|---|---|---|
| `cmake` o `cl` "non riconosciuto" | terminale normale | 8.3 |
| "Could not find a package configuration file provided by Qt6" | manca `-DCMAKE_PREFIX_PATH=...` | rilancia il primo comando di 8.4 |
| errori di collegamento tra x86 e x64 | configurato da un terminale a 32 bit | cancella `build\release`, riparti da 8.3 |
| Windows dice che `Qt6Core.dll` (o `Qt6Cored.dll`) non è stata trovata | mancano le DLL di Qt | 8.5, A o B |

### 8.6 Aprire il progetto nell'IDE di Visual Studio (per il debugger)

Per compilare e lanciare bastano 8.3-8.5. L'IDE serve soprattutto per il
**debugger**: il `launch.json` condiviso di VS Code usa gdb, pensato per Linux e
WSL, e non va bene per un programma compilato con MSVC.

> Questo percorso nasce dalla documentazione di Visual Studio e dai preset del
> progetto, ma **non è stato provato**. Se un passo non torna, i comandi da
> terminale di 8.3-8.5 restano la strada sicura: sono quelli del
> [README](../README.md), e la CI compila con MSVC nello stesso modo.

I preset del progetto non conoscono la tua cartella di Qt. La si dice in un file
personale, `CMakeUserPresets.json`, nella cartella del progetto (Git lo ignora,
quindi resta solo sul tuo PC):

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "win-debug",
      "displayName": "Windows Debug (Qt 6.8.3)",
      "inherits": "debug",
      "architecture": { "value": "x64", "strategy": "external" },
      "cacheVariables": { "CMAKE_PREFIX_PATH": "C:/Qt/6.8.3/msvc2022_64" },
      "environment": { "PATH": "C:/Qt/6.8.3/msvc2022_64/bin;$penv{PATH}" }
    }
  ],
  "buildPresets": [
    { "name": "win-debug", "configurePreset": "win-debug" }
  ]
}
```

Poi:

1. In Visual Studio scegli **Apri una cartella locale** e seleziona
   `C:\progetti\tecnigrafo`.
2. Nella barra degli strumenti scegli la configurazione **Windows Debug (Qt
   6.8.3)**: Visual Studio configura da solo il progetto (l'avanzamento è nella
   finestra *Output*).
3. Compila con il menu **Compila**, voce *Compila tutto* (`Ctrl+Maiusc+B`).
4. Come elemento di avvio scegli `tecnigrafo.exe`, poi `F5` per avviarlo nel
   debugger (`Ctrl+F5` senza debugger). Con questo preset il programma è in
   `build\win-debug\app`.

Se all'avvio mancano le DLL di Qt, usa `windeployqt --debug` come in 8.5, B, sul
file `build\win-debug\app\tecnigrafo.exe`.
