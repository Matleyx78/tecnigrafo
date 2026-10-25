# Tecnigrafo

CAD libero 2D/3D per Windows, Linux e macOS.

Il nome viene dal tecnigrafo, il tavolo da disegno con le squadre snodate su cui
generazioni di progettisti hanno disegnato a mano: lo strumento di precisione per
eccellenza, prima del computer.

**Obiettivo:** un CAD 2D nello spirito di AutoCAD e, sulla stessa base, un modellatore
3D parametrico nello spirito di Inventor. Software libero, per sempre (GPLv3).

> Stato: **versione 0.5.0 — file: formato nativo e DXF.** Si aprono e si salvano
> disegni in formato nativo `.cadx` e DXF con `OPEN`, `QSAVE`, `SAVE`, `SAVEAS` e
> `NEW`, anche da menu File, scorciatoie e finestre di dialogo. Si disegnano linee
> con `LINE`, polilinee con archi con `PLINE`, cerchi con `CIRCLE`, archi con `ARC`
> e testi allineati, obliqui o specchiati con `TEXT`. Si selezionano e cancellano
> entità con `ERASE`, si agganciano punti notevoli (estremo, medio, centro,
> inserimento) e si usa l'orto. Le modifiche si possono annullare e ripetere. Le
> [note della 0.5](docs/rilasci/0.5.md) riportano novità e limiti noti. Prossima
> tappa: [0.6](docs/tappe/0.6-layer-e-proprieta.md), layer, colori, tipi di linea,
> proprietà; impianto dell'interfaccia.

## Disegnare

Scrivi `l` per linee, `pl` per polilinee, `c` per cerchi, `a` per archi o `dt`
per testi e premi `Invio`, poi indica i punti con il clic sinistro o scrivendo
le coordinate (`100,50`, `@20,0`, `@50<90`). `Invio` conclude il comando, `c`
chiude linee e polilinee, `Esc` annulla il comando conservando i tratti già
completati. Il funzionamento completo è nel
[manuale dei comandi](docs/manuale/comandi.md).

Per cancellare, scegli le entità sul bordo oppure con un rettangolo a due clic,
poi premi `Canc`; oppure scrivi `e`, scegli le entità e conferma con `Invio`.
`F3` attiva o spegne lo snap a oggetto, `F8` l'orto; trovi le stesse funzioni nel
menu *Tools* e nei pulsanti OSNAP e ORTHO della barra di stato. All'avvio lo snap
è acceso con estremo, punto medio, centro e inserimento, mentre l'orto è spento.

| Azione | Come |
|---|---|
| Avviare un comando | scriverne il nome e premere `Invio` (anche con la vista in primo piano) |
| Indicare un punto | clic sinistro, oppure scrivere le coordinate |
| Selezionare / togliere dalla selezione | clic sul bordo / `Shift`+clic; a riposo o durante `Select objects:` |
| Finestra / intersezione | primo clic nel vuoto, secondo a destra / a sinistra del primo |
| Cancellare | `ERASE` o `E`; `Canc` o *Edit → Erase* con una selezione pronta |
| Svuotare la selezione | `Esc` a riposo; con un rettangolo aperto lo chiude prima |
| Snap a oggetto / orto | `F3` / `F8`; sui portatili può servire `Fn` |
| Concludere | `Invio`, `Spazio` o clic destro |
| Annullare il comando in corso | `Esc` |
| Ripetere l'ultimo comando | `Invio` a riga vuota |
| Annulla / Ripeti | `Ctrl+Z` / `Ctrl+Y`, oppure i comandi `U` e `REDO` |
| Nuovo / Apri / Salva | `Ctrl+N` / `Ctrl+O` / `Ctrl+S`, oppure i comandi `NEW`, `OPEN`, `QSAVE` |

Nel campo comandi `Canc` cancella entità solo se il campo è vuoto; con testo
presente modifica il testo. Le coordinate scritte restano esatte anche con snap
e orto attivi; la sola distanza (`d`) segue la direzione del puntatore corretto.
Uno snap disponibile prevale sull'orto. Impostazioni e selezione non si salvano
alla chiusura del programma.

## Comandi della vista

| Azione | Come |
|---|---|
| Zoom verso il cursore | rotella del mouse |
| Pan | trascinamento con il tasto centrale |
| Zoom estensioni | doppio clic con il tasto centrale, oppure `Ctrl+Shift+E` |

## Compilare

