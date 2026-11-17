# [F12] `script`: annunciare le domande `hatch-boundary` e `hatch-contours` di PT43

- **Tappa:** fuori tappa; sblocca RT43 della
  [tappa 0.8](../0.8-quote-retini-blocchi.md) (decisione del 2026-11-17,
  [DECISIONI.md](../../DECISIONI.md), «Domande di PT43 annunciate prima del
  lavoro»)
- **Issue:** #859
- **Difficoltà:** facile
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: cambia solo il catalogo delle domande e il
  loro annuncio, provato dai test di unità di `script`
- **Dipende da:** —
- **Modulo:** `script` (più il catalogo `docs/verifica/domande.md`)
- **Da leggere:** [ADR 0048](../../adr/0048-retini.md), punti 5 e 6;
  [ADR 0037](../../adr/0037-controllo-anticipato-dei-riferimenti.md),
  punto 4; [verifica/domande.md](../../verifica/domande.md),
  [verifica/casi.md](../../verifica/casi.md), punto 6 di «Scrivere un caso»;
  la riga `HATCH` di [verifica/rapporto.md](../../verifica/rapporto.md); la
  scheda [PT43](../0.8/PT43-contorni-dei-retini.md), punto 5

## Perché

RT43 scrive i casi di PT43 prima del lavoro, ma le domande `hatch-boundary` e
`hatch-contours` della scheda PT43, punto 5, non sono né nel catalogo né in
`kQuestions`: i casi risultano **non validi** invece che **in attesa**
(CI della pull request #855). È lo stesso problema di RT32 e RT19, risolto da
[F9](F9-domande-pedit.md) e [F11](F11-annuncio-import-blocco.md): il catalogo
descrive le domande e il programma le annuncia.

## File

- `docs/verifica/domande.md` (la sezione «Retini (tappa 0.8)»)
- `script/src/question_table.cpp` (solo le due righe annunciate)
- `tests/script/case_check_test.cpp`

## Cosa fare

1. In `domande.md`, dopo la sezione «Polilinee (tappa 0.8)», la sezione
   «Retini (tappa 0.8)» con le due domande esattamente come le specifica la
   scheda PT43, punto 5 (tabella, `<loop>` nella forma della riga `HATCH` di
   rapporto.md, curve candidate e contorni scelti delle funzioni di PT43), e
   la riga «Le realizza il pacchetto PT43 della 0.8; fino ad allora
   rispondono `error: question not implemented yet`».
2. In `kQuestions` le righe `hatch-boundary` e `hatch-contours`, forma della
   risposta `AnswerShape::Single` (la risposta è in ordine), funzione
   `nullptr`, con un commento che dice che sono annunciate per PT43 della 0.8
   (F12).
3. Nessun altro cambiamento: gli argomenti di una domanda annunciata non si
   controllano, come per quelle di F9 e F11.
4. PT43 realizza le domande in `hatch_questions.cpp`, sostituisce `nullptr`
   con le sue funzioni, toglie il commento dell'annuncio e la riga «fino ad
   allora» del catalogo.
5. La pull request di F12 **non** porta la riga «Avvia dopo il merge»: RT43 è
   in fascia forte e il responsabile lo riprende dalla chat di Codex.

## Test richiesti

- `AnnouncedQuestionsCanWaitButUnknownQuestionsStayInvalid` prova anche
  `hatch-boundary 5,5` e `hatch-contours A`: un caso in attesa che le usa dà
  "pending", non "INVALID"; una domanda sconosciuta (`transformx`,
  `hatch-boundaries`) rende ancora il caso non valido, anche con `--pending`.

## Fatto quando

- Un caso in attesa con `? hatch-boundary x,y -> …` o
  `? hatch-contours E … -> …` dà "pending", non "INVALID"; la CI è verde.
