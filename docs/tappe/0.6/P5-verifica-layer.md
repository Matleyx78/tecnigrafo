# [0.6-P5] `script`: layer e proprietà nella modalità di controllo

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #231
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: è il sistema di verifica (come 0.5-P8); si
  prova con i suoi test
- **Dipende da:** P4
- **Modulo:** `script` (più le specifiche in `docs/verifica/`)
- **Da leggere:** [ADR 0023](../../adr/0023-layer-e-proprieta.md) punto 7,
  [ADR 0024](../../adr/0024-colori.md) punto 6,
  [ADR 0025](../../adr/0025-tipi-di-linea-e-spessori.md) punti 4 e 6,
  [ADR 0011](../../adr/0011-modalita-di-controllo.md) punti 3–4;
  `docs/verifica/rapporto.md` ("Come cresce il formato"),
  `docs/verifica/domande.md`; `script/src/report.cpp`,
  `script/src/entity_reader.cpp`, `script/src/question_table.cpp`;
  `core/include/tg/core/layer.hpp` (P4) e le intestazioni di P2

## File

- `docs/verifica/rapporto.md`, `docs/verifica/domande.md`
- `script/include/tg/script/report.hpp`, `script/src/report.cpp`
- `script/include/tg/script/entity_reader.hpp`, `script/src/entity_reader.cpp`
- `script/src/questions.cpp` (definizioni `LAYER` e `LTYPE`)
- `script/src/question_table.hpp`, `script/src/question_table.cpp`
- `script/src/property_questions.cpp` (nuovo: `aci`, `effective` dichiarate;
  `layers`, `linetypes` sui file)
- `script/CMakeLists.txt`
- `tests/script/report_test.cpp`, `tests/script/entity_reader_test.cpp`,
  `tests/script/questions_test.cpp`

## Cosa fare

Prima i documenti, poi il codice: i pacchetti di riferimenti (R9–R14, R23)
si scrivono **solo** da ciò che pubblichi in `docs/verifica/`. Il catalogo
pubblica le domande di **tutta la tappa**: quelle che realizzano altri
pacchetti hanno nella tabella la funzione `nullptr` e rispondono "not
implemented yet" fino al loro pacchetto.

### Proprietà delle entità (rapporto e definizioni)

In fondo alla riga di ogni entità, prima delle proprietà derivate `~…`, e
**solo se diverse dal valore predefinito**, in quest'ordine:

| Proprietà | Valore | Predefinito |
|---|---|---|
| `layer` | nome tra virgolette (`layer="Muri esterni"`) | `"0"` |
| `color` | `formatColor` (ADR 0024, punto 6) | `bylayer` |
| `linetype` | nome tra virgolette | `"ByLayer"` |
| `ltscale` | numero | 1 |
| `lineweight` | `formatLineweight` (P2) | `bylayer` |

Anche `LWPOLYLINE` riceve una proprietà sua, `linetype-generation=yes`
(predefinito `no`), dopo `closed=`: la generazione continua del tipo di linea
(bit 128 del gruppo 70, ADR 0025 punto 4), che il modello riceve da P10.
Fino a P10 si legge e si scrive solo nel rapporto e nelle definizioni.

Nelle definizioni delle domande (`A = LINE … layer="Muri" color=1`) le
stesse proprietà, in qualunque ordine dopo la geometria. Un layer o un tipo
di linea non definito prima è un errore del file di domande.

### Tabelle e valori correnti (campi nuovi del rapporto)

Dopo `units:` e prima di `unhandled:`, ciascuno **solo se diverso da una
sessione nuova**:

```
layers:
  "0" color=7 linetype="Continuous" lineweight=default
  "Muri" color=1 linetype="ACAD_ISO02W100" lineweight=0.5 off frozen locked noplot
linetypes:
  "ACAD_ISO02W100" pattern=12,-3 description="ISO dash __ __ __"
current: layer="Muri" color=bylayer linetype="ByLayer" ltscale=1 lineweight=bylayer
ltscale: 2
lwdisplay: on
```

- `layers:` elenca **tutti** i layer, nell'ordine della tabella, se la tabella
  non è quella di una sessione nuova; gli stati compaiono solo se attivi
  (`off`, `frozen`, `locked`, `noplot`), in quest'ordine.
