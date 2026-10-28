# [0.7-P13] Manuale, prima parte

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** facile
- **Agente:** gemini
- **Modello e impegno:** leggero; impegno basso
- **Riferimenti:** non servono: documentazione
- **Dipende da:** P6, P10, P12
- **Modulo:** documentazione
- **Da leggere:** le schede P5, P6, P7, P8, P10, P11 e P12 di questa tappa (per i
  testi dei comandi); gli ADR 0031–0034 (solo le decisioni);
  `docs/manuale/comandi.md` e `docs/manuale/interfaccia.md` (come sono scritti
  oggi)

## File

- `docs/manuale/comandi.md`
- `docs/manuale/interfaccia.md`

## Cosa fare

- **`comandi.md`:**
  - una sezione per `MOVE`, `COPY`, `ROTATE`, `SCALE`, `MIRROR` e `MIRRTEXT`,
    `EXPLODE`, `POLARANG`, con le domande, le opzioni, i messaggi e un esempio da
    scrivere nella riga di comando, come le sezioni esistenti;
  - in `LINE`, `Invio` alla prima domanda;
  - una sezione "Negli script" con la tabella delle sequenze degli script di
    `docs/verifica/script.md`, per i comandi già realizzati (`TEXT`, `COPY`).
- **`interfaccia.md`:**
  - i grip: dove sono, che cosa fa ciascuno (la tabella dell'ADR 0034, punto 4),
    `Esc`;
  - i quattro snap nuovi e il loro marcatore;
  - il tracciamento polare con `F10`, l'esclusione con l'orto, `POLARANG`;
  - la finestra di selezione chiusa trascinando.

## Regole della tappa che valgono qui

- Testi in italiano; comandi, domande e messaggi come li scrive il programma, in
  inglese.
- Solo ciò che è già unito su `main`: per ogni comando controlla che il suo
  pacchetto sia chiuso.

## Test richiesti

Nessuno: la CI controlla i collegamenti della documentazione, se lo fa già.

## Fatto quando

- Ogni comando e ogni aiuto elencati sopra ha la sua sezione.
- La CI è verde.
