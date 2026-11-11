# [0.8-PT3] Oracolo ezdxf: entità e tabelle della 0.8

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: infrastruttura dei riferimenti
- **Dipende da:** PT9 (il formato delle righe nuove)
- **Modulo:** `tests/verifica/oracolo`
- **Da leggere:** [ADR 0020](../../adr/0020-fuzzing-e-oracolo-ezdxf.md);
  `tests/verifica/oracolo/README.md`, `ezdxf_attesi.py`, `controlla_casi.py`;
  [verifica/rapporto.md](../../verifica/rapporto.md), sezione «Tappa 0.8» (da
  PT9); [0.8/dxf-di-prova.md](dxf-di-prova.md)

## Perché

I riferimenti delle quote, dei blocchi, dei retini e del testo multiriga
copiano i valori attesi dal log dell'oracolo sui DXF di AutoCAD (ADR 0020):
l'oracolo deve rispondere sulle entità e sulle tabelle nuove prima che i loro
pacchetti di riferimenti partano.

## File

- `tests/verifica/oracolo/ezdxf_attesi.py`, `controlla_casi.py`, `README.md`
- `tests/verifica/oracolo/test_*.py` se esistono, o un test nuovo accanto

## Cosa fare

1. `managed()` riconosce gestiti anche `SOLID`, `ELLIPSE`, `MTEXT`, `ATTDEF`, `INSERT`,
   `DIMENSION` e `HATCH`, con le eccezioni dell'ADR 0044, punto 4 (`MINSERT`,
   blocchi esterni o mancanti, `ATTRIB` o `ATTDEF` multiriga, `MTEXT` con
   colonne, `HATCH` con ellissi, spline o sfumature, quote che non sono dei
   tipi della 0.8 restano `DIMENSION` con `kind=other`).
2. `? entity` risponde per i tipi nuovi con le righe di rapporto.md, «Tappa 0.8»;
   gli `ATTRIB` hanno la loro risposta con `owner=`.
3. Domande nuove dell'oracolo, con lo stesso stile delle altre:
   - `? blocks D`: le definizioni dei blocchi nella forma di `blocks:` del
     rapporto (escluse quelle dei layout e le `*D…`);
   - `? styles D`, `? dimstyles D`: le tabelle `STYLE` e `DIMSTYLE` nella forma
     di `text styles:` e `dimension styles:` (tutti i record, compresi
     `Standard` e `ISO-25`);
   - `? generated D <handle>` per `INSERT` e `DIMENSION`: le entità che ezdxf dà
     con `virtual_entities()`, nel formato delle forme di `generated`
     (domande.md), con le proprietà effettive. Per un `INSERT` con scala non
     uniforme ezdxf dà ellissi, come Tecnigrafo (ADR 0052): si confrontano.
4. Il README descrive le risposte nuove.

## Regole della tappa che valgono qui

- L'oracolo non esegue mai Tecnigrafo e non si collega al programma (ADR 0020).
- Le dipendenze in `requirements.txt` non cambiano; se una funzione di ezdxf
  richiede una versione diversa, fermati e scrivilo nella issue.

## Test richiesti

- Su un DXF scritto dal test con ezdxf (un blocco con un `ATTDEF`, un `INSERT`
  con `ATTRIB`, un `SOLID`, un `MTEXT`, una quota lineare, un `HATCH`), le
  risposte nuove hanno la forma attesa.

## Fatto quando

- L'oracolo risponde alle domande nuove sui DXF dei sorgenti pubblici, nel job
  `Verifica`.
- La CI è verde.
