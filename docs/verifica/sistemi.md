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
la issue che lo segue. Un caso dell'elenco non si confronta, e la CI scrive un
avviso che lo nomina; un caso dell'elenco che nessun sistema ha più è un
errore, perché la riga va tolta. Una differenza tra sistemi si corregge; si
aggiunge qui solo con una decisione del responsabile.

Nessuna.
