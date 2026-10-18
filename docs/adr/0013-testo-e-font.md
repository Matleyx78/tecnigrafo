# ADR 0013: Testo su una riga e font incluso

- **Stato:** Accettato (2026-10-18)
- **Data:** 2026-10-18

## Contesto

La tappa 0.4 introduce il testo su una riga (entità DXF `TEXT`). Il testo pone tre
problemi che le altre entità non hanno:

- il suo ingombro dipende dal font, ma `core` deve conoscerlo senza Qt (ADR 0001):
  servono il riquadro per l'indice spaziale, la selezione, lo snap e il rapporto
  della modalità di controllo al livello 1 (ADR 0011);
- lo stesso disegno deve apparire uguale su Windows, Linux e macOS, e i confronti
  di immagini del livello 2 devono essere ripetibili;
- il DXF ha molte proprietà del testo (allineamenti, fattore di larghezza,
  obliquo, specchiatura, stile) che la 0.4 non usa ma che vanno conservate
  (ADR 0006).

Gli stili di testo, con la scelta del font, arrivano nella tappa 0.8; il testo
come geometria (per la stampa) nella 0.9.

## Decisione

### 1. Entità

`Text` corrisponde a `TEXT` del DXF e ne ha tutte le proprietà:

| Proprietà | DXF | Nella 0.4 |
|---|---|---|
| `position` | 10/20 | primo punto d'allineamento |
| `alignmentPoint` | 11/21 | conservato |
| `height` | 40 | altezza delle maiuscole, > 0 |
| `rotation` | 50 | radianti nel modello, gradi nel DXF |
| `widthFactor` | 41 | usato nel disegno e nell'ingombro |
| `obliqueAngle` | 51 | conservato |
| `style` | 7 | sempre `Standard` fino alla 0.8 |
| `mirroredX`, `mirroredY` | 71 | conservati |
| `horizontal` | 72 | conservato; i comandi creano `Left` |
| `vertical` | 73 | conservato; i comandi creano `Baseline` |
| `value` | 1 | UTF-8, con i codici di controllo così come scritti |

- **Altezza** è l'altezza delle maiuscole, come in AutoCAD e nella norma
  ISO 3098.
- Disegno e ingombro della 0.4 usano posizione, altezza, rotazione e fattore di
  larghezza. Obliquo, specchiatura e allineamenti diversi da sinistra sulla linea
  di base si conservano ma non si disegnano: arrivano con l'import DXF della
  tappa 0.5, la cui scheda li deve includere.

### 2. Font incluso

- Un solo file di font, in `resources/fonts/`, usato per **tutti** i testi
  qualunque sia lo stile, fino alla tappa 0.8. Accanto ci sono la licenza e un
  `README.md` con fonte, versione e SHA-256.
- Requisiti: licenza compatibile con la GPLv3 che ne permetta la ridistribuzione;
  formato TrueType con tabella `OS/2` di versione 2 o successiva (altezza delle
  maiuscole); caratteri latini con accenti (Latin-1 e Latin Extended-A), greco,
  e i simboli `°`, `±`, `²`, `³`, `Ø`.
- **Font scelto: Liberation Sans Regular 2.1.5**, licenza SIL Open Font License
  1.1. Il file TTF si prende dall'archivio ufficiale della versione 2.1.5 del
  progetto `liberationfonts/liberation-fonts`. Verificato sui sorgenti della
  2.1.5: `OS/2` versione 3; latino con Latin Extended-A, greco, cirillico; `°`,
  `±`, `²`, `³`, `Ø` presenti. Ha le stesse larghezze di Arial: i disegni di
  AutoCAD con stili in Arial avranno lo stesso ingombro quando arriveranno gli
  stili (0.8).
- Liberation Sans **non ha** il segno di diametro `⌀` (U+2300) né `∅`
  (U+2205). Per questi c'è una tabella di sostituzione (punto 5).

### 3. Misure in `core`

- Un piccolo programma del progetto, `tools/fontmetrics` (C++ standard, nessuna
  libreria), legge dal file TTF le tabelle `head`, `hhea`, `OS/2`, `cmap` e
  `hmtx` e genera a ogni build una tabella con ascendente, discendente e
  larghezza di ogni carattere, in unità dell'altezza delle maiuscole. Il file
  del font è l'unica fonte delle misure.
- La larghezza di una riga è la somma delle larghezze dei caratteri: niente
  crenatura, legature o composizione dei caratteri. Le scritture da destra a
  sinistra e quelle che richiedono composizione (arabo, scritture indiane) non
  sono gestite.
- Un carattere che il font non ha prende la larghezza del suo carattere
  mancante (`.notdef`), e si disegna così.
- **Ingombro**: nel sistema del testo, da 0 a `larghezza · altezza ·
  widthFactor` in orizzontale e da `discendente · altezza` ad `ascendente ·
  altezza` in verticale, poi ruotato intorno a `position`.

### 4. Disegno

