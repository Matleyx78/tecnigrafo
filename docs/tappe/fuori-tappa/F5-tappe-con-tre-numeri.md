# [F5] `scripts`: tappe con tre numeri (`X.Y.Z`)

- **Tappa:** fuori tappa, strumento del responsabile come
  `scripts/apri-tappa.sh` ([F4](F4-dipendenze.md)); serve prima di aprire la
  [tappa 0.7.5](../0.7.5-correzione-di-rotta.md), che è la prima con tre numeri
- **Issue:** #…
- **Difficoltà:** facile
- **Agente:** codex
- **Modello e impegno:** leggero; impegno basso
- **Riferimenti:** non servono: gli strumenti non cambiano il comportamento di
  Tecnigrafo. Lo provano i test di unità degli script e la configurazione della
  verifica
- **Dipende da:** —
- **Modulo:** `scripts` (più l'elenco dei casi in attesa di `tests/verifica/`)
- **Da leggere:** [ROADMAP.md](../../ROADMAP.md), "Aprire una tappa";
  [tappe/0000-modello.md](../0000-modello.md), sezione 6 (la tabella che lo
  script legge); `scripts/apri-tappa.sh`, `scripts/dipendenze.py`,
  `tests/verifica/pending_list.cmake`

## Perché

La tappa 0.7.5 ha tre numeri, come una versione di correzione. Gli strumenti
accettano solo `X.Y`, quindi oggi non si può aprire:

- `scripts/apri-tappa.sh` rifiuta l'argomento (`^[0-9]+\.[0-9]+$`, riga 254) e
  il titolo del documento (`# Tappa [0-9]+\.[0-9]+`, riga 282);
- `scripts/dipendenze.py` rifiuta `--tappa 0.7.5` (riga 950) e, cercando la
  tappa di solid-space che corrisponde a `0.7`, prenderebbe anche una riga che
  dice `0.7.5` (`(?![0-9])` alla riga 355 lascia passare il punto);
- `tests/verifica/pending_list.cmake` rifiuta un file `0.7.5-R1.txt` (riga 58).

## File

- `scripts/apri-tappa.sh`
- `scripts/dipendenze.py`, `scripts/dipendenze_test.py`
- `tests/verifica/pending_list.cmake` (la regola del nome e il commento in testa)
- `docs/agenti/riferimenti.md`, `docs/GUIDA-RESPONSABILE.md` (B6), solo se
  scrivono che una tappa è `X.Y`

## Cosa fare

1. Cerca in `scripts/`, `.github/`, `tests/verifica/` e `cmake/` ogni punto che
   legge o scrive il numero di una tappa (`[0-9]+\.[0-9]+`, `X.Y`, `v$tappa`,
   `tappa-$tappa`, `tg-$tappa`) ed elencali nella pull request. Quelli fuori
   dai file qui sopra li dici nella pull request, senza cambiarli.
2. Una tappa è `X.Y` oppure `X.Y.Z`, con numeri senza zeri iniziali: la stessa
   espressione nei tre file. Etichetta, milestone, valore *Tappa* del Project,
   titoli delle issue e rami seguono senza altro cambio: `tappa-0.7.5`,
   `v0.7.5`, `tg-0.7.5`, `[0.7.5-P1]`, `p0.7.5-01-…`.
3. Nei confronti tra tappe un prefisso non basta: `0.7` non è `0.7.5`, e
   viceversa.
4. Nessun altro cambiamento di comportamento.

## Regole che valgono qui

- I prefissi `PT`/`PS` non si fanno qui: sono di 0.7.5-P2, che parte dopo.

## Test richiesti

- `dipendenze_test.py`: `--tappa 0.7.5` accettato; `0.7.05`, `0.7.` e `0.7.5.1`
  rifiutati; una riga di solid-space «per Tecnigrafo 0.7.5» non corrisponde a
  `0.7`, e viceversa.
- Una prova di `apri-tappa.sh 0.7.5 --prova` sul piano della 0.7.5, scritta
  nella pull request con il suo risultato (lo script mostra e non scrive).
- La configurazione della verifica accetta `0.7.5-R1.txt` e `0.7.5-R1a.txt`
  (la prova si fa in CI: il nome si controlla quando si configura).

## Fatto quando

- `scripts/apri-tappa.sh 0.7.5 --prova` legge il piano della 0.7.5 senza errori.
- I test qui sopra esistono e passano.
- La CI è verde.
