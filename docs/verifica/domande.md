# Domande al modello (`.tgq`)

Catalogo delle domande con cui la modalità di controllo verifica il modello
geometrico anche dove nessun comando arriva ancora
([ADR 0011](../adr/0011-modalita-di-controllo.md), punto 3). È un **contratto**,
con la stessa regola del [linguaggio degli script](script.md): una domanda si
aggiunge, non si cambia.

Le domande esistono solo in `tecnigrafo-script`, al livello 1: il programma
distribuito non le legge.

## Il file

Testo UTF-8, con o senza BOM, righe terminate da LF o CRLF. Una riga per
istruzione; gli spazi all'inizio e alla fine della riga non contano.

| Riga | Significato |
|---|---|
| vuota, o che inizia con `;` | ignorata |
| `!tolerance <tipo> <valore>` | cambia una tolleranza, come negli [script](script.md#dettagli), **per le domande che seguono** |
| `<Nome> = <entità>` | una **definizione** |
| `? <domanda> <argomenti> -> <risposta attesa>` | una **domanda** |

```
; polilinea a L e mezza circonferenza sotto l'asse
A = LWPOLYLINE vertex=0,0 vertex=10,0 vertex=10,10
B = LWPOLYLINE vertex=20,0 bulge=1 vertex=30,0
? bbox B                  -> 20,-5 30,0
? length B                -> 15.707963267948966
? closest A 5,5           -> 5,0
? snaps B                 -> Endpoint 20,0; Endpoint 30,0; Midpoint 25,-5; Center 25,0
? pick 25,-5.5 tolerance=1 -> B
? window -1,-6 31,11      -> A B
? snap 27,-4.5 aperture=1 -> Center 25,0
```

### Definizioni

- Il **nome** inizia con una lettera o `_` e continua con lettere, cifre e `_`;
  `none` non è un nome. Ogni nome si definisce una volta.
- L'**entità** si scrive nel formato del [rapporto](rapporto.md#entità), con le
  libertà delle [definizioni](rapporto.md#definizioni-nelle-domande): proprietà
  in qualunque ordine, predefinite omesse, derivate ignorate.
- Le entità definite formano un documento, numerate da 1 nell'ordine in cui
  compaiono. Una domanda vede le entità definite **sopra** di lei: si può
  definire, chiedere, definire ancora e chiedere di nuovo.

### Domande

- Dopo `?` vengono il nome della domanda e i suoi argomenti, separati da spazi;
  poi `->` e la risposta attesa. Un testo tra virgolette (`"a -> b"`) è un solo
  argomento, e `->` al suo interno non conta.
- Gli argomenti con nome (`tolerance=3`, `aperture=1`, `modes=…`) vengono dopo
  quelli senza, in qualunque ordine.
- Un **punto** è `x,y`; un **rettangolo** è due angoli opposti `x1,y1 x2,y2`, in
  qualunque ordine; i bordi fanno parte del rettangolo.
- I **modi dello snap** si danno con `modes=` e i nomi separati da virgole:
  `endpoint`, `midpoint`, `center` e, dal pacchetto P13, `insertion`;
  `modes=none` li spegne tutti. Senza `modes=` sono tutti accesi.

## Le risposte

Ogni domanda ha una forma fissa della risposta. Il confronto:

- tra la risposta attesa e quella ottenuta, più spazi di fila contano come uno;
- i **numeri** sono uguali entro le tolleranze in vigore a quella riga (per
  difetto `1e-9` assoluto), scritti in qualunque forma (`100`, `100.0`, `1e2`);
  il resto deve coincidere;
- le risposte che sono **elenchi** si confrontano **in qualunque ordine**: ogni
  elemento atteso deve corrispondere a un elemento ottenuto, e nessuno avanza;
- `none` è un elenco vuoto, o l'assenza di una risposta.

Valori: numeri e punti come nel [rapporto](rapporto.md#numeri); `yes` / `no`;
i nomi dei tipi di snap `Endpoint`, `Midpoint`, `Center`, `Insertion`.

## Catalogo

### Su un oggetto

`<E>` è il nome di un'entità definita sopra.

| Domanda | Risposta | Significato |
|---|---|---|
| `bbox <E>` | `xmin,ymin xmax,ymax`, o `empty` | il riquadro esatto, archi compresi |
| `length <E>` | numero | la lunghezza della curva |
| `distance <E> x,y` | numero, o `inf` | la distanza del punto dalla curva |
| `closest <E> x,y` | `x,y`, o `none` | il punto della curva più vicino; a pari distanza il primo segmento, e in un arco il punto iniziale |
| `inside <E> x1,y1 x2,y2` | `yes` / `no` | l'entità è tutta nel rettangolo (selezione a finestra) |
| `crosses <E> x1,y1 x2,y2` | `yes` / `no` | l'entità tocca il rettangolo o vi sta dentro (selezione a intersezione) |
| `snaps <E> [modes=…]` | elenco di `Tipo x,y` separati da `;`, o `none` | i punti di snap dell'entità, ognuno una volta |

Come ogni tipo di entità risponde ([ADR 0008](../adr/0008-selezione-e-snap.md),
[ADR 0012](../adr/0012-linea-cerchio-arco.md),
[ADR 0013](../adr/0013-testo-e-font.md)):

| Tipo | `length` | `distance`, `closest` | `snaps` |
|---|---|---|---|
| `LWPOLYLINE` | somma dei segmenti, archi compresi | dalla curva, non dall'interno; un vertice solo è un punto; senza vertici `inf` e `none` | estremo di ogni vertice, medio di ogni segmento (sull'arco), centro di ogni arco |
| `LINE` | lunghezza del segmento | dal segmento | estremi, medio |
| `ARC` | lunghezza dell'arco | dall'arco, mai dal resto del cerchio | estremi, medio sull'arco, centro |
| `CIRCLE` | circonferenza | dalla circonferenza: il centro è a distanza pari al raggio | centro |
| `TEXT` | `none` | dal rettangolo d'ingombro, che è una regione: distanza 0 e punto stesso all'interno | inserimento (`position`) |

### Sul documento

| Domanda | Risposta | Significato |
|---|---|---|
| `pick x,y tolerance=<t>` | un nome, o `none` | l'entità scelta con un clic in `x,y` entro `t` unità del disegno; a pari distanza vince l'identificativo più alto |
| `window x1,y1 x2,y2` | elenco di nomi separati da spazi, o `none` | le entità selezionate a finestra |
| `crossing x1,y1 x2,y2` | elenco di nomi separati da spazi, o `none` | le entità selezionate a intersezione |
| `snap x,y aperture=<a> [modes=…]` | `Tipo x,y`, o `none` | lo snap per il puntatore in `x,y` con l'apertura `a` in unità del disegno, con le regole di priorità dell'ADR 0008: tra estremi e medi entro l'apertura vince il più vicino (a pari distanza l'estremo), altrimenti il centro (e dalla tappa 0.4 l'inserimento) dell'entità su cui passa il puntatore |

### Costruzioni

Il risultato di un algoritmo che crea geometria, scritto come una
[definizione](rapporto.md#entità). Realizzate dal pacchetto P6.

| Domanda | Risposta | Significato |
|---|---|---|
| `circle-through x1,y1 x2,y2 x3,y3` | `CIRCLE center=… radius=…`, o `collinear` | il cerchio per tre punti; `collinear` per punti allineati o coincidenti secondo l'ADR 0012, punto 4 |
| `arc-through x1,y1 x2,y2 x3,y3` | `ARC center=… radius=… start-angle=… end-angle=…`, o `collinear` | l'arco dal primo punto per il secondo fino al terzo, scritto in senso antiorario come nel DXF |

### Testo

Realizzate dai pacchetti P11 (`text-width`, `font-metrics`) e P13 (`box`). Le
misure sono **in unità dell'altezza delle maiuscole** (ADR 0013, punto 3): un
testo alto 1, con fattore di larghezza 1.

| Domanda | Risposta | Significato |
|---|---|---|
| `text-width "<testo>"` | numero | la larghezza di una riga, somma delle larghezze dei caratteri, senza crenatura; i codici di controllo (`%%c`) **non** si convertono: si misurano i caratteri come sono scritti |
| `font-metrics` | `ascent=<a> descent=<d>` | ascendente (> 1) e discendente (< 0) del font incluso |
| `box <E>` | quattro punti `x,y` separati da spazi | il rettangolo d'ingombro di un `TEXT`, in senso antiorario dall'angolo in basso a sinistra del primo carattere |

## Errori

Un file che non rispetta questa specifica non è un caso valido: definizione o
domanda scritta male, nome già definito o non definito sopra, domanda
sconosciuta, argomenti mancanti o in più. `tecnigrafo-script` indica la riga e
il motivo ed esce con il codice 2 ([casi.md](casi.md)).

Non sono errori, ma risposte sbagliate, due situazioni previste da questo
catalogo e non ancora realizzate dal programma:

- una **domanda** del catalogo non ancora realizzata risponde
  `error: question not implemented yet`;
- una **definizione** di un tipo del [rapporto](rapporto.md#tappa-04) non
  ancora realizzato (`CIRCLE` prima del pacchetto P6) ferma il file con
  "not implemented yet".

In entrambi i casi il caso non è superato: se è elencato in
`tests/verifica/in-attesa.txt` aspetta il suo lavoro senza bloccare la CI.

## Valori attesi

Le risposte attese si ricavano da calcoli, da file pubblici o da programmi di
riferimento, **mai** eseguendo Tecnigrafo (ADR 0011, punto 1). Le misure del
font si leggono dal file del font con uno strumento indipendente, per esempio
fontTools. `tecnigrafo-script check` scrive accanto al risultato il file
`ottenuto.tgq`, con le risposte del programma al posto di quelle attese: serve
a chi lavora per vedere le differenze, mai a scrivere i riferimenti.
