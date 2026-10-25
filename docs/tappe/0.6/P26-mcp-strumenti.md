# [0.6-P26] `mcp`: strumenti e confronto con `tecnigrafo-script`

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** codex
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: il riferimento è `tecnigrafo-script`,
  confrontato su tutti i casi di verifica (ADR 0027, punto 6)
- **Dipende da:** P5, P25
- **Modulo:** `mcp`
- **Da leggere:** [ADR 0027](../../adr/0027-server-mcp-di-sviluppo.md) punti
  4–6; `docs/verifica/script.md`, `rapporto.md`, `casi.md`;
  `script/include/tg/script/level1.hpp`, `case_check.hpp`, `case_files.hpp`,
  `report.hpp`, `script.hpp`; `mcp/include/tg/mcp/server.hpp` e `tool.hpp` (P25)

## File

- `mcp/include/tg/mcp/session.hpp`, `mcp/src/session.cpp` (nuovi: la
  sessione dal vivo)
- `mcp/src/session_tools.cpp` (nuovo: `run`, `report`, `reset`, `open`, `save`)
- `mcp/src/case_tools.cpp` (nuovo: `run_script`, `check_case`, `ask`)
- `mcp/tool/main.cpp`, `mcp/CMakeLists.txt`
- `script/include/tg/script/level1.hpp`, `script/src/level1.cpp` (solo se
  serve esporre il passo di una riga, senza cambiarne il comportamento)
- `tests/mcp/session_test.cpp`, `tests/mcp/equivalence_test.cpp` (nuovi)
- `tests/CMakeLists.txt`, `tests/verifica/CMakeLists.txt` (il test di
  equivalenza sui casi)

## Cosa fare

### La sessione

Un documento, l'editor, il `CommandRunner` con i comandi di `cmd`, il
controllo delle descrizioni acceso (ADR 0026), la cronologia, e
`script::CaseFiles` con `--root` e `--sources` come accesso ai file (senza
`--root` nessun accesso). Le righe si eseguono **come il livello 1**:
stesso eco, stesse direttive (`!move`, `!click`, `!esc`, `!snap`…), stesse
regole di determinismo. Una direttiva del livello 2 è un errore dello
strumento.

### Strumenti (ADR 0027, punto 5)

| Strumento | Argomenti | Risultato |
|---|---|---|
| `run` | `lines`: array di stringhe | testo: le righe nuove della cronologia, poi `prompt: <domanda>` |
| `report` | — | il rapporto dello stato attuale, nel formato di `rapporto.md` (prima riga e una sezione `[final]`) |
| `reset` | — | `prompt: Command:` |
| `open` | `path` | come `run` con `OPEN` e il nome |
| `save` | `path` | come `run` con `SAVEAS` e il nome (con la conferma di sostituzione se il file esiste: `replace: true` risponde `Y`) |
| `run_script` | `script`: testo `.tgs` | il rapporto completo, **identico** all'uscita di `tecnigrafo-script run` sullo stesso testo |
| `check_case` | `path`: cartella del caso dentro `--root` | esito (`passed`, `not passed`, `invalid`, `needs level 2`) e le differenze, come `tecnigrafo-script check` |
| `ask` | `questions`: testo `.tgq` | le risposte e l'esito, come `tecnigrafo-script`; **solo con `--questions`** |

- `inputSchema` di ogni strumento con i tipi e i campi obbligatori.
- `path` relativo a `--root`, senza `..`, senza percorsi assoluti: altrimenti
  `isError` con il messaggio di `FileAccess`.
- Senza `--questions`, `ask` non compare in `tools/list` e una chiamata
  risponde come a uno strumento sconosciuto.

### Equivalenza

`equivalence_test.cpp` (etichetta CTest `verifica`): per ogni caso di
`tests/verifica/casi/` di livello 1, `run_script` dà lo stesso rapporto di
`tecnigrafo-script run` e `check_case` lo stesso esito di `tecnigrafo-script
check`, con le stesse cartelle `--sources`. Un caso in `in-attesa.txt` si
confronta lo stesso: l'equivalenza non dipende dall'esito.

## Regole della tappa che valgono qui

- `mcp` non usa Qt; nessuna porta di rete.
- Non cambiare il comportamento di `script`: se serve esporre qualcosa,
  solo nuove funzioni.

## Test richiesti

- Sessione: `run` con comandi, direttive e righe non valide; `report` dopo
  ogni passo; `reset`; `open` e `save` dentro e fuori da `--root`.
- `ask` presente solo con `--questions`.
- Equivalenza su tutti i casi.

## Fatto quando

- Il test di equivalenza passa su tutti i casi, sui tre sistemi.
- Un agente collegato a `tecnigrafo-mcp` esegue comandi, legge il rapporto,
  elenca i comandi, apre e salva DXF e `.cadx` (lo prova anche la prova
  manuale, passo 8).
- La CI è verde.
