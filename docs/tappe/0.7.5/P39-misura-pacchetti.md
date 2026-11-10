# [0.7.5-P39] Misura dei pacchetti della 0.7.5 (#442, condizione 7)

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: strumento del metodo e documento; test dello
  script
- **Dipende da:** P33, l'ultimo pacchetto della tappa: la misura conta tutte le
  pull request della 0.7.5, quindi parte quando le altre issue del milestone
  `v0.7.5` sono chiuse
- **Nata:** 2026-11-10. Il responsabile approva le condizioni dell'analisi della
  #442 e chiede due misure, alla fine della 0.7.5 e alla fine della 0.8, con lo
  stesso metodo ([DECISIONI.md](../../DECISIONI.md), 2026-11-10)
- **Modulo:** `scripts`, più il documento della misura
- **Da leggere:** l'[analisi](analisi-pacchetti-medi.md), sezioni «In breve»,
  1 (metodo, definizioni e limiti), 2 (le tabelle da rifare), 3.9 e 5,
  condizione 7, e 7 (dati grezzi e script); `scripts/dipendenze.py` e
  `scripts/dipendenze_test.py` (come sono fatti gli script con i loro test);
  il piano della tappa, sezione 6 (la tabella dei pacchetti, con difficoltà,
  agente, modello e impegno previsti)

## File

- `scripts/misura_pacchetti.py` (nuovo)
- `scripts/misura_pacchetti_test.py` (nuovo)
- `docs/tappe/0.7.5/misura-pacchetti.md` (nuovo)

Nessun altro file. L'analisi di P8 non si tocca: è la fotografia su cui il
responsabile ha deciso.

## Cosa fare

1. **Lo script.** `scripts/misura_pacchetti.py` riunisce in un solo programma
   gli script della sezione 7.2 dell'analisi (`pulls.py`, `runs.py`,
   `raccolta.py` e quelli che seguono), con le stesse definizioni della
   sezione 1: pull request principale e correzioni di ogni issue, commit, file e
   righe cambiate (senza `third_party/`), run di `ci.yml` dell'evento
   `pull_request`, giri di correzione, run annullate, ore tra apertura e
   unione, prese in carico, casi fini e ordinari. Differenze rispetto agli
   script dell'analisi:
   - le tappe sono un argomento (`--tappe 0.7.5`, o più tappe insieme), con le
     tappe di tre numeri e i prefissi `P`/`R` fino alla 0.7.5 e `PT`/`RT` dalla
     0.8 (LAVORO-IN-PARALLELO.md, «Come leggere i riferimenti»);
   - legge dalla tabella della sezione 6 del piano di ogni tappa difficoltà,
     agente, modello e impegno **previsti**, e dalla sezione sull'uso dell'IA di
     ogni pull request strumento, modello e impegno **dichiarati**; dove
     mancano, scrive «non dichiarato», senza dedurlo;
   - conta per ogni pacchetto le pull request di correzione unite dopo la sua
     pull request principale che ne citano il codice (`0.7.5-P24`) nel titolo o
     nella descrizione, distinte in correzioni di riferimenti e correzioni di
     codice;
   - per le pull request della 0.7.5 senza impegno dichiarato usa la
     dichiarazione del responsabile del 2026-11-10 (sotto, «Regole della tappa
     che valgono qui») e la segna come tale, distinta dall'impegno scritto
     nella pull request;
   - le pagine si chiedono una per una con `&page=N`, come nell'analisi, perché
     il proxy delle sessioni cloud rifiuta `gh api --paginate`; solo API REST;
   - salva i dati scaricati in una cartella che riceve come argomento, e i
     calcoli si possono rifare da quella cartella senza rete.
2. **I test.** `scripts/misura_pacchetti_test.py`, con `unittest`, su dati
   finti scritti nel test (niente rete): riconoscimento dei codici con due e
   tre numeri e con i due prefissi; abbinamento della pull request principale e
   delle correzioni; giri di correzione con run fallite, annullate e verdi;
   lettura di strumento, modello e impegno dalla sezione sull'uso dell'IA,
   compresi i casi senza impegno e senza modello; lettura della tabella dei
   pacchetti di un piano; classificazione di un caso fine e di uno ordinario
   con la regola meccanica della sezione 1. La CI li esegue già, come gli altri
   `scripts/*_test.py`.
3. **La misura.** Con lo script, misura la 0.7.5 e scrivi
   `docs/tappe/0.7.5/misura-pacchetti.md`:
   - autore e modello in testa, data e commit di `main` dei dati, come
     nell'analisi;
   - le tabelle della sezione 2 dell'analisi rifatte per la 0.7.5, più una
     tabella per fascia e impegno previsti e dichiarati;
   - il confronto con le tappe 0.4–0.7 dell'analisi, solo dove la differenza
     è grande, con la stessa prudenza della sezione 1 sui campioni piccoli;
   - per ogni condizione della sezione 5 dell'analisi, che cosa dice la 0.7.5:
     confermata, smentita o non misurabile con questi dati, e perché;
   - i limiti: la 0.7.5 ha quasi solo pacchetti medi e un solo pacchetto di
     riferimenti (R28b), e l'impegno si dichiara solo dalle pull request unite
     dopo il 2026-11-10; quindi questa misura dà la base dei pacchetti medi e
     non prova l'ipotesi della sezione 3.9 (codice impegnativo in fascia
     media), che si prova nella 0.8;
   - in fondo il comando esatto che ha prodotto i dati, e che la misura della
     0.8 rifarà.
4. Dove un dato manca o non si può ricavare, lo dici; non lo stimi senza dirlo.

## Regole della tappa che valgono qui

- Il documento distingue ciò che è misurato da ciò che è stimato.
- Scrive l'autore e il modello in testa (LAVORO-IN-PARALLELO.md, «Chi fa cosa»).
- **Impegno usato nella 0.7.5, dichiarato dal responsabile** il 2026-11-10,
  perché le pull request non lo scrivevano: per i pacchetti medi Opus 5.5 con
  impegno medio, o Codex sol con impegno medio; per gli impegnativi Opus 5.5
  con impegno alto, o Codex sol con impegno alto o astra light. Sonnet 5.5 non
  si usa più per i pacchetti di lavoro. È un dato dichiarato, non letto dalle
  pull request: il documento lo dice. Dove il modello scritto nella pull
  request non torna con questa dichiarazione, vale la pull request, e il
  documento segnala la differenza.
- Nessuna regola cambia con questo documento: se la misura suggerisce di
  correggere una condizione, lo scrive come proposta per il responsabile.

## Test richiesti

Quelli del punto 2.

## Fatto quando

- Lo script misura una o più tappe da un argomento, e i suoi test passano in CI.
- Il documento della misura c'è, con il confronto con l'analisi, un giudizio
  per ogni condizione e il comando che lo rifà.
- La CI è verde.
