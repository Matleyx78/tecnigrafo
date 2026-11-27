# Icone

Le icone di Tecnigrafo sono **nostre**: disegnate per il progetto in un unico
stile, in SVG, con la licenza del progetto, senza forme copiate da altri
programmi o raccolte, nemmeno ridisegnate ([ADR 0009](adr/0009-interfaccia-grafica.md),
punto 4). Seguono l'[ADR 0040](adr/0040-icone-a-ruoli.md): ogni elemento ha un
**ruolo**, che il tema colora, e ogni icona è disegnata su **due griglie**,
16 e 32 px, con un **tratto da 1 px allineato ai pixel** (decisione del
2026-11-10 in [DECISIONI.md](DECISIONI.md), che sostituisce il tratto
dell'ADR 0040, punto 3). Questo documento dà le regole, il modo in cui si
colorano, un esempio completo, l'elenco delle icone e i passi per aggiungerne
una.

I file stanno in [`resources/icons/`](../resources/icons/). Li controllano:

- `scripts/icone.py` (nel lavoro «Scripts (Python)» della CI, con i suoi test
  `scripts/icone_test.py`): le regole di questo documento, l'allineamento ai
  pixel e la geometria dichiarata, con file e riga di ogni problema;
- `tests/app/icons_test.cpp`: come Qt le disegna (dimensioni, margine,
  colori dei ruoli, scelta della griglia, misura e densità, righe di pixel
  pieni) e che ogni azione abbia la sua icona.

```sh
scripts/icone.py                   # tutte le icone di resources/icons/
scripts/icone.py <cartella>        # le icone di un'altra cartella
```

## Ruoli

Ogni elemento disegnato ha **uno e un solo** ruolo, scritto come classe
sull'elemento o sul gruppo `g` che lo contiene (l'elemento eredita il ruolo
del gruppo più vicino che ne ha uno):

| Classe | Ruolo | Esempio | Gettone del tema |
|---|---|---|---|
| `geo` | geometria che il comando crea | la circonferenza di `CIRCLE` | `iconGeo` |
| `pt` | punti che l'utente indica; l'elemento attivo di un'icona senza geometria | centro, estremi, punto per cui passa | `iconPt` |
| `cost` | costruzione: raggi, assi, direzioni, tratteggi | il raggio di *Center, Radius* | `iconCost` |
| `esist` | oggetti già nel disegno su cui il comando lavora; tutto il resto nelle icone senza geometria | la linea da tagliare in `TRIM`; un foglio, una cartella | `iconEsist`, il colore del testo |

I colori sono quelli dei gettoni di `ThemeTokens` (`app/src/theme.hpp`), con i
valori dell'ADR 0040, punto 1: blu la geometria, arancio i punti, grigio la
costruzione, il colore del testo gli oggetti esistenti. Le icone che non
disegnano geometria (file, annulla, pannelli, impostazioni) usano `esist` e,
per l'eventuale elemento attivo, `pt`, che prende il posto della vecchia
classe `accent`.

## Due griglie

| Griglia | Dove | File | Tratto | Linee orizzontali e verticali | Margine | Punti (raggio) | Tratteggio, ogni trattino e vuoto almeno |
|---|---|---|---|---|---|---|---|
| 32 px | pulsanti grandi del ribbon, pagina iniziale | `<nome>-32.svg` | 1 | mezzeria in `n + 0,5` | 2 px | 2,5 | 2 |
| 16 px | menu, pulsanti piccoli, barre, barra di stato, pannelli | `<nome>-16.svg` | 1 | mezzeria in `n + 0,5` | 1 px | 1,5 | 2 |

- La griglia da 16 si **disegna a parte**, non si rimpicciolisce la 32:
  meno dettagli, punti di 3 px di diametro, tratteggi che restano visibili.
  Il tratteggio minimo è il doppio del tratto perché le estremità arrotondate
  allungano ogni trattino di mezzo tratto per parte.
- La radice dice la griglia: `width="32" height="32" viewBox="0 0 32 32"` (o
  16). Nel **margine** non c'è inchiostro, tratto compreso: una linea di
  mezzeria sta almeno a margine più mezzo tratto dal bordo.
- Le dimensioni **logiche** fino a 24 px si disegnano dal file `-16`, le altre
  dal `-32`; se un'icona ha uno solo dei due file, quello vale per tutte. Ogni
  dimensione si disegna alla sua misura in pixel del dispositivo, misura per
  rapporto di densità: a 200% un'icona da 16 si disegna dal file `-16` a 32
  pixel, con il tratto da 2, e mai riscalando l'immagine di un'altra misura.
  Nel catalogo ogni icona ha entrambe le griglie, tranne `layer-check`,
  `ui-pin` e `ui-unpin`, usate solo nei pannelli a 16 px.
