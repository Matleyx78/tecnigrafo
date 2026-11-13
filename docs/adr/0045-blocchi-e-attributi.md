# ADR 0045: Blocchi e attributi

- **Stato:** Accettato
- **Data:** 2026-11-11
- **Corretto:** 2026-11-13 — punto 7: il blocco preso da un file ha punto base
  (0,0) e le entità spostate di −`$INSBASE`; con un nome che esiste già
  `-INSERT` usa la definizione che c'è, e `nome=percorso` ridefinisce senza
  domande, come AutoCAD (decisione del 2026-11-13 in `docs/DECISIONI.md`).

## Contesto

La tappa 0.8 porta i blocchi: definizione, inserimento con scala e rotazione,
esplosione, anche in DXF; gli attributi al livello (b) della
[#522](https://github.com/Matleyx78/tecnigrafo/issues/522) (decisione del
2026-11-07); l'inserimento di un file DXF come blocco e i valori predefiniti
degli attributi senza domande (decisioni del 2026-11-11,
[#538](https://github.com/Matleyx78/tecnigrafo/issues/538)).

I blocchi devono servire già all'elettrico. La specifica dei blocchi
intelligenti (bozza, nel raccoglitore grafico `tec-ss-grafica`,
`electrical/blocchi-intelligenti/SPECIFICA.md`) usa soltanto il DXF normale: un
file per simbolo, geometria sul layer `0` con colore e tipo di linea DaLayer,
parti piene come `SOLID`, un tipo di linea proprio, e attributi: i **pin** sono
`ATTDEF` costanti e invisibili (flag 70 = 3) nella posizione del collegamento,
gli altri dati `ATTDEF` variabili, visibili o no. Il programma dell'elettrico,
dopo la 1.0, leggerà questi attributi; la 0.8 deve rappresentarli, conservarli
e saperli leggere nelle coordinate del disegno.

## Decisione

### 1. La tabella dei blocchi

- `core::BlockTable` (ADR 0044, punto 3) tiene le definizioni per nome, con il
  confronto dei nomi delle tabelle (`sameTableName`), nell'ordine del file.
- `core::BlockDefinition`: nome, punto base, descrizione, unità (gruppo 70 del
  `BLOCK_RECORD`), *esplodibile* (280) e *scala uniforme* (281), segnali (70 del
  `BLOCK`: anonimo, con attributi), le entità gestite (`EntityRecord`, con i
  loro handle), quelle non gestite (`RawEntity`), e in `extras` i gruppi di
  `BLOCK`, `ENDBLK` e `BLOCK_RECORD` che il modello non usa.
- Fuori dalla tabella, come oggi: `*Model_Space`, `*Paper_Space…` e le loro
  entità; i blocchi esterni (xref, 70 bit 4); i blocchi anonimi `*D…` delle
  quote, che appartengono alla loro quota (ADR 0044, punto 5). Gli altri blocchi
  anonimi (`*U…`, `*X…`) stanno nella tabella.
- **Nessun ciclo.** Un blocco non contiene, nemmeno attraverso altri blocchi,
  un inserimento di sé stesso. La tabella rifiuta una definizione che creerebbe
  un ciclo; il lettore lascia non gestiti gli inserimenti che lo creano.

### 2. L'inserimento

- `core::Insert`: nome del blocco, punto d'inserimento, scala X e Y (la Z si
  conserva), rotazione, e gli attributi (`std::vector<Attribute>`, punto 6),
  ognuno con il suo handle.
- La trasformazione dal blocco al disegno è: togliere il punto base, scalare
  (X, Y), ruotare, portare al punto d'inserimento. Con `|X| = |Y|` è una
  similitudine (con `X·Y < 0`, una riflessione); altrimenti un'affinità
  (ADR 0044, punto 2).
- Scala zero e inserimenti annidati più profondi di quanto il programma può
  disegnare non esistono: la scala zero resta non gestita alla lettura e un
  comando la rifiuta; i cicli non ci sono (punto 1).

### 3. Proprietà dentro un blocco

Come in AutoCAD:

- un'entità del blocco sul layer `0` si mostra sul layer dell'inserimento, e
  ne segue lo stato (spento, congelato);
- colore, tipo e spessore di linea *DaBlocco* prendono quelli
  dell'inserimento; *DaLayer* quelli del layer su cui l'entità si mostra;
- un layer spento o congelato di un'entità del blocco la nasconde; il layer
  dell'inserimento spento o congelato nasconde tutto l'inserimento.

La geometria generata porta le proprietà già risolte (ADR 0044, punto 2).

### 4. Comandi e trasformazioni

- `MOVE`, `COPY`, `ROTATE`, `SCALE` e `MIRROR` cambiano i dati
  dell'inserimento (punto, scale, rotazione) e trasformano i suoi attributi
  nello stesso passo; una copia dà handle nuovi all'inserimento e ai suoi
  attributi ([ADR 0031](0031-modifiche-alle-entita.md), punto 6). `MIRROR`
  rende la riflessione con la scala X di segno opposto e la rotazione
  corrispondente, e tratta gli attributi come i testi, secondo `MIRRTEXT`.
- **`EXPLODE`** di un inserimento, come AutoCAD:
  - le entità del blocco, trasformate, entrano nel disegno con handle nuovi e
    con le **loro** proprietà (un'entità del layer `0` resta sul layer `0`);
  - gli inserimenti annidati restano inserimenti (si esplode un livello);
  - gli attributi spariscono con i loro valori, e al loro posto compaiono gli
    `ATTDEF` della definizione, trasformati;
  - con **scala non uniforme**, come AutoCAD, cerchi e archi diventano ellissi
    e archi d'ellisse ([ADR 0052](0052-ellisse.md)), i testi si deformano con
    fattore di larghezza e obliquo (ADR 0044, punto 2); un inserimento annidato
    resta un inserimento solo se la trasformazione composta è ancora scala più
    rotazione, altrimenti l'esplosione si rifiuta con un messaggio che lo dice;
  - un blocco non esplodibile (280 = 0) si rifiuta, con il messaggio di AutoCAD.
- **Grip:** uno solo, sul punto d'inserimento, che sposta l'inserimento con i
  suoi attributi. **Snap:** inserimento (`ins`) sul punto d'inserimento, e gli
  snap delle entità del blocco sulla geometria generata.

### 5. Definire e ridefinire

- `-BLOCK` (con l'alias `BLOCK`, come `-LAYER`, finché non c'è una finestra)
  crea una definizione dagli oggetti scelti, con nome e punto base; gli oggetti
  scelti si cancellano, come fa `-BLOCK` di AutoCAD. Se il nome esiste, il
  comando chiede se ridefinire.
- **Ridefinire** cambia le entità e il punto base della definizione; tutti gli
  inserimenti si aggiornano (ADR 0044, punto 2). Gli `ATTRIB` degli inserimenti
  che ci sono già non cambiano, come in AutoCAD senza `ATTSYNC`.
- Una definizione che ha `ATTDEF` ha il segnale «con attributi» (70 bit 2).

### 6. Attributi

- `core::AttributeDefinition` (`ATTDEF`): i campi di `Text` (ADR 0013, 0022) più
  etichetta (2), richiesta (3), valore predefinito (1) e segnali (70:
  invisibile 1, costante 2, da verificare 4, preimpostato 8), e il blocco di
  posizione bloccata (280) conservato. Sta nelle definizioni dei blocchi e, come
  in AutoCAD, anche nel disegno, dove si mostra con l'etichetta.
- `core::Attribute` (`ATTRIB`, dentro `Insert`): i campi di `Text` più etichetta,
  valore e segnali, con handle ed `extras` propri. L'inserimento li scrive dopo
  di sé, con `66 = 1` e `SEQEND` (ADR 0018).
- Le etichette si confrontano senza distinguere maiuscole e minuscole.
- **All'inserimento** (`ATTREQ` = 0, decisione del 2026-11-11): per ogni `ATTDEF`
  **non costante** della definizione, nell'ordine, nasce un `ATTRIB` con il
  valore predefinito, senza domande, nella posizione dell'`ATTDEF` portata nel
  disegno dalla trasformazione dell'inserimento (con la scomposizione del testo
  sotto un'affinità, ADR 0044, punto 2). Gli `ATTDEF` costanti non danno
  `ATTRIB`: restano nella definizione.
- **Lettura nel disegno.** `core` offre, per un inserimento, tutti i suoi
  attributi con etichetta, valore, segnali e posizione nel disegno, compresi
  quelli costanti della definizione: è il modo in cui l'elettrico leggerà i pin
  dei suoi simboli.
- Gli attributi invisibili non si disegnano e non si selezionano.
- Restano fuori i comandi per creare e modificare gli attributi (`ATTDEF`,
  `ATTEDIT`, `ATTSYNC`, `BATTMAN`) e le domande all'inserimento: dopo la 0.8.

### 7. Un file DXF come blocco

`-INSERT` accetta, al posto del nome di un blocco, il percorso di un file DXF
(come AutoCAD con un file DWG):

- la definizione ha il **nome del file** senza estensione, le entità dello
  spazio modello del file spostate di −`$INSBASE` e come punto base (0,0),
  come le scrive AutoCAD; lo spazio carta del file non entra;
- le **tabelle si fondono**: layer, tipi di linea, stili di testo, stili di
  quota e definizioni di blocchi del file entrano nel disegno, ma a parità di
  nome vince quella del disegno, senza messaggi, come AutoCAD; così una libreria
  di simboli porta anche i suoi tipi di linea;
- gli **handle** del file si rinumerano con quelli liberi del disegno, e i
  riferimenti tra loro (proprietari, reattori) si aggiornano; i dati non gestiti
  che nominano handle che non si sanno riconoscere si conservano come sono;
- **unità:** se il file e il disegno hanno tutti e due `$INSUNITS` diverso da
  zero e diversi tra loro, la scala proposta dell'inserimento si moltiplica per
  il fattore di conversione; altrimenti è 1;
- se un blocco con quel nome **esiste già**, il comando inserisce la
  definizione che c'è, senza domande; la forma **`nome=percorso`** definisce o
  ridefinisce il blocco `nome` dal file senza domande, come AutoCAD da script;
- un file illeggibile, o che contiene un inserimento di sé stesso, si rifiuta
  con il messaggio del lettore.

## Alternative considerate

- **Inserimenti come entità non gestite che si disegnano soltanto:** si
  vedrebbero, ma `MOVE`, `COPY` ed `EXPLODE` non li tratterebbero, e gli
  attributi resterebbero incoerenti.
- **`ATTRIB` come entità separate nel disegno:** come nel DXF, ma ogni
  operazione dovrebbe ricordarsi di portarle con l'inserimento; dentro `Insert`
  la coerenza viene da sé.
- **Domande sugli attributi all'inserimento (`ATTREQ` = 1):** il responsabile
  le ha rimandate ai comandi degli attributi, dopo la 0.8.
- **Esplodere la scala non uniforme in polilinee approssimate:** cambierebbe
  la geometria senza dirlo. Con l'ellisse (ADR 0052) l'esplosione è esatta.
- **XDATA o `extensions.json` per i dati dell'elettrico:** la specifica dei
  blocchi intelligenti li ha scartati (sezione 9): gli attributi si vedono in
  ogni CAD.

## Conseguenze

- `io` legge e scrive la sezione `BLOCKS` dal modello, non più come gruppi
  grezzi, e tiene coerente la tabella `BLOCK_RECORD`.
- La libreria di simboli dell'elettrico (un file per simbolo, CC0) è anche un
  buon insieme di casi per l'inserimento di file: se il responsabile la porta
  nel repository dei DXF di prova, i riferimenti la possono usare.
- L'elettrico, dopo la 1.0, parte da attributi già letti e scritti.
