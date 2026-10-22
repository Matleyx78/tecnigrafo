# [0.5-P5] `edit`: modifiche non salvate, unità, documento nuovo

- **Tappa:** 0.5 — [piano](../0.5-file-e-dxf.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: si vede solo con i comandi dei file; i casi
  sono in R15 e R16
- **Dipende da:** P1
- **Modulo:** `edit`
- **Da leggere:** [ADR 0021](../../adr/0021-comandi-dei-file.md) punti 1 e 3,
  [ADR 0005](../../adr/0005-comandi-e-annullamento.md) (transazioni);
  `edit/include/tg/edit/editor.hpp`, `edit/src/editor.cpp`,
  `core/include/tg/core/document.hpp` (dopo P1: `units()`, `setUnits()`)

## File

- `edit/include/tg/edit/editor.hpp`
- `edit/src/editor.cpp`
- `tests/edit/editor_test.cpp`

## Cosa fare

L'editor impara tre cose che servono ai comandi dei file.

### Unità come modifica annullabile

```cpp
/// Changes INSUNITS (docs/adr/0021). Returns false, changing nothing, if
/// the value is already `units`.
/// Preconditions: a transaction is open; 0 <= units <= 24.
bool setUnits(int units);
```

È un nuovo tipo di `Change` (oltre ad `Added` e `Removed`), che ricorda il
valore precedente: `undo` lo rimette, `redo` lo riapplica.

### Modifiche non salvate e nome del disegno

```cpp
/// True if the document differs from the last save or load: the undo
/// stack is not where it was when markSaved() or replaceDocument() was
/// last called. Undoing back to that point clears it, as in AutoCAD.
[[nodiscard]] bool isModified() const;

/// Records that the document has just been saved as `name` (UTF-8).
void markSaved(std::string name);

/// The file name of the drawing, "Drawing1.cadx" before the first save.
[[nodiscard]] const std::string& fileName() const;

/// True once the drawing has been saved or loaded under a name.
[[nodiscard]] bool hasFile() const;
```

- La posizione salvata si confronta in modo che "annulla, poi rifai una
  modifica diversa" non sembri il disegno salvato: dopo `commit` che svuota la
  pila di ripetizione oltre il punto salvato, il punto salvato non è più
  raggiungibile e `isModified()` resta vero.
- Una transazione vuota (`commit` senza modifiche) non cambia `isModified()`.

### Disegno nuovo o aperto

```cpp
/// Replaces the whole document (NEW, OPEN): empties both stacks, forgets
/// the saved point, and sets the name: `name` for an opened file, or
/// "Drawing1.cadx" with hasFile() false when `name` is empty.
/// Precondition: no transaction is open.
void replaceDocument(core::Document document, std::string name);
```

L'editor non possiede il documento: `replaceDocument` lo assegna al documento
che già referenzia (per spostamento), così chi tiene il riferimento (vista,
comandi) vede quello nuovo. Il listener si chiama una volta, alla fine.

## Regole della tappa che valgono qui

- Il nome predefinito è `Drawing1.cadx` (ADR 0021, punto 1); non c'è un
  contatore di disegni.
- Nessun file fuori da `edit` e dai suoi test.

## Test richiesti

- `setUnits` dentro una transazione; `undo` e `redo`; stesso valore: nessun
  passo d'annullamento.
- `isModified`: falso all'inizio; vero dopo una modifica; falso dopo
  `markSaved`; vero dopo un'altra modifica; falso dopo `undo` fino al punto
  salvato; vero dopo `undo` oltre il punto salvato; vero dopo "annulla e
  modifica diversa"; invariato da una transazione vuota.
- `replaceDocument`: pile vuote, `isModified` falso, nome e `hasFile` giusti
  nei due casi, entità del nuovo documento visibili dal riferimento.
- `fileName` dopo `markSaved` con un nome con lettere accentate.

## Fatto quando

- I test di `edit` passano, vecchi e nuovi, e la CI è verde.
- Le interfacce sono quelle scritte sopra, con gli stessi nomi.
