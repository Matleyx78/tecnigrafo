# [0.5-P10] `io`: lettura e scrittura conservative del DXF

- **Tappa:** 0.5 — [piano](../0.5-file-e-dxf.md)
- **Issue:** #164
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.5-R10]` (#163), agente gemini
- **Dipende da:** P8, R10
- **Modulo:** `io`
- **Da leggere:** [ADR 0017](../../adr/0017-modulo-io-e-dxf.md) punti 2, 5, 6,
  [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md) (tutto),
  `io/include/tg/io/io.hpp` e `dxf_groups.hpp` (P4),
  `core/include/tg/core/raw_drawing.hpp` e `document.hpp` (P1),
  `docs/verifica/domande.md` sezione "File" (P8); la *DXF Reference* di
  Autodesk, capitoli "HEADER Section", "TABLES Section" (`BLOCK_RECORD`) e
  "ENTITIES Section" (gruppi comuni)

## File

- `io/src/dxf_read.cpp` (nuovo)
- `io/src/dxf_write.cpp` (nuovo)
- `io/src/dxf_entities.hpp` (nuovo, interno)
- `io/src/dxf_entities.cpp` (nuovo: la tabella che smista le entità, vuota)
- `io/src/dxf_skeleton.cpp` (nuovo: lo scheletro R2018 dei disegni nuovi)
- `io/src/io.cpp` (tolte le versioni provvisorie di `readDxf` e `writeDxf`)
- `io/CMakeLists.txt`
- `tests/io/dxf_read_test.cpp` (nuovo)
- `tests/io/dxf_write_test.cpp` (nuovo)
- `tests/io/dati/` (nuova: pochi DXF scritti a mano per i test)
- `tests/CMakeLists.txt` (righe dei test nuovi)
- `tests/verifica/in-attesa.txt` (togliere le righe di R10)

## Cosa fare

`readDxf` e `writeDxf` funzionano davvero, ma **senza ancora gestire alcuna
entità**: dopo questo pacchetto ogni entità è conservata come non gestita
(ADR 0018, punto 3). P11 e P12 aggiungono le entità alla tabella.

### Lettura (`readDxf`)

1. `readGroups` (P4), poi la divisione in sezioni (`0 SECTION` / `2 nome` …
   `0 ENDSEC`, poi `0 EOF`). Sezioni annidate, `ENDSEC` mancante, gruppi dopo
   `EOF`: errore `Malformed`.
2. `core::RawDrawing`: versione, code page, tutte le sezioni nell'ordine del
   file, con `HEADER` **intera** (le variabili rappresentate si
   sostituiscono in scrittura, non si tolgono) e `ENTITIES` come segnaposto
   senza gruppi.
3. Unità: `$INSUNITS` se c'è (fuori da 0–24: errore), altrimenti 0.
4. Handle: ogni gruppo 5 (e 105 di `DIMSTYLE`) fuori da `ENTITIES` si
   riserva con `reserveHandle`; `setNextId` da `$HANDSEED` o dall'handle più
   alto più uno. Handle doppio: errore `Malformed` con la riga.
5. `ENTITIES`: ogni entità (da un `0` al successivo) riceve la chiave
   d'ordine della sua posizione. Si chiede alla tabella delle entità
   (`dxf_entities`) se il suo tipo è gestito **e** l'entità sta nello spazio
   modello (67 assente o 0, 330 assente o uguale all'handle di
   `*Model_Space` in `BLOCK_RECORD`) **e** l'estrusione è predefinita: solo
   allora la converte il lettore del tipo; altrimenti diventa una
   `core::RawEntity`. Un'entità senza handle ne riceve uno nuovo, con un
   avviso.
6. `ReadReport.unhandled` conta le entità non gestite per tipo.

### La tabella delle entità (`dxf_entities.hpp/.cpp`)

```cpp
struct EntityCodec {
    std::string_view type;  ///< "LINE"
    /// Converts the groups of one entity (after group 0) into a record;
    /// std::nullopt keeps it unhandled (a degenerate entity, docs/adr/0018).
    std::optional<core::EntityRecord> (*read)(const core::RawGroups& groups, EntityReadContext&);
    void (*write)(const core::EntityRecord& record, EntityWriteContext&, core::RawGroups& out);
};
[[nodiscard]] const EntityCodec* findCodec(std::string_view type);
[[nodiscard]] const EntityCodec* findCodec(const core::Entity& entity);
```

In questo pacchetto la tabella è vuota. I contesti portano la versione, la
code page e l'handle di `*Model_Space`; P11 e P12 aggiungono una riga
ciascuno per tipo, in ordine alfabetico.

### Scrittura (`writeDxf`)

- Disegno nuovo (`RawDrawing` vuoto): si parte dallo **scheletro R2018** di
  `dxf_skeleton.cpp`, un DXF minimo e valido scritto da noi (tabelle `VPORT`,
  `LTYPE` con `ByBlock`, `ByLayer`, `Continuous`, `LAYER` con `0`, `STYLE`
  con `Standard`, `VIEW`, `UCS`, `APPID` con `ACAD`, `DIMSTYLE` con
  `Standard`, `BLOCK_RECORD` con `*Model_Space` e `*Paper_Space`; blocchi
  corrispondenti; `OBJECTS` con dizionario radice, `ACAD_GROUP`,
  `ACAD_LAYOUT` con i layout `Model` e `Layout1`).
- Sezioni scritte nell'ordine in cui sono, uguali. Nell'`HEADER` si
  sostituiscono, al loro posto o aggiunte se mancano: `$ACADVER` (la
  versione scritta), `$DWGCODEPAGE`, `$HANDSEED` (`nextId()` in
  esadecimale), `$INSUNITS`.
- `ENTITIES`: tutte le entità, gestite e non, **in ordine di chiave**; le non
  gestite esattamente come lette; le gestite con il `write` della loro riga
  (nessuna, in questo pacchetto).
- Versione (ADR 0017, punto 2): `writeDxf(doc, v)` con `v` uguale alla
  versione letta, o `R2018` per un disegno nuovo o per la conversione; una
  versione **più vecchia** di quella letta dà `Unsupported`. Convertendo a
  R2018 cambiano `$ACADVER` e la codifica (UTF-8); il `ReadReport` non c'è
  in scrittura, quindi un'eventuale perdita si segnala con un errore
  `Unsupported` che dice cosa non si può convertire.
- Codifica: UTF-8 da `AC1021`, altrimenti la code page letta.

## Regole della tappa che valgono qui

- Ciò che non è gestito si riscrive **identico** (ADR 0018, punto 6): è il
  criterio di chiusura che R10 verifica con `roundtrip`.
- `io/src/dxf_entities.cpp` è un file comune: P11 e P12 vi aggiungono righe.
- Nessun file fuori da `io`, dai suoi test e da `in-attesa.txt`.

## Test richiesti

- Lettura e scrittura dei DXF di `tests/io/dati/`: gruppi riscritti uguali,
  tranne `$HANDSEED`.
- Handle: riservati, doppi (errore), mancanti (assegnati, avviso), nuova
  entità aggiunta dopo la lettura con un handle mai usato.
- Spazio carta e owner diverso da `*Model_Space`: non gestite anche quando il
  tipo sarà gestito (provato con una riga di prova nella tabella, solo nel
  test).
- Disegno nuovo: lo scheletro si rilegge senza errori ed è R2018.
- Conversione da R2000 a R2018: `$ACADVER`, codifica, testo accentato.
- Versione più vecchia richiesta: errore.
- File troncati e sezioni malformate: sempre un errore, mai un crash.

## Fatto quando

- I casi di `[0.5-R10]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- Un DXF con entità e oggetti non gestiti, aperto e salvato, li contiene
  ancora identici (`roundtrip`).
- Lo scheletro R2018 si rilegge senza errori né avvisi. Il controllo con
  `ezdxf audit` arriva con il primo caso che salva un disegno nuovo (R15),
  e la pull request dice come lo hai provato a mano, se puoi.
- La CI è verde, compreso il fuzzing.
