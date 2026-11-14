# Il confronto tra sistemi

Lo stesso calcolo deve dare gli stessi numeri su Linux, Windows e macOS
([ADR 0038](../adr/0038-ci-dei-due-progetti.md), punti 4 e 5). Il lavoro
«Cross-platform comparison» della CI lo controlla sui casi di verifica: blocca
la pull request se un caso dà un rapporto diverso su un sistema. Lo ha
introdotto il pacchetto 0.7.5-P5.

## Che cosa si confronta

- I lavori Release (`ci-release`) di Linux, Windows e macOS eseguono tutti i
  [casi di verifica](casi.md) e caricano la loro cartella di uscita come
  artefatto: `verifica-linux-release`, `verifica-windows`, `verifica-macos`.
- Dopo i tre, `scripts/confronta_sistemi.py` confronta, per ogni caso di
  livello 1 (`tests/verifica/casi/<area>/<nome>/` nell'artefatto), i
  **rapporti** che `tecnigrafo-script check` scrive: `ottenuto.txt` per uno
  script, `ottenuto.tgq` per le domande, `ottenuto-confine.txt` per uno script
  in attesa fino al suo confine.
- Ogni caso deve esistere su tutti i sistemi, con gli stessi rapporti, uguali
  **riga per riga e byte per byte**: un numero con un'ultima cifra diversa è
  una differenza, anche se entra nella tolleranza con cui `check` confronta il
  rapporto con `atteso.txt`, e lo è anche un `\r` in fondo a una riga.
- I casi in attesa si confrontano come gli altri: il loro rapporto non è
  ancora quello atteso, ma deve essere lo stesso sui tre sistemi.

Non si confrontano qui:

- le **immagini** del livello 2 (`casi2/`), che si confrontano già con la loro
  tolleranza, e su Windows e macOS il livello 2 esegue un solo caso;
- i **file scritti dagli script** (la cartella `file/` di ogni caso: DXF,
  `.cadx`), che non sono rapporti;
- i log di `ctest`.

### Righe che variano per costruzione

Lo script elenca in testa (`VARIABLE_LINES`) le righe che cambiano da un
sistema all'altro per costruzione e non sono risultati, come un percorso o un
tempo: si saltano quando sono di quel tipo su tutti i sistemi. Oggi
l'elenco è **vuoto**: il [rapporto](rapporto.md) non ha percorsi né tempi, e i
nomi dei file sono quelli che lo script ha scritto, relativi alla cartella del
caso. Una riga si aggiunge all'elenco solo se non è un risultato, e qui si
scrive perché.

## Eccezioni

Un caso per riga, nella forma ``- `<area>/<nome>`: <motivo>``, con il motivo e
la issue che toglierà la riga. Un caso dell'elenco non si confronta, e la CI
scrive un avviso che lo nomina. Un'eccezione è provvisoria: la CI dà errore se
il caso dell'elenco è ormai uguale su tutti i sistemi, o se nessun sistema lo
ha più, perché la riga va tolta. Una differenza tra sistemi si corregge; si
aggiunge qui solo con una decisione del responsabile.

Il primo confronto (0.7.5-P5) ha trovato 17 casi con le ultime cifre di alcuni
numeri diverse tra i sistemi: archi, testi ruotati, specchiature e grip, con
ogni probabilità per le funzioni trigonometriche della libreria di ciascun
sistema. PT1 ha sostituito seno, coseno e arcotangente con le funzioni
deterministiche di solid-space. Le eccezioni residue aspettano la seconda
parte, PT41 ([#620](https://github.com/Matleyx78/tecnigrafo/issues/620);
[DECISIONI.md](../DECISIONI.md), 2026-11-08). Le righe dei tre sistemi sono nella
[issue di P5](https://github.com/Matleyx78/tecnigrafo/issues/546#issuecomment-6725181081).

- `grip/modifica`: ultime cifre di alcuni numeri diverse su Windows e macOS (7 righe); si toglie con [#620](https://github.com/Matleyx78/tecnigrafo/issues/620)
- `pline/archi-e-linee`: ultime cifre di alcuni numeri diverse su macOS (1 riga); si toglie con [#620](https://github.com/Matleyx78/tecnigrafo/issues/620)
- `pline/arco-iniziale`: ultime cifre di alcuni numeri diverse su Linux (1 riga); si toglie con [#620](https://github.com/Matleyx78/tecnigrafo/issues/620)
- `pline/chiusura-arco-e-linea`: ultime cifre di alcuni numeri diverse su macOS (1 riga); si toglie con [#620](https://github.com/Matleyx78/tecnigrafo/issues/620)
- `testo/aligned-fit`: ultime cifre di alcuni numeri diverse su Linux e macOS (5 righe); si toglie con [#620](https://github.com/Matleyx78/tecnigrafo/issues/620)
- `testo/selezione-parallelogramma`: ultime cifre di alcuni numeri diverse su macOS (4 righe); si toglie con [#620](https://github.com/Matleyx78/tecnigrafo/issues/620)
- `trim/mezzo-polilinee-larghezze`: ultime cifre di alcuni numeri diverse su macOS (2 righe); si toglie con [#620](https://github.com/Matleyx78/tecnigrafo/issues/620)