Servono un compilatore C++20, CMake ≥ 3.24, Ninja e Qt ≥ 6.8 (solo il modulo Widgets).
GoogleTest viene scaricato da CMake se non è installato.

### Debian 13

Procedura completa, con VS Code e gli strumenti di formattazione:
[docs/AMBIENTE-SVILUPPO.md](docs/AMBIENTE-SVILUPPO.md). In breve:

```sh
sudo apt install build-essential cmake ninja-build git qt6-base-dev libgtest-dev clang-format
cmake --preset release
cmake --build --preset release
ctest --preset release
./build/release/app/tecnigrafo
```

Debian 12 e Ubuntu 24.04 hanno Qt 6.4: non basta (ADR 0028). Installa Qt 6.8 a
parte (installer ufficiale o aqtinstall) e passa a CMake
`-DCMAKE_PREFIX_PATH=<cartella di Qt 6.8>`; quei sistemi non si provano in CI.

### Windows

Installa Visual Studio 2022 Community (carico di lavoro "Sviluppo di applicazioni
desktop con C++") e Qt 6.8 per MSVC 2022 64-bit con l'installer ufficiale.

Serve un terminale **a 64 bit**. Il collegamento "Developer PowerShell for VS 2022"
del menu Start apre un ambiente a 32 bit, che non si collega a Qt `msvc2022_64`.
Usa "x64 Native Tools Command Prompt for VS 2022", oppure, in una PowerShell:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
```

La configurazione deve indicare il compilatore `Hostx64/x64/cl.exe`. Poi:

```powershell
cmake --preset release -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64
cmake --build --preset release
ctest --preset release
```

Prima di avviare `tecnigrafo.exe`, esegui `windeployqt` sull'eseguibile oppure aggiungi
`C:\Qt\6.8.3\msvc2022_64\bin` al `PATH`.

### macOS

La CI compila Tecnigrafo e ne esegue tutti i test su macOS a ogni modifica, ma il
responsabile non ha un Mac: la prova a mano dipende da chi lo usa. Se lavori su
macOS, le tue segnalazioni sono preziose.

```sh
xcode-select --install
brew install cmake ninja qt
cmake --preset release -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build --preset release
ctest --preset release
open build/release/app/tecnigrafo.app
```

## Struttura

```
core/     geometria e modello dati (C++20 puro, nessuna dipendenza)
edit/     modifiche al documento, transazioni, annulla e ripeti (nessun Qt)
cmd/      comandi, coordinate, selezione e aiuti al disegno (nessun Qt)
render/   viewport, scena, evidenziazione e marcatori (nessun Qt)
app/      applicazione desktop Qt: finestre, input, backend QPainter
tests/    test automatici (GoogleTest, Qt Test per app)
docs/     roadmap e tappe, decisioni architetturali (ADR), manuale, note di rilascio
```

Le dipendenze vanno in una sola direzione: `core ← edit ← cmd ← app` e
`core ← render ← app`. La mappa completa è in
[docs/ARCHITETTURA.md](docs/ARCHITETTURA.md).

## Roadmap

Il progetto avanza per tappe, una alla volta: ogni tappa ha obiettivo, confini,
criteri di chiusura e un piano diviso in pacchetti di lavoro. Lo stato è in
[docs/ROADMAP.md](docs/ROADMAP.md); quanto manca ad AutoCAD, comando per
comando, nel [catalogo di parità](docs/parita/comandi.md). Il perché delle
scelte di partenza è in [docs/VALUTAZIONI-INIZIALI.md](docs/VALUTAZIONI-INIZIALI.md).

## Contribuire

Persone e assistenti IA sono benvenuti, alle stesse regole:

- [CONTRIBUTING.md](CONTRIBUTING.md) — come scegliere un pacchetto di lavoro e
  portarlo fino al merge;
- [AGENTS.md](AGENTS.md) — istruzioni per gli agenti IA;
- [GOVERNANCE.md](GOVERNANCE.md) — chi decide e come.

Il progetto ha una direzione unica: le tappe le definisce il responsabile, chi
contribuisce realizza i pacchetti della tappa aperta.

## Licenza

GNU General Public License v3.0 o successiva. Vedi [LICENSE](LICENSE).

## Componenti di terze parti

Il font incluso **Liberation Sans Regular 2.1.5** è distribuito senza modifiche
con licenza [SIL Open Font License 1.1](resources/fonts/OFL.txt).
[Fonte, versione e SHA-256](resources/fonts/README.md) sono registrati accanto al font.
Le metriche usate dal modello sono generate dal file TTF durante la build.
