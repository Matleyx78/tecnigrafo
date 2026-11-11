# F6: stato del lavoro

Stato del pacchetto [F6](F6-aspetto-dell-interfaccia.md) (#658), aggiornato a
ogni sessione. Le aree 1–10 sono quelle dell'[ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md),
punto 1.

## Come si guarda

Sul PC del responsabile, dopo ogni giro (fuori dal repository, in `build/`):

- `build/galleria/`: le immagini della galleria;
- `build/riferimento/`: le immagini di `docs/adr/0039/` in PNG;
- `build/confronto/`: riferimento sopra e galleria sotto, una per tema;
- `build/crop/`: i ritagli usati per confrontare un'area.

## Regola per tutte le aree: le traduzioni

L'interfaccia sarà tradotta (tappa 0.9, con gli strumenti di Qt; scelta del
responsabile, 2026-11-11). Ogni testo nuovo passa da `tr()`, e nessuna misura
si fissa sulla lunghezza del testo inglese: pulsanti, colonne, combo ed
etichette devono reggere testi più lunghi (l'italiano lo è spesso di un
terzo), allargandosi o accorciando il testo con i puntini, mai tagliandolo.

## Aree

| Area | Stato |
|---|---|
| Superfici e colori di tutte le aree | approvata dal responsabile, 2026-11-11 |
| 1. Barra del titolo e barra di accesso rapido | approvata dal responsabile, 2026-11-11, come inizio: da rifinire con l'uso |
| 2. Schede del ribbon | approvata dal responsabile, 2026-11-11, come inizio: da rifinire con l'uso |
| 3. Ribbon a gruppi | approvata dal responsabile, 2026-11-11, come inizio: da rifinire con l'uso |
| 4. Riga delle schede dei documenti | da fare |
| 5. Area di disegno e barra di navigazione | da fare |
| 6. Riga di comando mobile | in corso |
| 7. Immissione dinamica | da fare |
| 8. Pannello *Properties* | da fare |
| 9. Pannello *Layers* | da fare |
| 10. Barra di stato | da fare |
| Interfaccia classica (menu e barre) | da fare |
| Finestre di dialogo | da fare |
| Icone | da fare |

## Prima lettura: differenze dal riferimento

Confronto del 2026-11-11 fra la galleria (`offscreen`, 1600×1000) e
`finestra-scuro.svg`/`finestra-chiaro.svg`, più i dettagli. La scena della
galleria è diversa (nessun `MOVE` in corso, un solo layer, un'altra
polilinea) e il font è un altro: si confrontano superfici, misure e
disposizione. I colori delle superfici già giusti: `surface.chrome` (titolo,
schede del ribbon), `surface.ribbon`, `surface.group`, `surface.canvas`,
`surface.field` delle combo, `selection`.

### 1. Barra del titolo

- Manca l'icona del programma a sinistra.
- La barra di accesso rapido sta in un riquadro proprio (fondo più chiaro,
  bordo, ~165×26 px) invece che direttamente sulla fascia `surface.chrome`;
  icone più piccole e più fitte del riferimento (16 px, passo ~25 px),
  manca la freccia del menu dopo *Redo*.
- I tre pulsanti della finestra hanno un separatore verticale a sinistra che il
  riferimento non ha; nel riferimento sono più distanziati (passo ~46 px).

### 2. Schede del ribbon

- *File* giusta nel colore; la scheda corrente (*Home*) nel riferimento ha il
  bordo superiore d'accento e il fondo `surface.ribbon` che si unisce al
  ribbon; nella galleria il bordo superiore non si vede e l'altezza della
  fascia differisce (schede ~26 px contro ~28 px).
- *View* nella galleria è nel testo secondario, nel riferimento nel testo
  normale.

### 3. Ribbon a gruppi

- Altezza: galleria ~85 px di pulsanti più ~16 px di fascia del nome
  (y 54–138); riferimento y 58–154, pulsanti più alti e fascia del nome
  ~22 px.
- Pulsanti grandi: icona 32 px nel riferimento, più piccola nella galleria
  (~24 px); testo più in basso nel riferimento.