- `linetypes:` elenca i tipi diversi da `ByBlock`, `ByLayer`, `Continuous`,
  nell'ordine della tabella; `pattern=` con gli elementi separati da `,`
  (`continuous` se non ne ha).
- `current:` tutti e cinque i valori, se almeno uno è diverso.

### Definizioni nelle domande

```
L = LAYER name="Muri" color=1 linetype="ACAD_ISO02W100" lineweight=0.5 off frozen locked noplot
T = LTYPE name="Tratteggio" pattern=5,-2,0,-2 description="mio"
```

- `LAYER`: `name` obbligatorio; gli altri come nel layer `0` se mancano. Il
  layer `0` esiste già e si può ridefinire una volta con `name="0"` per
  cambiarne colore e stati.
- `LTYPE`: `name` e `pattern` obbligatori; `ACAD_ISO02W100` e
  `ACAD_ISO10W100` si definiscono anche con il solo nome (`standardLinetype`).
- `!current layer="Muri" color=2` — direttiva nuova delle domande: cambia i
  valori correnti (serve a `effective` con `ByLayer` e alle domande sul
  documento).

### Domande nuove

| Domanda | Risposta | Realizzata da |
|---|---|---|
| `aci <n>` | `r,g,b` | P9 |
| `effective <E>` | `color=<r,g,b> linetype="<nome>" ltscale=<n> lineweight=<mm> visible=yes\|no locked=yes\|no` | P9 |
| `dashes <E>` | elenco di pezzi separati da `;` (`LINE x1,y1 x2,y2`, `ARC center=… radius=… start-angle=… end-angle=…`, `POINT x,y`), o `continuous` | P11 |
| `quickselect [type=<TIPO>] [layer\|color\|linetype\|lineweight<op><valore>] [within=<nomi>] [exclude]` con `<op>` `=` o `!=` | elenco di nomi, o `none` | P23 |
| `layers D` | elenco di righe come in `layers:`, separate da `;` | questo pacchetto |
| `linetypes D` | elenco di righe come in `linetypes:`, separate da `;`, o `none` | questo pacchetto |

- `effective`: `ByLayer` dal layer; `ByBlock` vale come colore 7, tipo
  `Continuous` e spessore `default` (ADR 0024, Conseguenze); `ltscale` è
  `LTSCALE` × la scala dell'entità; `lineweight` in millimetri, con `Default`
  risolto da `LWDEFAULT`; `visible=no` su layer spento o congelato.
- Le domande sul documento (`pick`, `window`, `crossing`, `snap`) seguono la
  tabella degli stati dell'ADR 0023, punto 4: lo scrivi in `domande.md`; lo
  realizza P9.
- `entity D <handle>` scrive anche le proprietà, come il rapporto.

### Codice

- Lettura e scrittura delle proprietà, delle tabelle, dei valori correnti e
  della direttiva `!current`.
- `layers`, `linetypes` e le proprietà in `entity` si realizzano qui (`io`
  le legge da P10; fino ad allora un DXF dà solo il layer `0`).
- `aci`, `effective`, `dashes`, `quickselect` entrano nella tabella con
  `nullptr`.

## Regole della tappa che valgono qui

- Il formato cresce **solo aggiungendo** (`rapporto.md`, "Come cresce il
  formato"): i rapporti e i casi già scritti restano validi senza ritocchi.
- `report.cpp`, `entity_reader.cpp` e `question_table.cpp` li toccano poi P9,
  P11 e P23 per le loro domande: solo le loro righe.

## Test richiesti

- Una sessione nuova: il rapporto non cambia di un carattere.
- Ogni proprietà, scritta solo se diversa dal predefinito; nomi con spazi e
  virgolette; rilettura senza perdite di una riga del rapporto.
- `layers:`, `linetypes:`, `current:`, `ltscale:`, `lwdisplay:` presenti solo
  quando servono.
- Definizioni `LAYER` e `LTYPE`: valide, nome doppio, layer o tipo
  sconosciuto in un'entità, `!current`.
- Le quattro domande non ancora realizzate rispondono "not implemented yet".

## Fatto quando

- `docs/verifica/rapporto.md` e `domande.md` descrivono tutto ciò che è qui.
- Tutti i casi di verifica esistenti passano senza modifiche.
- La CI è verde.
