# [0.6-P10] `io`: layer, tipi di linea e proprietà nel DXF

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #238
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.6-R10]` (#237), agente gemini
- **Dipende da:** P4, P5, P8, R10
- **Modulo:** `io`
- **Da leggere:** [ADR 0023](../../adr/0023-layer-e-proprieta.md) punti 1, 2,
  3, 6; [ADR 0024](../../adr/0024-colori.md) punto 2;
  [ADR 0025](../../adr/0025-tipi-di-linea-e-spessori.md) punti 1 e 6;
  [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md) (tutto);
  `docs/formati/cadx.md`; `io/src/dxf_read.cpp`, `dxf_write.cpp`,
  `dxf_entity_common.cpp`, `dxf_entities.cpp`, `dxf_skeleton.cpp`

## File

- `io/src/dxf_tables.hpp`, `io/src/dxf_tables.cpp` (nuovi: `LAYER` e `LTYPE`)
- `io/src/dxf_read.cpp`, `io/src/dxf_write.cpp`
- `io/src/dxf_entity_common.hpp`, `io/src/dxf_entity_common.cpp`
- `io/src/dxf_skeleton.cpp`, `core/include/tg/core/start_handles.hpp` (gli
  handle fissi dello scheletro)
- `io/src/dxf_lwpolyline.cpp`, `core/include/tg/core/polyline.hpp`,
  `core/src/polyline.cpp` (generazione continua del tipo di linea)
- `io/src/dxf_header.cpp` (nuovo, se serve: le variabili che entrano nel modello)
- `io/CMakeLists.txt`
- `tests/io/dxf_tables_test.cpp` (nuovo), `tests/io/dxf_read_test.cpp`,
  `tests/io/dxf_write_test.cpp`, `tests/io/cadx_test.cpp`
- `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R10)

## Cosa fare

### Lettura

- Dalla sezione `TABLES` grezza, i record delle tabelle `LAYER` e `LTYPE`
  entrano nel modello (P4), con il loro handle e i gruppi che il modello non
  rappresenta in `extras`. Il resto della sezione (intestazioni delle
  tabelle, le altre tabelle) resta grezzo; dove stavano i record resta un
  segnaposto, come per `ENTITIES` (ADR 0018, punto 2), così lo scrittore li
  rimette nello stesso posto e nello stesso ordine.
- `LAYER`: 2 nome, 62 colore (negativo = spento), 420 RGB, 6 tipo di linea,
  370 spessore, 70 bit 1 congelato e bit 4 bloccato (gli altri bit si
  conservano), 290 stampabile. `LTYPE`: 2 nome, 3 descrizione, 73, 40 e 49;
  un tipo con elementi complessi (74 ≠ 0) tiene in `extras` ciò che serve a
  riscriverlo uguale e nel modello solo tratti, spazi e punti.
- Entità: 8, 62, 420, 6, 48, 370 dalle proprietà comuni al modello; gli altri
  gruppi comuni restano in `extras.common` (430, 440, 60, 284, 347, 390…).
- `HEADER`: `$CLAYER`, `$CECOLOR`, `$CELTYPE`, `$CELWEIGHT`, `$CELTSCALE`,
  `$LTSCALE`, `$LWDISPLAY`, `$LWDEFAULT` escono dalla sezione grezza ed
  entrano nel documento.
- File incoerenti (ADR 0023, punto 6; ADR 0025, punto 6): layer o tipo
  mancante creato; nome doppio tenuto grezzo; spessore non ammesso
  arrotondato; colore fuori intervallo. Ogni caso si conta negli avvisi
  della lettura, una riga per specie: `1 missing layer added.`,
  `2 missing linetypes added.`, `1 duplicate layer kept as unhandled data.`,
  `3 lineweights rounded.`

### Scrittura

- I record si scrivono al loro segnaposto, nell'ordine della tabella; i
  record nuovi in fondo alla loro tabella. Il conteggio della tabella (70)
  si aggiorna.
- Colore: 62 per un indice; 420 e 62 dell'indice più vicino per un RGB, salvo
  un 62 letto e non cambiato (ADR 0024, punto 2). Layer spento: 62 negativo.
- Proprietà delle entità: si scrivono solo se diverse dal valore predefinito,
  tranne il layer (8), che si scrive sempre.
- Disegno nuovo: lo scheletro usa le tabelle del documento invece dei record
  fissi di oggi.
- Handle dello scheletro (ADR 0023, punto 1, "Handle di un disegno nuovo"):
  ogni oggetto che lo scheletro crea (tabelle, blocchi e record
  `*Model_Space` e `*Paper_Space`, dizionari, layout, gli altri record) ha un
  handle **fisso** della mappa di `start_handles.hpp`, sotto
  `start_handles::kLowest`, scendendo; `kLowest` diventa il più basso dato.
  Lo scheletro non prende più handle dal contatore delle entità
  (`makeSkeleton(firstId)` di oggi), e `$HANDSEED` è il `nextId()` del
  documento. Così un disegno nuovo salvato ha sempre gli stessi handle di
  partenza, comunque tante siano le entità.
- Il `.cadx` non cambia: le tabelle viaggiano in `drawing.dxf`.

### Generazione continua delle polilinee

Il bit 128 del gruppo 70 di `LWPOLYLINE` (oggi conservato tra i bit grezzi)
entra nel modello, perché P11 ne ha bisogno (ADR 0025, punto 4):
`bool Polyline::continuousLinetype() const` e
`void setContinuousLinetype(bool)`, falso per difetto; il lettore lo legge, lo
scrittore lo scrive insieme a `closed`. Nel rapporto è la proprietà
`linetype-generation=yes` (P5).

## Regole della tappa che valgono qui

- Riscrivere un DXF letto deve restare **identico** secondo l'ADR 0018, punto
  6 (`? roundtrip`): i casi esistenti di R10 della 0.5 devono passare ancora.
- Gli errori di lettura dicono file, riga e cosa non va.

## Test richiesti

- Lettura e scrittura di ogni gruppo delle due tabelle e delle entità; stati
  dei layer; RGB con e senza 62; tipi complessi conservati.
- Giro completo (lettura, scrittura, rilettura) uguale con disegni casuali
  che hanno layer, tipi e proprietà.
- Ogni file incoerente del punto 6, con il suo avviso.
- `.cadx` con layer e proprietà.
- Disegno nuovo salvato in DXF: gli handle dello scheletro e dei record di
  partenza stanno tutti tra 1 e 256, sono gli stessi con 0, 1 o 100 entità, e
  le entità partono da 257; riaperto e salvato di nuovo, il file è identico.

## Fatto quando

- I casi di `[0.6-R10]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`; i casi `roundtrip` della 0.5 passano ancora.
- I DXF scritti dai casi passano `ezdxf audit`.
- La CI è verde.
