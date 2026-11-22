# [0.8.1-PT1] Oracolo ezdxf: autoprove che raccolgono gli errori, `grid` e `attdisp`

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: è l'infrastruttura dei riferimenti; la prova è
  il `--self-test` della CI
- **Dipende da:** —
- **Modulo:** `tests/verifica/oracolo/`
- **Da leggere:** [ADR 0020](../../adr/0020-fuzzing-e-oracolo-ezdxf.md),
  punto 2; [agenti/comandi.md](../../agenti/comandi.md), «Autoprove degli
  script»; [verifica/domande.md](../../verifica/domande.md), «File» (le
  domande `grid` e `attdisp` annunciate da [PT2](PT2-annunci.md));
  [ADR 0055](../../adr/0055-griglia-snap-alla-griglia-icona-ucs.md), punto 1;
  [ADR 0056](../../adr/0056-comandi-degli-attributi.md), punto 1

## File

- `tests/verifica/oracolo/controlla_casi.py`
- `tests/verifica/oracolo/ezdxf_attesi.py`
- `tests/verifica/oracolo/README.md`

## Cosa fare

1. **Autoprove** ([DECISIONI.md](../../DECISIONI.md), 2026-11-19, *Autoprove*;
   2026-11-21, «0.8.1 e 0.8.2 divise fra kernel e no», *Onda 0*): in
   `self_test()` `require` non solleva più un'eccezione al primo controllo
   fallito; raccoglie nome del controllo, valore atteso e ottenuto, e alla fine
   li stampa tutti ed esce con 1. Lo stesso per ogni altro controllo dello
   script che oggi si ferma al primo errore. Nessun controllo cambia.
2. **`grid D`** (domanda del file, catalogo di PT2): l'oracolo risponde con la
   voce `*Active` della `VPORT` letta da ezdxf
   (`doc.viewports.get("*Active")`, la prima), nel formato della domanda:
   `grid=on|off snap=on|off grid-unit=x,y snap-unit=x,y snap-base=x,y
   display=<n> major=<n> ucsicon=<n> snap-angle=<gradi> snap-style=<n>`; senza
   la voce, i valori di un disegno nuovo (ADR 0055, punto 1).
3. **`attdisp D`**: `normal`, `on` o `off` da `$ATTMODE` (1, 2, 0; 1 se manca).
4. Il self-test prova le due domande su due DXF scritti con ezdxf nel test (una
   `VPORT` con valori diversi da quelli di un disegno nuovo, un `$ATTMODE` 0) e
   su un DXF senza `VPORT`.

## Regole della tappa che valgono qui

- Nessuna: l'elenco dei tipi della 0.8 attivi (`ACTIVE_STAGE_08_ENTITY_TYPES`)
  non cambia.

## Test richiesti

- `--self-test`: un controllo volutamente sbagliato in un test temporaneo,
  tolto prima della pull request, mostra che le righe fallite escono tutte
  (scrivilo nella pull request); `grid` e `attdisp` sui tre DXF del punto 4.

## Fatto quando

- Il passo `Oracolo ezdxf` della CI è verde e il self-test elenca tutti i
  controlli falliti in un giro solo.