- Gruppi: nel riferimento *Draw* (Line, Polyline, Circle, Arc), *Modify* in
  colonne da tre con le frecce delle varianti (Trim, Fillet), *Annotation*
  (Text), *Layers* (Layer Properties grande, combo con icone di stato e
  campione, Make Current), *Properties* (Properties grande, tre combo con icona
  a sinistra), *Utilities* (Quick Select grande). Nella galleria: *Text* in
  *Draw*, *Undo/Redo* in *Modify*, *Quick Select* piccolo in *Modify*,
  pulsanti testuali *Delete Layer*, *Layers Panel*, *New Layer*, *Properties
  Panel*; mancano i gruppi *Annotation* e *Utilities*. Si dispongono come
  nel riferimento (sotto, "Decisioni del responsabile").
- La combo dei layer nel riferimento mostra le icone di stato colorate e il
  campione di colore piccolo; nella galleria il campione è un grande
  rettangolo bianco e le icone sono grigie e ripetute fuori dalla combo.
- Le tre combo di *Properties* non entrano: la terza esce dal bordo e appare
  la freccia di scorrimento del ribbon a destra.
- Separatori fra gruppi: nel riferimento sottili (`border`) e non a tutta
  altezza; nella galleria linee chiare a tutta altezza.
- Manca la freccia del gruppo esteso (↘) in *Layers* e *Properties*.

### 4. Riga delle schede dei documenti