- Il `Painter` riceve `drawText(TextRun)`: testo già convertito in caratteri,
  origine della linea di base e altezza delle maiuscole in pixel, angolo, fattore
  di larghezza.
- `QtPainter` usa solo il font incluso: niente font di riserva del sistema,
  niente crenatura, niente hinting; ogni carattere va nella posizione calcolata
  con le misure di `core`, così testo disegnato e ingombro coincidono.
- Sotto 2 pixel di altezza delle maiuscole si disegna il solo rettangolo
  d'ingombro.
- I confronti di immagini con testo valgono solo su Linux (ADR 0011, punto 6).

### 5. Codici di controllo

Il valore si conserva com'è scritto. Nel disegno e nelle misure `%%c` diventa
`⌀` (U+2300), `%%d` `°`, `%%p` `±`, `%%%` `%`. Gli altri codici (`%%o`, `%%u`,
`%%nnn`) restano come sono scritti fino alla tappa 0.8.

**Sostituzioni.** Un carattere che il font non ha, ma che ha un sostituto
nell'elenco di `core`, si misura e si disegna con il sostituto: `⌀` (U+2300) e
`∅` (U+2205) diventano `Ø` (U+00D8). Il valore del testo non cambia: con un font
che ha `⌀` si vedrà il carattere vero. L'elenco si allarga con un nuovo ADR.

### 6. Selezione e snap

- Il testo è una **regione**: un clic dentro il rettangolo d'ingombro lo sceglie;
  la finestra lo prende se il rettangolo è tutto dentro; l'intersezione se lo
  tocca.
- Nuovo snap **inserimento** (`SnapKind::Insertion`, `SnapModes::insertion`,
  attivo all'avvio come gli altri): il punto `position`. Si aggancia passando
  sopra il testo (puntatore entro l'apertura dal rettangolo d'ingombro), con la
  stessa priorità del centro di un arco: vince solo se non ci sono estremi o
  medi entro l'apertura.
- Marcatore: due quadrati sovrapposti e sfalsati, distinto da quello
  dell'estremo.

### 7. Comando `TEXT`

- Domande come in AutoCAD: punto iniziale, altezza (predefinita 2,5), rotazione
  (predefinita 0), poi le righe; una riga vuota termina (ADR 0014 per le
  richieste di valori e di testo).
- **Interlinea** tra righe successive dello stesso comando: 5/3 dell'altezza,
  nella direzione perpendicolare alla rotazione, verso il basso del testo. Si
  confronterà con AutoCAD quando sarà possibile; cambiarla richiede un nuovo
  ADR.
- `Esc` conserva le righe già confermate (ADR 0015).

### 8. Verifica

Oltre alle domande generali dell'ADR 0011 (punto 3), il testo si verifica con
`font-metrics` e `text-width`, i cui valori attesi si leggono dal file del font con
uno strumento indipendente, e con `box`, il rettangolo d'ingombro.

## Alternative considerate

- **osifont:** disegnato secondo ISO 3098 per il disegno tecnico, ma con licenza
  GPL con eccezione per i font invece della OFL indicata nella scheda.
- **Noto Sans:** licenza OFL e copertura più ampia, ma larghezze diverse da
  Arial, il font TrueType più usato negli stili di AutoCAD.
- **Font di sistema:** il testo cambierebbe aspetto e ingombro da un sistema
  all'altro, e i confronti di immagini non sarebbero ripetibili.
- **Misure chieste a Qt e passate a `core`:** il livello 1 della modalità di
  controllo e i test di `core` non le avrebbero, e `core` dipenderebbe di fatto
  dall'interfaccia (ADR 0001).
- **Larghezza fissa per carattere:** semplice, ma l'ingombro sarebbe sbagliato e
  la selezione imprecisa.
- **FreeType o un'altra libreria in `core`:** vietato dall'ADR 0001; per sole
  larghezze basta leggere due tabelle del file.
- **Tabella delle misure scritta una volta e messa nel repository:** si
  disallineerebbe al primo aggiornamento del font.
- **Font SHX come AutoCAD (`txt.shx`, `isocp.shx`):** formato di Autodesk, non
  libero. Si potrà valutare la lettura di file SHX dell'utente con gli stili
  della 0.8.
- **Testo come geometria già nella 0.4:** serve alla stampa (0.9); per la vista
  il disegno con il font basta.

## Conseguenze

- Il repository contiene un file di font con la sua licenza OFL, da citare nel
  README e negli installatori (0.9). La OFL permette di includerlo e
  ridistribuirlo con il programma; non permette di venderlo da solo né di
  distribuire versioni modificate con il nome "Liberation".
- La scheda della 0.5 deve includere il disegno di tutti gli allineamenti, di
  obliquo e specchiatura.
- Gli stili di testo della 0.8 dovranno decidere come associare un nome di font
  a un file incluso o dell'utente; le misure in `core` restano generate dal
  file.
- Il testo come geometria della 0.9 potrà usare lo stesso file, estendendo
  `fontmetrics` ai contorni dei caratteri (tabella `glyf`).
