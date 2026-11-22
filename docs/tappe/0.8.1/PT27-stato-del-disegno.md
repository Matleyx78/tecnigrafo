# [0.8.1-PT27] `core` e `io`: griglia, snap, `UCSICON` e `ATTDISP` nel disegno

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8.1-RT27]` (#…), agente codex
- **Dipende da:** PT1, PT2, PT5, RT27
- **Modulo:** `core` e `io` (più i due setter di `edit` che segnano il disegno
  modificato, le risposte `grid` e `attdisp` e il campo `attdisp:` di
  `script`)
- **Da leggere:** [ADR 0055](../../adr/0055-griglia-snap-alla-griglia-icona-ucs.md),
  punti 1 e 2; [ADR 0056](../../adr/0056-comandi-degli-attributi.md), punto 1;
  [ADR 0006](../../adr/0006-modello-dati-dwg-e-formato-cadx.md);
  `core/include/tg/core/document.hpp`, `io/src/dxf_tables.cpp`,
  `io/src/dxf_header.cpp`, `io/src/dxf_skeleton.cpp`, `io/src/cadx.cpp`,
  `io/src/dxf_write.cpp`, `core/src/insert.cpp` (gli attributi invisibili),
  `core/src/attribute_definition.cpp` (la geometria dell'`ATTDEF`),
  `edit/src/document_tables.cpp` (`setLwdisplay`), `edit/src/editor.cpp`
  (`isModified`, `markSaved`, `replaceDocument`)

## File

- `core/include/tg/core/drawing_settings.hpp` (nuovo: `GridSettings`,
  `AttributeDisplay`)
- `core/include/tg/core/document.hpp`, `core/src/document.cpp`
- `core/src/insert.cpp` (gli attributi secondo `AttributeDisplay`)
- `core/src/attribute_definition.cpp` (l'`ATTDEF` nel disegno si disegna
  sempre)
- `io/src/dxf_tables.cpp`, `io/src/dxf_header.cpp`, `io/src/dxf_skeleton.cpp`,
  `io/src/dxf_write.cpp`, `io/src/cadx.cpp`
- `edit/include/tg/edit/document_tables.hpp`, `edit/src/document_tables.cpp`,
  `edit/include/tg/edit/editor.hpp`, `edit/src/editor.cpp` (`setGrid`,
  `setAttributeDisplay` e lo stato salvato)
- `script/src/file_questions.cpp`, `script/src/report.cpp`,
  `script/src/question_table.cpp` (file comune: le righe di `grid` e `attdisp`)
- `tests/core/document_test.cpp`, `tests/io/dxf_tables_test.cpp`,
  `tests/io/dxf_read_test.cpp`, `tests/io/dxf_write_test.cpp`,
  `tests/io/cadx_test.cpp`, `tests/edit/editor_test.cpp`,
  `tests/core/attributes_test.cpp`

## Cosa fare

1. ```cpp
   struct GridSettings {
       bool gridOn{true};
       Vec2 gridUnit{10, 10};   // >= 0; an axis at 0 has the snap spacing
       bool snapOn{false};
       Vec2 snapUnit{10, 10};   // > 0
       Vec2 snapBase{0, 0};
       int gridDisplay{3};      // bits 2 and 4 used, the others kept
       int gridMajor{5};        // 1..100
       int ucsIcon{3};          // bit 1 on, bit 2 at the origin
       double snapAngle{0};     // degrees, kept
       int snapStyle{0};        // kept
       int snapIsoPair{0};      // kept
   };
   enum class AttributeDisplay : std::uint8_t { Off, Normal, On };  // $ATTMODE 0, 1, 2
   ```
   `Document::grid()`, `setGrid()`, `attributeDisplay()`,
   `setAttributeDisplay()`, con i valori di un disegno nuovo qui sopra.
   `gridUnit` vale **per asse**: un asse a 0 ha il passo dello snap di
   quell'asse, così `0,0` è il caso di tutti e due e `0,10` ha in X il passo
   dello snap e in Y 10 (decisione della revisione PT5, [decisione] 2); il
   valore si conserva com'è scritto. La regola è la stessa per i comandi
   (PT28), la lettura del DXF e il setter, e `render` la applica allo stesso
   modo (PT29).
2. **Stato modificato** (verbale della revisione PT5, [bloccante] 1). In
   `edit::DocumentTables`, sul modello di `setLwdisplay()`, `setGrid(const
   core::GridSettings&)` e `setAttributeDisplay(core::AttributeDisplay)`:
   cambiano il `Document` dell'`Editor` senza un passo di annullamento e lo
   notificano; `Editor::isModified()` confronta anche griglia e `ATTDISP` con
   quelli salvati, che `markSaved()` e `replaceDocument()` registrano (così un
   disegno appena aperto non è modificato). Sono l'unica via per i comandi
   (PT18, PT28) e per l'interfaccia (PT32).
3. DXF come l'ADR 0055, punto 2: si legge la prima voce `*ACTIVE` della
   `VPORT`; in scrittura si cambiano solo i gruppi dei campi, e un gruppo il
   cui valore non è cambiato resta **il testo letto**, così una riscrittura
   senza modifiche è identica a quella di oggi. Un disegno nuovo scrive la
   `VPORT` con i valori di `acadiso.dwt`. **Senza `*ACTIVE`** (nessuna `VPORT`,
   o solo altre voci come `*ALT`) e con uno stato diverso da quello di un
   disegno nuovo, la scrittura aggiunge una voce `*ACTIVE`, e la tabella
   `VPORT` se manca, con handle nuovi presi come quelli delle entità nuove; le
   altre voci restano intatte; con lo stato di un disegno nuovo il file resta
   identico a quello letto ([bloccante] 2). `$ATTMODE` nell'intestazione,
   scritto come oggi se il valore non cambia.
4. Formato nativo: i campi con i nomi delle variabili.
5. `AttributeDisplay::On` disegna anche gli attributi invisibili, `Off`
   nessuno; selezione e snap seguono. Vale solo per gli `ATTRIB` degli
   inserimenti: un `ATTDEF` nel disegno (fuori dai blocchi) **si disegna
   sempre**, anche con il segnale *Invisible* e con `ATTDISP` *OFF*, come
   AutoCAD (ADR 0056, punto 1), e clic e snap lo trovano; oggi `generateOf`
   lo nasconde con *Invisible*, e qui si corregge ([bloccante] 3). Gli `ATTDEF`
   dentro le definizioni restano non disegnati (0.8-PT17, punto 5).
6. `grid D` e `attdisp D` (annunciate da PT2) e il campo `attdisp:` del
   rapporto, dopo `lwdisplay:`, solo se non è `Normal`.

## Regole della tappa che valgono qui

- File comuni: `script/src/question_table.cpp`.
- `edit/src/editor.cpp` e `edit/src/document_tables.cpp` in questa tappa li
  cambia solo questo pacchetto.

## Test richiesti

- Lettura e scrittura di ognuno dei file `autocad-griglia-*`, `-ucsicon-*`,
  `-snap-*` e `-attdisp-*`; un DXF senza `VPORT`; una `VPORT` con due voci
  `*Active`; la riscrittura identica; il formato nativo.
- **Stato modificato** ([bloccante] 1): due `Editor` con due disegni appena
  salvati; `setGrid` con `snapOn` acceso su A segna modificato **solo** A,
  senza un passo di annullamento (`U` non lo tocca); `markSaved` lo pulisce;
  `replaceDocument` con un disegno letto lo lascia pulito; lo stesso con
  `setAttributeDisplay`. Salvataggio in DXF e in formato nativo (che passa da
  `drawing.dxf`) e riapertura con `SNAPMODE` 1 e `ATTDISP` *ON*.
- **DXF senza `*ACTIVE`** ([bloccante] 2): un DXF senza `VPORT` e uno con la
  sola voce `*ALT`; si legge lo stato di un disegno nuovo, si mette `SNAPMODE`
  1, si riscrive nella stessa versione e si riapre: `SNAPMODE` 1, voce
  `*ACTIVE` con un handle nuovo e unico, `*ALT` intatta. Senza modifiche la
  riscrittura resta identica.
- **`ATTDEF` nel disegno** ([bloccante] 3): un `ATTDEF` fuori dai blocchi con
  *Invisible*, su un layer acceso, con `ATTDISP` *OFF*, *Normal* e *ON*: si
  disegna e il clic lo sceglie in tutti e tre; un `ATTRIB` invisibile dello
  stesso disegno segue `ATTDISP`.
- **`GRIDUNIT` per asse** ([decisione] 2): `0,10` letto da un DXF resta
  `0,10` e si riscrive uguale.

## Fatto quando

- I casi di RT27 passano; la CI è verde, con il passo dell'oracolo.

## Prova a secco dei riferimenti

```text
; oracle: ezdxf
# casi/dxf/vport-griglia/domande.tgq
D = DXF source=autocad-griglia-passi
? grid D -> …
? roundtrip D -> identical
E = DXF source=autocad-ucsicon-noorigin
? grid E -> …
F = DXF source=autocad-snap-iso
? grid F -> …
A = DXF source=autocad-attdisp-on
? attdisp A -> on

