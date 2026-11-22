# ADR 0054: Testo formattato ed editor nella finestra

- **Stato:** Proposto
- **Data:** 2026-11-21

## Contesto

L'[ADR 0046](0046-stili-di-testo-e-testo-multiriga.md), punto 2, riconosce i
codici di formato di `MTEXT` e li salta: il testo si mostra semplice e il valore
resta intatto. Il responsabile ha deciso per la tappa 0.8.1
([DECISIONI.md](../DECISIONI.md), 2026-11-21, «Testo formattato, griglia, snap
e attributi della 0.8.1», punto 1) che i formati si mostrano nel disegno e
nell'editor, che l'editor sta nella finestra (idea
[#662](https://github.com/Matleyx78/tecnigrafo/issues/662)) e che `\f` con un
nome qualsiasi si disegna con Liberation Sans, nelle quattro varianti della
stessa famiglia e licenza ([ADR 0013](0013-testo-e-font.md), un font solo).

Il DXF di prova `autocad-mtext-formati.dxf`
([tappe/0.8.1/dxf-di-prova.md](../tappe/0.8.1/dxf-di-prova.md)) ha, per ogni
formato, la copia esplosa da AutoCAD: dice quali pezzi fa AutoCAD, con quale
altezza, fattore di larghezza, obliquo e colore, e dove va a capo il paragrafo
largo 60. Le **posizioni** di AutoCAD però non sono le nostre: AutoCAD misura un
font TrueType in unità dell'altezza delle maiuscole **di Arial** (1467 su 2048),
Tecnigrafo in quelle di Liberation Sans (1409 su 2048, ADR 0013): a parità di
altezza le nostre righe sono più larghe del 4,1 % (`Normale ` è lungo 14,168 in
AutoCAD e 14,741 da noi, `rosso` 8,540 e 8,886). Con la larghezza di
riferimento, dunque, un paragrafo può andare a capo in un punto diverso. La
stessa differenza c'è dalla tappa 0.4 per ogni `TEXT` con uno stile Arial.

## Decisione

### 1. Formati interpretati

Nel disegno, nella selezione e nell'editor contano, oltre ai codici dell'ADR
0046, punto 2:

| Codice | Effetto | Fine |
|---|---|---|
| `\f<nome>\|b<0/1>\|i<0/1>…;`, `\F…;` | variante Regular, Bold, Italic o BoldItalic; il nome resta nel valore | fine del gruppo `{}` |
| `\H<n>;`, `\H<n>x;` | altezza assoluta, o relativa a quella corrente | fine del gruppo |
| `\W<n>;` | fattore di larghezza | fine del gruppo |
| `\Q<gradi>;` | obliquo | fine del gruppo |
| `\T<n>;` | spaziatura: ogni avanzamento si moltiplica per `n` | fine del gruppo |
| `\C<aci>;`, `\c<bgr>;` | colore indice, o colore vero in **BGR** come AutoCAD (`\c16744448;` è `0080FF` in RGB) | fine del gruppo |
| `\L` `\l`, `\O` `\o`, `\K` `\k` | sottolineato, sopralineato, barrato | codice di chiusura o fine del gruppo |
| `\S<a>/<b>;`, `\S<a>#<b>;`, `\S<a>^<b>;` | frazione impilata: orizzontale, diagonale, senza linea (tolleranza) | il `;` |

Un `\f` con qualunque nome usa Liberation Sans; con `|b1` e `|i1` la variante.
Restano saltati e conservati, come nella 0.8: `\A`, `\p…;` (rientri e
tabulazioni) e le colonne. Un codice malformato si mostra come testo, come oggi
(ADR 0046, punto 2).

### 2. Le quattro varianti del font

`resources/fonts` porta, accanto al Regular, `LiberationSans-Bold.ttf`,
`LiberationSans-Italic.ttf` e `LiberationSans-BoldItalic.ttf` dello stesso
archivio 2.1.5, con i loro SHA-256 nel README; `tools/fontmetrics` legge i
quattro file e genera quattro tabelle. `core::FontFace` (`Regular`, `Bold`,
`Italic`, `BoldItalic`) sceglie la tabella; le misure restano in unità
dell'altezza delle maiuscole **di ciascun file** e deterministiche sui tre
sistemi.

### 3. Impaginazione

Un paragrafo è una sequenza di **pezzi**: tratti di testo con gli stessi
formati, più le frazioni impilate, che sono un pezzo ciascuna. Regole, in unità
dell'altezza `h` del pezzo:

1. **Larghezza di un pezzo:** somma degli avanzamenti della sua variante ×
   `h` × fattore di larghezza × spaziatura; una frazione è larga quanto il più
   largo dei suoi due testi.
2. **A capo:** agli spazi, quando la parola successiva supererebbe la larghezza
   di riferimento (ADR 0046, punto 2), con le larghezze del punto 1; una parola
   che contiene pezzi diversi va a capo intera.
3. **Prima linea di base:** sotto il punto d'inserimento (allineamento in alto)
   della più grande altezza della prima riga; il sopralineato vi aggiunge
   `0,2 · h` del pezzo sopralineato più alto (AutoCAD: 3,0 per `h` 2,5).
4. **Interlinea con altezze diverse:** la distanza fra due linee di base è
   `5/3 · H · fattore d'interlinea`, con `H` la più grande altezza della riga
   **sotto**; con altezze tutte uguali è quella dell'ADR 0046.
5. **Frazioni:** i due testi hanno l'altezza del pezzo; il numeratore sta
   sopra, con la linea di base a `4/3 · h` dalla riga (AutoCAD: 1,337 · h), il
   denominatore sulla linea di base della riga; i due sono centrati sulla
   larghezza del pezzo; la linea di `/` è orizzontale a `7/6 · h` (AutoCAD:
   1,171 · h), larga quanto il pezzo; quella di
   `#` va dall'angolo in basso a sinistra a quello in alto a destra del pezzo;
   `^` non ha linea.
6. **Righe di decorazione:** sottolineato a `−0,2 · h`, sopralineato a
   `1,2 · h`, barrato a `0,5 · h` dalla linea di base, lunghe quanto il pezzo,
   con il colore del pezzo.

I numeri dei punti 3–6 sono regole nostre, in unità dell'altezza: AutoCAD li
ricava dalle forme vere dei caratteri di Arial, e i DXF di prova non bastano a
ricostruirli. I riferimenti si calcolano da queste regole e dalle misure di
Liberation Sans; da AutoCAD si prende solo la **struttura** (quali pezzi, quali
formati, che cosa va sulla stessa riga quando le larghezze non cambiano la
scelta) ([DECISIONI.md](../DECISIONI.md), 2026-11-22, «Decisioni D1–D10 del
piano della 0.8.1», punto 1).

### 4. Geometria generata

Per ogni pezzo nasce, nell'ordine di lettura, un **testo formattato**
(`GeneratedShape` nuova, `FormattedText`: un `Text` e la sua `FontFace`), con le
proprietà effettive del pezzo (il colore di `\C` o `\c`); le righe di
decorazione e le linee delle frazioni sono `TextLine` (una `Line` che non dà
snap, nemmeno dentro un blocco o una quota) con lo stesso colore. Un pezzo in
Regular resta un `Text`, come nella 0.8, così i casi di oggi senza formati non
cambiano. Selezione e snap del `MTEXT` restano quelli dell'ADR 0046, punto 2:
le righe non danno snap. Il numero di righe e il testo mostrato (rapporto,
quote) vengono dalle righe dell'impaginazione, non dal numero dei pezzi. Le quote che hanno testo formattato
usano la stessa impaginazione, quindi cambiano grafica come in AutoCAD.

### 5. Editor nella finestra

- Per `MTEXT` (dopo i due angoli), per `TEXTEDIT` su un `MTEXT` o su una quota,
  e per l'opzione *Mtext* delle quote, il programma apre sul disegno un
  riquadro largo quanto la colonna, in scala con lo zoom e ruotato con il
  testo, che va a capo dal vivo con le regole del punto 3. `Invio` va a capo
  (nuovo paragrafo), `Ctrl+Invio` o un clic fuori confermano, `Esc` annulla.
- Una barra dei formati applica alla parte scelta i formati del punto 1;
  l'editor **non perde mai** un codice che non sa mostrare: lo conserva nel
  punto del testo in cui stava.
- Su un `TEXT` l'editor è una riga sola, senza formati, sul testo stesso.
- Negli script, e quando la finestra non c'è, vale la forma da riga di comando:
  `MTEXT` chiede le righe come nella 0.8, `TEXTEDIT` e *Mtext* chiedono il
  valore con i codici su una riga (punto 6).

### 6. `TEXTEDIT` dalla riga di comando

`TEXTEDIT` (alias `ED`, nome lungo `DDEDIT`): `Select an annotation object or
[Mode]:`, con *Mode* `Single` o `Multiple` (predefinito `Multiple`, come
AutoCAD). Per l'oggetto scelto:

- `TEXT`, `MTEXT`, testo di una quota: `Enter new text <valore>:`; Invio tiene il
  valore; per la quota il valore è il testo sostitutivo, con `<>` per la misura,
  e la riga vuota scritta come `<>` torna alla misura;
- `ATTDEF`: `Enter attribute tag <…>:`, `Enter attribute prompt <…>:`, `Enter
  default attribute value <…>:`;
- `INSERT` con attributi: per ogni attributo non costante, nell'ordine, `<etichetta>
  <valore>:`.

Ogni modifica è un passo di annullamento. AutoCAD non ha una forma da riga di
comando di `TEXTEDIT` (apre sempre l'editor o una finestra): questa è di
Tecnigrafo, per gli script e per la verifica ([DECISIONI.md](../DECISIONI.md),
2026-11-22, «Decisioni D1–D10 del piano della 0.8.1», punto 2).

## Alternative considerate

- **Editor `QTextEdit` sopra la vista:** non ruota e non va a capo con le
  misure del disegno; l'editor mostrerebbe a capo diversi da quelli salvati.
- **Font diversi per ogni `\f`:** scartato dall'ADR 0013 e di nuovo dal
  responsabile (decisione citata, punto 1).
- **Posizioni copiate da AutoCAD,** con una tolleranza del confronto: le
  larghezze differiscono del 4,1 % e il paragrafo largo 60 può andare a capo in
  punti diversi.
- **Scala dei font TrueType sull'altezza delle maiuscole di Arial:** cambierebbe
  ogni caso di testo dalla tappa 0.4; fuori dalla 0.8.1.
- **`-TEXTEDIT` a parte per la riga di comando:** un comando che AutoCAD non
  ha.

## Conseguenze

- I casi di oggi con codici di formato (`mtext/codici-semplici`,
  `mtext/codici-letterali`, `dxf/mtext-proprieta`) cambiano risposta: li
  aggiorna il pacchetto di riferimenti dell'impaginazione
  ([0.8.1-PT8](../tappe/0.8.1/PT8-impaginazione-formati.md)).
- Il manuale dice che i font scelti nello stile o con `\f` si disegnano con
  Liberation Sans, e che le misure differiscono da quelle di AutoCAD con Arial.
