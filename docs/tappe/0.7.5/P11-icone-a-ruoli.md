# [0.7.5-P11] `app`: icone a ruoli in `themedIcon` e controllo della geometria delle icone

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: aspetto dell'interfaccia e uno strumento di
  controllo; test Qt e test dello script
- **Dipende da:** P10
- **Modulo:** `app` (più lo script del controllo)
- **Da leggere:** [ADR 0040](../../adr/0040-icone-a-ruoli.md) per intero;
  [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md), punto 2;
  [ICONE.md](../../ICONE.md); `app/src/theme.hpp`, `app/src/theme.cpp`,
  `tests/app/icons_test.cpp`, `app/CMakeLists.txt` (come entrano le icone)

## File

- `app/src/theme.hpp`, `app/src/theme.cpp`
- i file di `app/src` che chiamano `themedIcon`, solo per la firma nuova
- `tests/app/icons_test.cpp`, `tests/app/theme_test.cpp`
- `scripts/icone.py`, `scripts/icone_test.py` (nuovi)
- `docs/ICONE.md` (riscritto con le regole nuove)
- `resources/icons/draw-circle-cr-32.svg`, `draw-circle-cr-16.svg` (la prima
  icona con le regole nuove, come esempio in ICONE.md)

## Cosa fare

### 1. `themedIcon` a ruoli

```cpp
/// The icon `name` (docs/adr/0040): the -16 and -32 files when they exist,
/// else the old 24 px file; each role coloured by the tokens of `theme`.
[[nodiscard]] QIcon themedIcon(const QString& name, Theme theme);
```

- prende `<nome>-16.svg` per le dimensioni fino a 24 px e `<nome>-32.svg` per
  le altre; dove ne esiste una sola, usa quella; dove non esiste nessuna delle
  due, il file da 24 px di oggi;
- dà a ogni elemento il colore del suo ruolo (`geo`, `pt`, `cost`, `esist`) dai
  gettoni del tema; un elemento senza classe è `esist`, e la classe `accent` di
  oggi vale `pt`, finché P19–P22 non rifanno le icone;
- il meccanismo (riscrittura del testo SVG letto, o un foglio di stile SVG lo
  scegli tu, purché uguale sui tre sistemi e senza dipendenze nuove), lo scrivi
  in ICONE.md;
- la firma con `QPalette` sparisce; i chiamanti passano il tema.

### 2. Il controllo della geometria (ADR 0040, punto 4)

`scripts/icone.py [cartella]` (predefinita `resources/icons/`) legge ogni SVG e
segnala, con file e riga:

- le regole di ICONE.md: elementi ammessi, classi dei ruoli, griglia 16 o 32 dal
  nome, margine, spessore del tratto per la griglia, nessun colore scritto;
- la geometria dichiarata: `data-on="<id>"` su un punto (`circle` di classe `pt`)
  vuol dire che il suo centro sta sull'elemento con quell'`id` (segmento,
  cerchio, arco di un `path`); `data-center-of="<id>"` vuol dire che è il centro
  del cerchio o dell'arco con quell'`id`. Tolleranza 0,1 px;
- un `id` citato che non esiste.

Il controllo si esegue nel lavoro «Scripts (Python)» della CI, che scopre già i
file `*_test.py` e gira anche quando cambia solo un'icona (un SVG non è
documentazione per il filtro della CI): `icone_test.py` ha i casi dello script e
uno che esegue lo script su `resources/icons/` e fallisce se trova errori.
`ci.yml` non si tocca.

### 3. ICONE.md e la prima icona

ICONE.md si riscrive con le regole dell'ADR 0040 (ruoli, due griglie, tratti,
punti, attributi `data-on` e `data-center-of`, meccanismo dei colori, come si
aggiunge un'icona) e un esempio completo: `draw-circle-cr` a 32 e 16 px, con il
centro dichiarato e il punto sulla circonferenza dichiarato. L'elenco delle
icone di oggi resta, con una colonna che dice se è già rifatta.

## Regole della tappa che valgono qui

- `theme.cpp`: prima di te P10, dopo di te P22 (che toglie il ripiego sui 24 px).
- Le icone di oggi non si ridisegnano qui: lo fanno P19–P22.

## Test richiesti

- `icons_test.cpp`: un'icona a ruoli disegnata nei due temi ha i colori dei
  gettoni di ciascun ruolo; a 16 px usa il file `-16`, a 32 il file `-32`; un
  nome senza file nuovi usa quello da 24.
- `icone_test.py`: un punto che non sta sull'arco dichiarato (l'errore della
  prova della #483); un centro a distanza diversa dal raggio; un colore scritto;
  un elemento non ammesso; un `id` che manca; un'icona corretta.

## Fatto quando

- `themedIcon` colora per ruolo nei due temi e sceglie la griglia.
- `scripts/icone.py` passa su `resources/icons/` e i suoi test passano in CI.
- ICONE.md ha le regole nuove e l'esempio.
- La CI è verde.
