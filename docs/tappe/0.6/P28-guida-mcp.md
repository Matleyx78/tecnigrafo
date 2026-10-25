# [0.6-P28] Guida a `tecnigrafo-mcp` per gli agenti

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #262
- **Difficoltà:** facile
- **Agente:** gemini
- **Modello e impegno:** leggero; impegno basso
- **Riferimenti:** non servono: documentazione
- **Dipende da:** P26
- **Modulo:** documentazione
- **Da leggere:** [ADR 0027](../../adr/0027-server-mcp-di-sviluppo.md) (tutto);
  le schede P25 e P26; `docs/AMBIENTE-SVILUPPO.md` (sezione 6, come è scritta
  quella di Antigravity); `AGENTS.md` ("Modalità di controllo e riferimenti")

## File

- `docs/AMBIENTE-SVILUPPO.md` (sezione nuova "7. `tecnigrafo-mcp` per gli
  agenti")
- `docs/verifica/mcp.md` (nuovo: gli strumenti, con un esempio per ciascuno)
- `AGENTS.md` (una riga; la approva il responsabile)

## Cosa fare

- **`AMBIENTE-SVILUPPO.md`:** dove si trova l'eseguibile dopo la
  compilazione (`build<TG_BUILD_SUFFIX>/<preset>/mcp/tecnigrafo-mcp`), le
  opzioni, e come collegarlo a Claude Code, Codex e Antigravity con le loro
  configurazioni MCP su stdio. Verifica i comandi e i file di configurazione
  sulla documentazione ufficiale di ciascuno strumento e cita la pagina.
- **`docs/verifica/mcp.md`:** l'elenco degli strumenti con argomenti,
  risultato ed esempio, e il rapporto con `tecnigrafo-script` (stessi
  risultati, ADR 0027 punto 6).
- **`AGENTS.md`**, in "In un pacchetto di riferimenti", dopo la riga "Mai
  eseguendo Tecnigrafo": "Non usare `tecnigrafo-mcp` né `tecnigrafo-script`
  per ricavare un valore atteso (ADR 0027, punto 7)."

## Regole della tappa che valgono qui

- Testi in italiano; comandi e nomi in inglese.
- La modifica ad `AGENTS.md` è una regola: scrivila nella pull request, la
  approva il responsabile.

## Test richiesti

Nessuno: documentazione.

## Fatto quando

- Un agente, seguendo la guida, collega `tecnigrafo-mcp` senza altre
  istruzioni (prova manuale, passo 8).
- La CI è verde.
