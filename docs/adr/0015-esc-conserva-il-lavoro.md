# ADR 0015: Esc conserva il lavoro già completato

- **Stato:** Accettato (2026-10-18)
- **Data:** 2026-10-18
- **Sostituisce in parte:** ADR 0005, sezione "Comandi", regola "Esc la annulla"

## Contesto

L'ADR 0005 stabilisce che ogni comando lavora in una sola transazione e che `Esc`
la annulla. Per `PLINE` vuol dire che, dopo dieci vertici, un `Esc` per errore
cancella tutto.

AutoCAD si comporta in un altro modo: in `LINE` `Esc` lascia i segmenti già
disegnati e in `PLINE` lascia la polilinea fatta fino all'ultimo vertice. Chi
viene da AutoCAD si aspetta questo, e la tappa 0.4 aggiunge `LINE`, che senza
questa regola perderebbe a ogni `Esc` tutti i segmenti già visibili nel disegno.

## Decisione

### 1. Regola

`Esc` scarta solo l'elemento **in corso** e conserva ciò che il comando ha già
**completato**. Il lavoro conservato è un solo passo di annullamento, con il nome
del comando, come se il comando fosse finito con `Invio`.

| Comando | Cosa conserva `Esc` |
|---|---|
| `LINE` | i segmenti già disegnati |
| `PLINE` | la polilinea con i vertici già dati, se sono almeno due; aperta |
| `TEXT` | le righe già confermate con `Invio` |
| `CIRCLE`, `ARC` | niente: l'entità si completa solo con l'ultimo punto |
| `ERASE` | niente |

Nella fase "Select objects:" e con un rettangolo di selezione aperto `Esc` fa
quello che fa oggi (ADR 0008).

### 2. Meccanismo

- `cmd::Command` riceve una funzione virtuale:

  ```cpp
  /// Esc: the command keeps what it has completed and closes its
  /// transaction. The default rolls everything back.
  virtual void cancel(CommandContext& context);
  ```

  Il comportamento predefinito è quello di oggi (annulla la transazione), così i
  comandi che non hanno niente da conservare non cambiano.
- `CommandRunner`, quando riceve un `CancelInput` mentre un comando chiede un
  punto, un valore o un testo, chiama `cancel()`, scrive `*Cancel*` come oggi e
  chiude il comando. Il comando non riceve mai un `CancelInput` in `handle()`.
- Prima di uscire da `cancel()` il comando conferma o annulla la sua transazione,
  come prima di restituire `Finished`.

### 3. Il resto dell'ADR 0005

Resta valido: una sola transazione per comando, nessun comando legge testo,
annullamento e ripetizione per passi.

## Alternative considerate

- **Lasciare la regola dell'ADR 0005:** più semplice e uguale per tutti i
  comandi, ma diversa da AutoCAD proprio nei comandi più usati, e punitiva per
  `LINE`, i cui segmenti sono già nel disegno mentre si lavora.
- **Una transazione per segmento:** `LINE` diventerebbe tanti passi di
  annullamento, mentre AutoCAD ne fa uno solo per comando.
- **Decidere nel runner, con un elenco di comandi:** il runner dovrebbe
  conoscere i comandi; con `cancel()` ogni comando decide per sé.

## Conseguenze

- Il caso di verifica di `PLINE` con `Esc` a metà, scritto nei riferimenti
  iniziali della 0.4 per il comportamento della 0.3.1, cambia insieme al
  pacchetto che porta la nuova regola in `PLINE`.
- I comandi di modifica della 0.7 (per esempio `TRIM` ed `EXTEND`, che lavorano
  un oggetto alla volta) potranno conservare ciò che hanno già fatto allo stesso
  modo.
- Il manuale dei comandi descrive cosa conserva `Esc` per ogni comando.
