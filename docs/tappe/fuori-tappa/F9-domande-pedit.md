# [F9] `script`: annunciare le domande di PT32

- **Tappa:** fuori tappa; sblocca RT32 della
  [tappa 0.8](../0.8-quote-retini-blocchi.md) (decisione del 2026-11-15,
  [DECISIONI.md](../../DECISIONI.md), «Domande di PT32 annunciate prima del
  lavoro»)
- **Issue:** #771
- **Difficoltà:** facile
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: cambia solo il catalogo delle domande e il
  loro annuncio, che lo provano i test di unità di `script`
- **Dipende da:** —
- **Modulo:** `script` (più il catalogo `docs/verifica/domande.md`)
- **Da leggere:** [ADR 0050](../../adr/0050-pedit.md), punti 2–4;
  [ADR 0037](../../adr/0037-controllo-anticipato-dei-riferimenti.md),
  punto 4; [verifica/domande.md](../../verifica/domande.md),
  [verifica/casi.md](../../verifica/casi.md), punto 6 di «Scrivere un caso»;
  la scheda [PT32](../0.8/PT32-unione-con-distanza.md)

## Perché

RT32 scrive i casi di PT32 prima del lavoro, ma nessuna domanda arriva alla
conversione in polilinea, all'inversione e all'unione con distanza: `join`
chiede di `JOIN` e non conosce `fuzz`, il tipo di unione, `added` e
`consumed`. `PEDIT` negli script non serve, perché è PT33 e dipende da PT32
(#724). È lo stesso problema di RT34 con le direttive, risolto da
[F8](F8-annuncio-hover-otrack.md): il catalogo descrive le domande nuove e il
programma le annuncia, così un caso che le usa è **in attesa**, non **non
valido**.

## File

- `docs/verifica/domande.md` (la sezione «Polilinee (tappa 0.8)»)
- `script/src/question_table.cpp` (solo le tre righe annunciate)
- `tests/script/case_check_test.cpp`

## Cosa fare

1. In `domande.md`, dopo la tabella dell'**Unione** della tappa 0.7, la
   sezione «Polilinee (tappa 0.8)» con le tre domande e le loro regole, come
   le descrive la scheda [PT32](../0.8/PT32-unione-con-distanza.md),
   punto 5, e la riga «Le realizza il pacchetto PT32 della 0.8; fino ad allora
   rispondono `error: question not implemented yet`».
2. In `kQuestions` le righe `to-polyline`, `reverse` e `pedit-join`, forma
   della risposta `AnswerShape::Single`, funzione `nullptr`, con un commento
   che dice che sono annunciate per PT32 della 0.8.
3. Nessun altro cambiamento: gli argomenti di una domanda annunciata non si
   controllano, come per quelle della 0.7.

## Test richiesti

- `AnnouncedQuestionsCanWaitButUnknownQuestionsStayInvalid` prova anche
  `to-polyline A`, `reverse A` e `pedit-join A A fuzz=1`: oggi sono le sole
  domande annunciate e non realizzate.

## Fatto quando

- Un caso in attesa con `? pedit-join S L fuzz=2 type=both -> …` dà
  "pending", non "INVALID"; la CI è verde.
