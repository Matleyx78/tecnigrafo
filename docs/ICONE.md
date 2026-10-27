# Icone

Le icone di Tecnigrafo sono **nostre**: disegnate per il progetto in un unico
stile, in SVG, con la licenza del progetto ([ADR 0009](adr/0009-interfaccia-grafica.md),
punto 4). Questo documento dà le regole di disegno, l'elenco delle icone e i
passi per aggiungerne una. I file stanno in [`resources/icons/`](../resources/icons/);
il test che li controlla è `tests/app/icons_test.cpp`.

## Regole di disegno

- **Griglia.** 24×24 px: `width="24" height="24" viewBox="0 0 24 24"`. L'area
  utile è 20×20: nei 2 px di margine non c'è inchiostro, tratto compreso. Con
  il tratto di 1,5 px le linee di mezzeria stanno almeno a 2,75 px dal bordo.
- **Tratto.** 1,5 px, estremità e giunzioni arrotondate
  (`stroke-linecap="round"`, `stroke-linejoin="round"`).
- **Riempimenti.** Nessuno, salvo i **punti**: cerchi pieni di 3 px di diametro
  (`r="1.5"`, `fill="currentColor"`, `stroke="none"`).
- **Colore.** Uno solo, `currentColor`: lo sostituisce il tema. Il secondo
  colore, d'accento, si dà con la classe `accent` e solo a un punto agganciato
  o a un elemento attivo. Nessun colore scritto in chiaro, nessun `style`.
- **Niente testo.** Né elementi `<text>` né lettere disegnate. Niente forme
  copiate da altri programmi o raccolte, nemmeno ridisegnate a mano.
- **Elementi ammessi.** `svg`, `g`, `path`, `circle`, `line`, `rect`,
  `polyline`, `polygon`. Il test rifiuta gli altri (`text`, `image`, `use`,
  gradienti, filtri): sono quelli che non si tingono in modo prevedibile.
- **Nome del file.** Uguale al campo `icon` dell'azione: minuscolo, parole
  separate da trattini, di solito l'identificatore con i trattini al posto dei
  punti (`draw.line` ha `draw-line.svg`).
- **Licenza.** GPL-3.0-or-later, come il progetto: dopo la dichiarazione XML
  vanno i due commenti con l'intestazione, come in ogni file nuovo.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 The Tecnigrafo authors -->
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
    <g fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
        <path d="M5 19 L19 5"/>
        <circle cx="5" cy="19" r="1.5" fill="currentColor" stroke="none"/>
    </g>
