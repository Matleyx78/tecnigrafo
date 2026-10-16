# Ambiente di sviluppo su Debian con VS Code

Procedura verificata per Debian 12 (bookworm) e Debian 13 (trixie). Per Windows e
macOS vedi il [README](../README.md).

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
