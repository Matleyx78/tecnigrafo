# [0.7.5-P1] CI: immagini `windows-2025-vs2026` e `macos-26`, azioni allineate

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: infrastruttura, non cambia il comportamento; i
  test e i casi di verifica esistenti, invariati, sono la prova
- **Dipende da:** —
- **Modulo:** `.github/` e la versione di Qt (ADR 0043), più le correzioni al
  codice che le immagini nuove e Qt 6.12 chiedono, in qualunque modulo
- **Da leggere:** [ADR 0038](../../adr/0038-ci-dei-due-progetti.md), punti 1, 2 e
  7; [ADR 0043](../../adr/0043-qt-6-12.md);
  [DECISIONI.md](../../DECISIONI.md), 2026-11-07, «Immagini della CI» e
  «Qt 6.12»;
  `.github/workflows/ci.yml`, `solid-space-main.yml`, `copilot-setup-steps.yml`;
  in solid-space, in sola lettura, `.github/workflows/ci.yml` e la sua #199
  (le correzioni che le immagini nuove avevano chiesto lì)

## File

- `.github/workflows/ci.yml`, `.github/workflows/solid-space-main.yml`,
  `.github/workflows/copilot-setup-steps.yml`
- per Qt 6.12 (ADR 0043): `app/CMakeLists.txt`, `tests/CMakeLists.txt`,
  `tests/verifica/CMakeLists.txt`, `third_party/CMakeLists.txt` (le chiamate
  `find_package(Qt6 …)`); `README.md`, `docs/AMBIENTE-SVILUPPO.md`,
  `.github/copilot-instructions.md` (versione e installazione di Qt)
- i file di codice che il compilatore nuovo rifiuta (MSVC di Visual Studio 2026,
  Apple Clang di `macos-26`): li elenchi nella pull request, ognuno con
  l'avviso o l'errore che lo ha toccato

## Cosa fare

1. **Immagini:** il lavoro Windows su `windows-2025-vs2026`, il lavoro macOS su
   `macos-26`; Linux resta `ubuntu-24.04`. Nessuna etichetta `-latest`.
2. **Qt 6.12** ([ADR 0043](../../adr/0043-qt-6-12.md); decisione del
   2026-11-07, dopo che Qt 6.8.3 non collegava su `macos-26` per il framework
   AGL):
   - in ogni lavoro che installa Qt, `version: '6.12.0'`; su Windows
     l'architettura resta `win64_msvc2022_64`;
   - aqtinstall con `aqtsource` dallo SHA intero di un commit del ramo
     principale di aqtinstall che contiene `7e5a5c3` (al 2026-11-07 l'ultimo è
     `5a49f2b391ae8ee1d3c0dd87f8b33c1d49a4deb7`), in una sola variabile per
     workflow, uguale in tutti i workflow, con un commento che dice perché e
     quando toglierla;
   - `py7zrversion` a una versione esatta che quel commit accetta (al
     2026-11-07 l'ultima è la 1.1.4);
   - `find_package(Qt6 6.12 …)` al posto di `6.8` nei quattro `CMakeLists.txt`
     della lista;
   - nei documenti della lista, Qt 6.12 al posto di 6.8 e 6.8.3, e Qt da
     installare a parte su ogni sistema, Debian 13 compreso.

   Se con Qt 6.12 SARibbon non compila o un suo test fallisce, o se aqtinstall
   non installa Qt su un sistema, fermati e scrivilo nella issue (ADR 0043,
   punti 3 e 5).
3. **Azioni:** per `actions/checkout`, `actions/upload-artifact`,
   `actions/cache` e `actions/setup-python`, l'ultima versione principale
   pubblicata oggi, con il collegamento alla nota di rilascio nella pull request;
   la stessa in tutti i file di `.github/workflows/`. Le altre azioni
   (`jurplel/install-qt-action`, `ilammy/msvc-dev-cmd`) restano come sono,
   salvo che non funzionino sulle immagini nuove.
4. **Correzioni:** ogni avviso o errore nuovo si corregge nel codice, senza
   disattivare avvisi né controlli (AGENTS.md). Una correzione che cambia un
   risultato di un caso di verifica non è una correzione: fermati e scrivilo
   nella issue.
5. Nella pull request scrivi, per solid-space, che cosa deve cambiare lì perché
   le versioni delle azioni coincidano (ADR 0038, punto 7): il responsabile lo
   fa fare fuori tappa, in una chat di solid-space.

## Regole della tappa che valgono qui

- `ci.yml`: dopo di te P3, P4, P5 e P6, in quest'ordine.
- Le sessioni cloud non scaricano i log della CI: leggi gli errori con
  `scripts/ci-errori.sh`.

## Test richiesti

Nessun test nuovo: passano tutti quelli che esistono, sui tre sistemi.

## Fatto quando

- I lavori Windows e macOS girano sulle immagini nuove e sono verdi, con gli
  avvisi come errori.
- Ogni lavoro della CI che usa Qt installa Qt 6.12.0, e la configurazione chiede
  almeno Qt 6.12.
- Ogni file di `.github/workflows/` usa le stesse versioni principali delle
  azioni comuni.
- La CI è verde.
