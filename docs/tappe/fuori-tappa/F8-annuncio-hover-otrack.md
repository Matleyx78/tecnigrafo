# [F8] `script`: annunciare `!hover` e `!otrack`

- **Tappa:** fuori tappa; sblocca RT34 della
  [tappa 0.8](../0.8-quote-retini-blocchi.md) (decisione del 2026-11-13,
  [DECISIONI.md](../../DECISIONI.md), «Direttive del tracciamento annunciate
  prima di PT34»)
- **Issue:** #760
- **Difficoltà:** facile
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: cambia solo il controllo dei casi, che lo
  provano i test di unità di `script`
- **Dipende da:** —
- **Modulo:** `script` (più il catalogo `docs/verifica/script.md`)
- **Da leggere:** [ADR 0049](../../adr/0049-tracciamento-degli-snap-a-oggetto.md),
  punto 7; [ADR 0037](../../adr/0037-controllo-anticipato-dei-riferimenti.md),
  punto 4; [verifica/script.md](../../verifica/script.md),
  [verifica/casi.md](../../verifica/casi.md), punto 6 di «Scrivere un caso»;
  la scheda [PT34](../0.8/PT34-otrack-cmd.md)

## Perché

RT34 scrive i casi del tracciamento prima di PT34, e i casi usano `!hover` e
`!otrack`. Il catalogo non le descrive e `kAnnouncedDirectives` è vuoto, quindi
il controllo anticipato le tratta come direttive sconosciute: il caso è **non
valido**, non in attesa (#728). Nella 0.7 le direttive nuove erano già nel
catalogo e fra quelle annunciate prima dei loro pacchetti; nella 0.8 nessuno
l'aveva fatto, ed è l'unico caso della tappa (le altre schede non ne
introducono).

## File

- `script/src/script.cpp` (solo `kAnnouncedDirectives` e il suo commento)
- `tests/script/case_check_test.cpp`
- `docs/verifica/script.md`

## Cosa fare

1. In `kAnnouncedDirectives` i nomi `hover` e `otrack`; il commento dice che
   sono annunciate per PT34 della 0.8.
2. Nel catalogo, dopo `!polar`, le righe `!hover x,y` e `!otrack on` /
   `!otrack off`, con l'effetto dell'ADR 0049, punto 7, e «dalla tappa 0.8,
   pacchetto PT34»; nel paragrafo sulle direttive non ancora realizzate,
   `!hover` e `!otrack` fino a PT34.
3. Nessun altro cambiamento: gli argomenti delle direttive annunciate non si
   controllano, come per quelle della 0.7.

## Test richiesti

- `AnnouncedDirectiveIsTheFrontier` prova anche `!hover 0,0` e `!otrack on`:
  oggi sono il confine del caso.

## Fatto quando

- Un caso in attesa con `!otrack on` e `!hover x,y` dà "pending", non
  "INVALID"; la CI è verde.
