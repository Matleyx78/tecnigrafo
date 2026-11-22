# [0.8.1-PT27] `core` e `io`: griglia, snap, `UCSICON` e `ATTDISP` nel disegno

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8.1-RT27]` (#…), agente codex
- **Dipende da:** PT1, PT2, PT5, RT27
- **Modulo:** `core` e `io` (più le risposte `grid` e `attdisp` e il campo
  `attdisp:` di `script`)
- **Da leggere:** [ADR 0055](../../adr/0055-griglia-snap-alla-griglia-icona-ucs.md),
  punti 1 e 2; [ADR 0056](../../adr/0056-comandi-degli-attributi.md), punto 1;
  [ADR 0006](../../adr/0006-modello-dati-dwg-e-formato-cadx.md);
  `core/include/tg/core/document.hpp`, `io/src/dxf_tables.cpp`,
  `io/src/dxf_header.cpp`, `io/src/dxf_skeleton.cpp`, `io/src/cadx.cpp`,
  `core/src/insert.cpp` (gli attributi invisibili)

## File

- `core/include/tg/core/drawing_settings.hpp` (nuovo: `GridSettings`,
  `AttributeDisplay`)
- `core/include/tg/core/document.hpp`, `core/src/document.cpp`
- `core/src/insert.cpp` (gli attributi secondo `AttributeDisplay`)
- `io/src/dxf_tables.cpp`, `io/src/dxf_header.cpp`, `io/src/dxf_skeleton.cpp`,
  `io/src/cadx.cpp`
- `script/src/file_questions.cpp`, `script/src/report.cpp`,
  `script/src/question_table.cpp` (file comune: le righe di `grid` e `attdisp`)
- `tests/core/document_test.cpp`, `tests/io/dxf_tables_test.cpp`,
  `tests/io/dxf_read_test.cpp`, `tests/io/dxf_write_test.cpp`,
  `tests/io/cadx_test.cpp`

## Cosa fare

1. ```cpp
   struct GridSettings {
       bool gridOn{true};
       Vec2 gridUnit{10, 10};   // 0,0: the snap spacing
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
2. DXF come l'ADR 0055, punto 2: si legge la prima voce `*ACTIVE` della
   `VPORT`; in scrittura si cambiano solo i gruppi dei campi, e un gruppo il
   cui valore non è cambiato resta **il testo letto**, così una riscrittura
   senza modifiche è identica a quella di oggi. Un disegno nuovo scrive la
   `VPORT` con i valori di `acadiso.dwt`. `$ATTMODE` nell'intestazione, scritto
   come oggi se il valore non cambia.
3. Formato nativo: i campi con i nomi delle variabili.
4. `AttributeDisplay::On` disegna anche gli attributi invisibili, `Off`
   nessuno; selezione e snap seguono. Gli `ATTDEF` nel disegno non cambiano.
5. `grid D` e `attdisp D` (annunciate da PT2) e il campo `attdisp:` del
   rapporto, dopo `lwdisplay:`, solo se non è `Normal`.

## Regole della tappa che valgono qui

- File comuni: `script/src/question_table.cpp`.

## Test richiesti

- Lettura e scrittura di ognuno dei file `autocad-griglia-*`, `-ucsicon-*`,
  `-snap-*` e `-attdisp-*`; un DXF senza `VPORT`; una `VPORT` con due voci
  `*Active`; la riscrittura identica; il formato nativo.

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
- **Risposte condivise:** nessuna. `roundtrip` dei casi di oggi resta
  `identical` (punto 2); `attdisp:` manca per `$ATTMODE` 1, che è quello di
  tutti i DXF usati dai casi di oggi (controllato su `dxf-test-file`: solo i
  due `autocad-attdisp-*` hanno 0 e 2); `grid snap:` è di PT28.
