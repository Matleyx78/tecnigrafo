# [0.8.1-PT23] `core`: proprietà dei retini e delle quote, origine del motivo

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8.1-RT23]` (#…), agente codex, fascia forte
  (origine e righe del motivo)
- **Dipende da:** PT1, PT22, RT23
- **Modulo:** `core` (più la domanda `pattern-lines` di `script`)
- **Da leggere:** [ADR 0048](../../adr/0048-retini.md), punti 1–4;
  [ADR 0047](../../adr/0047-quote-e-stili-di-quota.md);
  `core/include/tg/core/hatch.hpp`, `core/include/tg/core/hatch_patterns.hpp`,
  `core/include/tg/core/dimension.hpp`; le schede sorelle
  [PT22](PT22-proprieta-core.md) e [PT24](PT24-proprieta-testi-inserimenti.md);
  `autocad-hatchedit.dxf` e la sua cronologia ([dxf-di-prova.md](dxf-di-prova.md),
  «Che cosa si è visto nei disegni»)

## File

- `core/src/entity_properties_annotations.cpp`
- `core/include/tg/core/hatch.hpp`, `core/src/hatch.cpp` (`withPatternOrigin`,
  `withPattern`)
- `script/src/hatch_questions.cpp`, `script/src/question_table.cpp` (file
  comune: la riga di `pattern-lines`), `script/src/new_entities_format.cpp`
  (`origin=` di `HATCH`)
- `tests/verifica/oracolo/ezdxf_attesi.py` (lo stesso campo)
- `tests/core/entity_properties_test.cpp`, `tests/core/hatch_test.cpp`

## Cosa fare

1. ```cpp
   // The pattern lines of `hatch` regenerated with base point `origin` (43/44
   // of every family), and the ACAD XDATA 1010/1020 of `extras` set to it,
   // added when missing (docs/tappe/0.8.1/dxf-di-prova.md).
   [[nodiscard]] Hatch withPatternOrigin(const Hatch& hatch, const Vec2& origin);
   [[nodiscard]] EntityExtras withPatternOriginXdata(EntityExtras extras, const Vec2& origin);
   // The hatch with another predefined pattern, scale or angle: the families
   // regenerated from the pattern, with the same origin; nullopt for a name
   // that predefinedPatternLines() cannot generate (today ANSI31 and NET;
   // SOLID gives a solid fill).
   [[nodiscard]] std::optional<Hatch> withPattern(const Hatch& hatch, std::string_view name,
                                                  double scale, double angle);
   ```
2. **`HATCH`** (`pattern`, `scale`, `angle`, `origin`, `islands`, `~area`): i
   primi tre con `withPattern`, quindi `pattern` accetta solo i motivi che
   `predefinedPatternLines` genera (`ANSI31`, `NET`) e `SOLID`, come la scelta
   del motivo di `-HATCH`; un altro nome è rifiutato. `scale` e `angle` di un
   retino con un motivo letto dal DXF ma non generabile seguono **[D10]**; `origin` con le due funzioni del punto 1;
   `islands` cambia lo stile; `~area` è l'area della regione piena (pari e
   dispari) con lo stile delle isole. `SOLID` come nome dà un riempimento pieno.
3. **`DIMENSION`** (`style`, `text`, `text-rotation`, `~measurement`): lo stile
   deve esistere; `text` è il testo sostitutivo (`""` torna alla misura);
   `text-rotation` è il gruppo 53; la grafica si rigenera come quando la quota
   nasce da un comando, con le regole della 0.8 (una quota che non si rigenera
   tiene la sua grafica).
4. `pattern-lines <H>` (annunciata da PT2), e il campo `origin=x,y` di
   `HATCH` nel rapporto e nella risposta di ezdxf (annunciato da PT2), scritto
   solo se l'XDATA dell'origine c'è ed è diversa da 0,0.

## Regole della tappa che valgono qui

- Le direzioni delle famiglie passano da `cardinalSinCos`
  ([REGOLE-NUMERICHE.md](../../REGOLE-NUMERICHE.md), sezione 2, punto 1); gli
  angoli digitati in gradi da `reducedDegreesToRadians` (punto 2).
- File comuni: `script/src/question_table.cpp`,
  `script/src/new_entities_format.cpp` e `tests/verifica/oracolo/ezdxf_attesi.py`
  (solo il campo `origin=`; gli altri campi li aggiungono PT8 e PT15).

## Test richiesti

- Origine, scala, angolo e stile dei retini di `autocad-hatchedit` (il cambio a
  `ANSI37` di quel DXF resta fuori: `ANSI37` non si genera); scala e angolo di
  un retino con un motivo non generabile, secondo [D10]; l'area di un rettangolo con un foro; una quota con lo
  stile cambiato e una con il testo `<> mm`.

## Fatto quando

- I casi di RT23 passano; la CI è verde.

## Prova a secco dei riferimenti

```text
# casi/proprieta-proprie/retini-quote/domande.tgq
H = HATCH pattern="ANSI31" solid=no angle=0 scale=1 islands=normal loop=external+polyline vertex=0,0 vertex=40,0 vertex=40,20 vertex=0,20
? properties H -> …
? set-property H origin=245,5 -> …
? pattern-lines H -> …
? set-property H pattern="NET" -> …
? set-property H pattern="ANSI37" -> invalid
? set-property H pattern="NONESISTE" -> invalid

; oracle: ezdxf (in un caso a sé)
D = DXF source=autocad-hatchedit
? entity D <handle del retino con l'origine 245,5> -> …
```

- **Strumenti:** `properties`, `set-property`, `pattern-lines` dell'onda 0
  (PT2); `entity` dell'oracolo per i retini di AutoCAD. RT23 aggiunge
  `autocad-hatchedit` a `sorgenti.json`.
- **Dipendenze:** nessun comando.
- **Fonte:** `autocad-hatchedit.dxf` (origine nell'XDATA e nei 43/44). Il
  motivo nuovo è `NET`, non `ANSI37` del DXF: su `main` si generano solo
  `ANSI31` e `NET` (verbale della revisione del piano, [bloccante] 4).
- **Risposte condivise:** nessuna: `pattern-lines` è nuova; `origin=` manca
  per l'origine 0,0, che è quella di tutti i retini dei DXF di oggi
  (controllato su `dxf-test-file`) e di quelli fatti con `-HATCH`.
