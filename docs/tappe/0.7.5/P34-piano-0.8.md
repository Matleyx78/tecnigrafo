# [0.7.5-P34] Piano della tappa 0.8

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: è un piano, scritto con il responsabile
- **Dipende da:** P8, P24, P25, P26, P27
- **Modulo:** `docs`
- **Da leggere:** [GUIDA-RESPONSABILE.md](../../GUIDA-RESPONSABILE.md), B1–B5;
  [LAVORO-IN-PARALLELO.md](../../LAVORO-IN-PARALLELO.md), sezione 1 (con le regole
  che il responsabile ha cambiato dopo P8, se le ha cambiate);
  [tappe/0000-modello.md](../0000-modello.md), [tappe/0000-pacchetto.md](../0000-pacchetto.md);
  la [scheda della 0.8](../0.8-quote-retini-blocchi.md); l'analisi di P8
  (`docs/tappe/0.7.5/analisi-pacchetti-medi.md`); gli
  [ADR 0041](../../adr/0041-documenti-e-ambienti.md) e
  [0040](../../adr/0040-icone-a-ruoli.md); le decisioni del piano della 0.7.5 sui
  blocchi, le quote e i retini (sezione 5)

## Perché

La 0.8 è densa (quote, retini, blocchi, `PEDIT`, canale MCP dell'utente) e deve
partire con basi solide: le decisioni della 0.7.5, le separazioni fatte e
un piano in pacchetti medi, con interfacce e ADR per primi (criteri di chiusura
della 0.7.5).

## File

- `docs/tappe/0.8-quote-retini-blocchi.md` (il piano dettagliato, sezioni 5–8,
  e la scheda dove le decisioni della 0.7.5 la cambiano)
- `docs/tappe/0.8/PT<N>-<nome>.md` (le schede che non dipendono da solid-space)
- `docs/adr/` (le bozze degli ADR della 0.8: entità composte e geometria
  generata, blocchi, canale MCP dell'utente; `PEDIT` se serve)

## Cosa fare

Il lavoro di B1 della guida, nella chat con il responsabile, con questi punti
fissi:

1. prefissi `PT`/`RT`/`CT` (LAVORO-IN-PARALLELO.md);
2. pacchetti medi dove l'analisi di P8 e le regole decise lo dicono; un
   impegnativo resta solo con il motivo in una riga;
3. un primo pacchetto di interfaccia, come la prova sulla carta di Codex nella
   #522 (punto 7), che non diventa un unico pacchetto enorme;
4. le decisioni sui blocchi (attributi), sul modello della quota e sui retini
   (le richieste a solid-space, già aperte o da aprire);
5. il pacchetto di `app` con icone e voci dei comandi nuovi (LAVORO-IN-PARALLELO.md,
   «Comandi nell'interfaccia»);
6. il canale MCP sulla sessione attiva (ADR 0041, punto 2).

La pull request del piano la apre questa sessione; la unisce il responsabile.

## Regole della tappa che valgono qui

- Il piano della 0.8 non si apre (B6) prima della chiusura della 0.7.5.

## Test richiesti

Nessuno: è un piano.

## Fatto quando

- La pull request del piano della 0.8 è unita.
- La CI è verde.
