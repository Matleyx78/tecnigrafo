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

1. L’oracolo ha un elenco esplicito dei tipi della 0.8 già attivi,
   inizialmente vuoto. Solo `? count` e `? unhandled` lo consultano: un tipo
   nuovo che non è nell’elenco conta come non gestito, come il segnaposto di
   Tecnigrafo. Eccezione per gli `ATTRIB` collegati a un `INSERT`: finché
   `ATTRIB` non è in `ACTIVE_STAGE_08_ENTITY_TYPES`, non compaiono né in
   `? count` né in `? unhandled`, perché restano dentro l’`INSERT` grezzo;
   quando è attivo compaiono in `? count`, oppure in `? unhandled` se non
   gestiti. Le altre risposte, compresa `? entity` degli `ATTRIB`, restano
   invariate. Per decisione del responsabile, questa correzione della scheda
   e di `docs/DECISIONI.md` entra nella stessa PR #794, in eccezione alla
   guida B10a, e la rivede il revisore nel giro normale. I casi in
   `tests/verifica/casi/` restano come su `main`: la riga di `r11-public`
   la aggiorna PT18, come dice la sua scheda. L’elenco dichiara la tappa
   raggiunta e non si ricava dal programma: l’oracolo continua a non eseguire
   Tecnigrafo né a collegarsi al programma (ADR 0020); il README dell’oracolo
   lo dice in una frase.
2. Per tutte le altre risposte l’oracolo riconosce subito `SOLID`, `ELLIPSE`,
   `MTEXT`, `ATTDEF`, `ATTRIB`, `INSERT`, `DIMENSION` e `HATCH`, con le
   eccezioni dell’ADR 0044, punto 4 (`MINSERT`, blocchi esterni o mancanti,
   `ATTRIB` o `ATTDEF` multiriga, `MTEXT` con colonne, `HATCH` con ellissi,
   spline o sfumature, quote che non sono dei tipi della 0.8 restano
   `DIMENSION` con `kind=other`).
3. `? entity` risponde per i tipi nuovi con le righe di rapporto.md, «Tappa 0.8»;
   gli `ATTRIB` hanno la loro risposta con `owner=`.
4. Domande nuove dell’oracolo, con lo stesso stile delle altre:
   - `? blocks D`: le definizioni dei blocchi nella forma di `blocks:` del
     rapporto (escluse quelle dei layout e le `*D…`);
   - `? styles D`, `? dimstyles D`: le tabelle `STYLE` e `DIMSTYLE` nella forma
     di `text styles:` e `dimension styles:` (tutti i record, compresi
     `Standard` e `ISO-25`). Per un gruppo essenziale assente in un record
     `DIMSTYLE`, valgono i predefiniti DXF di AutoCAD: `DIMASZ` 0,18,
     `DIMCEN` 0,09, `DIMDEC` 4, `DIMDSEP` `.`, `DIMEXE` 0,18, `DIMEXO`
     0,0625, `DIMGAP` 0,09, `DIMTAD` 0, `DIMTIH` 1, `DIMTOH` 1, `DIMTXT`
     0,18 e `DIMZIN` 0; per le altre variabili vale il valore di `ISO-25`;
   - `? generated D <handle>` per `INSERT` e `DIMENSION`: le entità che ezdxf dà
     con `virtual_entities()`, nel formato delle forme di `generated`
     (domande.md), con le proprietà effettive. Per un `INSERT` con scala non
     uniforme ezdxf dà ellissi, come Tecnigrafo (ADR 0052): si confrontano.
     L’oracolo scrive la risposta solo se tutte le entità sono forme (`LINE`,
     `ARC`, `CIRCLE`, `ELLIPSE`, `LWPOLYLINE`, `TEXT`, `SOLID`); altrimenti
     scrive `; generated <handle>: outside oracle scope (<tipi>)`, con i tipi
     non forma, e non scrive una risposta copiabile. Le proprietà *DaBlocco*
     prendono le proprietà effettive dell'inserimento o della quota, risolte
     sul suo layer; un proprietario su un layer spento o congelato non genera
     forme. Dopo le forme di un `INSERT` vengono gli `ATTRIB` visibili come
     `TEXT`, in ordine; quelli invisibili non vengono generati.
5. Il README descrive le risposte nuove.

## Regole della tappa che valgono qui

- L'oracolo non esegue mai Tecnigrafo e non si collega al programma (ADR 0020).
- Le dipendenze in `requirements.txt` non cambiano; se una funzione di ezdxf
  richiede una versione diversa, fermati e scrivilo nella issue.

## Test richiesti

- Su un DXF scritto dal test con ezdxf (un blocco con un `ATTDEF`, un `INSERT`
  con `ATTRIB`, un `SOLID`, un `MTEXT`, una quota lineare, un `HATCH`), le
  risposte nuove hanno la forma attesa.
- Sullo stesso DXF, un’entità generata che non è una forma non produce una
  risposta `? generated`: il log contiene invece per intero la riga
  `; generated <handle>: outside oracle scope (<tipi>)`.
- Sullo stesso DXF, con l'elenco dei tipi attivi vuoto il `SOLID` compare in
  `? unhandled` (`SOLID 1`) e non in `? count`; con `SOLID` nell'elenco compare
  in `? count` (`SOLID 1`) e non più in `? unhandled`; gli altri tipi nuovi
  restano in `? unhandled`, salvo gli `ATTRIB` collegati; tutte le altre
  risposte, compresa `? entity` del `SOLID`, sono identiche nei due casi.
- Sullo stesso DXF, con l’elenco vuoto `ATTRIB` non compare né in `? count`
  né in `? unhandled`; con `{"ATTRIB", "INSERT"}` compare `ATTRIB 1` in
  `? count` e non in `? unhandled`. Se l’attributo non è gestito (estrusione `1,0,0`),
  con i due tipi attivi compare `ATTRIB 1` solo in `? unhandled`.
  La risposta `? entity` dell’`ATTRIB` resta identica all’attivazione.
  Il self-test e questa eccezione sono nella PR #794, senza una PR di
  correzione separata (eccezione alla guida B10a autorizzata dal responsabile).

## Fatto quando

- L'oracolo risponde alle domande nuove sui DXF dei sorgenti pubblici, nel job
  `Verifica`.
- La CI è verde.