- Fondo: galleria `surface.ribbon` (#2b3038), riferimento `surface.tabs`
  (#23272d), con un bordo `border` sopra.
- Scheda attiva: nel riferimento ha il fondo `surface.canvas` e si unisce
  all'area di disegno; nella galleria ha un bordo d'accento in basso.
- Crocetta: nel riferimento una × grigia, nella galleria un quadrato rosso.
- *Start* nel riferimento ha l'icona della casa; manca il `+` al posto
  dell'icona attuale; il pulsante del menu a sinistra è un'icona diversa
  (≡ nel riferimento).
- Indicatore di modifica (`*`) nel riferimento fra nome e crocetta.

### 5. Area di disegno

- Manca la barra di navigazione verticale in alto a destra (sposta, zoom,
  zoom estensioni) su fondo arrotondato. **Da verificare** se le azioni
  esistono già (sarebbe solo aspetto) o se servirebbe una funzione nuova
  (fuori perimetro).
- Gli assi dell'origine nel riferimento sono un simbolo in basso a sinistra
  (Y verde, X rossa, con le lettere); nella galleria è una linea verde a tutta
  altezza (disegno di `render`: fuori perimetro, solo da annotare).
- Griglia e colori delle entità sono di `render::SceneStyle`: non si toccano.

### 6. Riga di comando

- Nella galleria è una riga fissa a tutta larghezza in basso (etichetta
  `Command:` e un campo), con sopra una fascia vuota `surface.field` di
  ~50 px (la cronologia?) che taglia l'area di disegno.
- Nel riferimento è un riquadro **mobile** sopra il fondo dell'area di
  disegno (x 200–1076, ~100 px di altezza), fondo `surface.panel`, bordo
  `border`, angoli arrotondati: tre righe di cronologia a passo fisso, sotto
  la riga di immissione (`surface.field`) con crocetta, `>`, nome del comando,
  richiesta, opzioni in `accent`. Testo a passo fisso.

### 7. Immissione dinamica

- Non è nella galleria (nessun comando in corso). Da aggiungere alla galleria
  un comando in corso, come permette la scheda, per confrontarla con
  `immissione-dinamica.svg`.

### 8. Pannello *Properties*

- Ordine dei pannelli: nel riferimento *Properties* sopra e *Layers* sotto;
  nella galleria il contrario.
- Intestazione: nel riferimento solo il titolo e la ×; nella galleria anche
  il pulsante per fissare (📌). Fondo dell'intestazione giusto
  (`surface.ribbon`).
- Fondo del pannello: nel riferimento `surface.panel` (#2b3038) senza
  cornice; nella galleria `surface.field` (#20242a) con un riquadro bordato
  intorno a combo e griglia e un margine di ~6 px.
- Combo della selezione: nel riferimento «Polyline (1)», nella galleria
  «Polyline» (testo: comportamento, da verificare se è solo formato).
- Griglia: righe da 21 px nel riferimento, ~22 px nella galleria (va bene);
  colonna delle etichette più stretta nel riferimento (~120 px contro
  ~125); gruppi *General*/*Geometry* su `surface.group` nel riferimento,
  nella galleria quasi uguali al fondo; il campione del colore nel
  riferimento è un quadratino, nella galleria una barra larga.
- Frecce di *Current Vertex*: nella galleria la freccia sinistra è quasi
  invisibile.

### 9. Pannello *Layers*

- Tabella: nel riferimento colonne a icone **senza intestazione testuale**
  (le icone stanno nella riga), poi *Name*, *Color* con campione e nome del
  colore, *Linetype*; nessuna griglia di celle, nessuna barra di scorrimento
  orizzontale. Nella galleria: colonne strette con testi tagliati
  (`.inetype`, `neweig`), combo dentro le celle (`V`, `C`, `D`), colonna
  *Plot* con casella, intestazioni con icone e bordi di cella.
- La galleria ha colonne in più (*Lineweight*, *Plot*) del riferimento:
  restano (sotto, "Decisioni del responsabile").
- Barra di icone: nel riferimento tre icone di layer (nuovo, elimina, rendi
  corrente) a 16 px; nella galleria tre rombi arancioni.
- Campo di ricerca con l'icona della lente nel riferimento.

### 10. Barra di stato

- Fondo: galleria `surface.ribbon` (#2b3038), riferimento `surface.chrome`
  (#1e2228).
- Coordinate: nel riferimento `x, y, z` a passo fisso, dopo uno spazio a
  sinistra (~150 px) per *Model*/*Layout*; nella galleria `X: … Y: …` a
  sinistra.
- Indicatore dell'ambiente: nel riferimento icona e «Drawing» senza
  freccia; nella galleria con una freccia ⌄.
- Interruttori: nel riferimento icone 16 px a passo ~28 px, accesi su
  `selection` senza bordo; nella galleria accesi con un bordo d'accento.
- Scala: nel riferimento `1:1`, nella galleria `7.986 px/unit` (testo:
  comportamento, da verificare). Impostazioni: ingranaggio contro icona dei
  cursori.
- Maniglia di ridimensionamento in basso a destra nella galleria, assente nel
  riferimento.

### Tema chiaro

Stesse differenze; in più la riga di comando e la fascia sopra sono bianche
a tutta larghezza.

### Interfaccia classica

- La barra di accesso rapido nella fascia del titolo è ripetuta prima dei
  menu; due righe di barre con molti pulsanti, *Recent Files* e *Quit* come
  testo. Da riprendere dopo il ribbon, con gli stessi gettoni e densità.

### Finestre di dialogo e icone

- Le finestre di dialogo non sono nella galleria: da aggiungere se il
  responsabile vuole confrontarle.
- Icone: da valutare area per area (ICONE.md, ADR 0040).

## Area in corso

**Aree 1–3: barra del titolo, schede e ribbon** (2026-11-11), approvate come inizio; i punti aperti qui sotto restano per i giri successivi.

Fatto nel giro del 2026-11-11, da far vedere al responsabile:

- gruppi di *Home* come il riferimento: *Draw*, *Modify*, *Annotation*
  (*Text*), *Layers*, *Properties*, *Utilities* (*Quick Select*); *Undo* e
  *Redo* solo nella barra di accesso rapido e nei menu;
- pulsanti grandi (icona 32 px) in *Draw*, *Annotation*, *Utilities* e per
  *Layers Panel* e *Properties Panel*, primi nel loro gruppo; pulsanti piccoli
  da 16 px;
- *Modify* in colonne: Move/Copy/Rotate, Mirror/Scale/Offset,
  Trim/Extend/Join, Fillet/Chamfer/Explode, Erase;
- le tre combo di *Properties* una sotto l'altra, ognuna con la sua icona;
- pagina del ribbon di 96 px con la fascia dei nomi di 22 px e i nomi a
  11 px (SARibbon li faceva all'80% della fascia);
- scheda corrente con il bordo superiore d'accento, le altre nel testo
  normale;
- tolta la fascia chiara a sinistra dei pulsanti della finestra (la barra
  vuota dei pulsanti aggiuntivi di SARibbon).

Secondo giro (2026-11-11), sulle risposte del responsabile:

- *Layers* come il riferimento: sotto la combo *New* e *Delete* solo icona
  (il nome nel suggerimento), un separatore e *Make Current* con il testo;
  gli interruttori dello stato del layer corrente restano a destra della
  combo, perché il riferimento non ha dove metterli e toglierli cambierebbe
  il comportamento;
- *Properties*: combo larghe 170 px come nel riferimento (prima stavano sul
  contenuto e il gruppo sembrava sparire), 6 px tra icona e combo;
- freccia ↘ all'angolo di *Layers* e *Properties*: apre il pannello, come il
  pulsante grande; icona nuova `ui-panel-launcher` (16 e 32 px, ruolo
  `esist`), centrata nel quadrato che SARibbon dà al pulsante.

Punti aperti dell'area:

- logo del programma: fatto, da guardare nella finestra vera (sotto,
  decisioni);
- testi dei pulsanti grandi: restano *Layers Panel* e *Properties Panel*
  (decisione del responsabile);
- menu della barra di accesso rapido dopo *Redo*: da decidere;
- interfaccia classica: *Text* e *Quick Select* stanno ora in due barre
  proprie, *Annotation* e *Utilities* (da riprendere con l'area classica);
- le icone piccole dei layer sono meno curate di quelle del riferimento
  (giudizio del responsabile): da riprendere con l'area delle icone.

**Area 6: riga di comando mobile** (2026-11-11), da far vedere al
responsabile:

- riquadro sopra il fondo del disegno, al centro, largo il 68% del disegno
  (876 su 1276 px nel riferimento, almeno 560 px), a 12 px dal fondo: fondo
  `surface.panel`, bordo `border`, angoli arrotondati; è fratello della vista
  e non figlio, così l'immagine della vista (`grab()`, lo screenshot degli
  script) mostra solo il disegno;
- tre righe di cronologia a passo fisso nel colore del testo secondario, le
  opzioni tra parentesi quadre nel colore d'accento;
- riga di immissione su `surface.field`: crocetta che annulla come Esc
  (icona nuova `ui-cancel`, suggerimento «Cancel (Esc)» tradotto con
  `tr()`), `>` nel colore d'accento, richiesta e campo;
- la galleria cattura la finestra con `MOVE` in corso, dopo i pannelli (che
  `MOVE` lascia senza selezione).

Punti aperti dell'area 6:

- il riquadro non si sposta col mouse: «mobile» dell'ADR letto come
  «sopra il disegno»; il responsabile non lo vuole trascinabile, per ora;
- le opzioni nella riga della richiesta restano nel colore del testo: il
  testo della richiesta è un'etichetta semplice, che i test e l'eco nella
  cronologia leggono;
- la cronologia con meno di tre righe sta in alto, non vicino alla riga di
  immissione come nel riferimento;
- la riga di `ui-cancel` è in `docs/ICONE.md`, con quella di `ui-panel-launcher`.

Sulla prova del responsabile nella finestra vera (2026-11-11):

- font: il ribbon, *File* e i titoli della pagina *Start* tornavano in
  Cantarell 11. Qt tiene anche un font per classe di widget, che
  `setStyle` e il cambio di tema tardivo di GNOME riempiono con quelli del
  desktop; ora il font dell'interfaccia si mette dopo lo stile, prima di
  costruire la finestra e a ogni cambio di tema del desktop. La galleria
  (`offscreen`) non lo mostrava: si guarda la finestra vera;
- puntatore sul disegno (area 5): non più la croce del tema del desktop ma
  una croce sottile con il quadratino della selezione al centro (3 px per
  lato, come la tolleranza di selezione), bianca sul fondo scuro e nera su
  quello chiaro; braccia di 27 px fuori dal quadratino (il responsabile le
  ha chieste lunghe il triplo dei primi 9 px);
- font a 10 pt (13 px) invece di 9 pt (12 px): nella finestra vera il
  responsabile lo trovava un po' piccolo. Da guardare: *Properties Panel*
  va su due righe, e nelle intestazioni di *Layers* *Linetype* e
  *Lineweight* sono tagliate;
- ribbon: aprendo un disegno dalla pagina *Start* la scheda scelta era
  *Home* ma la pagina sotto restava quella di *View*. Con tutte le schede
  nascoste, SARibbon rende corrente la prima scheda rimessa prima di
  legarla alla sua pagina; ora la pagina si riallinea alla scheda scelta.

Terzo giro (2026-11-11), sulla prova del responsabile: font a 10 pt
approvato, commit fatto. Con la cornice di sistema il logo stava fra la
barra di accesso rapido e *File* (lo stile compatto di SARibbon dispone prima
il widget d'angolo, poi la sua icona): ora l'icona di SARibbon è nascosta e il
logo è il primo elemento della barra di accesso rapido d'angolo
(`programLogo`), da far vedere al responsabile. A fine sessione la modifica
(`app/src/ui_builder.cpp`) è nel ramo locale senza commit: si fa il commit
quando il responsabile la approva.

Prossima sessione: far vedere il logo con la cornice di sistema, poi le aree
rimaste (8–9 pannelli, 10 barra di stato, 4 dettagli delle schede dei
documenti, 5 barra di navigazione se le azioni esistono, 7 immissione
dinamica, interfaccia classica, finestre di dialogo e icone).

Test da allineare alla chiusura (oltre a quelli del ribbon):
`UiBuilderTest.WithTheFrameOfTheSystem...` (due test) confrontano le azioni
della barra d'angolo, che ora comincia con il logo;
`DrawingAidsTest.F10AndF8...` cerca la cronologia nel genitore del campo;
`EnvironmentsTest.TheChosenRibbonTabStaysWhileItIsShown` usa *Home*, che
nell'ambiente di prova ora si nasconde perché *Undo*/*Redo* non sono più in
*Modify*.

## Decisioni del responsabile

- 2026-11-11, ribbon (area 3): i comandi si dispongono come nel riferimento,
  con i gruppi *Annotation* e *Utilities*, *Undo*/*Redo* fuori da *Modify*,
  *Layer Properties* e *Properties* come pulsanti grandi; azioni e
  `objectName` restano.
- 2026-11-11, *Layers* (area 9): le colonne *Lineweight* e *Plot*, che il
  riferimento non ha, restano.
- 2026-11-11, font (tutte le aree): Tecnigrafo fissa il font
  dell'interfaccia, Segoe UI, altrimenti Noto Sans, altrimenti DejaVu Sans, a
  9 pt (i 12 px del riferimento al 100%), e quello a passo fisso della riga
  di comando, Cascadia Mono, altrimenti DejaVu Sans Mono; non segue il font del
  desktop. Sotto GNOME il font del sistema era Cantarell 11 pt (15 px), che
  faceva sembrare il testo in grassetto. La galleria non forza più Liberation
  Sans a 13 px e mostra il font della finestra vera.
- 2026-11-11, barra di navigazione (area 5): si fa, se le azioni esistono già;
  altrimenti si chiede al responsabile.
- 2026-11-11: testi dei pulsanti grandi *Layers Panel* e *Properties
  Panel*, non quelli del riferimento; gruppo *Layers* come il riferimento;
  frecce ↘ dei gruppi dove servono (aprono il pannello).
- 2026-11-11, riga di comando (area 6): sopra il disegno, ferma in basso
  al centro, per ora non trascinabile; riga di `ui-cancel` in
  `docs/ICONE.md` permessa.
- 2026-11-11, logo (area 1): il logo del responsabile (repository
  tec-ss-grafica, `icon/program/gemini-svg.svg`) senza la versione, in
  `resources/logo/tecnigrafo.svg` da 48 px in su; a 16 e 32 px una versione
  semplificata (stessi colori, la T più spessa, senza griglia e quote), per
  ora. Sta in `resources/logo/`, non in `resources/icons/` che ammette solo
  icone a ruoli; il responsabile ha permesso la cartella e la riga di
  `ui-panel-launcher` in `docs/ICONE.md`.

## Scostamenti dal riferimento decisi

- *Layers*: le colonne *Lineweight* e *Plot* restano (sopra).
- Ribbon: i pulsanti grandi si chiamano *Layers Panel* e *Properties Panel*
  (sopra).
- Ribbon, gruppo *Layers*: gli interruttori acceso, congelato, bloccato del
  layer corrente restano a destra della combo, che il riferimento non ha.

## Cose rimandate

Nessuna.
