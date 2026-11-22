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
- `tests/core/entity_properties_test.cpp`, `tests/core/hatch_test.cpp`,
  `tests/io/dxf_hatch_test.cpp` (salvataggio e riapertura dell'origine)

## Cosa fare

1. Origine e unità non stanno nel `Hatch`: l'origine è nell'XDATA `ACAD`
   1010/1020 dell'`EntityRecord`, e il passo di `NET` dipende da
   `$MEASUREMENT` (3,175 o 0,125), che oggi `-HATCH` passa esplicitamente a
   `predefinedPatternLines`. Le funzioni le ricevono tutte e due (verbale della
   revisione delle interfacce PT4, #927, [bloccante] 7):

   ```cpp
   // The origin of the pattern: the ACAD XDATA 1010/1020 of `extras`, 0,0 when
   // missing (AutoCAD's HPORIGIN of a new drawing).
   [[nodiscard]] Vec2 patternOrigin(const EntityExtras& extras);
   // The families of `hatch`, whose origin is `from`, with origin `to`: every
   // base point (43/44) moves by to - from, generated pattern or read from the
   // file alike (docs/REGOLE-NUMERICHE.md, section 2, point 9: the families
   // stand relative to the origin). Offsets and dashes do not change.
   [[nodiscard]] Hatch withPatternOrigin(const Hatch& hatch, const Vec2& from, const Vec2& to);
   // The ACAD XDATA 1010/1020 of `extras` set to `origin`, added when missing
   // (docs/tappe/0.8.1/dxf-di-prova.md); the other groups untouched.
   [[nodiscard]] EntityExtras withPatternOriginXdata(EntityExtras extras, const Vec2& origin);
   // The hatch with another predefined pattern, scale or angle, its origin
   // being `origin` and its drawing's $MEASUREMENT `measurement`: the families
   // of predefinedPatternLines(name, scale, angle, measurement) with every base
   // point moved by `origin`. With the hatch's own pattern name when
   // predefinedPatternLines() cannot generate it, the families of the file
   // transformed in proportion to the new scale and angle around `origin`
   // (docs/REGOLE-NUMERICHE.md, section 2, point 9). nullopt for any other
   // name that cannot be generated (today ANSI31 and NET; SOLID gives a solid
   // fill) and for a scale, old or new, that is not finite and positive.
   [[nodiscard]] std::optional<Hatch> withPattern(const Hatch& hatch, const Vec2& origin,
                                                  std::string_view name, double scale,
                                                  double angle, int measurement);
   ```

   Esempi. Origine `(10,0)` portata a `(20,0)`, con due famiglie di un motivo
   del file di base `(13,0)` e `(17,0)`: le basi diventano `(23,0)` e `(27,0)`,
   non entrambe `(20,0)` (#927, [decisione] 6, applicazione della regola D10).
   Origine `(10,0)`, base di famiglia `(13,0)`, scala da 1 a 2: la base nuova è
   `(16,0)`, non `(26,0)` come con l'origine `(0,0)`. Senza XDATA l'origine è
   `(0,0)`. `withPattern` con `NET` in un disegno con `$MEASUREMENT` 1 dà il
   passo di `acadiso.pat` (3,175 per scala 1), con 0 quello di `acad.pat`
   (0,125).
2. **`HATCH`** (`pattern`, `scale`, `angle`, `origin`, `islands`, `~area`): i
   primi tre con `withPattern`, quindi `pattern` accetta solo i motivi che
   `predefinedPatternLines` genera (`ANSI31`, `NET`) e `SOLID`, come la scelta
   del motivo di `-HATCH`; un altro nome è rifiutato. `scale` e `angle` di un
   retino con un motivo letto dal DXF ma non generabile trasformano le sue
   famiglie in proporzione, con lo stesso `withPattern`
   ([REGOLE-NUMERICHE.md](../../REGOLE-NUMERICHE.md), sezione 2, punto 9);
   `origin` con `withPatternOrigin` (da `patternOrigin` degli extras) e
   `withPatternOriginXdata`, che vanno nella `PropertyChange` insieme;
   `islands` cambia lo stile; `~area` è l'area della regione piena (pari e
   dispari) con lo stile delle isole. `SOLID` come nome dà un riempimento pieno.
3. **`DIMENSION`** (`style`, `text`, `text-rotation`, `~measurement`): lo stile
   deve esistere; `text` è il testo sostitutivo (`""` torna alla misura);
   `text-rotation` è il gruppo 53; la grafica si rigenera come quando la quota
   nasce da un comando, con le regole della 0.8. Una quota che non si rigenera
   (tipo *altro*, variabili del punto 2 dell'ADR 0047, sovrascritture grafiche
   conservate) **rifiuta** la modifica di `style`, `text` e `text-rotation` con
   `Dimension cannot be regenerated by Tecnigrafo.` e resta com'era, entità e
   grafica (ADR 0047, punto 4, «Non rigenerabile»; #927, [decisione] 7,
   applicazione della regola): niente gruppo 1 nuovo con la grafica vecchia.
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
  un retino con un motivo non generabile (famiglie in proporzione, un quarto
  di giro esatto); l'area di un rettangolo con un foro; una quota con lo
  stile cambiato e una con il testo `<> mm`.
- Origine con due famiglie del file (le basi `(13,0)` e `(17,0)` con l'origine
  da `(10,0)` a `(20,0)` diventano `(23,0)` e `(27,0)`); scala da 1 a 2 con
  l'origine `(10,0)` (la base `(13,0)` diventa `(16,0)`); `NET` con
  `$MEASUREMENT` 1 e 0; un retino senza XDATA (origine `(0,0)`).
- L'origine cambiata e salvata: il DXF scritto con `io` e riletto ha l'XDATA
  e i 43/44 nuovi, gli altri gruppi intatti (#927, [bloccante] 6).
- Una quota non rigenerabile (`kind=other`) con il testo da «A» a «B»:
  rifiutata con `Dimension cannot be regenerated by Tecnigrafo.`, entità e
  grafica uguali a prima.

## Fatto quando

- I casi di RT23 passano; la CI è verde.

## Prova a secco dei riferimenti

```text
# casi/proprieta-proprie/retini-quote/domande.tgq
H = HATCH pattern="ANSI31" solid=no angle=0 scale=1 islands=normal loop=external+polyline vertex=0,0 vertex=40,0 vertex=40,20 vertex=0,20
? properties H -> …
? pattern-lines H -> …
? set-property H origin=245,5 -> …
? set-property H pattern="NET" -> …
? set-property H pattern="ANSI37" -> invalid
? set-property H pattern="NONESISTE" -> invalid

; oracle: ezdxf (in un caso a sé)
D = DXF source=autocad-hatchedit
? entity D <handle del retino con l'origine 245,5> -> …
```

- **Strumenti:** `properties`, `set-property`, `pattern-lines` dell'onda 0 (PT2);
  `entity` dell'oracolo per i retini di AutoCAD. `pattern-lines H` si chiede sul
  retino com'è: `set-property` non modifica il documento delle domande, e le
  famiglie dopo il cambio d'origine si provano nei test di unità, contro quelle
  del retino di AutoCAD (#927, [bloccante] 6). RT23 aggiunge `autocad-hatchedit`
  a `sorgenti.json`.
- **Dipendenze:** nessun comando.
- **Fonte:** `autocad-hatchedit.dxf` (origine nell'XDATA e nei 43/44). Il
  motivo nuovo è `NET`, non `ANSI37` del DXF: su `main` si generano solo
  `ANSI31` e `NET` (verbale della revisione del piano, [bloccante] 4).
- **Risposte condivise:** nessuna: `pattern-lines` è nuova; `origin=` manca
  per l'origine 0,0, che è quella di tutti i retini dei DXF di oggi
  (controllato su `dxf-test-file`) e di quelli fatti con `-HATCH`.
