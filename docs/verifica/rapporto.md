# Il rapporto

Formato del rapporto che la modalità di controllo scrive alla fine di uno
script ([ADR 0011](../adr/0011-modalita-di-controllo.md), punto 4). È un
**contratto**: i casi di verifica lo scrivono a mano in `atteso.txt`, e lo
stesso formato descrive le entità nelle [domande al modello](domande.md). Si
cambia solo aggiungendo, con le regole in fondo; per il resto serve un nuovo
ADR.

Il livello 1 e il livello 2 scrivono lo stesso rapporto per lo stesso script.

## Struttura

```
tecnigrafo-report 1

[report 1]
entities:
  1 LWPOLYLINE closed=yes vertex=0,0 vertex=100,0 vertex=100,50 vertex=0,50
messages:
  Command: PLINE
  Specify start point: 0,0
  Specify next point or [Undo]: 100,0
  Specify next point or [Undo]: @0,50
  Specify next point or [Close/Undo]: 50<90
  Specify next point or [Close/Undo]: C
prompt: Command:
preview:
selection: none
rectangle: none
undo:
  Polyline
redo:
snap: on
ortho: off
pointer: none
active snap: none
statistics:
  LWPOLYLINE 1

[final]
...
```

- La prima riga è la versione del formato: `tecnigrafo-report 1`.
- Poi una **sezione** per ogni `!report` dello script, nell'ordine, intitolate
  `[report 1]`, `[report 2]`…, e sempre per ultima la sezione `[final]` con lo
  stato alla fine dello script. Ogni sezione è preceduta da una riga vuota.
- Le righe di una sezione sono sempre quelle qui sotto, in quest'ordine. Un
  **elenco** è una riga che finisce con `:` seguita da una riga per elemento,
  rientrata di due spazi; un elenco vuoto è la sola riga d'intestazione. Gli
  altri **campi** stanno su una riga, `nome: valore`.
- Nessuna riga ha spazi in fondo, tranne quelli che l'utente ha scritto in
  fondo a una riga di comando, riportati nell'eco.

| Riga | Contenuto |
|---|---|
| `entities:` | le entità del disegno, **in ordine di identificativo**, una per riga: l'identificativo, uno spazio, l'entità nel formato qui sotto |
| `messages:` | le righe scritte nella cronologia della riga di comando **dalla sezione precedente** (vedi sotto) |
| `prompt:` | la domanda attuale della riga di comando (`Command:` a riposo) |
| `preview:` | le entità dell'anteprima del comando in corso, senza identificativo, nell'ordine in cui il comando le dà |
| `selection:` | gli identificativi della selezione corrente in ordine crescente, separati da uno spazio, o `none` |
| `rectangle:` | il rettangolo di selezione aperto: `window` o `crossing`, il primo angolo e quello che segue il puntatore (`window -5,-5 50,30`), o `none` |
| `undo:` | i nomi dei passi di annullamento, dal più vecchio: l'ultimo è quello che `U` annulla |
| `redo:` | i nomi dei passi da ripetere: l'ultimo è quello che `REDO` ripete |
| `snap:` | `on` o `off`; se qualche modo dello snap è spento, segue `, modes off:` e i modi spenti (`on, modes off: midpoint center`) |
| `ortho:` | `on` o `off` |
| `pointer:` | l'ultimo punto del disegno in cui è stato portato il puntatore (`!move`, `!click`), o `none` |
| `active snap:` | lo snap su cui si trova il puntatore mentre un comando chiede un punto: il tipo e il punto (`Endpoint 100,50`), o `none` |
| `statistics:` | per ogni tipo DXF presente nel disegno, in ordine alfabetico, il tipo, uno spazio e il numero di entità (`LWPOLYLINE 2`); i tipi assenti non si scrivono |

### La cronologia (`messages:`)

È ciò che la riga di comando dell'interfaccia mostra nella sua cronologia:

- per ogni riga normale dello script, l'**eco**: la domanda in quel momento,
  uno spazio e il testo scritto (`Specify start point: 0,0`); per una riga
  vuota la sola domanda (`Specify next point or [Undo]:`);
