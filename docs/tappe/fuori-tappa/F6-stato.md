# F6: stato del lavoro

Stato del pacchetto [F6](F6-aspetto-dell-interfaccia.md) (#658), aggiornato a
ogni sessione. Le aree 1–10 sono quelle dell'[ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md),
punto 1.

## Aree

| Area | Stato |
|---|---|
| 1. Barra del titolo e barra di accesso rapido | da fare |
| 2. Schede del ribbon | da fare |
| 3. Ribbon a gruppi | da fare |
| 4. Riga delle schede dei documenti | da fare |
| 5. Area di disegno e barra di navigazione | da fare |
| 6. Riga di comando mobile | da fare |
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
  Panel*; mancano i gruppi *Annotation* e *Utilities*. **Da decidere col
  responsabile**: la disposizione dei comandi tocca solo l'aspetto (stesse
  azioni, stessi `objectName`), ma va confermata.
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
- **Da decidere col responsabile**: la galleria ha colonne in più
  (*Lineweight*, *Plot*) del riferimento; nasconderle o restringerle è
  aspetto, toglierle cambierebbe il comportamento.
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

Nessuna: l'ordine lo sceglie il responsabile.

## Scostamenti dal riferimento decisi

Nessuno.

## Cose rimandate

Nessuna.