- La griglia da 24 px non è più usata: P22 ha migrato le ultime icone e tolto
  il ripiego su `<nome>.svg`. Il controllo Python e il test Qt rifiutano file senza suffisso 16/32 e
  SVG con dimensioni o `viewBox` da 24 px.

## Regole di disegno

- **Elementi ammessi.** `svg`, `g`, `path`, `circle`, `line`, `rect`,
  `polyline`, `polygon`. Gli altri (`text`, `image`, `use`, `ellipse`,
  gradienti, filtri) non si colorano in modo prevedibile o non si possono
  controllare.
- **Niente colori nel file.** `fill` e `stroke` valgono solo `none` o
  `currentColor`; niente attributi `style`, `color` né `transform` (la
  geometria si scrive in coordinate della griglia, così il controllo la legge
  com'è). Ogni elemento disegnato ha `fill` e `stroke` decisi, su di sé o su un
  gruppo: lasciati al valore predefinito sarebbero neri in ogni tema.
- **Tratto.** 1 px nelle due griglie, estremità e giunzioni arrotondate
  (`stroke-linecap="round"`, `stroke-linejoin="round"`). Di solito tutto
  questo sta sul gruppo `g` che contiene il disegno.
- **Allineamento ai pixel.** Le regole qui sotto, entro 0,01 px, fanno sì che
  il tratto da 1 px copra righe e colonne intere di pixel, senza sfumature,
  a 100% e a ogni scala intera:
  - ogni segmento orizzontale o verticale con tratto (`line`, `polyline`,
    `polygon`, `rect`, i tratti di un `path`) ha la mezzeria su un mezzo
    pixel, `n + 0,5`: la coordinata che non cambia lungo il segmento;
  - un cerchio o un arco con tratto ha centro più e meno raggio su un mezzo
    pixel, in x e in y: centro su un mezzo pixel e raggio intero, o centro
    su un pixel intero e raggio con il mezzo;
  - un punto ha il centro in `n + 0,5` e il raggio della tabella, così i suoi
    bordi stanno su pixel interi e il punto sta sulla mezzeria della linea su
    cui si trova;
  - un riempimento senza tratto ha i lati orizzontali e verticali su pixel
    interi;
  - diagonali e curve restano sfumate: non si allineano.

  Lo spostamento di mezzo pixel si sceglie verso l'interno, perché il margine
  resti libero. Un cerchio con un punto al centro (`data-center-of`) ha il
  centro su un mezzo pixel e il raggio intero; i punti che ci stanno sopra
  sono quelli a distanza intera, come (8, −6) da un centro con raggio 10 o i
  quattro quadranti. Un cerchio senza punto al centro può stare al centro
  della griglia (16, 16 o 8, 8) con un raggio che finisce in 0,5, e allora un
  punto in `n + 0,5` accanto a un quadrante gli sta sopra entro 0,1 px.
- **Riempimenti.** Solo i **punti**: cerchi pieni (`fill="currentColor"`,
  `stroke="none"`) del raggio della griglia, di classe `pt`.
- **Ruolo.** Uno per elemento, come detto sopra; un elemento senza ruolo è un
  errore del controllo.
- **Niente testo.** Né elementi `<text>` né lettere disegnate.
- **Nome del file.** Il campo `icon` dell'azione, minuscolo con i trattini
  (`draw.line` ha `draw-line`), seguito dalla griglia: `draw-line-32.svg`,
  `draw-line-16.svg`. Le varianti di un comando hanno un'icona ciascuna
  (`draw-circle-cr` per *Center, Radius*).
- **Licenza.** GPL-3.0-or-later, come il progetto: dopo la dichiarazione XML
  vanno i due commenti con l'intestazione, come in ogni file nuovo.

## Geometria dichiarata

Un'icona di un comando di disegno deve essere **giusta**: un punto che l'icona
mostra sulla circonferenza ci sta davvero. Lo dichiarano due attributi, solo su
un **punto** (un cerchio pieno di classe `pt`), e `scripts/icone.py` li
verifica entro **0,1 px**:

| Attributo | Significa che il centro del punto |
|---|---|
| `data-on="<id>"` | sta sull'elemento con quell'`id`: segmento (`line`), cerchio, rettangolo, `polyline`, `polygon`, o un tratto qualunque di un `path`, segmento, arco o curva |
| `data-center-of="<id>"` | è il centro del cerchio con quell'`id`, o di ogni arco del `path` con quell'`id` |

- L'`id` citato deve esistere, e ogni `id` è unico nel file.
- Un arco di un `path` si legge come lo disegna SVG (comando `A`): se il
  raggio scritto è troppo piccolo per i due estremi, SVG lo ingrandisce e il
  centro si sposta; il controllo usa il centro vero, quello che si vede.
- Un punto su un arco deve stare **dentro** l'arco, non solo sul suo cerchio.
- Si dichiarano tutti i punti che hanno un rapporto con la geometria: il
  centro, gli estremi, il punto per cui passa un arco, il punto che dà il
  raggio. Un punto senza dichiarazioni non si controlla.

## Come si colorano

`themedIcon(nome, tema)` (`app/src/theme.cpp`) sceglie i file (`<nome>-16.svg`
e `<nome>-32.svg` dalle risorse `:/tg/icons/`) e, prima di disegnarli, ne
**riscrive il testo**:

1. legge il file con `QXmlStreamReader` e lo riscrive con `QXmlStreamWriter`,
   elemento per elemento;
2. per ogni elemento trova il ruolo (la sua classe, o quella del gruppo più
   vicino che ne ha una; nessuna è `esist`, `accent` è `pt`) e il `fill` e lo
   `stroke` che gli arrivano, suoi o ereditati;
3. scrive sull'elemento stesso il `fill` e lo `stroke` risolti, con il colore
   del gettone del ruolo al posto di `currentColor`. Così un elemento di un
   ruolo dentro un gruppo di un altro ha il colore suo, non quello del gruppo;
4. mette i due testi riscritti, ciascuno nel suo `QSvgRenderer`, in un
   motore di icona (`QIconEngine`, non esposto): a ogni richiesta il motore
   disegna l'icona alla misura in pixel del dispositivo, misura per rapporto
   di densità, dal file della griglia che la misura logica sceglie, e
   restituisce un'immagine con quel rapporto; le immagini disegnate si
   conservano per misura e densità (0.7.5-P40).

Il meccanismo usa solo Qt Widgets e Qt SVG, che il programma ha già, e non
dipende da come Qt SVG tratta i fogli di stile o `currentColor`: il risultato
è lo stesso sui tre sistemi. Cambiando tema, chi mostra le icone le chiede di
nuovo con il tema nuovo; i pannelli che conoscono solo la propria tavolozza
ricavano il tema con `themeOf(palette)`.

## Esempio completo: `draw-circle-cr`

La variante *Center, Radius* di `CIRCLE`: la circonferenza che il comando crea
(`geo`), il raggio di costruzione (`cost`), il centro e il punto che dà il
raggio (`pt`). Il centro dichiara di essere il centro della circonferenza, il
punto dichiara di starci sopra. Il centro è su un mezzo pixel, quindi il
raggio è intero, 10, e il punto è a (8, −6) dal centro, proprio sulla
circonferenza.

`resources/icons/draw-circle-cr-32.svg`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 The Tecnigrafo authors -->
<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 32 32">
    <g fill="none" stroke="currentColor" stroke-width="1" stroke-linecap="round" stroke-linejoin="round">
        <circle id="circle" class="geo" cx="15.5" cy="15.5" r="10"/>
        <line class="cost" x1="15.5" y1="15.5" x2="23.5" y2="9.5"/>
        <circle class="pt" cx="15.5" cy="15.5" r="2.5" fill="currentColor" stroke="none" data-center-of="circle"/>
        <circle class="pt" cx="23.5" cy="9.5" r="2.5" fill="currentColor" stroke="none" data-on="circle"/>
    </g>
</svg>
```

`resources/icons/draw-circle-cr-16.svg`, disegnata a parte (raggio 5, punti
di raggio 1,5, il punto a (4, −3) dal centro):

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 The Tecnigrafo authors -->
<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16">
    <g fill="none" stroke="currentColor" stroke-width="1" stroke-linecap="round" stroke-linejoin="round">
        <circle id="circle" class="geo" cx="7.5" cy="7.5" r="5"/>
        <line class="cost" x1="7.5" y1="7.5" x2="11.5" y2="4.5"/>
        <circle class="pt" cx="7.5" cy="7.5" r="1.5" fill="currentColor" stroke="none" data-center-of="circle"/>
        <circle class="pt" cx="11.5" cy="4.5" r="1.5" fill="currentColor" stroke="none" data-on="circle"/>
    </g>
</svg>
```

Se il punto sulla circonferenza fosse a `cx="23.5" cy="10.5"`,
`scripts/icone.py` risponderebbe con la riga del punto:
`draw-circle-cr-32.svg:9: the point (23.5, 10.5) is 0.566 px from 'circle', which it is declared on (tolerance 0.1)`.
Con un raggio di 10,5 il centro su un mezzo pixel darebbe bordi su pixel
interi, e ognuno sarebbe segnalato:
`draw-circle-cr-32.svg:6: 'circle': the circle (centre (15.5, 15.5), radius 10.5) at x=5 is not on the middle of a pixel (n + 0.5)`.

## Forme che si ripetono

Le stesse idee si disegnano sempre allo stesso modo, così le icone nuove
somigliano alle vecchie:

- **Punto pieno:** un punto indicato dall'utente, come l'estremo di un
  segmento o di un arco, il primo e l'ultimo vertice di una polilinea, il
  centro di un cerchio; di classe `pt`.
- **Raggi e assi:** segmenti `cost` dal centro, sotto i punti.
- **Frecce:** punta aperta a 90° con le braccia di circa un quarto della
  griglia; annulla e ripeti sono una l'immagine speculare dell'altra.
- **Fogli e dischetti:** contorno con un angolo tagliato in alto a destra; i
  dettagli interni (piega, finestrella, etichetta) con lo stesso tratto.
- **Forme inclinate:** a 45° esatti.
- **Forme simmetriche:** su una griglia pari una linea da 1 px non sta nel
  mezzo esatto; si sposta di mezzo pixel, sempre dalla stessa parte nelle
  icone della stessa famiglia (i due lucchetti).

## Le icone

Tutte le icone seguono le regole a ruoli, con i file `-16` e `-32`.
0.7.5-P40 le ridisegna tutte con il tratto da 1 px su mezzo pixel, senza
cambiare forma, ruoli, `id` e `data-on`: le coordinate si spostano di mezzo
pixel verso l'interno, i punti diventano più grandi (raggio 2,5 e 1,5), i
cerchi e gli archi con un punto sopra cambiano di poco raggio o centro perché
i punti stiano sui mezzi pixel.
La spunta `layer-check` e le puntine `ui-pin` e `ui-unpin` sono usate
solo nei pannelli e hanno soltanto il file `-16`. La colonna
**Rifatta** registra la migrazione completata da P19–P22.

Le icone a ruoli (0.7.5-P11): l'esempio di questo documento, che 0.7.5-P19
collega alla variante *Center, Radius*; è nella tabella dei comandi di disegno
qui sotto.


P21 (0.7.5) rifà le 24 icone degli aiuti al disegno, dei file,
dell’annullamento, della vista e dell’interfaccia. Nelle loro righe `-*.svg`
indica la coppia `-16.svg` e `-32.svg`; i file da 24 px sono tolti. Gli snap
usano `esist` per l’oggetto e `pt` per il punto agganciato, con geometria
dichiarata. A 16 px si semplificano pieghe, etichette, suddivisioni, tacche e
archi accessori; annulla e ripeti restano speculari su entrambe le griglie.

P22 (0.7.5) completa la migrazione delle 15 icone di layer, proprietà e
pannelli. Usa `esist` per i simboli e `pt` per il punto corrente, il più,
la croce e la barra dell'occhio spento. A 16 px la pila ha due piani, l'occhio
barrato non ha la pupilla, il sole ha otto raggi, la scheda due righe e
la goccia perde il riflesso. I lucchetti usano un tratto per la serratura,
senza punti pieni che sembrino input. A 32 px restano i dettagli aggiuntivi.
Il lucchetto aperto a 16 px ha l’arco spostato a sinistra e staccato dal corpo.
La spunta di P13 resta invariata. Non resta nessun file sulla griglia da 24 px,
e `themedIcon` non cerca più il vecchio nome senza suffisso.

Prima parte (0.6-P18): disegno e modifica, file e annullamento.

| File | Azione | Disegno | Rifatta |
|---|---|---|---|
| `draw-line-*.svg`, `draw-pline-*.svg`, `draw-text-*.svg`, `draw-circle-*.svg`, `draw-arc-*.svg` | i comandi di disegno: rifatte da 0.7.5-P19, nella tabella della tappa 0.7.5 | | sì |
| `edit-erase-*.svg` | `edit.erase` | gomma inclinata | sì |
| `edit-undo-*.svg` | `edit.undo` | freccia a U verso sinistra; a 16 px larga quanto la griglia, la punta (`pt`) in arancio (F6) | sì |
| `edit-redo-*.svg` | `edit.redo` | freccia a U verso destra; a 16 px larga quanto la griglia, la punta (`pt`) in arancio (F6) | sì |
| `file-new-*.svg` | `file.new` | foglio con l'angolo piegato e un più; a 16 px il foglio alto quanto la griglia e il più (`pt`) fuori, in basso a destra (F6) | sì |
| `file-open-*.svg` | `file.open` | cartella aperta; a 16 px larga quanto la griglia, con il lembo aperto (`pt`) (F6) | sì |
| `file-save-*.svg` | `file.save` | dischetto; a 16 px con la finestrella e l'etichetta (`pt`) (F6) | sì |
| `file-saveas-*.svg` | `file.saveas` | due dischetti sovrapposti | sì |

Seconda parte (0.6-P19): vista, aiuti, interfaccia, layer e proprietà. Le icone
di layer e proprietà esistono prima delle azioni che le useranno (pannelli dei
layer e delle proprietà, 0.6-P22 e 0.6-P24).

| File | Azione | Disegno | Rifatta |
|---|---|---|---|
| `view-zoom-extents-*.svg` | `view.zoom-extents` | quattro angoli verso l'esterno intorno a un rettangolo | sì |
| `aids-osnap-*.svg` | `aids.osnap` | calamita a ferro di cavallo | sì |
| `aids-ortho-*.svg` | `aids.ortho` | due assi ad angolo retto con il segno dell'angolo retto | sì |
| `aids-snap-endpoint-*.svg` | `aids.snap.endpoint` | quadrato e segmento con il punto agganciato al suo estremo (`pt`, `data-on`) al centro | sì |
| `aids-snap-midpoint-*.svg` | `aids.snap.midpoint` | triangolo e segmento con il punto medio (`pt`, `data-on`) al centro | sì |
| `aids-snap-center-*.svg` | `aids.snap.center` | cerchio (`esist`) con il centro (`pt`, `data-center-of`) | sì |
| `aids-snap-insertion-*.svg` | `aids.snap.insertion` | due quadrati sovrapposti e sfalsati con il punto d’inserimento al vertice del secondo (`pt`, `data-on`) | sì |
| `aids-lwdisplay-*.svg` | `aids.lwdisplay` | tre barre di spessore crescente | sì |
| `ui-ribbon-*.svg` | `ui.interface` | finestra con la fascia delle schede e quella dei gruppi | sì |
| `ui-theme-*.svg` | `ui.theme` | cerchio diviso a metà, una metà tratteggiata | sì |
| `tools-settings-*.svg` | `tools.settings` | tre cursori a scorrimento | sì |
| `help-about-*.svg` | `help.about` | squadra da disegno con una squadra più piccola dentro | sì |
| `layer-panel-*.svg` | `layer.panel` (0.6-P22) | pila di tre piani; da F6 piani inclinati a 45°, come nel riferimento | sì |
| `layer-on-*.svg` | stato del layer, pannello dei layer (0.6-P22) | occhio aperto; a 16 px la pupilla è un punto (`pt`) (F6) | sì |
| `layer-off-*.svg` | stato del layer, pannello dei layer (0.6-P22) | occhio barrato; a 16 px l'occhio è `cost`, spento (F6) | sì |
| `layer-frozen-*.svg` | stato del layer, pannello dei layer (0.6-P22) | fiocco di neve | sì |
| `layer-thawed-*.svg` | stato del layer, pannello dei layer (0.6-P22) | sole; a 16 px il centro è `pt` e i raggi più lunghi (F6) | sì |
| `layer-locked-*.svg` | stato del layer, pannello dei layer (0.6-P22) | lucchetto chiuso; a 16 px la serratura è `pt` (F6) | sì |
| `layer-unlocked-*.svg` | stato del layer, pannello dei layer (0.6-P22) | lucchetto aperto | sì |
| `layer-current-*.svg` | `layer.current` (0.6-P22) | da F6 pila di due piani inclinati e una spunta (`pt`) in alto a destra | sì |
| `layer-new-*.svg` | `layer.new` (0.6-P22) | da F6 pila di due piani inclinati e un più (`pt`) in alto a destra | sì |
| `layer-delete-*.svg` | `layer.delete` (0.6-P22) | da F6 pila di due piani inclinati e una croce (`pt`) in alto a destra | sì |
| `properties-panel-*.svg` | `properties.panel` (0.6-P24) | scheda con un elenco di voci | sì |
| `quick-select-*.svg` | `edit.quick-select` (0.6-P24) | imbuto | sì |
| `property-color-*.svg` | controllo del colore, pannello delle proprietà (0.6-P24) | goccia | sì |
| `property-linetype-*.svg` | controllo del tipo di linea, pannello delle proprietà (0.6-P24) | linee continua, tratteggiata, tratto e punto | sì |
| `property-lineweight-*.svg` | controllo dello spessore, pannello delle proprietà (0.6-P24) | barra con la freccia dello spessore | sì |

Le azioni e i pannelli dei layer e delle proprietà usano queste icone;
i test verificano anche quelle nominate direttamente dai pannelli.
`ui.interface` ha l'icona `ui-ribbon` perché il
suo interruttore passa dai menu classici al ribbon.

La tappa 0.7 aggiunge i modi di snap e il tracciamento polare (0.7-P11).

| File | Azione | Disegno | Rifatta |
|---|---|---|---|
| `aids-snap-intersection-*.svg` | `aids.snap.intersection` | due segmenti che si incrociano a X, punto all’intersezione (`pt`, `data-on`) | sì |
| `aids-snap-perpendicular-*.svg` | `aids.snap.perpendicular` | perpendicolare con il segno dell’angolo retto, punto sul piede (`pt`, `data-on`) | sì |
| `aids-snap-tangent-*.svg` | `aids.snap.tangent` | cerchio con una linea tangente in alto, punto di tangenza (`pt`, `data-on`) | sì |
| `aids-snap-quadrant-*.svg` | `aids.snap.quadrant` | cerchio (`esist`) con punto sul quadrante destro (`pt`, `data-on`) | sì |
| `aids-polar-*.svg` | `aids.polar` | assi, semiretta a 45° e arco dell'angolo | sì |
| `aids-otrack-*.svg` | `aids.otrack` | guide orizzontale e verticale tratteggiate, con il più del punto acquisito (`pt`) | sì |

Chi aggiunge icone aggiunge le sue righe alla tabella della sua tappa; chi
rifà un'icona ne aggiorna la riga.

Il pannello Layers (0.7.5-P13) aggiunge una spunta sulla griglia da 16 px:

| File | Uso | Disegno | Rifatta |
|---|---|---|---|
| `layer-check-16.svg` | layer corrente nella tabella | spunta (`pt`) | sì, solo pannello a 16 px |

La tappa 0.7.5 aggiunge l'immissione dinamica (P16).

| File | Azione | Disegno | Rifatta |
|---|---|---|---|
| `aids-dynmode-16.svg`, `aids-dynmode-32.svg` | `aids.dynmode` | puntatore (`esist`) e campo vicino al cursore (`pt`) | sì |

La tappa 0.7.5 rifà le icone dei comandi di disegno e ne aggiunge una per
ogni variante di `CIRCLE` e `ARC` (P19). Ogni riga ha i due file, `-16.svg` e
`-32.svg`; i punti dichiarano dove stanno (`data-on`) e di che cosa sono il
centro (`data-center-of`). `draw.circle` e `draw.arc` mostrano l'icona della
variante che eseguono: la prima finché non se ne usa un'altra.

| File | Azione | Disegno | Rifatta |
|---|---|---|---|
| `draw-line` | `draw.line` | segmento (`geo`) con un punto a ogni estremo (`pt`) | sì |
| `draw-pline` | `draw.pline` | spezzata di tre tratti (`geo`), punti al primo e all'ultimo vertice (`pt`) | sì |
| `draw-text` | `draw.text` | due righe di testo (`geo`) sopra la linea di base tratteggiata (`cost`), punto d'inserimento al suo inizio (`pt`) | sì |
| `draw-circle-cr` | `draw.circle.center-radius`, *Center, Radius* (e `draw.circle` all'inizio) | circonferenza (`geo`), raggio (`cost`), centro e punto sulla circonferenza (`pt`) | sì |
| `draw-circle-cd` | `draw.circle.center-diameter`, *Center, Diameter* | circonferenza piccola (`geo`), dal centro un segmento lungo il diametro (`cost`), centro e punto in fondo al segmento (`pt`) | sì |
| `draw-circle-2p` | `draw.circle.2-point`, *2-Point* | circonferenza (`geo`), diametro orizzontale tratteggiato (`cost`), i suoi due estremi (`pt`) | sì |
| `draw-circle-3p` | `draw.circle.3-point`, *3-Point* | circonferenza (`geo`) con tre punti sopra (`pt`) | sì |
| `draw-circle-ttr` | `draw.circle.tan-tan-radius`, *Tan, Tan, Radius* | due linee ad angolo retto (`esist`), la circonferenza tangente a entrambe (`geo`), il suo raggio (`cost`), i punti di tangenza sulle linee (`pt`) | sì |
| `draw-arc-3p` | `draw.arc.3-point`, *3-Point* (e `draw.arc` all'inizio) | arco di 220° (`geo`) con i due estremi e un punto in cima (`pt`) | sì |
| `draw-arc-sce` | `draw.arc.start-center-end`, *Start, Center, End* | arco di 170° (`geo`), i due raggi agli estremi (`cost`), inizio, centro e fine (`pt`) | sì |

La tappa 0.7.5 aggiunge le icone dei dodici comandi di modifica della 0.7
(P20). Ogni riga ha i due file `-16.svg` e `-32.svg`: gli oggetti di partenza
sono `esist`, i risultati `geo`, le direzioni e le distanze `cost`, i punti
indicati `pt`. Le icone piccole semplificano i dettagli di costruzione.

| File | Azione | Disegno | Rifatta |
|---|---|---|---|
| `edit-move` | `edit.move` | rettangolo spostato, freccia di spostamento e punto base | sì |
| `edit-copy` | `edit.copy` | due rettangoli, freccia di copia e più sulla griglia grande | sì |
| `edit-rotate` | `edit.rotate` | angolo ruotato, freccia curva e punto base | sì |
| `edit-scale` | `edit.scale` | due rettangoli con lo stesso punto base e freccia di ingrandimento | sì |
| `edit-mirror` | `edit.mirror` | due triangoli speculari e asse verticale | sì |
| `edit-explode` | `edit.explode` | rettangolo e quattro lati separati, con freccia | sì |
| `edit-trim` | `edit.trim` | limite verticale (`esist`), linea intera (`esist`), porzione conservata (`geo`) e punto di selezione sulla porzione da tagliare (`pt`, `data-on`) | sì |
| `edit-extend` | `edit.extend` | segmento prolungato fino al limite e freccia verso il limite | sì |
| `edit-offset` | `edit.offset` | due angoli paralleli e distanza tra i lati | sì |
| `edit-fillet` | `edit.fillet` | angolo raccordato, raggio e centro sulla griglia grande | sì |
| `edit-chamfer` | `edit.chamfer` | angolo smussato, estremi dello smusso sulla griglia grande | sì |
| `edit-join` | `edit.join` | due segmenti e spezzata risultante, punti di giunzione | sì |

I pannelli che si nascondono da soli (0.7.5-P18) aggiungono la puntina della
barra del titolo, sulla sola griglia da 16 px.

| File | Uso | Disegno | Rifatta |
|---|---|---|---|
| `ui-pin-16.svg` | puntina di un pannello agganciato, *Auto-hide* | puntina in piedi: testa e piattello (`esist`), ago (`pt`) | sì, solo 16 px |
| `ui-unpin-16.svg` | puntina di un pannello nascosto, nel riquadro: *Dock* | la stessa puntina coricata, con l'ago verso sinistra | sì, solo 16 px |

I documenti in schede e la pagina *Start* (0.7.5-P29) aggiungono tre icone,
con i due file `-16.svg` e `-32.svg`; nessuna azione le nomina, le usa la riga
delle schede (`documentTabsIconNames()`).

| File | Uso | Disegno | Rifatta |
|---|---|---|---|
| `ui-tab-new` | pulsante `+` della riga delle schede: un disegno nuovo | un più (`pt`); prima di F6 anche il bordo di una scheda | sì |
| `ui-tab-menu` | pulsante del menu dei documenti aperti | tre righe uguali (`esist`), ≡ come nel riferimento (F6); la freccia del menu non si mostra | sì |
| `ui-home` | scheda *Start*, come nel riferimento (F6) | casa (`esist`) con la porta (`pt`) | sì |
| `doc-drawing` | tipo disegno: icona della scheda di un disegno, del pulsante *Drawing* di *Start* e dei file recenti | foglio con l'angolo piegato (`esist`) e una spezzata disegnata (`geo`) | sì |

Gli ambienti (0.7.5-P30) hanno un'icona ciascuno, nell'indicatore
dell'ambiente della barra di stato; la nomina l'ambiente
(`registerBuiltinDocumentTypes()`), con i due file `-16.svg` e `-32.svg`.

| File | Uso | Disegno | Rifatta |
|---|---|---|---|
| `env-drawing2d` | ambiente `drawing2d`, il disegno 2D: indicatore dell'ambiente | squadra (`esist`) e un segmento tracciato lungo il suo lato inclinato, a 45°, con i due estremi (`geo`, `pt`) | sì |

La tappa 0.8 aggiunge il canale e il pannello dell’assistente (PT38).

| File | Azione | Disegno | Rifatta |
|---|---|---|---|
| `assistant-channel-*.svg` | `assistant.channel` | onde del collegamento sopra il punto dell’assistente (`pt`) | sì |
| `assistant-panel-*.svg` | `assistant.panel` | fumetto con tre punti (`pt`) | sì |

Il ribbon e la riga di comando sul riferimento (F6) aggiungono la freccia
all'angolo dei gruppi che apre il pannello e la crocetta che annulla, con i
due file `-16.svg` e `-32.svg`; F6 aggiunge poi la lente della ricerca dei
layer e le icone delle finestre di avvertimento e di domanda, che `Style`
disegna al posto di quelle della piattaforma; nessuna azione le nomina: la freccia la usa
il ribbon (`launcherIcon()` in `app/src/ui_builder.cpp`), che la disegna da
16 px al centro del quadrato del pulsante, la crocetta la riga di comando.

| File | Uso | Disegno | Rifatta |
|---|---|---|---|
| `ui-panel-launcher` | freccia ↘ dei gruppi *Layers* e *Properties*: apre il pannello | diagonale verso il basso a destra con la punta ad angolo retto (`esist`) | sì |
| `ui-cancel` | crocetta della riga di comando: annulla come Esc | due diagonali a X (`esist`) | sì |
| `ui-search` | lente all'inizio del campo di ricerca del pannello *Layers* (`layerPanelIconNames()`) | cerchio e manico (`cost`), discreti come il testo d'aiuto del campo | sì |
| `ui-warning` | icona delle finestre di avvertimento, al posto di quella della piattaforma (`styleIconNames()`) | triangolo (`esist`), barra e punto (`pt`) | sì |
| `ui-question` | icona delle domande («Save changes?»), al posto di quella della piattaforma (`styleIconNames()`) | cerchio (`esist`), uncino e punto (`pt`): un segno, non una lettera | sì |

Il logo del programma non è un'icona a ruoli: ha colori suoi e sta in
[`resources/logo/`](../resources/logo/), fuori da questo controllo.

PT39 porta nell’interfaccia i primi comandi nuovi della tappa.

| File | Azione | Disegno | Rifatta |
|---|---|---|---|
| `draw-ellipse` | `draw.ellipse` | ellisse e asse maggiore | sì |
| `draw-mtext` | `draw.mtext` | righe di testo e limite sinistro | sì |
| `edit-pedit` | `edit.pedit` | polilinea esistente con vertice attivo | sì |
| `block-create` | `block.create` | due oggetti e simbolo di creazione | sì |
| `block-insert` | `block.insert` | blocco originale e copia inserita | sì |
| `text-style` | `text.style` | campioni di righe esistenti con selezione attiva | sì |
| `dimension-style` | `dimension.style` | linea di quota esistente fra due estensioni | sì |
| `dimension-linear` | `dimension.linear` | oggetto orizzontale (`esist`) con i due punti misurati (`pt`), richiami staccati (`cost`), linea di quota con le frecce sopra (`geo`) | sì |
| `dimension-aligned` | `dimension.aligned` | come `dimension-linear`, con oggetto e quota paralleli, inclinati a 45° | sì |
| `dimension-angular` | `dimension.angular` | angolo e arco centrato nel vertice | sì |
| `dimension-radius` | `dimension.radius` | cerchio e raggio quotato | sì |
| `dimension-diameter` | `dimension.diameter` | cerchio e diametro quotato | sì |

PT45 aggiunge il retino all'interfaccia.

| File | Azione | Disegno | Rifatta |
|---|---|---|---|
| `draw-hatch` | `draw.hatch` | contorno esistente e linee del retino | sì |

PT37 porta nell'interfaccia i comandi di completamento della tappa 0.8.1.

| File | Azione | Disegno | Rifatta |
|---|---|---|---|
| `block-attdef` | `block.attdef` | etichetta di attributo con punto di inserimento | sì |
| `block-attdisp` | `block.attdisp` | blocco con attributo visibile | sì |
| `block-attedit` | `block.attedit` | attributo di blocco con matita | sì |
| `block-attsync` | `block.attsync` | due attributi collegati da frecce | sì |
| `block-battman` | `block.battman` | elenco di attributi di un blocco | sì |
| `draw-polygon` | `draw.polygon` | poligono regolare con centro | sì |
| `draw-rectang` | `draw.rectang` | rettangolo con vertici opposti | sì |
| `edit-hatchedit` | `edit.hatchedit` | retino esistente con matita | sì |
| `edit-textedit` | `edit.textedit` | testo esistente con cursore | sì |
| `view-ucsicon` | `view.ucsicon` | assi X e Y dell'UCS | sì |

La barra dei formati dell'editor MTEXT (0.8.1-PT13) usa direttamente sette
icone, ciascuna con i due file `-16.svg` e `-32.svg`.

| File | Uso | Disegno | Rifatta |
|---|---|---|---|
| `format-bold` | grassetto | fila di tratti verticali doppi | sì |
| `format-italic` | corsivo | fila di tratti obliqui paralleli | sì |
| `format-underline` | sottolineato | fila di tratti con linea attiva sotto | sì |
| `format-overline` | sopralineato | fila di tratti con linea attiva sopra | sì |
| `format-strike` | barrato | fila di tratti con linea attiva in mezzo | sì |
| `format-stack` | impila frazioni | due file corte separate da una linea attiva | sì |
| `format-unstack` | disimpila frazioni | due file affiancate separate da una diagonale attiva | sì |

## Aggiungere un'icona

1. Disegnala con le regole sopra, sulle due griglie, partendo da un'icona
   simile già rifatta: prima la 32, poi la 16 a parte. Dai a ogni elemento il
   suo ruolo e dichiara la geometria di ogni punto (`data-on`,
   `data-center-of`), con gli `id` che servono.
2. Salvala in `resources/icons/<nome>-32.svg` e `<nome>-16.svg` con
   l'intestazione. I file entrano da soli nelle risorse `/tg/icons`:
   `app/CMakeLists.txt` include tutti gli `*.svg` della cartella.
3. Lancia `scripts/icone.py`: deve finire senza problemi.
4. Scrivi `<nome>` (senza griglia) nel campo `icon` dell'azione, in
   `app/src/builtin_actions.cpp`.
5. Aggiungi o aggiorna la riga nella tabella qui sopra.
6. Guardala a 16 e 32 px, nel tema chiaro e nello scuro.
7. Lancia i test dell'applicazione: `tests/app/icons_test.cpp` controlla che il
   file si disegni sulla sua griglia senza inchiostro nel margine e che ogni
   azione con un'icona abbia i suoi file.