# casi/attributi/attdisp-snap/script.tgs
OPEN
@sources/autocad-attdisp-on
!view 0,0 0.1 800x600
!osnap ins
LINE
!move <posizione dell'attributo invisibile>
!report
!esc
```

Righe da controllare nello script: `attdisp: on` e `active snap: Insertion …`
sull'attributo invisibile; lo stesso script con `autocad-attdisp-off` dà
`attdisp: off` e `active snap: none`.

- **Strumenti:** `grid`, `attdisp` e `attdisp:` dell'onda 0 (PT2), con le
  risposte di ezdxf di PT1; `roundtrip`, `!osnap`, `!move` su `main`. RT27
  aggiunge i file a `sorgenti.json`.
- **Dipendenze:** nessun comando nuovo.
- **Fonte:** i DXF di prova della 0.8.1, con la tabella dei gruppi in
  [dxf-di-prova.md](dxf-di-prova.md).
- **Risposte condivise:** l'`ATTDEF` invisibile fuori dai blocchi
  (punto 5) cambia il caso della 0.8
  `attributi/attdef-invisibile-proprieta`: per l'`ATTDEF` `I` le risposte
  `generated`, `pick`, `crossing`, `window` e `snap` non sono più `none`
  (resta `none` per `F`, sul layer congelato). Le riallinea **RT27**, che
  aggiorna quel caso insieme ai suoi (verbale della revisione della
  correzione #992, [bloccante] 1): finché RT27 non è unito PT27 non parte, e
  il caso non è mai rosso. `roundtrip` dei casi di oggi resta
  `identical` (punto 2); `attdisp:` manca per `$ATTMODE` 1, che è quello di
  tutti i DXF usati dai casi di oggi (controllato su `dxf-test-file`: solo i
  due `autocad-attdisp-*` hanno 0 e 2); `grid snap:` è di PT28.
