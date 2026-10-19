# ADR 0016: AddressSanitizer e UBSan in CI

- **Stato:** Accettato (2026-10-19)
- **Data:** 2026-10-19

## Contesto

Tecnigrafo è scritto in C++20 (ADR 0002). Un accesso fuori dai limiti di un
vettore, l'uso di un oggetto già distrutto o un overflow di un intero con segno
possono passare tutti i test senza farsi notare: il comportamento è indefinito,
e spesso "funziona" sulla macchina di chi scrive e si rompe altrove, o solo con
certi dati.

I pacchetti li scrivono più agenti in parallelo e la modalità di controllo
(ADR 0011) dice se il risultato coincide con i riferimenti, non se il programma
ci è arrivato senza errori di memoria. Dalla tappa 0.5 il lettore DXF riceverà
file da chiunque: un errore di memoria lì diventa un crash o una falla.

GCC e Clang hanno due strumenti di controllo durante l'esecuzione:
**AddressSanitizer** (accessi fuori dai limiti, uso dopo la liberazione, perdite
di memoria con LeakSanitizer) e **UndefinedBehaviorSanitizer** (overflow con
segno, spostamenti di bit non validi, conversioni fuori intervallo, puntatori
nulli, chiamate virtuali su tipi sbagliati). Non richiedono librerie nuove: sono
opzioni del compilatore.

Idea di partenza: issue #106, punto 2.

## Decisione

### 1. Opzione e preset

- Nuova opzione CMake `TG_SANITIZE` (spenta per difetto). Accesa, compila
  **tutti** i bersagli, test compresi, con
  `-fsanitize=address,undefined -fno-sanitize-recover=all
  -fno-omit-frame-pointer`, e collega con `-fsanitize=address,undefined`.
- `-fno-sanitize-recover=all`: il primo errore ferma il programma, quindi il test
  fallisce. Un errore stampato ma ignorato non serve a niente.
- Con MSVC l'opzione dà un errore di configurazione: il controllo si fa solo con
  GCC e Clang.
- Preset `asan` (configurazione, build e test), che eredita `ci`: avvisi come
  errori, `Debug`. Il preset di test imposta le opzioni a tempo di esecuzione
  (perdite di memoria attive, interruzione al primo errore, uso dello stack dopo
  il ritorno, traccia dello stack per UBSan).

### 2. CI

- Nuovo job **`Linux (ASan, UBSan)`** su `ubuntu-24.04`, con Qt: compila con il
  preset `asan` ed esegue **tutti** i test, compresi i casi della modalità di
  controllo e i test dell'interfaccia con la piattaforma `offscreen`.
- È un controllo obbligatorio come gli altri (GUIDA-RESPONSABILE.md, A3).
- Solo Linux: Windows e macOS restano come sono. Il codice è lo stesso sui tre
  sistemi; un errore di memoria trovato su Linux è un errore ovunque.

### 3. Soppressioni

- Le perdite segnalate dentro librerie che non compiliamo (Qt, fontconfig,
  driver grafici) si possono sopprimere in `tests/sanitizer/lsan.supp`, una riga
  per libreria con il motivo. Il file nasce vuoto.
- Il nostro codice non si sopprime mai: un errore nel nostro codice si corregge.
- Non si spegne un controllo (`detect_leaks=0`, `-fno-sanitize=…`) per far
  passare la CI; serve un nuovo ADR.

### 4. In locale

Chi lavora può eseguire lo stesso controllo:

```sh
cmake --preset asan && cmake --build --preset asan && ctest --preset asan
```

Non è obbligatorio prima di aprire una pull request: fa fede la CI.

## Alternative considerate

- **Sanitizer solo in locale:** dipende dalla buona volontà di chi lavora, e gli
  agenti lo dimenticherebbero. In CI non si dimentica.
- **Valgrind:** trova errori simili senza ricompilare, ma rende i test da 20 a 50
  volte più lenti e non vede il comportamento indefinito.
- **ThreadSanitizer:** oggi il programma non usa thread; si potrà aggiungere,
  con un nuovo ADR, quando arriveranno (per esempio nel caricamento dei file).
- **MemorySanitizer:** richiede che anche Qt e la libreria standard siano
  compilate con lo strumento; troppo costoso da mantenere.
- **Sanitizer nel job `ci` esistente:** un solo job, ma i sanitizer rallentano la
  build e confondono gli avvisi di `clang-tidy`; job separati falliscono per
  motivi distinti e si leggono meglio.

## Conseguenze

- Un job in più su Linux a ogni pull request che tocca il codice (qualche minuto).
- Errori di memoria e comportamenti indefiniti si scoprono nella pull request che
  li introduce, non mesi dopo.
- Il fuzzing del lettore DXF (tappa 0.5, issue #106, punto 3) userà la stessa
  opzione: i sanitizer sono ciò che trasforma un input malformato in un errore
  visibile.
- Quando un pacchetto aggiunge una dipendenza che perde memoria per conto suo,
  la soppressione va nella stessa pull request, con il motivo.
