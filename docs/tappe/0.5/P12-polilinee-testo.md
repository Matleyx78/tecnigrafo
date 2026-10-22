# [0.5-P12] `io`: polilinee e testo

- **Tappa:** 0.5 — [piano](../0.5-file-e-dxf.md)
- **Issue:** #167
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.5-R12]` (#166), agente gemini
- **Dipende da:** P10, R12
- **Modulo:** `io` (più le righe delle larghezze in `script`)
- **Da leggere:** [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md)
  punti 3, 4 e 6, [ADR 0017](../../adr/0017-modulo-io-e-dxf.md) punti 3–5,
  [ADR 0013](../../adr/0013-testo-e-font.md) punto 1 (proprietà di `Text`);
  `io/src/dxf_entities.hpp` e `dxf_read.cpp` (P10),
  `core/include/tg/core/entity_extras.hpp`, `polyline.hpp`, `text.hpp`;
  `docs/verifica/rapporto.md` (proprietà nuove, P8); la *DXF Reference*,
  "LWPOLYLINE", "TEXT" e "Common Group Codes for Entities"

## File

- `io/src/dxf_entity_common.hpp` (nuovo, interno)
- `io/src/dxf_entity_common.cpp` (nuovo)
- `io/src/dxf_lwpolyline.cpp` (nuovo)
- `io/src/dxf_text.cpp` (nuovo)
- `io/src/dxf_entities.cpp` (due righe nella tabella)
- `io/CMakeLists.txt`
- `script/src/entity_reader.cpp` (proprietà `width`, `start-width`, `end-width`)
- `script/src/report.cpp` (le stesse in scrittura)
- `tests/io/dxf_lwpolyline_test.cpp` (nuovo)
- `tests/io/dxf_text_test.cpp` (nuovo)
- `tests/script/entity_reader_test.cpp`
- `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R12)

## Cosa fare

Le prime due entità gestite. Questo pacchetto scrive anche la parte comune a
tutte, che P11 riusa.

### Parte comune (`dxf_entity_common`)

Divide i gruppi di un'entità (dopo il gruppo 0) nelle parti di
`core::EntityExtras` e li ricompone in scrittura:

- handle (5); `beforeEntity`: i gruppi `102 {…` … `102 }` e 330, prima di
  `100 AcDbEntity`;
- `common`: i gruppi di `AcDbEntity` (8, 62, 6, 48, 370, 60, 420, 430, 440 e
  ogni altro), tranne 67, che ha già deciso il lettore;
- i gruppi della sottoclasse: quelli del modello li legge il lettore del
  tipo, `placement` raccoglie 30, 31, 38, 39, 210, 220, 230, `specific` tutti
  gli altri;
- `extended`: dal primo 1001 alla fine.

In scrittura: `5`, `beforeEntity` (per un'entità nuova: `330` con l'handle
di `*Model_Space`), `100 AcDbEntity`, `common` (per un'entità nuova: `8 0`),
poi i gruppi del tipo con `placement` al posto giusto (30 dopo 20, 31 dopo
21, 38 e 39 dove li vuole la specifica, 210–230 in fondo alla sottoclasse),
poi `specific`, poi `extended`. I numeri del modello con `formatNumber`, gli
angoli in gradi.

### `LWPOLYLINE`

- 90 (numero di vertici, deve coincidere con i vertici letti), 70 (bit 1:
  chiusa; gli altri bit si conservano e si riscrivono), 43 (larghezza
  costante), vertici 10/20 con 40, 41, 42 dopo il loro 10/20.
- Restano **non gestite**: zero vertici, 90 diverso dai vertici, gruppi 91
  (identificativi dei vertici), vertici con valori non validi.

### `TEXT`

- Tutte le proprietà di `Text` (ADR 0013, punto 1): 10/20, 40, 1, 50 (gradi),
  41 (predefinito 1), 51 (gradi), 7 (predefinito `Standard`), 71 (bit 2 e 4;
  gli altri bit si conservano), 72, 11/21 (se assenti, 0,0), e 73 dopo il
  secondo `100 AcDbText`.
- Il valore resta com'è scritto, codici di controllo compresi (`%%c`); la
  decodifica dei caratteri l'ha già fatta P4.
- Restano **non gestite**: altezza ≤ 0, fattore di larghezza ≤ 0, 72 o 73
  fuori intervallo.

Con estrusione diversa da `(0,0,1)` entrambe restano non gestite, come
decide P10: la conversione di `(0,0,-1)` arriva con P11.

### Modalità di controllo

`script/src/entity_reader.cpp` e `report.cpp` leggono e scrivono `width=`,
`start-width=` ed `end-width=` di `LWPOLYLINE`, con il formato già fissato
in `docs/verifica/rapporto.md` (P8).

## Regole della tappa che valgono qui

- Ciò che il modello non rappresenta si conserva (ADR 0018); un'entità che
  non si può rappresentare resta non gestita, mai scartata.
- `io/src/dxf_entities.cpp`, `io/CMakeLists.txt`, `tests/CMakeLists.txt`
  sono file comuni con P11: solo le tue righe.
- La parte comune serve anche a P11: interfaccia semplice e commentata.

## Test richiesti

- Andata e ritorno di polilinee e testi casuali (generatore con seme fisso,
  qualche centinaio di entità): modello uguale entro 1e-9, gruppi in più
  uguali.
- Polilinea: chiusa e aperta, bulge, larghezze, bit di 70 diversi da 1,
  casi non gestiti.
- Testo: tutte le proprietà, valori predefiniti assenti dal file, lettere
  accentate, `%%c`, casi non gestiti.
- Entità nuova (senza dati in più): gruppi scritti come quelli di AutoCAD per
  un'entità sul layer `0`.
- XDATA e reattori conservati.

## Fatto quando

- I casi di `[0.5-R12]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- I DXF pubblici con polilinee e testi si leggono con i valori di ezdxf
  (R12) e si riscrivono identici (`roundtrip`).
- La CI è verde, compreso il fuzzing.
