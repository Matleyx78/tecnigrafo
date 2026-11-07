# [0.7.5-P8] Analisi: più pacchetti medi al posto di pochi impegnativi (#442)

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: è un'analisi, un documento
- **Dipende da:** —
- **Modulo:** `docs`
- **Da leggere:** la [#442](https://github.com/Matleyx78/tecnigrafo/issues/442),
  con il commento del 2026-10-31 sui riferimenti; la
  [#522](https://github.com/Matleyx78/tecnigrafo/issues/522), sezione 7.3 e il
  parere di Codex, punto 7 (la prova sulla carta della 0.8);
  [LAVORO-IN-PARALLELO.md](../../LAVORO-IN-PARALLELO.md), sezione 1 («Pacchetti
  leggeri», «Modello e impegno»); i piani delle tappe
  [0.4](../0.4-entita-base.md)–[0.7](../0.7-modifiche.md) (sezione 6) e la
  [scheda della 0.8](../0.8-quote-retini-blocchi.md)

## Perché

La #442 chiede un'analisi vera, non un'adesione: se privilegiare i pacchetti
medi regge, dove no e con quali condizioni, prima di scrivere il piano della
0.8, che si prepara dopo la chiusura della 0.7.5. La 0.7 è chiusa, quindi i suoi dati sono completi.

## File

- `docs/tappe/0.7.5/analisi-pacchetti-medi.md` (nuovo)

Nessun altro documento cambia: le regole si cambiano solo dopo che il
responsabile ha letto l'analisi.

## Cosa fare

1. **Dati**, con `gh api` (solo REST), per le pull request di lavoro e di
   riferimenti delle tappe 0.4–0.7, distinte per difficoltà della issue che
   chiudono: numero di commit, numero di run di `ci.yml`, giri di correzione
   (run fallite prima della verde), giorni tra apertura e unione, file e righe
   cambiate. Una tabella per tappa e una d'insieme; i dati grezzi in fondo, con
   il comando che li ha prodotti.
2. **Le otto domande della #442**, una per una, con i dati dove ci sono: costi
   fissi; contrasto con «Non troppo piccoli» e con la misura massima (dove sta la
   soglia sotto cui un medio è troppo piccolo); divisibilità reale dei sei
   impegnativi della 0.7 (P2, P10, P14, P16, P17, P19), con il confine e che
   cosa ne sarebbe uscito; dipendenze e parallelismo, stimati sulla tabella della
   0.7 con più sessioni di Claude; riferimenti (quanti casi delicati e ordinari
   avrebbero avuto R2, R10, R14, R16, R17, R19 divisi, e quanti pacchetti di
   riferimenti in fascia forte, verificando l'ipotesi del responsabile del
   2026-10-31); assegnazione; solid-space; qualità del risultato (scheda madre,
   ADR, pacchetto iniziale di interfaccia).
3. **Prova sulla carta sulla 0.8**, partendo dalla divisione proposta da Codex
   nella #522 (punto 7): i gruppi diventano pacchetti con modulo, file stimati,
   difficoltà e dipendenze; dove un pacchetto resta impegnativo, perché.
4. **Raccomandazione:** sì, sì con condizioni, o no; e, se non è no, il testo
   proposto per le regole da cambiare (`LAVORO-IN-PARALLELO.md`, sezione 1, e
   ciò che ne segue), scritto come proposta nel documento, non applicato.
5. Dove un dato manca o non si può ricavare, lo dici; non lo stimi senza dirlo.

## Regole della tappa che valgono qui

- Il documento distingue ciò che è misurato da ciò che è stimato.
- Scrive l'autore e il modello in testa (LAVORO-IN-PARALLELO.md, «Chi fa cosa»).

## Test richiesti

Nessuno: è un documento.

## Fatto quando

- Il documento risponde alle otto domande con i dati, contiene la prova sulla
  carta della 0.8 e una raccomandazione.
- La CI è verde.
