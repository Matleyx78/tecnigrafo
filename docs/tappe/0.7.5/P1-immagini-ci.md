# [0.7.5-P1] CI: immagini `windows-2025-vs2026` e `macos-26`, azioni allineate

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: infrastruttura, non cambia il comportamento; i
  test e i casi di verifica esistenti, invariati, sono la prova
- **Dipende da:** —
- **Modulo:** `.github/` (più le correzioni al codice che le immagini nuove
  chiedono, in qualunque modulo)
- **Da leggere:** [ADR 0038](../../adr/0038-ci-dei-due-progetti.md), punti 1, 2 e
  7; [DECISIONI.md](../../DECISIONI.md), 2026-11-07, «Immagini della CI»;
  `.github/workflows/ci.yml`, `solid-space-main.yml`, `copilot-setup-steps.yml`;
  in solid-space, in sola lettura, `.github/workflows/ci.yml` e la sua #199
  (le correzioni che le immagini nuove avevano chiesto lì)

## File

- `.github/workflows/ci.yml`, `.github/workflows/solid-space-main.yml`,
  `.github/workflows/copilot-setup-steps.yml`
- i file di codice che il compilatore nuovo rifiuta (MSVC di Visual Studio 2026,
  Apple Clang di `macos-26`): li elenchi nella pull request, ognuno con
  l'avviso o l'errore che lo ha toccato

## Cosa fare

1. **Immagini:** il lavoro Windows su `windows-2025-vs2026`, il lavoro macOS su
   `macos-26`; Linux resta `ubuntu-24.04`. Nessuna etichetta `-latest`.
2. **Qt su Windows:** `install-qt-action` installa ancora Qt 6.8.3; con MSVC
   2026 si tiene l'architettura `win64_msvc2022_64` se compila e passa (l'ABI di
   MSVC è compatibile dal 2015). Se non basta, fermati e scrivilo nella issue:
   cambiare la versione di Qt è una decisione (ADR 0028).
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
- Ogni file di `.github/workflows/` usa le stesse versioni principali delle
  azioni comuni.
- La CI è verde.
