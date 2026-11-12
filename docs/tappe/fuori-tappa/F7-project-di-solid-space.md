# [F7] `scripts`: `dipendenze.py` allinea nel Project anche i pacchetti di solid-space

- **Tappa:** fuori tappa, strumento del responsabile come
  [F4](F4-dipendenze.md) (decisione del 2026-11-12,
  [DECISIONI.md](../../DECISIONI.md))
- **Issue:** #752
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: lo script non cambia il comportamento di
  Tecnigrafo. Lo provano i test di unità con testi e risposte dell'API
  costruiti apposta, come in F4
- **Dipende da:** —
- **Modulo:** `scripts` (più la guida del responsabile)
- **Da leggere:** questa scheda; [F4](F4-dipendenze.md), sezioni «Che cosa
  legge» e «Che cosa controlla»; `scripts/dipendenze.py` (il commento in
  testa, `parse_ss_roadmap`, `parse_ss_plan`, il controllo 4 e il caricamento
  del Project); [GUIDA-RESPONSABILE.md](../../GUIDA-RESPONSABILE.md), A4; come
  esempio di tabella vera, il
  [piano 0.3 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/tappe/0.3-regioni-ed-ellisse.md),
  sezione 6

## Perché

Il Project «Tecnigrafo» tiene le issue dei due repository, ma
`scripts/dipendenze.py <tappa>` sposta tra *Bloccato* e *Da fare* solo le
issue del piano di Tecnigrafo. Quelle della tappa gemella di solid-space il
responsabile le sposta a mano, ogni volta che un merge in solid-space sblocca
un pacchetto. Lo script legge già il piano di solid-space (per i controlli 1 e
5) e il Project: gli manca solo di applicare la stessa regola anche alle righe
di quel piano.

## File

- `scripts/dipendenze.py`
- `scripts/dipendenze_test.py`
- `docs/GUIDA-RESPONSABILE.md`: A4 (e le righe di B11 che oggi dicono di
  spostare a mano le schede di solid-space, se ce ne sono)

## Cosa fare

### Quando

Solo nella forma `scripts/dipendenze.py <tappa>`, quando la tappa di
solid-space della riga «Per la tappa di Tecnigrafo» `<tappa>` è **aperta** e il
suo piano si legge (oggi: `ctx.ss_plan` non vuoto). Se la tappa di solid-space
non c'è o non è aperta, non cambia niente rispetto a oggi. `--sottomodulo` e
`--controlla-puntatore` non cambiano.

### Pronta o bloccata, in solid-space

Per ogni riga della tabella dei pacchetti (sezione 6) del piano di solid-space
con la issue **aperta**, nell'ordine del piano: è pronta se tutte le righe
della sua colonna "Dipende da" (gli id `PSN`, `RSN`, `CS` e le forme vecchie,
come già le legge `parse_ss_plan`; le note tra parentesi e il testo libero non
contano) hanno la issue chiusa. Una dipendenza senza issue la blocca, come in
Tecnigrafo. Le righe senza issue si saltano in silenzio: il piano di
solid-space non è compito di questo script.

Le righe di solid-space vanno nelle stesse sezioni *Ready* e *Blocked* di
quelle di Tecnigrafo, dopo di esse, con il riferimento completo e il tag del
piano, per esempio
`Matleyx78/solid-space#236 [0.3-PS1] media agente:codex`, e «(in charge)» se
porta `AA-Presa in carico`. Chi blocca si scrive come oggi
(`Matleyx78/solid-space#222`).

### Project

Per le stesse righe vale il controllo 4, con la stessa regola: *Bloccato* e
pronta, oppure *Da fare* o vuoto e bloccata, va corretto; gli altri stati non
si toccano. Una issue di solid-space che non è nel Project va in *To fix* con
«not in the Project: add it by hand» (`apri-tappa.sh --solo-project` lavora
solo sulle issue di Tecnigrafo). `--senza-project` salta anche questo.

Con `--applica` le modifiche di Status delle righe di solid-space stanno
nello stesso elenco e sotto la stessa domanda `Go on? [s/N]`. Il Project è del
responsabile, non di solid-space: cambiare il campo *Status* di un suo
elemento non tocca la issue. Lo script continua a **non scrivere niente in
solid-space**: né testo, né etichette, né relazioni «bloccata da», né stato
delle issue. I controlli 2 e 3 (testo e relazioni) restano solo per le issue di
Tecnigrafo.

### Codici di uscita e commento in testa

Uno Status da correggere di una riga di solid-space conta come gli altri del
controllo 4 (uscita 1). Il commento in testa al file dice che il controllo 4
vale anche per le righe del piano di solid-space quando la sua tappa è aperta,
e che «never writes to solid-space» resta vero perché il Project non è di
solid-space.

### Guida del responsabile

In A4: le sezioni *Ready* e *Blocked* comprendono i pacchetti di solid-space
della tappa gemella, e `--applica` sposta nel Project anche le loro schede.
Nella tabella «Quando si usa», la riga «Ogni volta che unisci un pacchetto di
solid-space» dice che diventano pronte issue dei due repository.

## Regole che valgono qui

- Codice, commenti e messaggi dello script in inglese; la guida in italiano.
- Solo libreria standard di Python e `gh`: nessuna dipendenza nuova.
- In solid-space lo script legge soltanto (AGENTS.md, "Dove puoi scrivere").
- Nessuna modifica a `.github/`: i test dello script girano già nella CI (F4).

## Test richiesti

In `scripts/dipendenze_test.py`, senza rete, come quelli di F4:

- Un piano di solid-space con righe pronte, bloccate da una riga aperta,
  bloccate da una dipendenza senza issue, con la issue chiusa (saltata) e
  senza issue (saltata); note tra parentesi e testo libero in "Dipende da"
  ignorati.
- Le righe di solid-space in *Ready* e *Blocked*, dopo quelle di Tecnigrafo,
  con riferimento completo e tag.
- Project: *Bloccato* e pronta → *Da fare*; *Da fare* e vuoto e bloccata →
  *Bloccato*; un altro stato lasciato com'è; issue non nel
  Project segnalata.
- Con `--applica` le chiamate di Status degli elementi di solid-space verso
  `users/<owner>/projectsV2/...`, e **nessuna** chiamata di scrittura verso
  `repos/Matleyx78/solid-space/...`.
- Tappa di solid-space chiusa o assente, e `--senza-project`: nessuna riga di
  solid-space nel Project, uscita come prima.
- I test di F4 continuano a passare.

## Fatto quando

- Con i dati veri, `scripts/dipendenze.py 0.8 --senza-project` gira e mostra
  in *Ready* e *Blocked* i pacchetti aperti della tappa 0.3 di solid-space;
  l'uscita è incollata nella pull request (senza il Project, perché la sessione
  dell'agente di solito non ha lo scope `project` di `gh`).
- Tutti i test richiesti esistono e passano, anche nella CI.
- La guida del responsabile, A4, lo dice.
- La CI è verde.