- i messaggi del programma, nell'ordine in cui arrivano (`Undone: Polyline.`,
  `*Cancel*`, `3 found, 5 total`, `Unknown command: XYZ.`).

Clic, movimenti, `!esc`, `!snap` e `!ortho` non hanno eco. Ogni sezione
riporta solo le righe arrivate dopo la sezione precedente: nessuna riga compare
due volte.

### Lo snap attivo

`active snap:` si scrive ai due livelli, perché i due rapporti siano uguali: al
livello 1 lo calcola il runner dei comandi dal puntatore e dalla scala di
`!view`, esattamente come per il marcatore che il livello 2 disegna. I nomi dei
tipi sono `Endpoint`, `Midpoint`, `Center` e, dalla tappa 0.4 (pacchetto P13),
`Insertion`.

## Numeri

Ogni numero si scrive con la rappresentazione più breve che, riletta, dà lo
stesso `double`, indipendente dalla lingua del sistema:

- punto decimale, nessun separatore delle migliaia, nessuno `.0` finale:
  `100`, `0.1`, `-2.5`, `0.3333333333333333`;
- notazione esponenziale per gli esponenti decimali sotto −4 o sopra 15, con
  il segno e almeno due cifre: `1e+16`, `1e-05`, `3.061616997868383e-15` (come
  `repr()` di Python, salvo lo `.0` finale);
- `-0` si scrive `0`; infinito e NaN sono `inf`, `-inf`, `nan`.

Chi scrive `atteso.txt` non deve indovinare l'ultima cifra: il
[confronto](casi.md) accetta i numeri entro la tolleranza (`1e-9` per difetto),
e accetta anche `100.0` al posto di `100` o `0.00001` al posto di `1e-05`.

Un **punto** è `x,y`, senza spazi. Gli **angoli** sono in gradi, come nel DXF.

## Entità

Un'entità sta su **una riga**: il tipo DXF, poi le proprietà `nome=valore`
separate da uno spazio, sempre nell'ordine indicato. Valori:

- numeri e punti come sopra;
- `yes` / `no` per le proprietà vere o false;
- testi tra virgolette doppie, con `\"` per le virgolette, `\\` per la barra
  rovesciata e `\u{XX}` per i caratteri di controllo (da U+0000 a U+001F e
  U+007F); ogni altro carattere, accentato o no, si scrive com'è, in UTF-8.
  Nelle definizioni delle domande `\u{…}` accetta qualunque carattere Unicode,
  da una a sei cifre esadecimali (`\u{2300}` è `⌀`).

Le proprietà il cui nome inizia con `~` sono **derivate**: le calcola il
programma, servono a controllarlo e non si rileggono.

### Tappa 0.4

Il formato di tutte le entità della tappa è fissato qui, prima che esistano,
perché i riferimenti si possano scrivere prima del lavoro. Dalla tappa 0.4 il
modulo `script` scrive e legge `LWPOLYLINE`; `LINE`, `CIRCLE` e `ARC` li
aggiunge il pacchetto P6, `TEXT` il pacchetto P13.

**`LWPOLYLINE`** (ADR 0004)

```
LWPOLYLINE closed=no vertex=0,0 vertex=10,0 bulge=0.5 vertex=20,0
```

| Proprietà | DXF | Valore |
|---|---|---|
| `closed` | 70, bit 1 | `yes` o `no` |
| `vertex` | 10/20 | un vertice; si ripete per ogni vertice, nell'ordine |
| `bulge` | 42 | il bulge del segmento che **parte** dal vertice appena scritto; si scrive solo se diverso da 0 |

Una polilinea senza vertici è `LWPOLYLINE closed=no`.

**`LINE`** (ADR 0012)

```
LINE start=0,0 end=10,5
```

| Proprietà | DXF | Valore |
|---|---|---|
| `start` | 10/20 | punto iniziale |
| `end` | 11/21 | punto finale |

**`CIRCLE`** (ADR 0012)

```
CIRCLE center=5,5 radius=7.0710678118654755
```

| Proprietà | DXF | Valore |
|---|---|---|
| `center` | 10/20 | centro |
| `radius` | 40 | raggio, > 0 |