</svg>
```

### Come si colorano

`themedIcon` (`app/src/theme.cpp`) carica `:/tg/icons/<nome>.svg`, lo disegna
alle dimensioni di menu, barre e ribbon e riempie tutto ciò che è disegnato con
il colore del testo del tema: così la stessa icona vale per il tema chiaro e per
lo scuro. Oggi il colore è uno solo, anche per la classe `accent`, che segna
già gli elementi da distinguere quando il tema avrà un colore per loro. Per
questo un'icona deve restare comprensibile anche senza l'accento.

### Forme che si ripetono

Le stesse idee si disegnano sempre allo stesso modo, così le icone nuove
somigliano alle vecchie:

- **Punto pieno:** un punto definito dall'utente, come l'estremo di un
  segmento o di un arco, il primo e l'ultimo vertice di una polilinea, il
  centro di un cerchio.
- **Frecce:** punta aperta a 90° con le braccia di circa 6 px; annulla e ripeti
  sono una l'immagine speculare dell'altra.
- **Fogli e dischetti:** contorno con un angolo tagliato in alto a destra; i
  dettagli interni (piega, finestrella, etichetta) con lo stesso tratto.
- **Forme inclinate:** a 45° esatti.

## Le icone

Prima parte (0.6-P18): disegno e modifica, file e annullamento.

| File | Azione | Disegno |
|---|---|---|
| `draw-line.svg` | `draw.line` | segmento con un punto a ogni estremo |
| `draw-pline.svg` | `draw.pline` | spezzata di tre tratti, punti agli estremi |
| `draw-circle.svg` | `draw.circle` | circonferenza con il suo centro |
| `draw-arc.svg` | `draw.arc` | arco di circa 220° con i due estremi |
| `draw-text.svg` | `draw.text` | cursore di testo |
| `edit-erase.svg` | `edit.erase` | gomma inclinata |
| `edit-undo.svg` | `edit.undo` | freccia a U verso sinistra |
| `edit-redo.svg` | `edit.redo` | freccia a U verso destra |
| `file-new.svg` | `file.new` | foglio con l'angolo piegato e un più |
| `file-open.svg` | `file.open` | cartella aperta |
| `file-save.svg` | `file.save` | dischetto |
| `file-saveas.svg` | `file.saveas` | due dischetti sovrapposti |

Seconda parte (0.6-P19): vista, aiuti, interfaccia, layer e proprietà. Le icone
di layer e proprietà esistono prima delle azioni che le useranno (pannelli dei
layer e delle proprietà, 0.6-P22 e 0.6-P24).

| File | Azione | Disegno |
|---|---|---|
| `view-zoom-extents.svg` | `view.zoom-extents` | quattro angoli verso l'esterno intorno a un rettangolo |
| `aids-osnap.svg` | `aids.osnap` | calamita a ferro di cavallo |
| `aids-ortho.svg` | `aids.ortho` | due assi ad angolo retto con il segno dell'angolo retto |
| `aids-snap-endpoint.svg` | `aids.snap.endpoint` | quadrato con il punto d'accento al centro |
| `aids-snap-midpoint.svg` | `aids.snap.midpoint` | triangolo con il punto d'accento al centro |
| `aids-snap-center.svg` | `aids.snap.center` | cerchio con il punto d'accento al centro |
| `aids-snap-insertion.svg` | `aids.snap.insertion` | due quadrati sovrapposti e sfalsati con il punto d'accento |
| `aids-lwdisplay.svg` | `aids.lwdisplay` | tre barre di spessore crescente |
| `ui-ribbon.svg` | `ui.interface` | finestra con la fascia delle schede e quella dei gruppi |
| `ui-theme.svg` | `ui.theme` | cerchio diviso a metà, una metà tratteggiata |
| `tools-settings.svg` | `tools.settings` | tre cursori a scorrimento |
| `help-about.svg` | `help.about` | squadra da disegno con una squadra più piccola dentro |
| `layer-panel.svg` | `layer.panel` (0.6-P22) | pila di tre piani |
| `layer-on.svg` | stato del layer, pannello dei layer (0.6-P22) | occhio aperto |
| `layer-off.svg` | stato del layer, pannello dei layer (0.6-P22) | occhio barrato |
| `layer-frozen.svg` | stato del layer, pannello dei layer (0.6-P22) | fiocco di neve |
| `layer-thawed.svg` | stato del layer, pannello dei layer (0.6-P22) | sole |
| `layer-locked.svg` | stato del layer, pannello dei layer (0.6-P22) | lucchetto chiuso |
| `layer-unlocked.svg` | stato del layer, pannello dei layer (0.6-P22) | lucchetto aperto |
| `layer-current.svg` | `layer.current` (0.6-P22) | pila di tre piani, il primo con il punto d'accento |
| `layer-new.svg` | `layer.new` (0.6-P22) | pila di tre piani, il primo con un più |
| `layer-delete.svg` | `layer.delete` (0.6-P22) | pila di tre piani, il primo con una croce |
| `properties-panel.svg` | `properties.panel` (0.6-P24) | scheda con un elenco di voci |
| `quick-select.svg` | `edit.quick-select` (0.6-P24) | imbuto |
| `property-color.svg` | controllo del colore, pannello delle proprietà (0.6-P24) | goccia |
| `property-linetype.svg` | controllo del tipo di linea, pannello delle proprietà (0.6-P24) | linee continua, tratteggiata, tratto e punto |
| `property-lineweight.svg` | controllo dello spessore, pannello delle proprietà (0.6-P24) | barra con la freccia dello spessore |

Le voci di layer e proprietà non hanno ancora un'azione registrata: le
collegano i pacchetti indicati, che le tolgono dall'elenco delle icone riservate
in `tests/app/icons_test.cpp`. `ui.interface` ha l'icona `ui-ribbon` perché il
suo interruttore passa dai menu classici al ribbon.

Chi aggiunge icone aggiunge le sue righe a questa tabella.

## Aggiungere un'icona

1. Disegnala con le regole sopra, partendo da un'icona simile già esistente.
2. Salvala in `resources/icons/<nome>.svg` con l'intestazione. Il file entra
   da solo nelle risorse `/tg/icons`: `app/CMakeLists.txt` include tutti gli
   `*.svg` della cartella.
3. Scrivi `<nome>` nel campo `icon` dell'azione, in `app/src/builtin_actions.cpp`.
4. Aggiungi la riga alla tabella qui sopra.
5. Guardala a 16 e 24 px, nel tema chiaro e nello scuro.
6. Lancia i test dell'applicazione: `tests/app/icons_test.cpp` controlla che il
   file sia un SVG valido 24×24 che rispetta le regole (margine, tratto, punti,
   colori, elementi) e che ogni azione con un'icona abbia il suo file.
