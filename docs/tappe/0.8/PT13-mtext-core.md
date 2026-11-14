# [0.8-PT13] `core`: testo multiriga nel modello e impaginazione

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8-RT13]` (#…), agente codex, fascia forte
- **Dipende da:** PT5 (revisione delle interfacce), PT8, PT9, PT11, RT13
- **Modulo:** `core`
- **Da leggere:** [ADR 0046](../../adr/0046-stili-di-testo-e-testo-multiriga.md),
  punto 2; [ADR 0013](../../adr/0013-testo-e-font.md) e
  [ADR 0022](../../adr/0022-allineamenti-obliquo-specchiatura-testo.md) (misure
  del font, `Text`); [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md),
  punto 2; `core/include/tg/core/text.hpp`, `text_metrics.hpp`;
  la scheda [PT6](PT6-scheletro-core.md), punti 1 e 4

## File

- `core/include/tg/core/mtext.hpp`, `core/src/mtext.cpp`
- `core/src/mtext_layout.cpp`, `core/src/mtext_layout.hpp` (nuovi, privati)
- `tests/core/mtext_test.cpp` (nuovo), `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. `supported(const MText&)` restituisce `true`.
2. **Codici.** Una funzione privata trasforma `value` in paragrafi di testo
   semplice: `\P` chiude un paragrafo; `\~` è uno spazio che non va a capo;
   `\\`, `\{`, `\}` sono i caratteri; `{` e `}` da soli si tolgono; `\U+XXXX` è
   il carattere; `%%c`, `%%d`, `%%p`, `%%%` come `displayText` (ADR 0013, punto
   5); i codici con argomento fino al `;` (`\f…;`, `\F…;`, `\H…;`, `\W…;`,
   `\Q…;`, `\T…;`, `\A…;`, `\C…;`, `\c…;`, `\p…;`) si tolgono; `\S…;` lascia il
   testo fra `\S` e `;` con `^` e `#` sostituiti da `/`; `\L`, `\l`, `\O`, `\o`,
   `\K`, `\k` si tolgono; un `\` seguito da altro resta com'è. I codici si
   decodificano **una volta sola**: il testo semplice che ne esce si mostra
   così com'è, senza un secondo passaggio di `displayText` (`%%%c` mostra
   `%c`; `%%%%c` mostra `%%c`, non il simbolo del diametro). Un codice con argomento senza il suo `;` resta letterale, ma un
   `\P` dentro va comunque a capo: `A\H2\PB` dà le righe `A\H2` e `B`. `\~`
   si misura come uno spazio normale (PT5, punti B01 e B02: [DECISIONI.md](../../DECISIONI.md), 2026-11-14, «Revisione delle interfacce PT5»).
3. **Impaginazione** (ADR 0046, punto 2), con `h` = altezza dell'entità, `f` =
   fattore di larghezza dello stile (1 se lo stile non c'è), e le larghezze del
   font incluso:
   - la larghezza di una stringa è `textWidth(s) · h · f`;
   - con `width > 0` un paragrafo va a capo prima della parola che porterebbe la
     riga oltre `width`; le parole sono separate da spazi normali; gli spazi
     dove si va a capo non restano in nessuna delle due righe; una parola più
     larga di `width` sta da sola sulla sua riga, intera;
   - paragrafo vuoto = riga vuota; una riga di soli spazi conta come una riga
     vuota (occupa la sua linea di base, non genera testo);
   - distanza fra due linee di base: `5/3 · h · lineSpacing`;
   - colonna: larghezza `W = width` se maggiore di 0, altrimenti la riga più
     larga; riga `i` (da 0) allineata a sinistra, al centro o a destra in
     `[0, W]`;
   - verticale, con `n` righe e `L = (n − 1) · 5/3 · h · lineSpacing`, nel
     sistema della colonna (Y verso l'alto, origine nel punto d'inserimento): in
     alto la prima linea di base è a `y = −h`; in mezzo è a `y = (h + L)/2 − h`,
     così il blocco dalla sommità delle maiuscole della prima riga alla linea di
     base dell'ultima, alto `h + L`, è centrato sul punto; in basso l'ultima
     linea di base è a `y = 0`, cioè la prima a `y = L`;
   - orizzontale: la colonna comincia a 0 (sinistra), a `−W/2` (centro), a `−W`
     (destra);
   - poi rotazione e punto d'inserimento.
4. **Geometria generata:** un `Text` per riga con almeno un carattere che non
   sia uno spazio, `horizontal = Left`,
   `vertical = Baseline`, `position` = inizio della linea di base, altezza `h`,
   rotazione dell'entità, fattore di larghezza e obliquo dello stile, stile
   dell'entità, `value` = la riga semplice. Un `MTEXT` senza righe da
   mostrare (valore vuoto o di soli spazi) non genera forme: il suo ingombro
   è il punto d'inserimento, e si prende con un clic o una finestra su quel
   punto (PT5, punto B04). Con `directionByStyle` e lo stile
   all'indietro o capovolto, i segnali di specchiatura del testo seguono lo
   stile.
5. **Trasformazione:** punto d'inserimento trasformato, altezza e larghezza per
   il fattore di scala, rotazione più l'angolo della similitudine; con una
   riflessione l'`MTEXT` **resta leggibile qualunque sia `MIRRTEXT`**, come
   AutoCAD: vale la tabella del testo dell'ADR 0031, punto 3, con `MIRRTEXT`
   = 0, applicata alla direzione della colonna (PT5, punto B03).
6. **Snap e grip:** inserimento sul punto d'inserimento; grip sul punto
   d'inserimento, che sposta il testo.

## Regole della tappa che valgono qui

- Niente funzioni trigonometriche di `<cmath>` (`core::sinCos`, PT1).
- Le misure del font si prendono solo da `text_metrics`: i riferimenti le
  calcolano dal file del font con uno strumento indipendente (come `~box`).
- File comune: `tests/CMakeLists.txt`.

## Test richiesti

- `mtext_test.cpp`: ogni codice del punto 2; a capo con una parola più larga
  della colonna; i nove punti di allineamento con due righe; interlinea 1,5;
  rotazione 30°; riflessione con `MIRRTEXT` 0 e 1 (stesso risultato,
  leggibile), anche con uno stile all'indietro; `%%%c` dà la riga `%c` e `%%%%c` la riga `%%c`;
  `A\H2\PB` dà `A\H2` e `B`; `A\~B C` in una colonna larga quanto `A B`
  tiene `A B` insieme e manda `C` a capo; `"  \PZ"` dà due linee di base e un
  solo `Text` (`Z`); un `MTEXT` vuoto si prende con un clic sul punto
  d'inserimento.

## Fatto quando

- I casi di RT13 passano; la CI è verde.
