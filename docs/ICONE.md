# Icone

Le icone di Tecnigrafo sono **nostre**: disegnate per il progetto in un unico
stile, in SVG, con la licenza del progetto, senza forme copiate da altri
programmi o raccolte, nemmeno ridisegnate ([ADR 0009](adr/0009-interfaccia-grafica.md),
punto 4). Seguono l'[ADR 0040](adr/0040-icone-a-ruoli.md): ogni elemento ha un
**ruolo**, che il tema colora, e ogni icona è disegnata su **due griglie**,
16 e 32 px. Questo documento dà le regole, il modo in cui si colorano, un
esempio completo, l'elenco delle icone e i passi per aggiungerne una.

I file stanno in [`resources/icons/`](../resources/icons/). Li controllano:

- `scripts/icone.py` (nel lavoro «Scripts (Python)» della CI, con i suoi test
  `scripts/icone_test.py`): le regole di questo documento e la geometria
  dichiarata, con file e riga di ogni problema;
- `tests/app/icons_test.cpp`: come Qt le disegna (dimensioni, margine,
  colori dei ruoli, scelta della griglia) e che ogni azione abbia la sua icona.

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

| Griglia | Dove | File | Tratto | Margine | Punti (raggio) | Tratteggio, ogni trattino e vuoto almeno |
|---|---|---|---|---|---|---|
| 32 px | pulsanti grandi del ribbon, pagina iniziale | `<nome>-32.svg` | 1,75 | 2 px | 2 | 3,5 |
| 16 px | menu, pulsanti piccoli, barre, barra di stato, pannelli | `<nome>-16.svg` | 1,25 | 1 px | 1,25 | 2,5 |
| 24 px | le icone di oggi, finché P19–P22 non le rifanno | `<nome>.svg` | 1,5 | 2 px | 1,5 | 3 |

- La griglia da 16 si **disegna a parte**, non si rimpicciolisce la 32:
  meno dettagli, punti di almeno 2 px di diametro, tratteggi che restano
  visibili. Il tratteggio minimo è il doppio del tratto perché le estremità
  arrotondate allungano ogni trattino di mezzo tratto per parte.
- La radice dice la griglia: `width="32" height="32" viewBox="0 0 32 32"` (o
  16). Nel **margine** non c'è inchiostro, tratto compreso: una linea di
  mezzeria sta almeno a margine più mezzo tratto dal bordo.
- Le dimensioni fino a 24 px si disegnano dal file `-16`, le altre dal `-32`;
  se un'icona ha uno solo dei due file, quello vale per tutte. Le dimensioni
  in mezzo e quelle per gli schermi ad alta densità vengono così dalla griglia
  più vicina.
- La griglia da 24 px resta solo per le icone non ancora rifatte, con le sue
  regole di prima (una sola classe, `accent`); un'icona rifatta ha i due file
  nuovi e perde quello da 24. P22 toglie il ripiego.

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
- **Tratto.** Lo spessore della griglia, estremità e giunzioni arrotondate
  (`stroke-linecap="round"`, `stroke-linejoin="round"`). Di solito tutto
  questo sta sul gruppo `g` che contiene il disegno.
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
e `<nome>-32.svg` dalle risorse `:/tg/icons/`, o `<nome>.svg` se mancano
entrambi) e, prima di disegnarli, ne **riscrive il testo**:

1. legge il file con `QXmlStreamReader` e lo riscrive con `QXmlStreamWriter`,
   elemento per elemento;
2. per ogni elemento trova il ruolo (la sua classe, o quella del gruppo più
   vicino che ne ha una; nessuna è `esist`, `accent` è `pt`) e il `fill` e lo
   `stroke` che gli arrivano, suoi o ereditati;
3. scrive sull'elemento stesso il `fill` e lo `stroke` risolti, con il colore
   del gettone del ruolo al posto di `currentColor`. Così un elemento di un
   ruolo dentro un gruppo di un altro ha il colore suo, non quello del gruppo;
4. disegna il testo riscritto con `QSvgRenderer` a 16, 20 e 24 px dal file
   `-16`, a 32, 40, 48, 64, 96 e 128 px dal `-32`, e mette le immagini in un
   `QIcon`.

Il meccanismo usa solo Qt Widgets e Qt SVG, che il programma ha già, e non
dipende da come Qt SVG tratta i fogli di stile o `currentColor`: il risultato
è lo stesso sui tre sistemi. Cambiando tema, chi mostra le icone le chiede di
nuovo con il tema nuovo; i pannelli che conoscono solo la propria tavolozza
ricavano il tema con `themeOf(palette)`.

## Esempio completo: `draw-circle-cr`

