# [0.8.1-PT2] `script`: annunciare modi di snap, domande e campi della tappa

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: cambia solo il controllo dei casi, che lo
  provano i test di unità di `script` (come F8 e F9 della 0.8)
- **Dipende da:** —
- **Modulo:** `script` (più il catalogo in `docs/verifica/`)
- **Da leggere:** [ADR 0037](../../adr/0037-controllo-anticipato-dei-riferimenti.md),
  punti 4 e 6; [verifica/casi.md](../../verifica/casi.md), «Il confine di un
  caso in attesa»; [F8](../fuori-tappa/F8-annuncio-hover-otrack.md) e
  [F9](../fuori-tappa/F9-domande-pedit.md) (come si annuncia);
  `script/src/script.cpp` (`!osnap`), `script/src/question_table.cpp`; gli ADR
  [0054](../../adr/0054-testo-formattato-ed-editor.md),
  [0055](../../adr/0055-griglia-snap-alla-griglia-icona-ucs.md),
  [0056](../../adr/0056-comandi-degli-attributi.md),
  [0057](../../adr/0057-snap-vicino-prolungamento-parallelo.md)

## Perché

Le prove a secco della tappa usano tre modi di snap, cinque domande, due
argomenti e sette campi del rapporto che non ci sono. Se il catalogo non li
descrive, i casi di riferimento scritti prima del lavoro sono **non validi**
invece che in attesa (com'è successo con `!hover` nella 0.8, #728). Questo
pacchetto li annuncia tutti insieme, in testa alla tappa; ogni pacchetto che ne
realizza uno toglie il suo annuncio.

## File

- `script/src/script.cpp` (i modi annunciati di `!osnap`)
- `script/src/question_table.cpp` e un file nuovo
  `script/src/announced_questions.cpp` (le domande annunciate, che rispondono
  `error: question not implemented yet`)
- `script/src/questions.cpp` (gli argomenti annunciati `face=` e il valore
  `nea` di `modes=`)
- `tests/script/case_check_test.cpp`, `tests/script/questions_test.cpp`
- `docs/verifica/script.md`, `docs/verifica/domande.md`,
  `docs/verifica/rapporto.md`

## Cosa fare

1. **`!osnap`:** `nea`, `ext` e `par` sono modi annunciati: in un caso in
   attesa sono il confine (ADR 0037, punto 4.1), non un errore. Nel catalogo
   «dalla tappa 0.8.1: `nea` con PT30, `ext` e `par` con PT31».
2. **Domande annunciate** (nel catalogo, ciascuna con «la realizza PTn; fino
   ad allora `error: question not implemented yet`»):

   | Domanda | Risposta | Chi |
   |---|---|---|
   | `grid D` | `grid=on\|off snap=on\|off grid-unit=x,y snap-unit=x,y snap-base=x,y display=<n> major=<n> ucsicon=<n> snap-angle=<gradi> snap-style=<n>` (ADR 0055, punto 1; senza `*ACTIVE` i valori di un disegno nuovo) | PT27 |
   | `attdisp D` | `normal`, `on` o `off` (`$ATTMODE` 1, 2, 0) | PT27 |
   | `properties <E>` | le proprietà proprie dell'entità, `nome=valore` separate da `;`, nell'ordine della tabella sotto; le derivate (in lettura) con `~`; `none` per i tipi di prima della 0.8 | PT22, PT23, PT24 |
   | `set-property <E> <nome>=<valore>` | l'entità dopo la modifica, nel formato del rapporto, oppure `invalid` se il valore si rifiuta | PT22, PT23, PT24 |
   | `pattern-lines <H>` | le famiglie del motivo del retino, `angle=<gradi> origin=x,y offset=x,y dashes=a,b,…` separate da `;`, nell'ordine; `none` per un riempimento pieno | PT23 |

   `properties` e `set-property` per un tipo il cui pacchetto non è unito
   rispondono `entity type not implemented yet` (domanda saltata, ADR 0037,
   punto 4).
3. **Proprietà proprie** (nomi per `properties` e `set-property`; valori come
   nel rapporto: numeri, `x,y`, gradi, testi tra virgolette, `yes`/`no`):

   | Tipo | Proprietà, in ordine | Pacchetto |
   |---|---|---|
   | `ELLIPSE` | `center`, `major-radius`, `minor-radius`, `ratio`, `start-angle`, `end-angle`, `~area` | PT22 |
   | `SOLID` | `vertex1`, `vertex2`, `vertex3`, `vertex4` | PT22 |
   | `HATCH` | `pattern`, `scale`, `angle`, `origin`, `islands` (`normal`, `outer`, `ignore`), `~area` | PT23 |
   | `DIMENSION` | `style`, `text` (il testo sostitutivo, `""` per la misura), `text-rotation`, `~measurement` | PT23 |
   | `MTEXT` | `value`, `style`, `attachment`, `width`, `height`, `rotation`, `line-spacing`, `line-spacing-style` (`at-least`, `exactly`) | PT24 |
   | `ATTDEF` | `tag`, `prompt`, `default`, `invisible`, `constant`, `verify`, `preset`, `lock-position`, `height`, `rotation`, `style` | PT24 |
   | `INSERT` | `block`, `insertion`, `scale` (X,Y), `rotation`, poi un `attribute="<TAG>":"<valore>"` per attributo, nell'ordine | PT24 |

4. **Argomenti annunciati:** `face=regular|bold|italic|bold-italic` di
   `text-width` e `font-metrics` (PT6); il valore `nea` in `modes=` di `snap`
   (PT30).
5. **Campi del rapporto** (in `rapporto.md`, ciascuno con il pacchetto che lo
   realizza; tutti scritti solo quando servono, quindi i casi di oggi non
   cambiano):
   - `grid snap: unit=x,y base=x,y`, dopo `ortho:`, solo con lo snap alla
     griglia acceso (PT28);
   - `attdisp: on|off`, dopo `lwdisplay:`, solo se `$ATTMODE` non è 1 (PT27);
   - `active snap:` con il tipo `Nearest` (PT30);
   - in `acquired:` le righe `extension x,y` e `parallel x1,y1 x2,y2`; in
     `active tracking:` i tipi `Extension` e `Parallel` e gli incroci che li
     usano (PT31; ADR 0057, punto 5);
   - in `ATTDEF` e `ATTRIB` `lock-position=yes`, dopo `preset` (per `ATTRIB`
     dopo `invisible`), solo se acceso (PT15);
   - in `HATCH` `origin=x,y` dopo `islands`, solo se l'origine del motivo
     (XDATA `ACAD` 1010/1020) c'è ed è diversa da 0,0: nei DXF di
     `dxf-test-file` di oggi è sempre 0,0 (PT23);
   - nei `TEXT` della geometria generata (`generated`) `face=bold`,
     `face=italic` o `face=bold-italic` dopo `vertical`, solo se non Regular
     (PT8).
6. Nel paragrafo del catalogo sulle cose descritte prima dei loro pacchetti,
   l'elenco di questa tappa.

## Regole della tappa che valgono qui

- File comuni: `script/src/question_table.cpp` (dopo questo pacchetto ogni
  pacchetto che realizza una domanda cambia solo la sua riga).

## Test richiesti

- `AnnouncedDirectiveIsTheFrontier` (o un test nuovo accanto): `!osnap
  end,nea` in un caso in attesa è il confine.
- Ognuna delle cinque domande risponde `error: question not implemented yet`;
  `text-width "A" face=bold` e `snap 0,0 aperture=1 modes=nea` danno l'errore
  «not implemented yet», non un caso non valido.

## Fatto quando

- Un caso in attesa con ciascuna delle cose annunciate dà «pending», non
  «INVALID»; la CI è verde.
