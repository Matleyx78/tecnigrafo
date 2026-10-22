# ADR 0019: Archivi ZIP per il formato `.cadx`

- **Stato:** Proposto
- **Data:** 2026-10-22

## Contesto

Un file `.cadx` è un archivio ZIP (ADR 0006, punto 3). Tecnigrafo deve
scriverlo e leggerlo, anche quando lo ha prodotto un altro programma o una
persona con un comune strumento di compressione: quindi deve leggere almeno i
metodi *stored* (nessuna compressione) e *deflate*.

Un archivio ZIP ha due parti molto diverse:

- il **contenitore**: intestazioni locali, elenco centrale, record di chiusura,
  controllo CRC-32. È semplice, ben documentato (specifica APPNOTE di PKWARE)
  e sono poche centinaia di righe;
- la **compressione deflate** (RFC 1951). La decompressione è il codice più
  esposto ai file malevoli: un errore lì diventa un crash o una falla.

Vincoli: nessuna libreria esterna senza un ADR; licenze compatibili con la
GPLv3 (ADR 0003); `io` non usa Qt (ADR 0017); il lettore ha un bersaglio di
fuzzing (ADR 0020).

## Decisione

### 1. zlib per la compressione

- Si usa **zlib** per deflate e inflate, e per il CRC-32. È la libreria di
  compressione più diffusa e più controllata: la usano i sistemi operativi, i
  browser e Qt stesso.
- Licenza zlib, permissiva e compatibile con la GPLv3.
- La si **copia nel repository**, in `third_party/zlib/`, solo con i file che
  servono, con un `README.md` che riporta fonte, versione e SHA-256
  dell'archivio originale, e il testo della licenza. Non si usa la zlib del
  sistema: Windows non ne ha una, e la versione deve essere la stessa ovunque.
- Si compila come libreria statica interna, collegata **solo** da `io`. È
  codice di altri: non riceve i nostri avvisi né `clang-tidy` (sarebbe
  rumore che non possiamo correggere), ma entra nel job dei sanitizer e nel
  fuzzing come il resto.
- Aggiornare zlib è un pacchetto a sé, con il nuovo hash nel `README.md`.

### 2. Il contenitore lo scriviamo noi

In `io`, dietro un'interfaccia piccola (leggere le parti di un archivio,
scrivere un archivio da un elenco di parti).

**Lettura.**

- Si parte dal record di chiusura e dall'elenco centrale; le dimensioni si
  prendono da lì, quindi si leggono anche gli archivi con descrittori dei dati
  (bit 3).
- Metodi ammessi: *stored* (0) e *deflate* (8). Altri metodi, archivi cifrati,
  ZIP64 e archivi su più dischi danno un errore che dice cosa non è
  supportato.
- Si controlla il CRC-32 di ogni parte.

**Limiti, controllati prima e durante la decompressione.**

- al massimo 256 parti;
- al massimo 1 GiB decompresso in tutto, e un rapporto di compressione di al
  massimo 1000 a 1 per parte;
- la decompressione si ferma appena supera la dimensione dichiarata: un
  archivio che mente sulle dimensioni è un errore, non una sorpresa;
- nomi in UTF-8, relativi, senza `..`, senza `\` e senza duplicati.

**Scrittura.**

- `mimetype` per primo, non compresso, senza campi extra (ADR 0006); le altre
  parti con deflate.
- Data e ora fisse (1980-01-01 00:00) in ogni intestazione: lo stesso disegno
  dà sempre lo stesso file, e i test lo possono confrontare byte per byte.

### 3. Prove

- Test con archivi creati da strumenti diversi (lo strumento `zip`, il modulo
  `zipfile` di Python, quello di Windows): sono file prodotti da programmi di
  riferimento, ammessi dall'ADR 0011.
- Bersaglio di fuzzing sul lettore dei `.cadx`, nello stesso job del lettore
  DXF (ADR 0020).

## Alternative considerate

- **miniz** (MIT, un solo file): comodo, ma è un progetto piccolo e sta
  proprio nel punto più esposto ai file malevoli.
- **Deflate scritto da noi:** fattibile, ma la decompressione è il codice più
  rischioso del formato e si aggiungerebbero due pacchetti impegnativi senza
  un vantaggio per chi usa il programma.
- **Solo *stored*, senza compressione:** non leggerebbe i `.cadx` compressi da
  altri strumenti, e un disegno in DXF si comprime di molte volte.
- **minizip o libzip:** contenitore e compressione insieme, ma più codice
  esterno del necessario, e su Windows una dipendenza da installare.
- **`QZipReader` di Qt:** API privata, non garantita, e fuori dagli strati:
  `io` non può usare Qt.

## Conseguenze

- `third_party/zlib/` è la prima libreria esterna oltre a Qt. La regola
  "nessuna nuova dipendenza" di AGENTS.md resta valida per tutto il resto;
  questo ADR fa un'eccezione solo per `io` e solo per zlib.
- Serve un pacchetto per il contenitore e l'integrazione di zlib (tappa 0.5,
  P13), prima di quello del formato `.cadx`.
- Gli aggiornamenti di sicurezza di zlib si seguono e si applicano con un
  pacchetto dedicato.