La variante *Center, Radius* di `CIRCLE`: la circonferenza che il comando crea
(`geo`), il raggio di costruzione (`cost`), il centro e il punto che dà il
raggio (`pt`). Il centro dichiara di essere il centro della circonferenza, il
punto dichiara di starci sopra (a 45°: 11 · cos 45° ≈ 7,778).

`resources/icons/draw-circle-cr-32.svg`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 The Tecnigrafo authors -->
<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 32 32">
    <g fill="none" stroke="currentColor" stroke-width="1.75" stroke-linecap="round" stroke-linejoin="round">
        <circle id="circle" class="geo" cx="16" cy="16" r="11"/>
        <line class="cost" x1="16" y1="16" x2="23.778" y2="8.222"/>
        <circle class="pt" cx="16" cy="16" r="2" fill="currentColor" stroke="none" data-center-of="circle"/>
        <circle class="pt" cx="23.778" cy="8.222" r="2" fill="currentColor" stroke="none" data-on="circle"/>
    </g>
</svg>
```

`resources/icons/draw-circle-cr-16.svg`, disegnata a parte (raggio 5,5,
punti di raggio 1,25):

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 The Tecnigrafo authors -->
<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16">
    <g fill="none" stroke="currentColor" stroke-width="1.25" stroke-linecap="round" stroke-linejoin="round">
        <circle id="circle" class="geo" cx="8" cy="8" r="5.5"/>
        <line class="cost" x1="8" y1="8" x2="11.889" y2="4.111"/>
        <circle class="pt" cx="8" cy="8" r="1.25" fill="currentColor" stroke="none" data-center-of="circle"/>
        <circle class="pt" cx="11.889" cy="4.111" r="1.25" fill="currentColor" stroke="none" data-on="circle"/>
    </g>
</svg>
```

Se il punto sulla circonferenza fosse a `cx="23" cy="9"`, `scripts/icone.py`
risponderebbe con la riga del punto:
`draw-circle-cr-32.svg:9: the point (23, 9) is 1.101 px from 'circle', which it is declared on (tolerance 0.1)`.

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

## Le icone

La colonna **Rifatta** dice se l'icona segue già le regole a ruoli, con i
file `-16` e `-32`; le altre sono ancora sulla griglia da 24 px, e le rifanno
0.7.5-P19–P22.

Le icone a ruoli (0.7.5-P11): l'esempio di questo documento.

| File | Azione | Disegno | Rifatta |
|---|---|---|---|
| `draw-circle-cr-32.svg`, `draw-circle-cr-16.svg` | variante *Center, Radius* di `CIRCLE` (la collega 0.7.5-P19) | circonferenza (`geo`), raggio (`cost`), centro e punto sulla circonferenza (`pt`) | sì |


Prima parte (0.6-P18): disegno e modifica, file e annullamento.

| File | Azione | Disegno | Rifatta |
|---|---|---|---|
| `draw-line.svg` | `draw.line` | segmento con un punto a ogni estremo | no |
| `draw-pline.svg` | `draw.pline` | spezzata di tre tratti, punti agli estremi | no |
| `draw-circle.svg` | `draw.circle` | circonferenza con il suo centro | no |
| `draw-arc.svg` | `draw.arc` | arco di circa 220° con i due estremi | no |
| `draw-text.svg` | `draw.text` | cursore di testo | no |
| `edit-erase.svg` | `edit.erase` | gomma inclinata | no |
| `edit-undo.svg` | `edit.undo` | freccia a U verso sinistra | no |
| `edit-redo.svg` | `edit.redo` | freccia a U verso destra | no |
| `file-new.svg` | `file.new` | foglio con l'angolo piegato e un più | no |
| `file-open.svg` | `file.open` | cartella aperta | no |
| `file-save.svg` | `file.save` | dischetto | no |
| `file-saveas.svg` | `file.saveas` | due dischetti sovrapposti | no |

Seconda parte (0.6-P19): vista, aiuti, interfaccia, layer e proprietà. Le icone
di layer e proprietà esistono prima delle azioni che le useranno (pannelli dei
layer e delle proprietà, 0.6-P22 e 0.6-P24).

