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
- Dalla tappa 0.5 una definizione può anche leggere un **disegno da un
  file** ([File](#file)): il nome indica allora il disegno, che non entra nel
  documento delle entità definite.

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
| `TEXT` | `none` | dal parallelogramma d'ingombro, che è una regione: distanza 0 e punto stesso all'interno | punto d'ancoraggio; `position` per `Aligned` e `Fit` (ADR 0022, punto 5) |

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
| `box <E>` | quattro punti `x,y` separati da spazi | il parallelogramma d'ingombro di un `TEXT`, nell'ordine definito sotto (ADR 0022, punto 5) |

`box` trasforma, nell'ordine, i punti `(0, d·h)`, `(w, d·h)`, `(w, a·h)`,
`(0, a·h)` del sistema del testo: `w = L(valore)·h·f`, con altezza `h` e
fattore di larghezza `f` usati (ricavati per `Aligned` e `Fit`). Allineamento,
obliquo, specchiatura e rotazione seguono l'[ADR 0022](../adr/0022-allineamenti-obliquo-specchiatura-testo.md).
Senza specchiatura i punti girano in senso antiorario; con una sola
specchiatura in senso orario. `bbox` contiene i quattro angoli; selezione,
`distance` e `closest` usano la regione del parallelogramma. Lo snap
inserimento è il punto d'ancoraggio (`alignment-point` salvo `Left` sulla
linea di base), oppure `position` per `Aligned` e `Fit`.

### File

Le domande sui disegni letti da un file: la lettura e la scrittura dei
formati della tappa 0.5 e la conservazione dei dati non gestiti
([ADR 0018](../adr/0018-conservazione-dati-non-gestiti.md),
[ADR 0021](../adr/0021-comandi-dei-file.md)). Realizzate dal pacchetto P8;
le risposte dipendono dal modulo `io` (pacchetti P10–P14).

**Definizioni.** Il nome indica un disegno letto con il modulo `io`:

| Riga | Significato |
|---|---|
| `D = DXF source=<nome>` | legge con `io::readDxf` il file `<nome>` di `tests/verifica/sorgenti.json`, dalla cartella di `--sources` |
| `D = DXF file=<nome>` | lo stesso con un file della cartella del caso ([casi.md](casi.md#file-del-caso)) |
| `C = CADX source=<nome>` / `C = CADX file=<nome>` | lo stesso con `io::readCadx` |

- `<nome>` segue le regole dei nomi degli [script](script.md#file): per
  `source=` lettere minuscole, cifre e `-`; per `file=` un nome relativo,
  senza `..`.
- Un errore di lettura (file che manca, nome non valido, `source=` senza
  `--sources`, file danneggiato) rende il file di domande **non valido**,
  con il messaggio di `io`. Finché `io` non sa leggere il formato
  (`NotImplemented`, prima dei pacchetti P10 e P14), la definizione ferma il
  file con "not implemented yet", come un tipo di entità non ancora
  realizzato: il caso non è superato.
- Il nome di un disegno non si ridefinisce, né come disegno né come entità.

**Le entità di un disegno.** Nelle domande su un oggetto (`bbox`, `length`,
`distance`, `closest`, `inside`, `crosses`, `snaps`, `box`) un'entità di un
disegno letto si indica con `D:<handle>`: il nome del disegno, `:` e
l'handle dell'entità in esadecimale, maiuscolo o minuscolo (`D:2F`). Deve
essere un'entità **gestita**: un handle che non c'è, o di un'entità non
gestita, è un errore del file di domande. La domanda tratta l'entità da sola,
come se fosse l'unica definita.

| Domanda | Risposta | Significato |
|---|---|---|
| `entity D <handle>` | un'entità nel formato del [rapporto](rapporto.md#entità), `unhandled <TIPO>`, o `none` | l'entità con quell'handle: gestita, non gestita (con il suo tipo DXF), o assente (anche l'handle di una tabella o di un oggetto) |
| `count D` | elenco di `TIPO n` separati da `;`, o `none` | le entità gestite per tipo, in ordine alfabetico |
| `unhandled D` | elenco di `TIPO n` separati da `;`, o `none` | lo stesso per le entità non gestite |
| `version D` | `AC1015`…, o `none` | `$ACADVER` letto; `none` se il file non lo dice |
| `units D` | intero | `INSUNITS` del disegno, come lo dà la lettura |
| `warnings D` | elenco di avvisi separati da `;`, o `none` | gli avvisi della lettura, come li scrive `io` |
| `roundtrip D [as=dxf\|cadx]` | `identical`, o `different: <la prima differenza>` | scrive il disegno e lo rilegge, e confronta il file riscritto con l'originale (sotto); per difetto nello stesso formato e nella stessa versione |
| `open-error source=<nome>` / `open-error file=<nome>` | il messaggio tra virgolette, o `none` | prova a leggere il file e dice l'errore della lettura, con la sua riga (`"line 812: expected a group code, found \"abc\""`); `none` se si legge |

- Gli elenchi si confrontano in qualunque ordine, come gli altri.
- `open-error` legge un `.dxf` con `io::readDxf` e un `.cadx` con
  `io::readCadx`, secondo l'estensione, come `OPEN`; i file di
  `sorgenti.json`, che non hanno estensione, sono DXF. Un file che manca o un
  nome non valido è un errore del file di domande, non una risposta.
- Un errore di `io` durante `roundtrip` (per esempio un formato che non sa
  ancora scrivere) risponde `error: <messaggio>`.

**`roundtrip`** applica la definizione di "identico" dell'ADR 0018, punto 6.
Confronta, **a livello di gruppi** (`io::readGroups`), il file **originale**
con quello **riscritto**: così non basta che il lettore sia coerente con sé
stesso.

1. Scrive il disegno letto: in DXF (`as=dxf`) nella versione letta, o R2018
   per un disegno letto da un `.cadx`; in `.cadx` (`as=cadx`) con
   `io::writeCadx`. Rilegge il file scritto: un errore è una differenza
   (`different: the rewritten file cannot be read: …`).
2. Prende i gruppi dell'originale e del riscritto. Di un `.cadx` si prendono
   i gruppi del suo `drawing.dxf` come lo riscrive il modello (in R2018):
   `tecnigrafo-script` non apre gli archivi.
3. Confronta i gruppi uno per uno, nell'ordine: stesso codice e stesso
   valore, carattere per carattere. Fanno eccezione i numeri delle **entità
   gestite**, che si riscrivono dal modello e sono uguali entro le
   tolleranze in vigore (`1.0` e `1`); il valore di `$HANDSEED`, che può
   cambiare; e, se le versioni sono diverse, i valori di `$ACADVER` e
   `$DWGCODEPAGE`.

La prima differenza si descrive con la riga dell'originale:
`different: line 812: 10 "1.5" became 10 "2.5"`,
`different: line 900: 0 "HATCH" is missing`,
`different: the rewritten file has more groups: 0 "CIRCLE"`.

```
; un DXF pubblico: conservazione e conteggi (valori da ezdxf)
D = DXF source=librecad-sample
? version D               -> AC1015
? count D                 -> ARC 2; LINE 12
? unhandled D             -> HATCH 1; INSERT 3
? entity D 2F             -> LINE start=0,0 end=100,0
? bbox D:2F               -> 0,0 100,0
? roundtrip D             -> identical
? roundtrip D as=cadx     -> identical
? open-error file=troncato.dxf -> "line 41: the file ends inside a group"
```

I numeri dell'esempio mostrano solo la forma.

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
