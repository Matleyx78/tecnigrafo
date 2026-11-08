# [0.7.5-P35] `app`: le icone colorate si calcolano una volta per tema

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** facile
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: le icone restano identiche, cambia solo quante
  volte si calcolano; test Qt
- **Dipende da:** P3 (stesso blocco di `tests/CMakeLists.txt`)
- **Nata:** 2026-11-08, nella chat del responsabile, dai tempi della CI di
  Windows: `app_widgets` a 234,66 s con limite 240 su `main` (esecuzione
  42398809911) e fermata dal limite in una della #587 (42403532210); la #587
  porta il limite a 300 s come misura provvisoria
- **Modulo:** `app` (più i suoi test e una riga di `tests/CMakeLists.txt`)
- **Da leggere:** [ADR 0040](../../adr/0040-icone-a-ruoli.md) (icone a ruoli);
  `app/src/theme.hpp` e `app/src/theme.cpp` (`themedIcon`, `iconFiles`);
  `MainWindow::setTheme` e `updateThemeIcons` in `app/src/main_window.cpp`, per
  vedere quante volte sono chiamate; il blocco `if(TG_BUILD_APP)` di
  `tests/CMakeLists.txt`

## File

- `app/src/theme.hpp`
- `app/src/theme.cpp`
- `tests/app/icons_test.cpp`
- `tests/CMakeLists.txt` (limite e commento di `app_widgets`)

## Cosa fare

Oggi `themedIcon` a ogni chiamata legge i due file dell'icona, li colora, li
interpreta e disegna 9 immagini; `MainWindow::setTheme` la chiama per ogni
icona dei comandi, e ogni finestra chiama `setTheme` due o tre volte (nel
costruttore, in `setScriptMode`, in `restoreUiSettings`). I test costruiscono
circa 150 finestre: in Debug con MSVC è la parte più cara dei test
dell'applicazione.

1. **Cache.** `themedIcon(name, theme)` conserva per tutta la vita del processo
   l'icona già calcolata di ogni coppia (file risolti da `iconFiles`, tema),
   **solo per le icone delle risorse**, cioè quando i file risolti cominciano
   con `:/`: sono compilate nel programma e non cambiano. Le icone da file su
   disco (percorsi assoluti, che usano i test) si ricalcolano a ogni chiamata,
   come oggi. Un'icona vuota (file mancante o non valido) non si conserva.
   Firma, nome e comportamento visibile restano quelli di oggi:

   ```cpp
   [[nodiscard]] QIcon themedIcon(const QString& name, Theme theme);
   ```

   Il commento in `theme.hpp` aggiunge una frase: le icone delle risorse si
   calcolano una volta per tema e si riusano. La cache sta in `theme.cpp`, in
   un namespace anonimo; solo il thread dell'interfaccia chiama `themedIcon`,
   quindi niente lock, e il commento lo dice.
2. **Nient'altro cambia**: niente modifiche a `setTheme`, a `applyTheme`, ai
   colori, ai ruoli o ai file SVG.
3. **Limite.** Il `TIMEOUT` di `app_widgets` è tre volte la durata di
   `app_widgets` nella CI di Windows del ramo, arrotondata ai 30 s in su e mai
   sotto 120 s. Se il valore così calcolato supera 180 s, il limite resta 300 s
   e la pull request lo segnala: la divisione della voce si deciderà con i
   tempi (piano, «Modifiche al piano», 2026-11-08). Il commento sopra
   `add_test(NAME app_widgets …)` dice da dove viene il valore e che il limite
   ferma solo un blocco vero; non parla più di misura provvisoria.
4. **Tempi.** La pull request riporta la durata di `app_widgets` prima e dopo
   su Windows, Linux (GCC) e Linux (ASan): la riga di `ctest` nel log del passo
   *Test*; se il log non è raggiungibile, la durata del passo *Test* dall'API
   REST dei job.

## Regole della tappa che valgono qui

- Le icone non cambiano: nessun file in `resources/icons/` è toccato. La
  galleria di P3 del ramo la guarda il responsabile al merge.
- `tests/CMakeLists.txt` è un file comune: aggiungi o cambia solo le tue righe
  e, se un altro pacchetto è arrivato prima, aggiornati.
- La parte `app` su Windows e macOS si prova solo in CI: la pull request lo
  dice.

## Test richiesti

In `tests/app/icons_test.cpp`:

- due chiamate con la stessa icona delle risorse (per esempio
  `draw-circle-cr`) e lo stesso tema danno icone con lo stesso
  `QIcon::cacheKey()`; con temi diversi, chiavi diverse, e i colori dei ruoli
  restano quelli dei test di oggi;
- un'icona da file su disco riscritto fra due chiamate (cartella temporanea,
  stessa forma con un elemento in più, o un'altra forma) dà la seconda volta
  l'immagine nuova, con `cacheKey()` diverso;
- un nome che non esiste dà un'icona vuota anche alla seconda chiamata;
- i test esistenti su colori, ruoli e griglie passano senza modifiche.

## Fatto quando

- I test sopra esistono e passano sui tre sistemi.
- Nella CI di Windows del ramo `app_widgets` dura meno della metà dei 123 s
  misurati sulla #587 (esecuzione 42405732328), oppure la pull request spiega
  con i tempi perché no.
- Il `TIMEOUT` di `app_widgets` segue il punto 3, e il commento lo spiega.
- Nessun file in `resources/icons/` è toccato.
- CI verde sui tre sistemi.
