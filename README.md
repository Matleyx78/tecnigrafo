# Tecnigrafo

CAD libero 2D/3D per Windows, Linux e macOS.

Il nome viene dal tecnigrafo, il tavolo da disegno con le squadre snodate su cui
generazioni di progettisti hanno disegnato a mano: lo strumento di precisione per
eccellenza, prima del computer.

**Obiettivo:** un CAD 2D nello spirito di AutoCAD e, sulla stessa base, un modellatore
3D parametrico nello spirito di Inventor. Software libero, per sempre (GPLv3).

> Stato: **v0.1.0, fondamenta.** Una finestra mostra un disegno di esempio fatto di
> polilinee (con archi), con griglia, zoom e pan. Nessuna funzione di disegno
> interattivo, nessun file ancora.

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
render/   viewport e disegno della scena, indipendente dalla grafica (nessun Qt)
app/      applicazione desktop Qt: finestre, input, backend QPainter
tests/    test automatici (GoogleTest)
docs/     roadmap e tappe, decisioni architetturali (ADR), stile, guide
```

Le dipendenze vanno in una sola direzione: `core ← render ← app`. La mappa
completa è in [docs/ARCHITETTURA.md](docs/ARCHITETTURA.md).

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
