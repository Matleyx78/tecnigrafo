# ADR 0028: Versione minima di Qt 6.8

- **Stato:** Accettato (2026-10-25)
- **Sostituisce in parte:** ADR 0002, tabella delle decisioni, riga "Interfaccia"
- **Data:** 2026-10-25

## Contesto

L'ADR 0002 fissa Qt 6.4 come versione minima, quella dei pacchetti di Ubuntu
24.04 e di Debian 12. La prova di SARibbon (pacchetto 0.6-P3, ADR 0009 punto 2)
l'ha messa alla prova e non regge: SARibbon v2.9.5 non compila con Qt 6.4.2 su
Linux, perché `SARibbon.cpp` (riga 8430) usa `QEvent::DevicePixelRatioChange`
dietro una soglia di versione errata (6.2). L'evento esiste solo da Qt 6.6; la
stessa libreria, più avanti nel file, usa la soglia giusta. Su Windows e macOS,
con Qt 6.8.3, la libreria compila e il test di prova passa.

La scheda del pacchetto e l'ADR 0009 impongono, in questo caso, di cambiare la
versione minima di Qt o la libreria prima di procedere. Il responsabile ha scelto
il 2026-10-25 di alzare la versione minima.

La CI prova già Qt 6.8.3 su Windows e macOS; su Linux prova la 6.4 dei pacchetti
di sistema, che nessun altro sistema usa.

## Decisione

1. **Versione minima di Qt: 6.8** (versione LTS), solo per il modulo `app` e per i
   test che lo usano. `core`, `render`, `edit` e `cmd` restano senza Qt. Le
   chiamate `find_package(Qt6 6.4 …)` diventano `find_package(Qt6 6.8 …)`.
2. **La CI prova la stessa versione su tutti e tre i sistemi.** I job Linux che
   usano Qt (`linux`, `verifica`, `sanitizers`) installano Qt 6.8.3 con
   `jurplel/install-qt-action`, con la cache attiva, come Windows e macOS. Il
   compilatore, Ninja, GoogleTest e clang-tidy restano quelli dei pacchetti di
   sistema. Il job di fuzzing non usa Qt e non cambia.
3. **Sistemi supportati con i pacchetti di sistema:** Debian 13. Debian 12 e
   Ubuntu 24.04 hanno Qt 6.4: per compilare Tecnigrafo servono Qt 6.8 installato a
   parte (installer ufficiale o aqtinstall) e `CMAKE_PREFIX_PATH`. Quei sistemi
   non si provano in CI.
4. **SARibbon resta alla v2.9.5, senza modifiche.** La prova di SARibbon
   (0.6-P3) riprende con questa versione minima.

## Alternative considerate

- **Correggere la soglia nella copia di SARibbon** (una riga): la copia
  smetterebbe di essere identica a quella del progetto originale e la patch
  andrebbe riapplicata a ogni aggiornamento. Resta possibile in futuro se serve
  una versione più bassa.
- **Minimo Qt 6.6:** basterebbe a SARibbon, ma la CI proverebbe solo la 6.8.3: un
  minimo che nessun controllo verifica non è una garanzia.
- **Cambiare libreria per la ribbon:** molto più lavoro, per un errore che è di una
  sola riga, e scarta una libreria che funziona su due sistemi su tre.

## Conseguenze

- Chi lavora su Debian 12 o Ubuntu 24.04 deve installare Qt 6.8 a parte. Il
  README e `docs/AMBIENTE-SVILUPPO.md` lo dicono.
- I job Linux della CI scaricano Qt alla prima esecuzione (poi dalla cache): la
  prima esecuzione è più lenta. La durata della CI non cambia il costo del lavoro
  degli agenti, che la aspettano con un solo comando (AGENTS.md).
- Chi usa Qt dai pacchetti di sistema (Debian 13) può avere una 6.8 diversa dalla
  6.8.3 della CI: lo scarto è tra versioni dello stesso ramo.
- La definizione `QT_DISABLE_DEPRECATED_UP_TO` resta a Qt 6.4; alzarla è un lavoro
  a parte, perché può far emergere API deprecate nel nostro codice.
- L'ADR 0009 (punto 2) non cambia: la versione minima è sempre quella dell'ADR
  corrente su Qt.
