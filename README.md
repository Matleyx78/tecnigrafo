# Tecnigrafo

CAD libero 2D/3D per Windows, Linux e macOS.

Il nome viene dal tecnigrafo, il tavolo da disegno con le squadre snodate su cui
generazioni di progettisti hanno disegnato a mano: lo strumento di precisione per
eccellenza, prima del computer.

**Obiettivo:** un CAD 2D nello spirito di AutoCAD e, sulla stessa base, un modellatore
3D parametrico nello spirito di Inventor. Software libero, per sempre (GPLv3).

> Stato: **v0.2.0, comandi e annullamento.** Si disegnano polilinee di segmenti
> retti con il comando `PLINE`, indicando i punti con il mouse o scrivendo le
> coordinate sulla riga di comando, e si annulla e ripete il lavoro. Non si
> possono ancora salvare file, selezionare o cancellare entità. Novità e limiti
> sono nelle [note di rilascio](docs/rilasci/0.2.md).

## Disegnare

Scrivi `pl` e premi `Invio`, poi indica i punti con il clic sinistro o scrivendo
le coordinate (`100,50`, `@20,0`, `@50<90`). `Invio` conclude la polilinea, `c`
la chiude, `Esc` annulla il comando. Il funzionamento completo è nel
[manuale dei comandi](docs/manuale/comandi.md).

| Azione | Come |
|---|---|
| Avviare un comando | scriverne il nome e premere `Invio` (anche con la vista in primo piano) |
| Indicare un punto | clic sinistro, oppure scrivere le coordinate |
| Concludere | `Invio`, `Spazio` o clic destro |
| Annullare il comando in corso | `Esc` |
| Ripetere l'ultimo comando | `Invio` a riga vuota |
| Annulla / Ripeti | `Ctrl+Z` / `Ctrl+Y`, oppure i comandi `U` e `REDO` |

## Comandi della vista

| Azione | Come |
|---|---|
| Zoom verso il cursore | rotella del mouse |
| Pan | trascinamento con il tasto centrale |
| Zoom estensioni | doppio clic con il tasto centrale, oppure `Ctrl+Shift+E` |

## Compilare

Servono un compilatore C++20, CMake ≥ 3.24, Ninja e Qt ≥ 6.4 (solo il modulo Widgets).
GoogleTest viene scaricato da CMake se non è installato.

### Debian 12 e 13 / Ubuntu 24.04

Procedura completa, con VS Code e gli strumenti di formattazione:
[docs/AMBIENTE-SVILUPPO.md](docs/AMBIENTE-SVILUPPO.md). In breve:

```sh
sudo apt install build-essential cmake ninja-build git qt6-base-dev libgtest-dev clang-format
cmake --preset release
cmake --build --preset release
ctest --preset release
./build/release/app/tecnigrafo
```

### Windows

Installa Visual Studio 2022 Community (carico di lavoro "Sviluppo di applicazioni
desktop con C++") e Qt 6.8 per MSVC 2022 con l'installer ufficiale. Da un
"Developer PowerShell for VS 2022":

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
cmd/      comandi, riga di comando, lettura delle coordinate (nessun Qt)
render/   viewport e disegno della scena, indipendente dalla grafica (nessun Qt)
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
[docs/ROADMAP.md](docs/ROADMAP.md).

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