| File | Azione | Disegno | Rifatta |
|---|---|---|---|
| `view-zoom-extents.svg` | `view.zoom-extents` | quattro angoli verso l'esterno intorno a un rettangolo | no |
| `aids-osnap.svg` | `aids.osnap` | calamita a ferro di cavallo | no |
| `aids-ortho.svg` | `aids.ortho` | due assi ad angolo retto con il segno dell'angolo retto | no |
| `aids-snap-endpoint.svg` | `aids.snap.endpoint` | quadrato con il punto d'accento al centro | no |
| `aids-snap-midpoint.svg` | `aids.snap.midpoint` | triangolo con il punto d'accento al centro | no |
| `aids-snap-center.svg` | `aids.snap.center` | cerchio con il punto d'accento al centro | no |
| `aids-snap-insertion.svg` | `aids.snap.insertion` | due quadrati sovrapposti e sfalsati con il punto d'accento | no |
| `aids-lwdisplay.svg` | `aids.lwdisplay` | tre barre di spessore crescente | no |
| `ui-ribbon.svg` | `ui.interface` | finestra con la fascia delle schede e quella dei gruppi | no |
| `ui-theme.svg` | `ui.theme` | cerchio diviso a metà, una metà tratteggiata | no |
| `tools-settings.svg` | `tools.settings` | tre cursori a scorrimento | no |
| `help-about.svg` | `help.about` | squadra da disegno con una squadra più piccola dentro | no |
| `layer-panel.svg` | `layer.panel` (0.6-P22) | pila di tre piani | no |
| `layer-on.svg` | stato del layer, pannello dei layer (0.6-P22) | occhio aperto | no |
| `layer-off.svg` | stato del layer, pannello dei layer (0.6-P22) | occhio barrato | no |
| `layer-frozen.svg` | stato del layer, pannello dei layer (0.6-P22) | fiocco di neve | no |
| `layer-thawed.svg` | stato del layer, pannello dei layer (0.6-P22) | sole | no |
| `layer-locked.svg` | stato del layer, pannello dei layer (0.6-P22) | lucchetto chiuso | no |
| `layer-unlocked.svg` | stato del layer, pannello dei layer (0.6-P22) | lucchetto aperto | no |
| `layer-current.svg` | `layer.current` (0.6-P22) | pila di tre piani, il primo con il punto d'accento | no |
| `layer-new.svg` | `layer.new` (0.6-P22) | pila di tre piani, il primo con un più | no |
| `layer-delete.svg` | `layer.delete` (0.6-P22) | pila di tre piani, il primo con una croce | no |
| `properties-panel.svg` | `properties.panel` (0.6-P24) | scheda con un elenco di voci | no |
| `quick-select.svg` | `edit.quick-select` (0.6-P24) | imbuto | no |
| `property-color.svg` | controllo del colore, pannello delle proprietà (0.6-P24) | goccia | no |
| `property-linetype.svg` | controllo del tipo di linea, pannello delle proprietà (0.6-P24) | linee continua, tratteggiata, tratto e punto | no |
| `property-lineweight.svg` | controllo dello spessore, pannello delle proprietà (0.6-P24) | barra con la freccia dello spessore | no |

Le voci di layer e proprietà non hanno ancora un'azione registrata: le
collegano i pacchetti indicati, che le tolgono dall'elenco delle icone riservate
in `tests/app/icons_test.cpp`. `ui.interface` ha l'icona `ui-ribbon` perché il
suo interruttore passa dai menu classici al ribbon.

La tappa 0.7 aggiunge i modi di snap e il tracciamento polare (0.7-P11).

| File | Azione | Disegno | Rifatta |
|---|---|---|---|
| `aids-snap-intersection.svg` | `aids.snap.intersection` | due segmenti che si incrociano a X | no |
| `aids-snap-perpendicular.svg` | `aids.snap.perpendicular` | perpendicolare con il segno dell'angolo retto | no |
| `aids-snap-tangent.svg` | `aids.snap.tangent` | cerchio con una linea tangente in alto | no |
| `aids-snap-quadrant.svg` | `aids.snap.quadrant` | rombo | no |
| `aids-polar.svg` | `aids.polar` | assi, semiretta a 45° e arco dell'angolo | no |

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

## Aggiungere un'icona

1. Disegnala con le regole sopra, sulle due griglie, partendo da un'icona
   simile già rifatta: prima la 32, poi la 16 a parte. Dai a ogni elemento il
   suo ruolo e dichiara la geometria di ogni punto (`data-on`,
   `data-center-of`), con gli `id` che servono.
2. Salvala in `resources/icons/<nome>-32.svg` e `<nome>-16.svg` con
   l'intestazione. I file entrano da soli nelle risorse `/tg/icons`:
   `app/CMakeLists.txt` include tutti gli `*.svg` della cartella. Se rifai
   un'icona da 24 px, togli il file vecchio.
3. Lancia `scripts/icone.py`: deve finire senza problemi.
4. Scrivi `<nome>` (senza griglia) nel campo `icon` dell'azione, in
   `app/src/builtin_actions.cpp`.
5. Aggiungi o aggiorna la riga nella tabella qui sopra.
6. Guardala a 16 e 32 px, nel tema chiaro e nello scuro.
7. Lancia i test dell'applicazione: `tests/app/icons_test.cpp` controlla che il
   file si disegni sulla sua griglia senza inchiostro nel margine e che ogni
   azione con un'icona abbia i suoi file.
