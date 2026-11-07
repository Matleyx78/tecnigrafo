# Pacchetto di riferimenti

Da leggere quando la issue ha titolo `[X.Y-RTN]` o porta l'etichetta
`riferimenti`, o quando tocchi `tests/verifica/casi/` o
`tests/verifica/sorgenti.json` ([AGENTS.md](../../AGENTS.md), "Se ... leggi
anche ..."). Vale insieme al nucleo. Perché i riferimenti si scrivono prima e da
un altro agente: [pacchetto.md](pacchetto.md), "Modalità di controllo e
riferimenti".

I nomi qui sotto usano i prefissi dalla 0.8: `PT`/`RT`/`CT` in Tecnigrafo,
`PS`/`RS`/`CS` in solid-space. Per le tappe fino alla 0.7.5 restano `P`/`R`/`C`,
anche nei file in attesa (`0.7.5-R1.txt`); nulla si rinomina. I rami seguono:
`pt0.8-05-…` per il lavoro, `rt0.8-05-…` per i riferimenti.

## Il prossimo pacchetto di riferimenti

Per "il prossimo pacchetto di riferimenti" valgono le condizioni 1–3 e 5 di
[pacchetto.md](pacchetto.md), "Se ti chiedono di prendere il prossimo
pacchetto", sulle issue con l'etichetta `riferimenti`. Se nessuna issue va bene,
non iniziare: scrivi all'utente quali issue sono tue e cosa le blocca. I
pacchetti di riferimenti pronti presi di fila sono in [lotti.md](lotti.md).

## In un pacchetto di riferimenti (`[X.Y-RTN]`)

- Tocca solo `tests/verifica/casi/`, `tests/verifica/sorgenti.json` e
  `tests/verifica/in-attesa/`: la CI rifiuta una pull request che mescola
  riferimenti e altri file.
- Non lavorare a un pacchetto di riferimenti se il pacchetto di lavoro collegato
  è assegnato a te.
- Ricava i valori attesi da calcoli, da file pubblici o da programmi di
  riferimento. **Mai eseguendo Tecnigrafo**: registreresti il suo comportamento,
  errori compresi.
- I valori geometrici che stanno nell'archivio dei casi di solid-space
  (`tests/corpus/`) li prendi da lì invece di rifare il calcolo, e nella pull
  request citi il caso (percorso in solid-space e commit)
  ([ADR 0030](../adr/0030-solid-space-kernel-di-tecnigrafo.md), punto 14). Se
  un valore che ricavi tu non concorda con l'archivio, non sceglierne uno:
  scrivilo nella issue con i due valori e fermati. Decide l'archivio, e la issue
  in solid-space la apre il responsabile.
- Non usare `tecnigrafo-mcp` né `tecnigrafo-script` per ricavare un valore atteso
  ([ADR 0027](../adr/0027-server-mcp-di-sviluppo.md), punto 7).
- Ogni condizione "Fatto quando" del pacchetto di lavoro ha almeno un caso che la
  verifica. Aggiungi i casi limite: valori nulli, punti coincidenti o quasi
  allineati, Esc a metà comando, annulla e ripeti.
- Per ogni file preso dal web scrivi in `sorgenti.json` indirizzo, versione, hash
  SHA-256 e licenza, e nella pull request il link alla pagina della licenza. Solo
  licenze compatibili con la GPLv3 che permettano la ridistribuzione; nel dubbio,
  non usare il file e scrivilo nella pull request.
- Elenca i nuovi casi nel tuo file `tests/verifica/in-attesa/<X.Y>-RT<N>.txt`
  (`X.Y` oppure `X.Y.Z`, con numeri senza zeri iniziali; il tag della tua issue
  di riferimenti, con la lettera se il blocco è diviso:
  `<X.Y>-RT<N>a.txt`), una riga per caso, con il numero della issue del lavoro. Il
  file è tuo: nessun altro pacchetto lo tocca.
- La pull request è finita con la CI verde, **controlli anticipati compresi**
  ([ADR 0037](../adr/0037-controllo-anticipato-dei-riferimenti.md)): la forma
  dei file, l'eco di ogni riga dello script in `atteso.txt` e, per uno script
  in attesa, il confronto della parte prima del **confine**, il primo punto
  che il programma non sa ancora fare
  ([docs/verifica/casi.md](../verifica/casi.md), "Controlli anticipati").
- La pull request tocca solo casi di riferimento: non si compila e non si
  eseguono i casi in locale (`scripts/controlla.sh --solo formato` resta
  permesso). Se i casi che scrivi sono validi lo dicono i controlli anticipati
  della CI ([comandi.md](comandi.md), "Pull request senza
  compilazione locale").
- Se il lavoro cambia **apposta** un comportamento che esiste già (un'opzione
  nuova cambia la domanda di un comando, un nuovo snap acceso per difetto),
  scrivi nello script, **prima** della riga da cui il comportamento cambia, il
  commento `; cambia con #<numero della issue del lavoro>` (ADR 0037, punto 5).
  Non usarlo per far passare un controllo: il responsabile guarda ogni
  `; cambia con` nuovo in revisione.
- Se il controllo segnala una differenza **prima del confine**, rifai il
  valore dalle tue fonti (calcoli, file pubblici, AutoCAD). Se le fonti
  confermano il tuo valore, la differenza può essere un errore del programma:
  non adattare il riferimento, fermati e scrivilo nella issue, con il caso, il
  valore atteso e quello che dà il programma; decide il responsabile (ADR 0037,
  punto 7).
- Non aggiungere `esente` in `tests/verifica/in-attesa/`; toglilo dalla riga di
  un caso che correggi.

## I buchi fermano anche i riferimenti

Prima di scrivere casi valgono anche per te i buchi della scheda
([pacchetto.md](pacchetto.md), "Un buco nella scheda") e del contratto
([contratto.md](contratto.md), "Un buco nel contratto").
