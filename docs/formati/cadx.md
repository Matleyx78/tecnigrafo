<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright (C) 2026 The Tecnigrafo authors -->

# Specifica del formato `.cadx`

- **Versione del formato:** 1
- **Stato:** specifica della versione 1, tappa 0.5
- **Licenza di questo documento:** [Creative Commons Attribution 4.0
  International (CC BY 4.0)](https://creativecommons.org/licenses/by/4.0/),
  come stabilito dall'[ADR 0006, punto 3](../adr/0006-modello-dati-dwg-e-formato-cadx.md).

Questa specifica descrive il formato nativo di Tecnigrafo. Un'implementazione
può leggerlo e scriverlo senza usare il codice di Tecnigrafo. Le parole
**deve**, **non deve** e **può** indicano rispettivamente un obbligo, un divieto
e una possibilità; **consigliato** indica una raccomandazione.

Le regole derivano dagli ADR [0006](../adr/0006-modello-dati-dwg-e-formato-cadx.md),
[0017](../adr/0017-modulo-io-e-dxf.md),
[0018](../adr/0018-conservazione-dati-non-gestiti.md) e
[0019](../adr/0019-archivi-zip.md), e dalla
[scheda P3](../tappe/0.5/P3-specifica-cadx.md) che ne precisa le parti.

## 1. Archivio ZIP

Un `.cadx` deve essere un archivio ZIP conforme alla specifica
[APPNOTE di PKWARE](https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT),
con le restrizioni di seguito. Non è un nuovo algoritmo di compressione.

- I soli metodi ammessi sono **stored** (codice 0, senza compressione) e
  **deflate** (codice 8).
- Cifratura, ZIP64 e archivi distribuiti su più dischi non sono ammessi.
  Il lettore deve rifiutarli con un errore che indica la caratteristica
  non supportata.
- I nomi delle parti devono essere in UTF-8, relativi, senza `..`, senza
  barra inversa (`\`) e senza duplicati. Un nome assoluto non è valido.
  Le barre nei nomi sono `/`; il nome identifica la parte nell'archivio,
  non un percorso da aprire liberamente nel filesystem.
- Il lettore deve usare il record di chiusura e l'elenco centrale per
  individuare le parti e le loro dimensioni. Deve leggere anche gli archivi
  con descrittori dei dati (bit 3 del campo dei flag ZIP).
- Il lettore deve controllare il CRC-32 di ogni parte, anche di quelle
  sconosciute. Un CRC errato rende l'archivio danneggiato.

Queste regole e i limiti seguenti vengono dall'ADR 0019, punto 2. Un lettore
conforme può rifiutare un archivio che supera uno dei limiti, indicando il
motivo:

| Grandezza | Limite |
|---|---|
| Numero totale di parti, comprese quelle sconosciute | 256 |
| Somma delle dimensioni decompresse di tutte le parti | 1 GiB (1 073 741 824 byte) |
| Rapporto fra dimensione decompressa e compressa di ciascuna parte | 1000 a 1 |

I limiti devono essere controllati prima e durante la decompressione. Il
lettore deve interromperla appena l'uscita supera la dimensione dichiarata:
le dimensioni dichiarate non autorizzano a produrre dati senza limite.
Un archivio che mente sulle dimensioni deve produrre un errore.

## 2. Parti della versione 1

I nomi nella tabella sono esatti, comprese le lettere minuscole. Le parti
obbligatorie devono essere presenti una sola volta.

| Nome | Presenza | Contenuto |
|---|---|---|
| `mimetype` | Obbligatoria | Identificatore del formato, descritto sotto |
| `manifest.json` | Obbligatoria | Oggetto JSON con versione, generatore ed elenco delle parti |
| `drawing.dxf` | Obbligatoria | Disegno completo in DXF ASCII R2018, UTF-8 |
| `extensions.json` | Facoltativa | Dati propri di Tecnigrafo, riferiti agli handle DXF |
| `thumbnail.png` | Facoltativa | Immagine PNG di anteprima per i gestori di file |
| Altri nomi | Facoltativa | Parti sconosciute da conservare e riscrivere |

### `mimetype`

Deve essere la prima parte nell'ordine fisico delle intestazioni locali ZIP,
con metodo stored (0), senza campi extra. Il suo contenuto deve essere
esattamente la sequenza ASCII di 31 byte:

```text
application/vnd.tecnigrafo.cadx
```

Non ci sono virgolette, BOM, spazi aggiunti né un a capo finale. Le stesse
restrizioni su metodo e campi extra valgono per la voce dell'elenco centrale.
Questa parte permette di riconoscere il tipo di file senza decomprimere il
disegno (ADR 0006, punto 3; ADR 0019, punto 2).

### `manifest.json`

Deve essere un oggetto JSON. I nomi delle chiavi e i tipi dei valori richiesti
sono:

| Chiave | Tipo | Regola |
|---|---|---|
| `format` | Intero | Versione del formato; per questa specifica è `1` |
| `generator` | Stringa | Programma che ha scritto il file, per esempio `Tecnigrafo 0.5.0` |
| `parts` | Elenco di stringhe | Nomi di tutte le altre parti effettivamente presenti nell'archivio |

`parts` deve elencare ogni altra parte una sola volta, incluse `mimetype`,
le parti facoltative presenti e quelle sconosciute. Non deve includere
`manifest.json` stesso, né nomi di parti assenti. L'ordine di questo elenco
non sostituisce l'ordine fisico dell'archivio.

Ogni altra chiave è ammessa: il lettore deve conservarne nome e valore
JSON e riscriverli al salvataggio. Non è richiesto che spaziatura e ordine
delle chiavi JSON restino identici. Non si deve scartare una chiave perché
non la si conosce (ADR 0006, punto 2; scheda P3, punto 3).

### `drawing.dxf`

Deve contenere l'intero disegno, in **DXF ASCII R2018**, con `$ACADVER`
uguale a `AC1032` e testo in **UTF-8**. «ASCII» indica la rappresentazione
testuale del DXF mediante coppie codice/valore, contrapposta al DXF binario;
non limita il testo ai caratteri ASCII.

Il file deve essere completo e autosufficiente: estratto dall'archivio, deve
potersi aprire in un CAD che legge DXF R2018 senza `manifest.json`,
`extensions.json` o altre parti di `.cadx`. Deve contenere anche tabelle,
blocchi e oggetti necessari al disegno. I dati standard DXF non devono essere
spostati in `extensions.json` (ADR 0006, punti 1 e 3; ADR 0017, punti 2 e 3).

Un disegno proveniente da una versione DXF precedente deve essere convertito
a R2018 quando viene inserito in `.cadx`. Se la conversione non può
conservare qualche dato, il programma deve avvisare prima di salvare, senza
scartarlo in silenzio. Gli handle devono essere conservati, perché
identificano le entità anche nei riferimenti fra parti.

La conservazione dei dati non gestiti segue l'ADR 0018, punti 2 e 6: sezioni,
entità e gruppi non gestiti restano uguali carattere per carattere dopo la
decodifica e nello stesso ordine; i valori delle entità gestite si conservano
entro la tolleranza di verifica (predefinita 1e-9). La riscrittura può
aggiornare `$HANDSEED` e, nella conversione di versione, `$ACADVER` e codifica.
Layer, stili, blocchi, dizionari e dati estesi non si perdono solo perché il
programma non li modifica ancora.

### `extensions.json`

È riservata ai dati propri di Tecnigrafo, per esempio futuri vincoli,
parametri o legami 3D. I riferimenti alle entità devono usare i loro handle
DXF, non indici temporanei o posizioni nell'archivio (ADR 0006, punto 3).

Nella versione 1 non sono definiti dati da generare in questa parte: uno
scrittore può ometterla oppure scrivere un oggetto JSON vuoto (`{}`). Se la
parte esiste già, il lettore deve conservarla così com'è e riscriverne gli
stessi byte di contenuto, anche quando non ne interpreta i dati (scheda P3,
punto 3; ADR 0006, punto 2).

### `thumbnail.png` e parti sconosciute

`thumbnail.png`, se presente, contiene un'anteprima PNG. Non è necessaria
per ricostruire il disegno. La versione 1 non fissa dimensioni o risoluzione
dell'anteprima.

Le parti sconosciute non sono motivo di rifiuto se rispettano le regole
dell'archivio. Il lettore deve conservarne nome e byte del contenuto
decompresso e riscriverli al salvataggio, mantenendole anche in `parts`.
Non è richiesta la conservazione della rappresentazione compressa o dei
metadati ZIP: una parte può essere ricompressa senza cambiarne il contenuto
(ADR 0006, punto 2; scheda P3, punto 3).

## 3. Versioni e compatibilità

La versione è l'intero `format` del manifest, non la versione del programma
in `generator`, né `$ACADVER` del DXF.

Un lettore deve aprire tutte le versioni del formato fino alla propria. Per
un lettore della versione 1 ciò significa la versione 1; questa specifica
non definisce una versione 0. Deve rifiutare un file di versione più recente
con un messaggio che indica sia la versione trovata sia quella supportata,
per esempio:

```text
Unsupported CADX format version 2; this reader supports up to version 1.
```

Una modifica che un lettore precedente interpreterebbe male richiede un
incremento della versione: per esempio cambiare il significato di una
chiave obbligatoria o il formato richiesto del disegno. Aggiungere una chiave
o una parte facoltativa, che un vecchio lettore può conservare senza
interpretare, non richiede un incremento (ADR 0006, punti 2 e 3; scheda P3,
punto 4).

Un file danneggiato o privo di una parte obbligatoria deve produrre un errore,
non un crash o un documento aperto a metà (ADR 0017, punto 6).

## 4. Scrittura deterministica

L'ADR 0019, punto 2, richiede data e ora fisse **1980-01-01 00:00:00** nelle
intestazioni ZIP scritte da Tecnigrafo. Un lettore deve accettare anche
archivi con altre date, come quelli costruiti con strumenti comuni.

Per una scrittura deterministica è inoltre consigliato questo ordine,
nell'archivio e nell'elenco centrale:

1. `mimetype`;
2. `manifest.json`;
3. `drawing.dxf`;
4. tutte le altre parti in ordine di nome.

`mimetype` deve comunque essere sempre primo, anche se non si cerca la
deterministicità. Lo scrittore di Tecnigrafo usa deflate per le altre parti
(ADR 0019, punto 2); il lettore deve accettare anche stored per queste parti.
Per ottenere file identici byte per byte a parità di disegno devono restare
stabili anche serializzazione dei contenuti, metadati ZIP e impostazioni di
compressione. L'ordine consigliato non impone un ordine di lettura delle
parti successive a `mimetype`.

## 5. Esempio di costruzione manuale

Servono gli strumenti `zip` e `unzip` e un file **già valido** `drawing.dxf`
in DXF ASCII R2018 (`AC1032`), UTF-8 e autosufficiente. Non basta cambiare
`$ACADVER` in un DXF precedente: occorre la conversione descritta sopra.

Eseguire i comandi seguenti in una cartella di lavoro contenente solo quel
`drawing.dxf`, senza un precedente `example.cadx`. Il manifest dell'esempio
è completo; non ci sono parti facoltative:

```sh
printf '%s' 'application/vnd.tecnigrafo.cadx' > mimetype
cat > manifest.json <<'JSON'
{
  "format": 1,
  "generator": "Manual ZIP example",
  "parts": ["mimetype", "drawing.dxf"]
}
JSON

touch -t 198001010000.00 mimetype manifest.json drawing.dxf
zip -0 -X example.cadx mimetype
zip -9 -X example.cadx manifest.json drawing.dxf
unzip -l example.cadx
unzip -t example.cadx
```

La prima invocazione crea l'archivio con `mimetype` stored e senza campi extra;
la seconda aggiunge le altre due parti. `-X` esclude i campi extra e `touch`
fissa data e ora. Per dati piccoli, `zip` può scegliere stored invece di
deflate se la compressione non conviene: entrambi sono metodi ammessi.

`unzip -l` deve elencare nell'ordine `mimetype` (31 byte), `manifest.json` e
`drawing.dxf`, tutti con data `1980-01-01 00:00`. `unzip -t` deve terminare
senza errori CRC. Per vedere il metodo di ciascuna parte si può usare
`unzip -v example.cadx`; per estrarre il disegno indipendente:

```sh
unzip -p example.cadx drawing.dxf > extracted.dxf
cmp drawing.dxf extracted.dxf
```

Il confronto non deve rilevare differenze. Questo esempio verifica la
costruzione dell'archivio; la validità DXF del file fornito resta un
prerequisito, da controllare con un lettore DXF R2018 indipendente.
