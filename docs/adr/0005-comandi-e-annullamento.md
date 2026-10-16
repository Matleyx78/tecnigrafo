# ADR 0005: Comandi, annullamento e input dell'utente

- **Stato:** Proposto (da accettare all'apertura della tappa 0.2)
- **Data:** 2026-10-15

## Contesto

Dalla tappa 0.2 l'utente modifica il disegno. Servono tre cose che resteranno per
sempre al centro del programma: un modo unico di modificare il documento, un
annullamento affidabile e un sistema di comandi in stile AutoCAD, usabile sia con
il mouse sia scrivendo sulla riga di comando. Nessuna di queste deve dipendere da
Qt, perché in futuro dovranno funzionare anche da script e dai test.

## Decisione

### Due nuovi moduli

```
core  ←  edit  ←  cmd  ←  app
core  ←  render  ←────────  app
```

- **edit** (`tg::edit`): l'unica porta per modificare un documento. Contiene
  `Editor`, le transazioni e la pila di annullamento.
- **cmd** (`tg::cmd`): i comandi interattivi, la loro esecuzione e la lettura
  delle coordinate scritte dall'utente.

Entrambi sono C++20 puro, senza Qt, come `core` e `render`.

### Modificare il documento

- `core::Document` offre le operazioni elementari: aggiungere, togliere, trovare
  un'entità, reinserirla con il suo `EntityId` originale.
- Nessun codice fuori da `edit` chiama le operazioni che modificano il documento:
  i comandi ricevono un `edit::Editor&` e un `const core::Document&`. La regola è
  verificata in revisione.
- Ogni modifica avviene dentro una **transazione** con un nome leggibile
  ("Polyline"). Una transazione confermata è un passo di annullamento; una
  transazione annullata non lascia traccia.
- Annullare e ripetere ripristinano le entità con lo **stesso `EntityId`**.
- `Editor` avvisa chi si è registrato (la vista, per ridisegnare) dopo ogni
  conferma, annullamento o ripetizione.

### Comandi

- Un comando è una classe che implementa `cmd::Command`: dichiara la domanda da
  mostrare all'utente (`Prompt`, con le eventuali parole chiave tra parentesi
  quadre come in AutoCAD), riceve gli input uno alla volta e fornisce la
  geometria di anteprima.
- Gli input sono tipi valore in un `std::variant`: punto, parola chiave, conferma
  (Invio, spazio, clic destro), annullamento (Esc). Il movimento del puntatore
  arriva a parte e aggiorna solo l'anteprima.
- I comandi **non leggono testo**: è `cmd::CommandRunner` a trasformare ciò che
  l'utente scrive in un input (coordinata, distanza, parola chiave), così la
  sintassi è identica in tutti i comandi.
- I comandi si registrano in un `CommandRegistry` con nome e alias (`PLINE`,
  `PL`), senza distinzione tra maiuscole e minuscole.
- Ogni comando lavora in una sola transazione: Esc la annulla, la fine normale
  la conferma.

### Coordinate scritte

Sintassi di AutoCAD, con il punto come separatore decimale e la virgola tra X e Y:

| Scrittura | Significato |
|---|---|
| `x,y` | punto assoluto |
| `@dx,dy` | relativo all'ultimo punto |
| `d<a` | polare assoluto: distanza `d`, angolo `a` in gradi |
| `@d<a` | polare relativo all'ultimo punto |
| `d` | distanza nella direzione del puntatore, dall'ultimo punto |

### Testi dei comandi

Domande e messaggi dei moduli `cmd` ed `edit` sono in inglese. La traduzione
dell'interfaccia sarà decisa con un ADR nella tappa 0.9; fino ad allora nessun
testo viene composto incollando pezzi di frase, per non rendere difficile la
traduzione.

### Test dell'applicazione

Il modulo Qt Test è ammesso per i test del modulo `app`.

## Alternative considerate

- **Annullamento a istantanee** (copia del documento a ogni passo): semplice ma
  impraticabile con disegni grandi.
- **Comandi che leggono direttamente eventi Qt**: più rapido da scrivere, ma
  legherebbe la logica all'interfaccia e la renderebbe impossibile da testare.
- **Ogni comando interpreta il proprio testo**: porta inevitabilmente a sintassi
  diverse da un comando all'altro.

## Conseguenze

- Ogni comando futuro si scrive e si prova senza aprire una finestra.
- Script, macro e file di prova potranno pilotare il programma attraverso lo
  stesso `CommandRunner`.
- Le operazioni che modificano `Document` sono pubbliche per necessità tecnica: la
  disciplina di passare da `edit` dipende dalla revisione.
