# [0.8.1-PT3] `cmd`: alias brevi dei comandi della 0.8 e opzione *Mtext* delle quote

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8.1-RT3]` (#…), agente claude
- **Dipende da:** RT3
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0026](../../adr/0026-descrizione-dei-comandi.md)
  (`CommandDescription::aliases`); il [catalogo di
  parità](../../parita/comandi.md), righe segnate «0.8.1»;
  [ADR 0047](../../adr/0047-quote-e-stili-di-quota.md) (testo sostitutivo);
  [ADR 0054](../../adr/0054-testo-formattato-ed-editor.md), punto 5

## Perché in testa

Tocca nove file di comandi della 0.8 che altri pacchetti della tappa cambiano
(`insert_command.cpp` in PT17, `hatch_command.cpp` in PT26, `mtext_command.cpp`
e i comandi delle quote in PT11): fatto per primo, li toglie dalle loro catene.
È anche **trasversale**: l'opzione *Mtext* cambia la richiesta delle quote in
21 casi di oggi, che così si aggiornano una volta, in testa.

## File

- `cmd/src/commands/ellipse_command.cpp` (`EL`)
- `cmd/src/commands/mtext_command.cpp` (`T`, `MT`)
- `cmd/src/commands/style_commands.cpp` (`ST`)
- `cmd/src/commands/dimension_commands.cpp` (`DLI`, `DAL`; *Mtext*)
- `cmd/src/commands/dimension_angular_commands.cpp` (`DAN`, `DRA`, `DDI`;
  *Mtext*)
- `cmd/src/commands/dimstyle_commands.cpp` (`D`)
- `cmd/src/commands/hatch_command.cpp` (`H`)
- `cmd/src/commands/block_command.cpp` (`B`)
- `cmd/src/commands/insert_command.cpp` (`I`)
- `tests/cmd/dimension_commands_test.cpp`
- `tests/cmd/command_registry_test.cpp`
- `docs/parita/comandi.md`

## Cosa fare

1. Gli alias dell'AutoCAD **inglese** della colonna «Alias» del catalogo, per
   i comandi della 0.8 qui sopra, nella descrizione di ciascun comando
   ([DECISIONI.md](../../DECISIONI.md), 2026-11-21, «Esiti dei DXF di prova e
   della seduta della 0.8.1», punto 6). Gli alias italiani non si usano.
2. Gli alias dei comandi nuovi della 0.8.1 (`REC`, `POL`, `ED`, `HE`, `ATT`,
   `ATE`) li porta ciascun pacchetto con il suo comando.
3. ***Mtext*** in `DIMLINEAR`, `DIMALIGNED`, `DIMANGULAR`, `DIMRADIUS` e
   `DIMDIAMETER`, prima di *Text*, come AutoCAD (`Specify dimension line
   location or [Mtext/Text/Angle/Horizontal/Vertical/Rotated]:` e simili, con
   le parole della cronologia della 0.8): chiede `Enter dimension text
   <misura>:` e scrive il testo sostitutivo, con `<>` per la misura, come
   *Text*. L'editor nella finestra lo aggiunge PT11 (`Command::textEditing()`),
   senza cambiare la richiesta.
4. Nel catalogo si tolgono le frasi «manca l'alias …» e «manca *Mtext*».

## Regole della tappa che valgono qui

- Un alias non deve coincidere con il nome o l'alias di un altro comando:
  `CommandRegistry` lo rifiuta già.

## Test richiesti

- Per ogni alias, la registrazione lo trova e avvia il comando giusto.
- *Mtext* in ciascuno dei cinque comandi, con un testo e con `<>`.

## Fatto quando

- I casi di RT3 passano; la CI è verde.

## Prova a secco dei riferimenti

```text
# casi/alias/comandi-0.8/script.tgs
EL
!esc
T
!esc
MT
!esc
ST
!esc
DLI
!esc
D
!esc
H
!esc
B
!esc
I
!esc
```

Righe da controllare nel rapporto: in `messages:` l'eco `Command: EL` seguita
dalla prima richiesta di `ELLIPSE` (`Specify axis endpoint of ellipse or
[Arc/Center]:`), poi `*Cancel*`; lo stesso per gli altri (`DAL`, `DAN`, `DRA`,
`DDI` in un secondo script).

```text
# casi/dimlinear/mtext/script.tgs
DIMLINEAR
0,0
30,0
M
{\C1;<>} mm
0,5
```

Righe da controllare: la richiesta con `[Mtext/Text/…]`, `Enter dimension
text <30>:` e la quota con il testo sostitutivo.

- **Strumenti:** solo righe normali e `!esc`, su `main`.
- **Dipendenze:** i comandi sono tutti su `main`.
- **Fonte:** le parole di *Mtext* dalla cronologia delle quote della 0.8
  ([0.8/dxf-di-prova.md](../0.8/dxf-di-prova.md)).
- **Risposte condivise:** per gli alias nessuna; nessun caso di oggi scrive questi alias a
  `Command:` (controllato: nessun `atteso.txt` ha `Command: <alias>` né
  `Unknown command: <alias>.`; le righe uguali negli script sono opzioni di
  `FILLET`, `CHAMFER`, `MTEXT`, `-DIMSTYLE` e `PEDIT`). Per *Mtext* cambiano
  le richieste delle quote in 21 casi, quelli che oggi hanno `[Text/Angle…` o
  `or [Text` in `atteso.txt`: `dimlinear/` seleziona-oggetto, opzioni,
  layer-e-proprieta, anteprima-esc-annulla, punti-direzione-automatica;
  `dimaligned/` origini-uguali, punti-testo-angolo; `dimangular/`
  settore-1-annulla, settore-2, settore-3, settore-4, posizione-nel-vertice,
  segmento-polilinea, quadrant, linee-parallele, non-linea; `dimradius/` e
  `dimdiameter/` arco-polilinea, cerchio; `explode/quota-lineare`. RT3 li
  aggiorna con `; cambia con #<issue di PT3>` prima della prima richiesta che
  cambia e il file in `in-attesa/`; PT3 li toglie.
