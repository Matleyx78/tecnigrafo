# ADR 0046: Stili di testo e testo multiriga

- **Stato:** Accettato
- **Data:** 2026-11-11

## Contesto

La scheda della tappa 0.8 mette dentro «stili di testo e testo multiriga». Oggi
il testo su una riga (`TEXT`, [ADR 0013](0013-testo-e-font.md) e
[0022](0022-allineamenti-obliquo-specchiatura-testo.md)) conserva il nome dello
stile ma la tabella `STYLE` resta grezza, e `MTEXT` è un dato non gestito. Le
quote della 0.8 scrivono il loro testo come `MTEXT` dentro il blocco `*D…`,
come AutoCAD, e usano uno stile di testo (`DIMTXSTY`).

## Decisione

### 1. Stili di testo

- `core::TextStyleTable` (ADR 0044, punto 3) con i record `STYLE`: nome, file
  del font (3) e del font esteso (4), altezza fissa (40, 0 = nessuna), fattore di
  larghezza (41), obliquo (50), generazione (71: all'indietro, capovolto),
  ultima altezza usata (42), e gli altri gruppi in `extras`. I record che
  descrivono file di forme (70 bit 1) restano grezzi nella tabella.
- Il disegno usa **sempre il font incluso**, Liberation Sans (ADR 0013): il nome
  del font si legge, si conserva e si riscrive, non si usa.
- Lo stile corrente è `$TEXTSTYLE`; un disegno nuovo ha `Standard`.
- `TEXT` (e `MTEXT`) creano il testo con lo stile corrente: se lo stile ha
  un'altezza fissa, `TEXT` non chiede l'altezza; fattore di larghezza e obliquo
  dello stile si copiano nel testo nuovo, come in AutoCAD.
- **`-STYLE`** (alias `STYLE`), nella forma da riga di comando di AutoCAD:
  nome (o `?` per l'elenco), font, altezza, fattore di larghezza, obliquo,
  all'indietro, capovolto; crea lo stile o cambia quello che c'è, e lo rende
  corrente. **`TEXTSTYLE`** dice e cambia lo stile corrente. Rinominare e
  cancellare gli stili resta fuori (con `RENAME` e `PURGE`).

### 2. Testo multiriga

- `core::MText`: punto d'inserimento (10), larghezza di riferimento (41; 0 =
  senza a capo), altezza (40), punto di allineamento (71, da 1 a 9), direzione
  (72: solo da sinistra a destra o «secondo lo stile»; le altre restano
  grezze), rotazione (50, o il vettore 11/21 se c'è), stile (7), interlinea
  (73, 44), il valore (gruppi 3 e 1 uniti), e il resto in `extras` (sfondo,
  colonne vuote). Un `MTEXT` con colonne resta non gestito (ADR 0044, punto 4).
- **Codici di formato.** Per l'impaginazione contano `\P` (nuovo paragrafo),
  `\~` (spazio che non va a capo), `\\`, `\{`, `\}`, le graffe di gruppo,
  `%%c`, `%%d`, `%%p`, `%%%` e `\U+XXXX`. Gli altri codici (`\f`, `\H`, `\W`,
  `\Q`, `\T`, `\A`, `\C`, `\S`, `\L`, `\l`, `\O`, `\o`, `\K`, `\k`, `\p…;`) si
  riconoscono e si saltano: il testo si mostra semplice, all'altezza
  dell'entità, e il valore resta intatto nel file.
- **Impaginazione.** Le righe sono i paragrafi; con la larghezza maggiore di 0,
  ogni paragrafo va a capo agli spazi, quando la parola successiva supererebbe
  la larghezza, misurata con le misure del font incluso, l'altezza e il fattore
  di larghezza dello stile. Una parola più larga della riga resta intera. La
  distanza fra le linee di base è `5/3 · altezza · fattore d'interlinea`, come
  nell'ADR 0013 e in AutoCAD.
- **Allineamento.** Le righe stanno in una colonna larga quanto la larghezza di
  riferimento, o quanto la riga più lunga se è 0; ogni riga è a sinistra, al
  centro o a destra della colonna; la colonna sta rispetto al punto
  d'inserimento secondo il punto di allineamento: in alto la prima linea di
  base è un'altezza sotto il punto, in basso l'ultima linea di base è sul punto,
  in mezzo il blocco delle righe è centrato.
- **Geometria generata** (ADR 0044, punto 2): un `Text` per riga, allineato a
  sinistra sulla linea di base, con rotazione, fattore di larghezza e obliquo
  dello stile. Disegno, selezione e snap riusano quelli del testo; lo snap
  d'inserimento è sul punto d'inserimento del `MTEXT`, il grip anche.
- **`MTEXT`** nella forma da riga di comando di AutoCAD: primo angolo, angolo
  opposto con le opzioni *Height*, *Justify*, *Line spacing*, *Rotation*,
  *Style* e *Width*, poi le righe del testo, ognuna un paragrafo, fino a una
  riga vuota. L'editor nella finestra resta fuori, dopo la 0.8 (idea
  [#662](https://github.com/Matleyx78/tecnigrafo/issues/662), con `TEXTEDIT`).

## Alternative considerate

- **Font veri per ogni stile:** richiederebbe di cercare e caricare i font del
  sistema, con risultati diversi da un PC all'altro; l'ADR 0013 ha scelto un
  font solo.
- **Interpretare tutti i codici di formato:** caratteri, colori e frazioni
  impilate cambiano le misure e il disegno di ogni riga; si conserva tutto e si
  mostra il testo semplice.
- **`MTEXT` come entità che si disegna da sé:** duplicherebbe disegno,
  selezione e snap del testo.

## Conseguenze

- Le quote misurano il loro testo con l'impaginazione del `MTEXT`.
- Un disegno di AutoCAD con testo formattato si vede con il testo giusto ma
  senza i formati; lo si scrive nel manuale.