**`ARC`** (ADR 0012)

```
ARC center=0,0 radius=10 start-angle=0 end-angle=90
```

| Proprietà | DXF | Valore |
|---|---|---|
| `center` | 10/20 | centro |
| `radius` | 40 | raggio, > 0 |
| `start-angle` | 50 | angolo iniziale in gradi, in [0, 360) |
| `end-angle` | 51 | angolo finale in gradi, in [0, 360), diverso dall'iniziale |

L'arco va sempre in senso antiorario dall'angolo iniziale a quello finale, come
nel DXF: un arco che attraversa 0° ha l'angolo finale minore dell'iniziale
(`start-angle=300 end-angle=30`).

**`TEXT`** (ADR 0013)

```
TEXT position=0,0 alignment-point=0,0 height=2.5 rotation=0 width-factor=1 oblique=0 style="Standard" mirror-x=no mirror-y=no horizontal=left vertical=baseline value="Hello" ~box=0,-0.5;5.5,-0.5;5.5,3;0,3
```

| Proprietà | DXF | Valore |
|---|---|---|
| `position` | 10/20 | primo punto d'allineamento |
| `alignment-point` | 11/21 | secondo punto d'allineamento |
| `height` | 40 | altezza delle maiuscole, > 0 |
| `rotation` | 50 | gradi |
| `width-factor` | 41 | fattore di larghezza |
| `oblique` | 51 | angolo obliquo, gradi |
| `style` | 7 | nome dello stile, tra virgolette |
| `mirror-x` | 71, bit 2 | `yes` o `no` |
| `mirror-y` | 71, bit 4 | `yes` o `no` |
| `horizontal` | 72 | `left`, `center`, `right`, `aligned`, `middle`, `fit` |
| `vertical` | 73 | `baseline`, `bottom`, `middle`, `top` |
| `value` | 1 | il testo come è scritto, codici di controllo compresi (`%%c`), tra virgolette |
| `~box` | — | **derivata**: i quattro angoli del rettangolo d'ingombro, separati da `;`, in senso antiorario dall'angolo in basso a sinistra del primo carattere (ADR 0013, punto 3) |

`~box` permette di controllare le misure del font: si calcola dalle larghezze
lette dal file del font, con uno strumento indipendente. I numeri dell'esempio
mostrano solo la forma, non sono misure di Liberation Sans.

### Definizioni nelle domande

Nelle [domande al modello](domande.md) un'entità si scrive nello stesso
formato, con tre libertà:

- le proprietà si possono scrivere in qualunque ordine, ognuna una volta sola
  (tranne `vertex` e `bulge` di `LWPOLYLINE`, il cui ordine conta);
- le proprietà con un valore predefinito si possono omettere:
  `LWPOLYLINE` `closed=no`; `TEXT` `alignment-point=0,0` (come nel DXF),
  `height=2.5`, `rotation=0`, `width-factor=1`, `oblique=0`,
  `style="Standard"`, `mirror-x=no`, `mirror-y=no`, `horizontal=left`,
  `vertical=baseline`. Le altre sono obbligatorie;
- le proprietà derivate (`~…`) si accettano e si ignorano, così si può copiare
  una riga del rapporto.

Un'entità scritta dal rapporto si rilegge senza perdite: i numeri hanno tutte
le cifre che servono. Per gli angoli, convertiti tra gradi e radianti, vale la
tolleranza del confronto.

## Come cresce il formato

- Una **proprietà nuova** di un'entità (per esempio il layer, tappa 0.6) si
  scrive **solo quando è diversa dal valore predefinito**, in fondo alla riga
  prima delle derivate. Così i rapporti già scritti restano validi.
- Un **campo nuovo** dello stato si aggiunge solo con lo stesso criterio (si
  scrive solo quando ha un valore diverso da quello di una sessione nuova),
  come già fa `snap:` con i modi spenti.
- Un **tipo nuovo** di entità si aggiunge a questo documento, con le sue
  proprietà, prima del pacchetto che lo realizza.
- Ogni altro cambiamento richiede un nuovo ADR e una nuova versione nella prima
  riga (`tecnigrafo-report 2`).
