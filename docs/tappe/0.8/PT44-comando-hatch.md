# [0.8-PT44] `cmd`: `-HATCH`

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8-RT44]` (#…), agente claude: il comando
  (richieste, opzioni, messaggi, il retino creato: motivo, scala, angolo,
  stile delle isole, punti interni, numero dei contorni e loro segnali),
  dagli script eseguiti dal responsabile in AutoCAD LT e da
  `autocad-retini.dxf` letto con l'oracolo. La forma dei contorni la verifica
  RT43, le righe del motivo RT42: qui non si ripetono
- **Dipende da:** PT31, PT40 (revisione delle interfacce), PT42, PT43, RT44
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0048](../../adr/0048-retini.md), punti 2, 5 e 6; le
  schede [PT30](PT30-retini-core.md) (i motivi predefiniti, punto 2),
  [PT42](PT42-righe-dei-motivi.md) (`patternLines`, `kMaxHatchLines`) e
  [PT43](PT43-contorni-dei-retini.md) (`detectHatchBoundary`,
  `chosenHatchContours`, i segnali); `cmd/include/tg/cmd/command.hpp`
  (`CommandDefaults`); `cmd/src/commands/mirror_commands.cpp` (`MIRRTEXT`, una
  variabile come comando); un comando che chiede una selezione, per esempio
  `cmd/src/commands/offset_command.cpp`; la cronologia di `-HATCH` copiata dal
  responsabile in AutoCAD LT ([dxf-di-prova.md](dxf-di-prova.md))

## Intestazioni di solid-space che usa

Nessuna direttamente: il comando usa la facciata di `core` scritta da PT42 e
PT43 sul contratto rivisto in
[`[0.3-CS]` #222](https://github.com/Matleyx78/solid-space/issues/222), al
commit [`fd69e80`](https://github.com/Matleyx78/solid-space/commit/fd69e807d4ebafe12f73f46ba26072a46ff72809)
di `main` di solid-space (`region.hpp`, `boundary.hpp`, `clipping.hpp`). Il
sottomodulo non si sposta: lo hanno già portato PT42 e PT43.

## File

- `cmd/include/tg/cmd/commands/hatch_command.hpp`,
  `cmd/src/commands/hatch_command.cpp` (nuovi)
- `cmd/include/tg/cmd/command.hpp` (`CommandDefaults`: quattro campi)
- `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt` (file comuni)
- `tests/cmd/hatch_command_test.cpp` (nuovo), `tests/CMakeLists.txt` (file
  comune)
- `tests/verifica/in-attesa/0.8-RT44.txt` (da cancellare)

## Cosa fare

Richieste e messaggi dalla cronologia di AutoCAD LT; quelli attesi sono qui
sotto. Se la cronologia dice altro, vale la cronologia e la pull request lo
segnala.

### 1. I valori della sessione

In `CommandDefaults`, come `MIRRTEXT` (non si salvano nel disegno, come in
AutoCAD):

```cpp
/// HPNAME, HPSCALE, HPANG: the pattern of the next -HATCH, ANSI31, 1 and 0
/// at start-up (docs/adr/0048, point 6).
std::string hatchPattern{"ANSI31"};
double hatchScale{1.0};
double hatchAngle{0.0};  ///< radians
/// HPISLANDDETECTION: 1 (Outer) at start-up, as AutoCAD; 0 Normal, 2 Ignore.
core::IslandStyle hatchIslands{core::IslandStyle::Outer};
```

`HPISLANDDETECTION` è anche un comando, come `MIRRTEXT`:
`Enter new value for HPISLANDDETECTION <1>:`, valori 0, 1 e 2. È l'unico
modo di scegliere lo stile delle isole, perché l'opzione *Advanced* resta fuori
(ADR 0048, punto 6).

### 2. Il comando

`-HATCH`, alias `HATCH` (ADR 0048, punto 6):

1. `Specify internal point or [Properties/Select objects]:` (le altre opzioni
   di AutoCAD non compaiono nella richiesta).
   - **Un punto:** `detectHatchBoundary` (PT43). Con `Enclosed` i contorni si
     aggiungono a quelli del retino, salvo che il contorno esterno sia uguale,
     con gli stessi `double`, a un contorno esterno già preso: allora il punto
     non aggiunge nulla (un'area indicata due volte non si annulla con la
     regola pari-dispari). Il punto va nei punti interni (gruppo 98). Con
     `OnCurve` e `NotEnclosed`: il messaggio di AutoCAD
     (`Valid hatch boundary not found.`) e nulla si aggiunge. Poi la stessa
     richiesta.
   - ***Select objects*:** la selezione di sempre (`Select objects:`); con
     `Invio`, `chosenHatchContours` (PT43) sugli oggetti scelti. Ogni volta i
     contorni scelti **sostituiscono** quelli scelti prima, come AutoCAD; i
     contorni dei punti restano. Gli oggetti in `leftOut` non contano, senza
     messaggio. Poi la stessa richiesta.
   - ***Properties*:** `Enter a pattern name or [Solid] <ANSI31>:` (il valore
     fra `<>` è quello corrente); `ANSI31`, `NET` e `SOLID`, maiuscole o
     minuscole, e l'opzione *Solid*. Un altro nome: il messaggio di AutoCAD
     per un motivo che non trova, e la richiesta si ripete. Per un motivo che
     non è `SOLID`: `Specify a scale for the pattern <1.0000>:` (maggiore di
     0) e `Specify an angle for the pattern <0>:` (gradi). Poi la richiesta
     del punto 1.
   - **`Invio`:** se ci sono contorni, si crea il retino (punto 3) e il
     comando finisce; senza contorni il comando finisce senza creare nulla.
     `Esc` annulla tutto.
2. **Il retino** (`core::Hatch`, nel layer e con le proprietà correnti):
   `pattern` e `solid` dal motivo, `islands` da `HPISLANDDETECTION`,
   `patternType` 1, `angle` e `scale` dai valori della sessione, `doubled`
   falso, le righe del motivo dalla funzione dei motivi predefiniti di PT30
   (punto 2: nome, scala, angolo e `$MEASUREMENT` del disegno, 1 se il disegno
   non lo dice, come un disegno nuovo in millimetri), i punti interni e i
   contorni: prima quelli dei punti, nell'ordine dei punti, poi quelli scelti,
   ciascuno con i segnali di PT43. Associatività 0.
3. **Troppo fitto:** se `patternLines` (PT42) dà `TooDense`, il messaggio di
   AutoCAD (`Hatch spacing too dense, or dash size too small.`) e il retino non
   si crea; il comando finisce.
4. Un passo di annullamento: `Hatch`. Nessuna anteprima, come `-HATCH` di
   AutoCAD.

## Regole della tappa che valgono qui

- Niente funzioni trigonometriche di `<cmath>` (ADR 0044, punto 8).
- File comuni: `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt`,
  `tests/CMakeLists.txt`.
- Icona e voci di `-HATCH` le aggiunge PT45, non questo pacchetto;
  `HPISLANDDETECTION`, come `MIRRTEXT`, non ha icona.

## Test richiesti

- `hatch_command_test.cpp`: un punto in un rettangolo con un cerchio dentro
  (due contorni, `islands` *outer*, il punto interno); due punti nella stessa
  area (un contorno solo); un punto dentro il cerchio dopo uno fuori (il
  cerchio due volte, la regola pari-dispari riempie tutto); un punto fuori da
  ogni contorno (messaggio, nulla); *Select objects* due volte (vale la
  seconda); *Properties* con `NET`, scala 2 e angolo 30; *Solid*; un nome
  sconosciuto; `HPISLANDDETECTION` 0 e 2; un motivo troppo fitto; `Invio`
  senza contorni; `U`.

## Fatto quando

- I casi di RT44 passano.
- La CI è verde.
