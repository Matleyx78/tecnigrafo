# Misura dei pacchetti della 0.8.1 (#442)

- **Pacchetto:** [0.8.1-PT39](PT39-misura-pacchetti.md), issue #985
- **Autore:** Claude (Claude Code), modello Claude Opus 5.5, impegno medio
- **Data:** 2026-11-28
- **Dati:** API REST di GitHub, lette dallo script il 2026-11-28 alle 05:11
  UTC, e `main` al commit `c5aeb6143500b880c29bf01c80da2a6b3c7c5b94`
  (solid-space al commit fissato `aa29390f5ef454add5f9e75a5cb023924ccf9418`),
  dopo l'unione di PT33 (#1067), l'ultima della tappa
- **Stato:** misura per il responsabile. **Nessuna proposta nuova**: la
  proposta di decisione definitiva sulla #442 della
  [misura della 0.8](../0.8/misura-pacchetti.md), sezione 6, resta in sospeso
  come l'ha lasciata il responsabile; qui ci sono solo le cifre della 0.8.1 e
  dove confermano o smentiscono quella proposta (sezione 5).

È la terza misura con lo stesso script, dopo quella della
[0.7.5](../0.7.5/misura-pacchetti.md) e quella della
[0.8](../0.8/misura-pacchetti.md), aggiunta al piano il 2026-11-22
([piano](../0.8.1-completamenti.md), sezione 8): stesso script, stesso metodo,
stesse tabelle. I numeri delle sezioni 2 e 7 vengono da
`scripts/misura_pacchetti.py`, con le due correzioni a mano della sezione 1;
quelli della sezione 2.8 da un conteggio sui commenti con le regole della
misura della 0.8, sezione 1. Dove il testo stima, lo dice.

## In breve

Lo script trova 56 pull request principali unite: 36 pacchetti di lavoro (34
medi, uno facile, uno impegnativo) e 20 pacchetti di riferimenti. PT4 e PT5,
le revisioni delle interfacce, non hanno pull request: il verbale è nella
issue. Cinque cose:

1. **Le fermate sono scese a un terzo.** 20 commenti «Serve una decisione del
   responsabile» (16 nei pacchetti di lavoro), contro 64 (55) della 0.8; 5
   correzioni delle schede durante il lavoro, contro 21. I fermi di tutti i
   pacchetti sono 184 ore (0.8: 258; 0.7.5: 5). È la preparazione della tappa
   decisa il 2026-11-19 (#764), che la 0.8 non aveva ancora. Le fermate
   rimaste vengono però soprattutto da casi di riferimento che sembravano
   sbagliati (13 su 20), anche in fascia forte: la precisazione 3 della
   proposta della 0.8 è confermata nella difesa, non del tutto nell'origine
   (sezione 5).
2. **Nessuna correzione del codice dopo l'unione**, come nella 0.7.5 e nella
   0.8. L'unica che lo script trova (#1060 per PT19) è la pull request
   principale vera, unita dopo una correzione della scheda con il codice nel
   titolo (sezione 1).
3. **Claude ha usato davvero Opus 5.5 medio**, su sei medi che il piano dava
   con impegno medio (PT6, PT16, PT19, PT24, PT25, PT34): due con una run
   fallita, nessuna correzione dopo l'unione, quattro fermate in tre
   pacchetti. I dieci medi con impegno alto previsto sono andati allo stesso
   modo (due su dieci con una run fallita). Ma nessuno dei sei era un
   impegnativo di un tempo: la domanda lasciata aperta dalla 0.8 resta aperta.
4. **La differenza è fra gli autori e fra `app` e il resto.** I medi di Claude
   hanno avuto un giro di CI nel 25 % dei casi (0.8: 57 %), quelli di Codex
   nel 61 % (0.8: 74 %); i pacchetti che toccano `app/` nell'83 % (10 su 12),
   gli altri nel 25 %. I giri non crescono con la misura (Spearman −0,09 sui
   medi).
5. **La catena è veloce come nella 0.8, ma il tempo sta nel lavoro e non nei
   fermi.** 135 ore dalla prima issue all'ultima unione per 56 pull request
   principali (0.8: 178 ore per 79), 2,4 ore per pull request (0.8: 2,25). Il
   percorso critico dura quanto quello della 0.8 (132 ore contro 130,5) con 6
   nodi invece di 13, e il 62 % è lavoro dei nodi (0.8: 44 %); la mediana fra
   il verde e l'unione è salita da 0,76 a 2,5 ore (sezione 6).

## 1. Metodo e limiti

Il metodo è quello della [misura della 0.8](../0.8/misura-pacchetti.md),
sezione 1, con lo stesso script e le stesse definizioni, **senza modifiche**:
`scripts/misura_pacchetti.py` legge già i codici a tre numeri (`0.8.1-PT39`),
l'etichetta `tappa-0.8.1` e il piano `docs/tappe/0.8.1-completamenti.md`.
L'impegno è quello scritto nella pull request, o «non noto» / «non
dichiarato»; quello dei commenti di avvio (`[impegno: …]`) lo script non lo
legge (la precisazione 4 della proposta della 0.8 non è stata decisa) e qui
compare solo nella tabella della sezione 4, contato a mano.

**Correzioni a mano.** Due, come le due della 0.8 (PT21 e PT29), ma questa
volta applicate ai dati prima del calcolo, così **tutte le tabelle sono già
corrette** e si rifanno con il comando della sezione 7.2:

- **PT19:** la #1049 («[0.8.1-PT19] docs: parole chiave non scritte delle
  domande Object», 52 righe di documenti) è la correzione della scheda, unita
  prima del codice; la principale vera è la #1060, che lo script contava come
  «correzione del codice». Nella copia dei dati il titolo della #1049 perde il
  codice del pacchetto e la descrizione si svuota (così non diventa una
  correzione di documenti: la misura della 0.8 conta le correzioni delle
  schede a parte, sezione 2.8).
- **RT31:** la principale vera è la #1027 («test(snap): add extension and
  parallel references», 221 righe, unita il 23/11), che non porta né il codice
  nel titolo né `Closes`; lo script prendeva la #1057, la correzione dei
  riferimenti del 26/11. Nella copia dei dati la descrizione della #1027
  riceve la riga `Closes #974`. La #1027 non ha la sezione sull'uso dell'IA:
  l'autore resta «?» nelle tabelle; dal ramo e dal piano è Codex.

Con le due correzioni la sola «correzione del codice» che lo script trova (la
#1060) sparisce: **nessuna pull request ha corretto il codice di un pacchetto
della 0.8.1 dopo la sua unione**. L'ho controllato anche a mano sulle pull
request dalla #984 che lo script non attribuisce: il piano e le issue (#984,
#986), le decisioni delle revisioni delle interfacce (#992, #993, #994), le
correzioni delle schede (#999, #1026, #1036, #1049, #1052), la CI (#1048,
F13) e un tentativo non unito di PT14 (#1051). Nessuna tocca il codice dopo
un'unione.

**Giri di revisione, fermate e correzioni delle schede (sezione 2.8).** Con le
regole della misura della 0.8, sezione 1, sui commenti delle pull request
principali e delle issue letti il 2026-11-28: tutti i commenti con «Giro di
revisione:», tutti i commenti che cominciano con «Serve una decisione del
responsabile» (tolti quelli della issue che rimandano alla pull request), e le
pull request unite durante la tappa che correggono la scheda di un pacchetto
già in lavoro, o i cui riferimenti sono in lavoro:

- PT14: #1026 (domande del testo di `-ATTDEF`, da una fermata su RT14) e
  #1052 (domanda per una sola parola);
- PT19: #1049; PT23: #1036 (famiglie del motivo); PT34: #999 (prefissi `#` e
  `*`, da una fermata su RT34).

Le #992, #993 e #994 non contano: sono le decisioni delle revisioni PT4 e
PT5, unite il 22/11 prima della presa dei pacchetti che correggono.

**Limiti** (oltre a quelli delle misure precedenti, che valgono tutti):

- **Finestra corta.** I pacchetti sono stati uniti fra il 22 e il 28 novembre,
  e la prova manuale della tappa (sezione 7 del piano) non è ancora fatta.
- **Chi rivede cambia con l'autore**, come nella 0.8: Codex rivede Claude e
  Claude rivede Codex.
- **Impegno di Codex ancora poco noto.** 10 delle 19 pull request di lavoro di
  Codex scrivono «impegno non noto» o niente, 5 non dichiarano il modello; i
  commenti di avvio di Codex non portano `[impegno: …]`.
- **Campioni piccoli.** Sei pacchetti con Opus medio, dieci con Opus alto, un
  solo impegnativo di codice.
- **Fermo e catena.** Come nella 0.8: ogni ripresa dopo una decisione è una
  presa nuova, e le decisioni prese di notte allungano il fermo di ore senza
  lavoro.

## 2. Dati

Uscita dello script sui dati corretti della sezione 1.

### 2.1 Tappa 0.8.1

Colonne come nella misura della 0.7.5, sezione 2.1.

| Tipo | Difficoltà | PR | Commit (med.) | Run CI (med.; tot.) | Run fallite (tot.) | Run annullate (tot.) | PR con ≥1 giro | Ore aperta→unita (med.) | Ore presa→unita (med.) | File (med.) | Righe (med.) | Righe di codice (med.) | Correzioni dei riferimenti (tot.) | Correzioni del codice (tot.) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| pacchetto | facile | 1 | 3 | 3; 3 | 0 | 0 | 0 | 10,7 | 10,7 | 3 | 195 | 0 | 0 | 0 |
| pacchetto | media | 34 | 4 | 3,5; 156 | 23 | 65 | 15 | 7,9 | 16,1 | 11,5 | 646 | 318 | 0 | 0 |
| pacchetto | impegnativa | 1 | 2 | 2; 2 | 1 | 0 | 1 | 4,6 | 5,1 | 9 | 1575 | 1190 | 0 | 0 |
| riferimenti | media | 20 | 2 | 2; 54 | 5 | 17 | 3 | 3,2 | 3,3 | 10 | 598 | 0 | 13 | 0 |

### 2.2 Per difficoltà e autore, pacchetti di lavoro

| Pacchetti di lavoro | PR | Run fallite | PR con ≥1 giro | Quota con ≥1 giro | Run (med.) | Righe (med.) | Run fallite per 1000 righe |
|---|---|---|---|---|---|---|---|
| facile, codex | 1 | 0 | 0 | 0 % | 3 | 195 | 0 |
| media, claude | 16 | 6 | 4 | 25 % | 2 | 1311 | 0,31 |
| media, codex | 18 | 17 | 11 | 61 % | 5 | 454 | 1,91 |
| impegnativa, claude | 1 | 1 | 1 | 100 % | 2 | 1575 | 0,63 |

Pacchetti di lavoro che toccano `app/` o `tests/app/` (Qt): 12, con almeno un giro 10 (0.8.1-PT30, 0.8.1-PT34, 0.8.1-PT9, 0.8.1-PT20, 0.8.1-PT12, 0.8.1-PT32, 0.8.1-PT13, 0.8.1-PT21, 0.8.1-PT33, 0.8.1-PT37).
Pacchetti di lavoro che non toccano `app/` o `tests/app/` (Qt): 24, con almeno un giro 6 (0.8.1-PT35, 0.8.1-PT11, 0.8.1-PT14, 0.8.1-PT19, 0.8.1-PT10, 0.8.1-PT26).
Spearman tra righe e run fallite prima del verde, pacchetti di lavoro tutti (36): -0,02; con le righe di codice: -0,01.
Spearman tra righe e run fallite prima del verde, pacchetti di lavoro medi (34): -0,09; con le righe di codice: -0,09.

### 2.3 Per fascia e impegno, previsti e dichiarati

Come nella misura della 0.8, sezione 2.3. «Claude Opus 5.5 + Claude Opus» è
lo stesso modello nominato due volte nella sezione «Uso dell'IA».

**Previsti dal piano**

| Difficoltà | Fascia prevista | Impegno previsto | PR | PR con ≥1 giro | Run fallite (tot.) | Righe (med.) | Correzioni del codice (tot.) |
|---|---|---|---|---|---|---|---|
| facile | leggero | basso | 1 | 0 | 0 | 195 | 0 |
| impegnativa | forte | alto | 1 | 1 | 1 | 1575 | 0 |
| media | medio | alto | 10 | 2 | 2 | 1452 | 0 |
| media | medio | medio | 24 | 13 | 21 | 528 | 0 |

**Dichiarati**

| Autore | Modello dichiarato | Impegno | Fonte dell'impegno | PR | PR con ≥1 giro | Run fallite (tot.) | Righe (med.) | Correzioni del codice (tot.) |
|---|---|---|---|---|---|---|---|---|
| claude | Claude Opus 5.5 | alto | pull request | 8 | 2 | 2 | 1486 | 0 |
| claude | Claude Opus 5.5 | medio | pull request | 2 | 2 | 4 | 972 | 0 |
| claude | Claude Opus 5.5 | non noto | non dichiarato | 1 | 0 | 0 | 1505 | 0 |
| claude | Claude Opus 5.5 + Claude Opus | alto | pull request | 2 | 1 | 1 | 1264 | 0 |
| claude | Claude Opus 5.5 + Claude Opus | medio | pull request | 4 | 0 | 0 | 776 | 0 |
| codex | GPT-5.6 sol | medio | pull request | 9 | 6 | 8 | 410 | 0 |
| codex | GPT-5.6 sol | non noto | non dichiarato | 3 | 2 | 4 | 695 | 0 |
| codex | GPT-6 | non noto | non dichiarato | 2 | 0 | 0 | 396 | 0 |
| codex | non dichiarato | non dichiarato | non dichiarato | 5 | 3 | 5 | 521 | 0 |

I pacchetti di Claude con impegno alto dichiarato sono 10 (i dieci medi
previsti con impegno alto, meno PT7 che non lo scrive, più PT12); quelli con
impegno medio 6, esattamente i sei medi previsti con impegno medio. Nessun
pacchetto di Claude ha cambiato l'impegno previsto dal piano.

### 2.4 Misura dei pacchetti (condizione 2)

Pacchetti di lavoro uniti: 36; con più di 15 file (test e documenti compresi): 9; con più di 600 righe di codice: 12; con meno di 150 righe di codice: 8; righe di codice, mediana 318.

Oltre le 600 righe di codice: PT7, PT8, PT10, PT12, PT19, PT22, PT23, PT24,
PT25, PT28, PT31 e PT35; quattro di loro hanno avuto almeno una run fallita
prima del verde (33 %: PT10, PT12, PT19, PT35), contro 12 su 24 degli altri
(50 %).

### 2.5 Costi fissi

- Run verde, durata mediana: 22,3 minuti (120 run verdi delle pull request principali, di correzione e dei tentativi).
- Vita della pull request principale: apertura→ultimo verde 1,98 ore, verde→unione 2,50 ore (mediane su 56).
- Pull request principali unite: 56; correzioni dei riferimenti 13, del codice 0, di documenti 0; tentativi non uniti 1.
- Run delle pull request principali: 215, di cui 29 fallite e 82 annullate.

### 2.6 Tempi

«Fermo», «ultimo lavoro», «attesa» e percorso critico come nella misura della
0.7.5, sezione 2.6. Orari UTC.

| Pacchetto | Difficoltà | Ultima dipendenza unita | Prima presa | Ultima presa prima della PR | Unito | Fermo (h) | Ultimo lavoro (h) | Attesa da pronto a presa (h) |
|---|---|---|---|---|---|---|---|---|
| PT1 | media | — | 22/11 14:07 | 22/11 14:07 | 22/11 17:06 | 0 | 3,0 | 0,5 |
| PT2 | media | — | 22/11 14:07 | 22/11 14:07 | 22/11 17:03 | 0 | 2,9 | 0,5 |
| PT3 | media | RT3 22/11 17:07 | 22/11 17:10 | 22/11 17:10 | 23/11 05:40 | 0 | 12,5 | 0,1 |
| PT35 | media | RT35 22/11 17:07 | 22/11 17:11 | 22/11 17:11 | 23/11 09:17 | 0 | 16,1 | 0,1 |
| PT36 | media | RT36 22/11 19:29 | 22/11 19:32 | 22/11 19:32 | 23/11 11:18 | 0 | 15,8 | 0,1 |
| PT6 | media | RT6 22/11 18:34 | 22/11 18:35 | 22/11 18:35 | 23/11 06:22 | 0 | 11,8 | 0 |
| PT7 | media | — | 22/11 17:14 | 22/11 17:14 | 23/11 09:17 | 0 | 16,0 | 3,6 |
| PT11 | media | RT11 23/11 08:31 | 23/11 08:34 | 23/11 08:34 | 25/11 06:33 | 0 | 46,0 | 0,1 |
| PT17 | media | RT17 23/11 08:06 | 23/11 08:09 | 23/11 08:09 | 23/11 11:44 | 0 | 3,6 | 0 |
| PT22 | media | RT22 22/11 18:34 | 22/11 18:35 | 23/11 09:20 | 23/11 10:28 | 14,7 | 1,1 | 0 |
| PT27 | media | RT27 22/11 20:11 | 22/11 20:12 | 23/11 11:19 | 23/11 16:47 | 15,1 | 5,5 | 0 |
| PT30 | media | RT30 23/11 09:16 | 23/11 09:17 | 23/11 09:17 | 23/11 14:56 | 0 | 5,7 | 0 |
| PT34 | media | RT34 23/11 09:18 | 23/11 09:19 | 23/11 11:19 | 24/11 14:23 | 2 | 27,1 | 0 |
| PT15 | media | RT15 24/11 04:28 | 24/11 04:29 | 25/11 05:51 | 25/11 11:08 | 25,4 | 5,3 | 0 |
| PT8 | media | PT30 23/11 14:56 | 23/11 16:42 | 24/11 04:27 | 24/11 09:35 | 11,8 | 5,1 | 1,8 |
| PT14 | media | PT15 25/11 11:08 | 25/11 11:27 | 25/11 16:41 | 27/11 06:00 | 5,2 | 37,3 | 0,3 |
| PT16 | media | PT15 25/11 11:08 | 25/11 11:12 | 25/11 11:12 | 25/11 14:24 | 0 | 3,2 | 0,1 |
| PT19 | media | PT15 25/11 11:08 | 25/11 11:15 | 26/11 05:06 | 26/11 10:32 | 17,8 | 5,4 | 0,1 |
| PT23 | media | RT23 24/11 14:24 | 24/11 14:25 | 25/11 06:31 | 25/11 14:24 | 16,1 | 7,9 | 0 |
| PT24 | media | PT15 25/11 11:08 | 25/11 11:16 | 25/11 14:23 | 26/11 10:31 | 3,1 | 20,1 | 0,1 |
| PT28 | media | PT34 24/11 14:23 | 25/11 05:18 | 25/11 06:37 | 25/11 14:25 | 1,3 | 7,8 | 14,9 |
| PT29 | media | PT27 23/11 16:47 | 23/11 16:55 | 23/11 16:55 | 24/11 04:26 | 0 | 11,5 | 0,1 |
| PT9 | media | PT8 24/11 09:35 | 25/11 05:19 | 25/11 05:19 | 25/11 11:11 | 0 | 5,9 | 19,7 |
| PT10 | media | PT8 24/11 09:35 | 25/11 05:18 | 25/11 05:18 | 25/11 11:12 | 0 | 5,9 | 19,7 |
| PT18 | media | PT16 25/11 14:24 | 25/11 15:30 | 26/11 05:13 | 26/11 12:57 | 13,7 | 7,7 | 1,1 |
| PT20 | media | PT16 25/11 14:24 | 25/11 15:28 | 25/11 16:38 | 26/11 10:31 | 1,2 | 17,9 | 1,1 |
| PT25 | media | PT24 26/11 10:31 | 26/11 14:02 | 26/11 14:02 | 27/11 06:03 | 0 | 16,0 | 3,5 |
| PT26 | media | PT23 25/11 14:24 | 25/11 15:27 | 26/11 05:15 | 26/11 13:43 | 13,8 | 8,5 | 1,1 |
| PT31 | media | PT28 25/11 14:25 | 25/11 15:15 | 26/11 05:06 | 26/11 10:30 | 13,9 | 5,4 | 0,8 |
| PT12 | impegnativa | PT10 25/11 11:12 | 25/11 11:17 | 25/11 11:17 | 25/11 16:21 | 0 | 5,1 | 0,1 |
| PT32 | media | PT31 26/11 10:30 | 26/11 14:04 | 26/11 14:04 | 27/11 06:02 | 0 | 16,0 | 3,6 |
| PT13 | media | PT12 25/11 16:21 | 25/11 16:43 | 25/11 16:43 | 27/11 06:03 | 0 | 37,3 | 0,4 |
| PT21 | media | PT20 26/11 10:31 | 26/11 14:03 | 26/11 14:03 | 27/11 14:08 | 0 | 24,1 | 3,5 |
| PT33 | media | PT25 27/11 06:03 | 27/11 06:06 | 27/11 06:06 | 28/11 05:06 | 0 | 23,0 | 0,1 |
| PT37 | media | PT21 27/11 14:08 | 27/11 14:13 | 27/11 14:13 | 27/11 18:19 | 0 | 4,1 | 0,1 |
| PT38 | facile | PT37 27/11 18:19 | 27/11 18:22 | 27/11 18:22 | 28/11 05:04 | 0 | 10,7 | 0,1 |

Percorso critico realizzato: PT2 → RT15 → PT15 → PT24 → PT25 → PT33; dall'unione di PT2 all'ultima unione 132 ore, ultimo lavoro dei nodi dopo il primo 82,2 ore; fermi di tutti i pacchetti 184,1 ore; attesa da pronto a presa, mediana 0,1 ore su 36 pacchetti di lavoro; dalla prima issue all'ultima unione 135,4 ore.

### 2.7 Riferimenti

Casi fini e ordinari contati con la regola della sezione 1 dell'analisi, su
tutte le cartelle di casi toccate da ciascun pacchetto, come sono oggi su
`main`.

| Riferimenti | Chi li ha scritti (modello dichiarato) | Domande (fini) | Script (fini) | Correzioni dei riferimenti | Righe |
|---|---|---|---|---|---|
| 0.8.1-RT3 | claude (Claude Opus 5.5) | 0 (0) | 31 (19) | — | 780 |
| 0.8.1-RT35 | claude (Claude Opus 5.5) | 26 (4) | 5 (5) | — | 1080 |
| 0.8.1-RT36 | claude (Claude Opus 5.5) | 13 (9) | 6 (5) | — | 521 |
| 0.8.1-RT6 | codex (GPT-6) | 40 (39) | 0 (0) | — | 59 |
| 0.8.1-RT11 | claude (Claude Opus 5.5) | 0 (0) | 6 (5) | — | 1165 |
| 0.8.1-RT17 | claude (Claude Opus 5.5) | 0 (0) | 4 (4) | — | 1890 |
| 0.8.1-RT22 | codex (GPT-6) | 51 (21) | 0 (0) | #1009 | 115 |
| 0.8.1-RT27 | codex (GPT-6) | 49 (2) | 2 (2) | #1008, #1017 | 689 |
| 0.8.1-RT30 | codex (GPT-6) | 21 (1) | 2 (2) | — | 770 |
| 0.8.1-RT34 | codex (non dichiarato) | 0 (0) | 3 (0) | #1016 | 227 |
| 0.8.1-RT15 | codex (GPT-5.6 sol) | 3 (1) | 0 (0) | #1034, #1035 | 19 |
| 0.8.1-RT8 | codex (non dichiarato) | 15 (5) | 0 (0) | #1030 | 84 |
| 0.8.1-RT14 | claude (Claude Opus 5.5) | 0 (0) | 7 (7) | #1054 | 901 |
| 0.8.1-RT19 | codex (non dichiarato) | 0 (0) | 2 (0) | #1058 | 271 |
| 0.8.1-RT23 | codex (GPT-5.6 sol) | 22 (11) | 0 (0) | #1037 | 59 |
| 0.8.1-RT24 | codex (GPT-5.6 sol) | 41 (9) | 0 (0) | #1050 | 658 |
| 0.8.1-RT28 | codex (GPT-5.6 sol) | 0 (0) | 5 (5) | #1040 | 538 |
| 0.8.1-RT18 | claude (Claude Opus 5.5 + Claude Opus) | 0 (0) | 6 (5) | — | 6836 |
| 0.8.1-RT26 | claude (Claude Opus 5.5) | 0 (0) | 7 (4) | — | 1014 |
| 0.8.1-RT31 | ? (non dichiarato) | 0 (0) | 2 (1) | #1057 | 221 |

Per fascia prevista e autore:

| Fascia prevista | Autore | Pacchetti | Correzioni | Per pacchetto | 0.8 |
|---|---|---|---|---|---|
| forte | codex (GPT-6, GPT-5.6 sol o non dichiarato) | 6 | 5 | 0,8 | 1,0 |
| leggero | claude (Opus 5.5) | 8 | 1 | 0,1 | 0,4 |
| leggero | codex (GPT-6, GPT-5.6 sol o non dichiarato) | 6 | 7 | 1,2 | 1,9 |

Dai titoli, almeno una correzione viene da una decisione e non da un errore
del caso: la #1054 (RT14), che toglie un'etichetta con lo spazio dopo la
decisione del 2026-11-25 sulla domanda per una sola parola di `-ATTDEF`. Le
altre dicono «correggi» senza la causa: la riga che la proposta della 0.8
chiedeva (precisazione 3) non è una regola, e la misura non separa i due casi.

### 2.8 Giri di revisione, fermate e correzioni delle schede

Gruppi della sezione 4, con le regole della sezione 1. Tutti i pacchetti di
lavoro tranne PT38 (facile, documenti: due giri di revisione, nessuna run
fallita, nessuna fermata).

| Gruppo | Pacchetti | Con ≥1 run fallita | Run fallite | Giri di revisione (med.; tot.) | Con ≥2 giri di revisione | Fermate per una decisione (pacchetti) | Correzioni delle schede | Correzioni del codice dopo l'unione | Ore presa→unione (med.) | Fermi (h) | Righe di codice (med.) |
|---|---|---|---|---|---|---|---|---|---|---|---|
| A. Medi di Claude con impegno alto previsto | 10 | 2 | 2 | 1; 14 | 4 | 10 (8) | 1 | 0 | 16,5 | 98,3 | 877 |
| B. Medi di Claude con impegno medio previsto | 6 | 2 | 4 | 1,5; 9 | 3 | 4 (3) | 2 | 0 | 19,6 | 22,9 | 462 |
| C. Medi di Codex | 18 | 11 | 17 | 2; 43 | 16 | 2 (2) | 2 | 0 | 16,0 | 33,9 | 226 |
| D. Impegnativo di codice in fascia forte (PT12) | 1 | 1 | 1 | 1; 1 | 0 | 0 (0) | 0 | 0 | 5,1 | 0 | 1190 |

A: PT7, PT8, PT10, PT15, PT22, PT23, PT27, PT28, PT30, PT31. B: PT6, PT16,
PT19, PT24, PT25, PT34. C: PT1, PT2, PT3, PT9, PT11, PT13, PT14, PT17, PT18,
PT20, PT21, PT26, PT29, PT32, PT33, PT35, PT36, PT37.

Le altre quattro fermate sono nei riferimenti: tre per RT14 (una nella issue,
due nella pull request) e una per RT15.

Nella 0.8, per confronto: A (impegnativi di un tempo in fascia media) 20
fermate in 8 pacchetti su 9, B (altri medi di Claude con impegno alto) 19 in 9
su 11, C (medi con impegno medio) 12 in 8 su 20.

## 3. Confronto con la 0.7.5 e con la 0.8

Solo dove la differenza è grande, con la prudenza dell'analisi sui campioni
piccoli. Le colonne 0.7.5 e 0.8 sono quelle della misura della 0.8, sezione 3.

| Che cosa | 0.7.5 | 0.8 | 0.8.1 | Giudizio |
|---|---|---|---|---|
| Pull request principali (lavoro + riferimenti) | 39 | 79 (49 + 30) | 56 (36 + 20) | — |
| Fermi di tutti i pacchetti | 5 h | 258 h | 184 h | meno della 0.8, lontano dalla 0.7.5 |
| Fermate per una decisione (nei pacchetti di lavoro) | — | 64 (55) | 20 (16) | grande: un terzo |
| Correzioni delle schede durante il lavoro | — | 21 | 5 | grande |
| Durata della tappa (prima issue → ultima unione) | 71 h | 178 h | 135 h | con 56 pull request invece di 79 |
| Medi con almeno un giro di CI | 48 % (15 su 31) | 65 % (26 su 40) | 44 % (15 su 34) | come la 0.7.5 |
| Medi di Claude / di Codex con almeno un giro | 47 % / 50 % | 57 % / 74 % | 25 % (4 su 16) / 61 % (11 su 18) | Claude più basso (stima, campioni piccoli) |
| Lavoro con almeno un giro, che tocca `app` / gli altri | 59 % / 19 % | 50 % / 61 % | 83 % (10 su 12) / 25 % (6 su 24) | torna l'ipotesi di `app` della 0.7.5 |
| Spearman righe di codice / giri, medi | 0,38 | 0,00 | −0,09 | nessuna relazione, come nella 0.8 |
| Correzioni dei riferimenti per pacchetto di riferimenti | 0 (su 1) | 0,94 (29 su 31) | 0,65 (13 su 20) | meno, soprattutto per Claude |
| Correzioni del codice dopo l'unione | 0 | 0 | 0 | uguale, su finestre corte |
| Durata mediana di una run verde | 13,0 min | 16,2 min | 22,3 min | la CI cresce con il codice |
| Verde → unione, pull request principali (mediana) | 0,14 h | 0,76 h | 2,50 h | grande: la revisione della catena |
| Medi con ≥2 giri di revisione, di Claude / di Codex | — | 6 su 18 / 16 su 19 | 7 su 16 / 16 su 18 | uguale |

**Le fermate.** Il calo viene, per quanto si vede, dalla preparazione della
tappa: revisione del piano (#924), seduta in AutoCAD prima dell'apertura,
revisioni delle interfacce PT4 e PT5 con le decisioni applicate prima della
presa (#992, #993, #994). Ho letto le 20 fermate una per una: 13 sono casi di
riferimento che sembravano sbagliati a chi realizzava il pacchetto (PT8, PT15
due, PT19, PT22, PT23, PT24, PT27 due, PT28, PT30, PT31, PT34), 3 buchi delle
schede (PT19, RT14, RT15), 3 una CI che non finiva o restava rossa per motivi
estranei (RT14 due, PT13), una una [decisione] di revisione (PT21). Le 13 sui
casi stanno sui riferimenti in fascia forte (6, uno per ciascuno dei sei
pacchetti forti) e su quelli leggeri di Codex (7, in cinque pacchetti su
sei); nessuna sui riferimenti leggeri di Claude.

**`app`.** Nella 0.8 il codice fuori da `app` era geometria e DXF, con molti
giri; nella 0.8.1 è soprattutto modello e comandi, coperti dai riferimenti, e
i giri sono scesi al 25 %. I pacchetti di `app` (editor, finestre, pannello,
vista, interfaccia) hanno avuto quasi tutti un giro: test Qt che si vedono
solo nella CI.

**Le revisioni.** Come nella 0.8, i medi di Codex rivisti da Claude hanno quasi
sempre due o più giri (16 su 18), quelli di Claude rivisti da Codex meno di
metà (7 su 16). Il modello di chi scrive non si separa da quello di chi rivede.

## 4. Pacchetto per pacchetto: previsto e successo

La scheda chiede, per ogni pacchetto, difficoltà, fascia e impegno previsti
contro quello che è successo. «Previsti» sono le colonne del piano, sezione 6;
«Dichiarato» la sezione «Uso dell'IA» della pull request principale;
«Impegno nell'avvio» i valori `[impegno: …]` dei commenti di avvio della
issue, in ordine (contati a mano; Codex non li scrive). Le altre colonne come
nella sezione 2.8; ore e fermo dallo script. PT4 e PT5 (impegnativi,
revisioni in fascia forte) non hanno pull request e nessuna fermata nella
issue.

| Pacchetto | Diff. | Agente | Previsti (fascia, impegno) | Dichiarato (modello, impegno) | Impegno nell'avvio | Righe di codice | Run fallite | Giri di revisione | Fermate | Correzioni della scheda | Correzioni del codice | Ore presa→unione | Fermo (h) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| PT1 | media | codex | medio, medio | GPT-6, non noto | — | 0 | 0 | 1 | 0 | — | 0 | 3,0 | 0 |
| PT2 | media | codex | medio, medio | GPT-6, non noto | — | 113 | 0 | 1 | 0 | — | 0 | 2,9 | 0 |
| PT3 | media | codex | medio, medio | GPT-5.6 sol, medio | — | 115 | 0 | 2 | 0 | — | 0 | 12,5 | 0 |
| PT6 | media | claude | medio, medio | Claude Opus 5.5, medio | medio | 144 | 0 | 1 | 0 | — | 0 | 11,8 | 0 |
| PT7 | media | claude | medio, alto | Claude Opus 5.5, non noto | — | 870 | 0 | 1 | 0 | — | 0 | 16,0 | 0 |
| PT8 | media | claude | medio, alto | Claude Opus 5.5, alto | alto | 1011 | 0 | 1 | 1 | — | 0 | 16,9 | 11,8 |
| PT9 | media | codex | medio, medio | GPT-5.6 sol, medio | — | 93 | 1 | 3 | 0 | — | 0 | 5,9 | 0 |
| PT10 | media | claude | medio, alto | Claude Opus 5.5, alto | alto | 1364 | 1 | 1 | 0 | — | 0 | 5,9 | 0 |
| PT11 | media | codex | medio, medio | GPT-5.6 sol, medio | — | 490 | 3 | 3 | 0 | — | 0 | 46,0 | 0 |
| PT12 | impegnativa | claude | forte, alto | Claude Opus 5.5, alto | alto | 1190 | 1 | 1 | 0 | — | 0 | 5,1 | 0 |
| PT13 | media | codex | medio, medio | GPT-5.6 sol, non noto | — | 298 | 3 | 3 | 1 | — | 0 | 37,3 | 0 |
| PT14 | media | codex | medio, medio | non dichiarato, non dichiarato | — | 550 | 1 | 2 | 0 | #1026, #1052 | 0 | 42,6 | 5,2 |
| PT15 | media | claude | medio, alto | Claude Opus 5.5, alto | alto | 339 | 0 | 2 | 2 | — | 0 | 30,6 | 25,4 |
| PT16 | media | claude | medio, medio | Claude Opus 5.5, medio | medio | 262 | 0 | 1 | 0 | — | 0 | 3,2 | 0 |
| PT17 | media | codex | medio, medio | GPT-5.6 sol, medio | — | 175 | 0 | 2 | 0 | — | 0 | 3,6 | 0 |
| PT18 | media | codex | medio, medio | GPT-5.6 sol, non noto | — | 235 | 0 | 2 | 0 | — | 0 | 21,4 | 13,7 |
| PT19 | media | claude | medio, medio | Claude Opus 5.5, medio | medio | 1002 | 1 | 2 | 2 | #1049 | 0 | 23,3 | 17,8 |
| PT20 | media | codex | medio, medio | GPT-5.6 sol, non noto | — | 567 | 1 | 4 | 0 | — | 0 | 19,1 | 1,2 |
| PT21 | media | codex | medio, medio | GPT-5.6 sol, medio | — | 217 | 1 | 3 | 1 | — | 0 | 24,1 | 0 |
| PT22 | media | claude | medio, alto | Claude Opus 5.5, alto | alto | 922 | 0 | 1 | 1 | — | 0 | 15,9 | 14,7 |
| PT23 | media | claude | medio, alto | Claude Opus 5.5, alto | alto | 726 | 0 | 2 | 1 | #1036 | 0 | 24,0 | 16,1 |
| PT24 | media | claude | medio, medio | Claude Opus 5.5, medio | medio | 662 | 0 | 2 | 1 | — | 0 | 23,2 | 3,1 |
| PT25 | media | claude | medio, medio | Claude Opus 5.5, medio | medio | 885 | 0 | 1 | 0 | — | 0 | 16,0 | 0 |
| PT26 | media | codex | medio, medio | non dichiarato, non dichiarato | — | 402 | 2 | 3 | 0 | — | 0 | 22,3 | 13,8 |
| PT27 | media | claude | medio, alto | Claude Opus 5.5, alto | alto | 584 | 0 | 2 | 2 | — | 0 | 20,6 | 15,1 |
| PT28 | media | claude | medio, alto | Claude Opus 5.5, alto | alto | 883 | 0 | 2 | 1 | — | 0 | 9,1 | 1,3 |
| PT29 | media | codex | medio, medio | GPT-5.6 sol, medio | — | 211 | 0 | 2 | 0 | — | 0 | 11,5 | 0 |
| PT30 | media | claude | medio, alto | Claude Opus 5.5, alto | alto, medio | 223 | 1 | 1 | 1 | — | 0 | 5,7 | 0 |
| PT31 | media | claude | medio, alto | Claude Opus 5.5, alto | alto | 1230 | 0 | 1 | 1 | — | 0 | 19,2 | 13,9 |
| PT32 | media | codex | medio, medio | GPT-5.6 sol, medio | — | 143 | 1 | 2 | 0 | — | 0 | 16,0 | 0 |
| PT33 | media | codex | medio, medio | non dichiarato, non dichiarato | — | 261 | 2 | 3 | 0 | — | 0 | 23,0 | 0 |
| PT34 | media | claude | medio, medio | Claude Opus 5.5, medio | medio, alto | 217 | 3 | 2 | 1 | #999 | 0 | 29,1 | 2 |
| PT35 | media | codex | medio, medio | GPT-5.6 sol, medio | — | 606 | 1 | 2 | 0 | — | 0 | 16,1 | 0 |
| PT36 | media | codex | medio, medio | non dichiarato, non dichiarato | — | 256 | 0 | 2 | 0 | — | 0 | 15,8 | 0 |
| PT37 | media | codex | medio, medio | GPT-5.6 sol, medio | — | 114 | 1 | 3 | 0 | — | 0 | 4,1 | 0 |
| PT38 | facile | codex | leggero, basso | non dichiarato, non dichiarato | — | 0 | 0 | 2 | 0 | — | 0 | 10,7 | 0 |

**Impegno previsto e usato.** Claude ha seguito il piano in tutti i 17
pacchetti: alto dove il piano lo chiedeva, medio negli altri sei. Due
eccezioni nei commenti di avvio: PT30 è stato ripreso con impegno medio per
una correzione, PT34 con impegno alto all'ultima ripresa. Codex dichiara
«medio» in 9 pacchetti su 19; negli altri l'impegno non si sa.

**Opus 5.5 medio (gruppo B).** Sei pacchetti fra 144 e 1002 righe di codice:
PT19 e PT34 hanno avuto run fallite (una e tre) e due giri di revisione, PT24
due giri; nessuna correzione dopo l'unione. Delle quattro fermate una è un
buco della scheda (le parole chiave non scritte delle domande *Object*, PT19)
e tre casi di riferimento che sembravano sbagliati (PT19, PT24, PT34), non
errori del codice. Con impegno alto (gruppo A) i pacchetti erano più grandi
(mediana 877 righe contro 462) e hanno avuto più fermate (10 in 8 su 10,
tutte su casi di riferimento) e più fermi (98 ore): il piano ha messo in A i
pacchetti con i riferimenti in fascia forte, cioè con calcoli delicati, e lì
si sono fermati.

**L'impegnativo (PT12).** L'editor nella finestra, 1190 righe di codice, una
run fallita, un giro di revisione, nessuna fermata, 5,1 ore dalla presa
all'unione: il più veloce dei pacchetti grossi. La sua scheda era stata
corretta prima della presa dalle decisioni della revisione PT4 e delle prove
in AutoCAD (#993, #994); se sia questo il motivo, una tappa sola non lo
dice.

**Codex (gruppo C).** I pacchetti più piccoli (mediana 226 righe), con più giri
di CI (11 su 18) e di revisione (16 su 18 con almeno due), ma quasi nessuna
fermata (PT13 e PT21, una ciascuno, nella pull request). PT13 (18 run, 12
annullate) e PT33 (19 run, 14 annullate) da soli fanno un terzo delle run
annullate delle pull request principali.

## 5. Dove le cifre confermano o smentiscono la proposta della 0.8

La proposta è quella della [misura della 0.8](../0.8/misura-pacchetti.md),
sezione 6, con le sue quattro precisazioni. Non la cambio e non ne aggiungo:
dico solo che cosa ne dicono le cifre della 0.8.1.

| Precisazione | Che cosa dice la 0.8.1 | Perché |
|---|---|---|
| 1. Condizione 3 come è stata provata: medi con decisioni scritte e riferimenti forti in fascia media, Opus 5.5 con l'impegno della scheda; Opus medio sugli impegnativi di un tempo da provare nella 0.8.1 | confermata per Opus alto; Opus medio provato solo sui medi ordinari | Gruppo A: 2 su 10 con una run fallita, nessuna correzione dopo l'unione. Gruppo B (Opus medio): 2 su 6, nessuna correzione. Ma il piano della 0.8.1 non ha messo un impegnativo di un tempo in Opus medio: la prova che la precisazione chiedeva non è stata fatta. |
| 2. Condizione 2 come indicazione: 600 righe di codice per cercare una divisione, non un limite | confermata | Spearman −0,09 sui medi; dei 12 pacchetti oltre le 600 righe 4 hanno avuto una run fallita (33 %), degli altri 12 su 24 (50 %). |
| 3. Condizioni 4 e 5: le fermate vengono dalle schede, non dalla fascia; la difesa è la preparazione della tappa del 2026-11-19; la causa scritta nelle correzioni dei riferimenti | confermata la difesa e «non dalla fascia»; smentita in parte l'origine; la causa non si misura | Con la preparazione le fermate scendono da 64 a 20 e le correzioni delle schede da 21 a 5. Ma le fermate rimaste vengono soprattutto dai casi di riferimento (13 su 20), e la fascia forte non le evita: sei, una per ogni pacchetto di riferimenti forte (sezione 3). La riga sulla causa non è una regola: delle 13 correzioni dei riferimenti solo una (#1054) dice nel titolo da dove viene. |
| 4. L'impegno anche dalla catena: quello del commento di avvio quando la pull request non lo sa | ancora necessaria, ma non basta | 11 pacchetti di lavoro su 36 non dichiarano l'impegno; uno solo è di Claude (PT7, che non ha neanche un avvio con `[impegno: …]`), gli altri dieci sono di Codex, i cui avvii non lo scrivono. Leggere l'avvio coprirebbe quasi solo Claude, che già lo dichiara. |

Le condizioni della #442 nella tabella della sezione 5 della misura della 0.8
non cambiano giudizio, con due differenze: per la condizione 5 (revisione
delle interfacce prima delle parti) la 0.8.1 dice «necessaria e, con la
preparazione, quasi sufficiente» (16 fermate nei pacchetti di lavoro invece di
55); per la condizione 7 (si registra e si misura) il modello è dichiarato da
31 pull request di lavoro su 36 e l'impegno da 25 (0.8: 46 e 34 su 49).

**Che cosa le cifre non dicono**, oltre a quello che la 0.8 non diceva già:

- se Opus 5.5 medio basterebbe per un impegnativo di un tempo: nella 0.8.1 non
  ce n'era nessuno in fascia media con impegno medio;
- se i pacchetti di `app` hanno più giri per il codice Qt o perché i loro test
  girano solo nella CI: la misura non separa le due cose;
- se il codice regge nell'uso: la prova manuale della tappa non è fatta.

## 6. Velocità della catena

Come la vuole la scheda: per Tecnigrafo soltanto, dal primo all'ultimo
pacchetto della tappa con tutto ciò che sta in mezzo (attese, notti, fermate,
riprese), contro la 0.8, con i tempi che lo script già calcola. La misura del
pacchetto stesso (PT47 nella 0.8, PT39 qui) non c'è in nessuna delle due. I
numeri della 0.8 sono quelli della sua misura, con la #905 per PT21 e la #907
per PT29.

| Che cosa | 0.8 | 0.8.1 |
|---|---|---|
| Pull request principali unite (lavoro + riferimenti) | 79 (49 + 30) | 56 (36 + 20) |
| Prima issue → ultima unione | 178 h (7,4 giorni) | 135,4 h (5,6 giorni) |
| Prima presa in carico → ultima unione | 168,9 h | 135,0 h |
| Ore per pull request principale | 2,25 | 2,42 |
| Pull request principali al giorno | 10,7 | 9,9 |
| Pacchetti di lavoro al giorno | 6,6 | 6,4 |
| Righe di codice delle pull request principali | 28 850 | 17 360 |
| Righe di codice al giorno | 3 890 | 3 080 |
| Percorso critico (dall'unione del primo nodo all'ultima unione) | 130,5 h, 13 nodi | 132 h, 6 nodi |
| Ultimo lavoro dei nodi dopo il primo | 57,3 h (44 %) | 82,2 h (62 %) |
| Fermi di tutti i pacchetti | 257,8 h | 184,1 h |
| Attesa da pronto a presa, mediana | 0,2 h | 0,1 h |
| Apertura → ultimo verde, mediana | 0,78 h | 1,98 h |
| Verde → unione, mediana | 0,76 h | 2,50 h |

Le righe di codice della 0.8 sono la somma della colonna «Righe di codice»
della sua sezione 7.1, con le 826 della #905 e le 617 della #907 al posto di
quelle della #900 e della #897; le ore per pull request e le pull request al
giorno dividono la durata dalla prima issue.

**Che cosa ne viene.** La catena produce pull request al ritmo della 0.8 (2,4
ore l'una contro 2,25), e la tappa è durata meno perché aveva meno pacchetti.
Ma il tempo si è spostato: i fermi sono calati di 74 ore, mentre ogni pull
request resta aperta più a lungo, prima del verde (2 ore invece di 0,8) e
dopo (2,5 invece di 0,8). Il percorso critico, con meno della metà dei nodi,
dura quanto quello della 0.8: ogni nodo è più lungo (PT33 23 ore con 19 run,
PT24 20, PT25 16), e la catena PT2 → RT15 → PT15 → PT24 → PT25 → PT33 è fatta
quasi tutta di lavoro e di revisione. Sono stime su una tappa sola: una run
verde costa 22 minuti invece di 16, e le run annullate delle pull request
principali sono 82 su 215.

## 7. Dati grezzi e comando

### 7.1 Per pacchetto

Come nella misura della 0.7.5, sezione 7.1, sui dati corretti della sezione 1.

| Codice | Issue | Tipo | Diff. | Previsti (agente, fascia, impegno) | Autore | Modello dichiarato | Impegno (fonte) | PR | Commit | File | Righe | Righe di codice | Run | Fallite | Annullate | Giri | Ore PR | Ore presa→unione | Correzioni riferimenti | Correzioni codice | Altre PR |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 0.8.1-PT1 | #925 | pacchetto | media | codex, medio, medio | codex | GPT-6 | non noto | #987 | 1 | 3 | 534 | 0 | 1 | 0 | 0 | 0 | 2,9 | 3,0 | — | — | — |
| 0.8.1-PT2 | #926 | pacchetto | media | codex, medio, medio | codex | GPT-6 | non noto | #988 | 3 | 11 | 258 | 113 | 3 | 0 | 2 | 0 | 2,8 | 2,9 | — | — | — |
| 0.8.1-PT4 | #927 | pacchetto | impegnativa | codex, forte, alto | ? | non dichiarato | non dichiarato | — | — | — | — | — | — | — | — | — | — | — | — | — | — |
| 0.8.1-PT5 | #928 | pacchetto | impegnativa | codex, forte, alto | ? | non dichiarato | non dichiarato | — | — | — | — | — | — | — | — | — | — | — | — | — | — |
| 0.8.1-RT3 | #929 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 | non noto | #989 | 1 | 63 | 780 | 0 | 1 | 0 | 0 | 0 | 2,8 | 2,9 | — | — | — |
| 0.8.1-PT3 | #930 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | medio (pull request) | #1000 | 4 | 14 | 221 | 115 | 4 | 0 | 1 | 0 | 12,3 | 12,5 | — | — | — |
| 0.8.1-RT35 | #931 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 | non noto | #990 | 1 | 14 | 1080 | 0 | 1 | 0 | 0 | 0 | 2,8 | 2,9 | — | — | — |
| 0.8.1-PT35 | #932 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | medio (pull request) | #1005 | 5 | 9 | 741 | 606 | 5 | 1 | 2 | 1 | 15,6 | 16,1 | — | — | — |
| 0.8.1-RT36 | #933 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 | non noto | #998 | 2 | 15 | 521 | 0 | 2 | 0 | 0 | 0 | 2,2 | 2,3 | — | — | — |
| 0.8.1-PT36 | #934 | pacchetto | media | codex, medio, medio | codex | non dichiarato | non dichiarato | #1007 | 5 | 9 | 375 | 256 | 5 | 0 | 3 | 0 | 15,4 | 15,8 | — | — | — |
| 0.8.1-RT6 | #935 | riferimenti | media | codex, leggero, medio | codex | GPT-6 | non noto | #995 | 1 | 2 | 59 | 0 | 1 | 0 | 0 | 0 | 1,4 | 1,4 | — | — | — |
| 0.8.1-PT6 | #936 | pacchetto | media | claude, medio, medio | claude | Claude Opus 5.5 + Claude Opus | medio (pull request) | #1006 | 3 | 10 | 233 | 144 | 3 | 0 | 1 | 0 | 11,6 | 11,8 | — | — | — |
| 0.8.1-PT7 | #937 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | non noto | #1004 | 1 | 6 | 1505 | 870 | 1 | 0 | 0 | 0 | 15,7 | 16,0 | — | — | — |
| 0.8.1-RT11 | #938 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 | non noto | #1003 | 3 | 14 | 1165 | 0 | 3 | 0 | 0 | 0 | 14,9 | 15,1 | — | — | — |
| 0.8.1-PT11 | #939 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | medio (pull request) | #1013 | 11 | 23 | 875 | 490 | 8 | 3 | 1 | 3 | 45,4 | 46,0 | — | — | — |
| 0.8.1-RT17 | #940 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 | non noto | #1002 | 2 | 13 | 1890 | 0 | 2 | 0 | 0 | 0 | 14,6 | 14,7 | — | — | — |
| 0.8.1-PT17 | #941 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | medio (pull request) | #1012 | 4 | 9 | 247 | 175 | 3 | 0 | 1 | 0 | 3,1 | 3,6 | — | — | — |
| 0.8.1-RT22 | #942 | riferimenti | media | codex, forte, alto | codex | GPT-6 | non noto | #996 | 1 | 6 | 115 | 0 | 1 | 0 | 0 | 0 | 1,3 | 1,4 | #1009 | — | — |
| 0.8.1-PT22 | #943 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | alto (pull request) | #1014 | 2 | 16 | 1348 | 922 | 1 | 0 | 0 | 0 | 1,0 | 15,9 | — | — | — |
| 0.8.1-RT27 | #944 | riferimenti | media | codex, leggero, medio | codex | GPT-6 | non noto | #1001 | 3 | 12 | 689 | 0 | 3 | 0 | 0 | 0 | 2,7 | 2,8 | #1008, #1017 | — | — |
| 0.8.1-PT27 | #945 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | alto (pull request) | #1021 | 2 | 22 | 1274 | 584 | 2 | 0 | 0 | 0 | 5,1 | 20,6 | — | — | — |
| 0.8.1-RT30 | #946 | riferimenti | media | codex, forte, alto | codex | GPT-6 | non noto | #997 | 3 | 9 | 770 | 0 | 3 | 0 | 0 | 0 | 16 | 16,1 | — | — | — |
| 0.8.1-PT30 | #947 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | alto (pull request) | #1015 | 2 | 15 | 537 | 223 | 2 | 1 | 0 | 1 | 5,3 | 5,7 | — | — | — |
| 0.8.1-RT34 | #948 | riferimenti | media | codex, leggero, medio | codex | non dichiarato | non dichiarato | #1010 | 1 | 7 | 227 | 0 | 1 | 0 | 0 | 0 | 3,9 | 16,1 | #1016 | — | — |
| 0.8.1-PT34 | #949 | pacchetto | media | claude, medio, medio | claude | Claude Opus 5.5 | medio (pull request) | #1020 | 5 | 20 | 567 | 217 | 5 | 3 | 0 | 3 | 27,0 | 29,1 | — | — | — |
| 0.8.1-RT15 | #950 | riferimenti | media | codex, leggero, medio | codex | GPT-5.6 sol | non noto | #1019 | 8 | 3 | 19 | 0 | 6 | 1 | 4 | 1 | 17,7 | 17,8 | #1034, #1035 | — | — |
| 0.8.1-PT15 | #951 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 + Claude Opus | alto (pull request) | #1041 | 3 | 12 | 952 | 339 | 2 | 0 | 1 | 0 | 5 | 30,6 | — | — | — |
| 0.8.1-RT8 | #952 | riferimenti | media | codex, forte, alto | codex | non dichiarato | non dichiarato | #1018 | 4 | 7 | 84 | 0 | 4 | 0 | 3 | 0 | 1,0 | 1,1 | #1030 | — | — |
| 0.8.1-PT8 | #953 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | alto (pull request) | #1033 | 1 | 20 | 1915 | 1011 | 1 | 0 | 0 | 0 | 4,6 | 16,9 | — | — | — |
| 0.8.1-RT14 | #954 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 | medio (pull request) | #1032 | 4 | 15 | 901 | 0 | 4 | 0 | 1 | 0 | 25,8 | 42,8 | #1054 | — | — |
| 0.8.1-PT14 | #955 | pacchetto | media | codex, medio, medio | codex | non dichiarato | non dichiarato | #1056 | 10 | 15 | 687 | 550 | 4 | 1 | 0 | 1 | 37,3 | 42,6 | — | — | #1045 (non unita) |
| 0.8.1-PT16 | #956 | pacchetto | media | claude, medio, medio | claude | Claude Opus 5.5 + Claude Opus | medio (pull request) | #1044 | 1 | 4 | 540 | 262 | 1 | 0 | 0 | 0 | 2,9 | 3,2 | — | — | — |
| 0.8.1-RT19 | #957 | riferimenti | media | codex, leggero, medio | codex | non dichiarato | non dichiarato | #1029 | 2 | 6 | 271 | 0 | 2 | 0 | 0 | 0 | 4,7 | 4,8 | #1058 | — | — |
| 0.8.1-PT19 | #958 | pacchetto | media | claude, medio, medio | claude | Claude Opus 5.5 | medio (pull request) | #1060 | 3 | 12 | 1376 | 1002 | 3 | 1 | 0 | 1 | 5,3 | 23,3 | — | — | — |
| 0.8.1-RT23 | #959 | riferimenti | media | codex, forte, alto | codex | GPT-5.6 sol | non noto | #1028 | 9 | 4 | 59 | 0 | 8 | 3 | 3 | 3 | 26,5 | 26,6 | #1037 | — | — |
| 0.8.1-PT23 | #960 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | alto (pull request) | #1043 | 2 | 16 | 1399 | 726 | 2 | 0 | 0 | 0 | 7,5 | 24,0 | — | — | — |
| 0.8.1-RT24 | #961 | riferimenti | media | codex, leggero, medio | codex | GPT-5.6 sol | non noto | #1025 | 5 | 6 | 658 | 0 | 5 | 1 | 3 | 1 | 4,9 | 4,9 | #1050 | — | — |
| 0.8.1-PT24 | #962 | pacchetto | media | claude, medio, medio | claude | Claude Opus 5.5 + Claude Opus | medio (pull request) | #1053 | 4 | 4 | 1012 | 662 | 2 | 0 | 0 | 0 | 19,9 | 23,2 | — | — | — |
| 0.8.1-RT28 | #963 | riferimenti | media | codex, forte, alto | codex | GPT-5.6 sol | alto (pull request) | #1022 | 2 | 11 | 538 | 0 | 2 | 0 | 1 | 0 | 3,2 | 3,2 | #1040 | — | — |
| 0.8.1-PT28 | #964 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | alto (pull request) | #1042 | 3 | 12 | 1573 | 883 | 2 | 0 | 0 | 0 | 7,5 | 9,1 | — | — | — |
| 0.8.1-PT29 | #965 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | medio (pull request) | #1031 | 4 | 5 | 488 | 211 | 4 | 0 | 2 | 0 | 11,0 | 11,5 | — | — | — |
| 0.8.1-PT9 | #966 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | medio (pull request) | #1038 | 5 | 6 | 191 | 93 | 5 | 1 | 2 | 1 | 5,6 | 5,9 | — | — | — |
| 0.8.1-PT10 | #967 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | alto (pull request) | #1039 | 3 | 5 | 1899 | 1364 | 3 | 1 | 1 | 1 | 5,5 | 5,9 | — | — | — |
| 0.8.1-RT18 | #968 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 + Claude Opus | medio (pull request) | #1023 | 1 | 18 | 6836 | 0 | 1 | 0 | 0 | 0 | 3,2 | 3,3 | — | — | — |
| 0.8.1-PT18 | #969 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | non noto | #1061 | 5 | 8 | 395 | 235 | 4 | 0 | 1 | 0 | 7,5 | 21,4 | — | — | — |
| 0.8.1-PT20 | #970 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | non noto | #1055 | 7 | 6 | 900 | 567 | 7 | 1 | 4 | 1 | 17,9 | 19,1 | — | — | — |
| 0.8.1-PT25 | #971 | pacchetto | media | claude, medio, medio | claude | Claude Opus 5.5 + Claude Opus | medio (pull request) | #1066 | 6 | 9 | 1200 | 885 | 6 | 0 | 5 | 0 | 15,9 | 16,0 | — | — | — |
| 0.8.1-RT26 | #972 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 | medio (pull request) | #1024 | 1 | 16 | 1014 | 0 | 1 | 0 | 0 | 0 | 3,1 | 3,3 | — | — | — |
| 0.8.1-PT26 | #973 | pacchetto | media | codex, medio, medio | codex | non dichiarato | non dichiarato | #1062 | 8 | 15 | 604 | 402 | 7 | 2 | 1 | 2 | 8,3 | 22,3 | — | — | — |
| 0.8.1-RT31 | #974 | riferimenti | media | codex, forte, alto | ? | non dichiarato | non dichiarato | #1027 | 3 | 7 | 221 | 0 | 3 | 0 | 2 | 0 | 3,1 | 3,1 | #1057 | — | — |
| 0.8.1-PT31 | #975 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | alto (pull request) | #1063 | 2 | 14 | 1970 | 1230 | 2 | 0 | 1 | 0 | 4,8 | 19,2 | — | — | — |
| 0.8.1-PT12 | #976 | pacchetto | impegnativa | claude, forte, alto | claude | Claude Opus 5.5 + Claude Opus | alto (pull request) | #1046 | 2 | 9 | 1575 | 1190 | 2 | 1 | 0 | 1 | 4,6 | 5,1 | — | — | — |
| 0.8.1-PT32 | #977 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | medio (pull request) | #1065 | 6 | 22 | 345 | 143 | 6 | 1 | 3 | 1 | 15,9 | 16,0 | — | — | — |
| 0.8.1-PT13 | #978 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | non noto | #1059 | 18 | 23 | 695 | 298 | 18 | 3 | 12 | 3 | 37,3 | 37,3 | — | — | — |
| 0.8.1-PT21 | #979 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | medio (pull request) | #1064 | 9 | 11 | 419 | 217 | 9 | 1 | 5 | 1 | 24,0 | 24,1 | — | — | — |
| 0.8.1-PT33 | #980 | pacchetto | media | codex, medio, medio | codex | non dichiarato | non dichiarato | #1067 | 19 | 7 | 521 | 261 | 19 | 2 | 14 | 2 | 22,7 | 23,0 | — | — | — |
| 0.8.1-PT37 | #981 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | medio (pull request) | #1068 | 6 | 26 | 410 | 114 | 6 | 1 | 2 | 1 | 4,0 | 4,1 | — | — | — |
| 0.8.1-PT38 | #982 | pacchetto | facile | codex, leggero, basso | codex | non dichiarato | non dichiarato | #1069 | 3 | 3 | 195 | 0 | 3 | 0 | 0 | 0 | 10,7 | 10,7 | — | — | — |
| 0.8.1-PT39 | #985 | pacchetto | media | claude, medio, medio | ? | non dichiarato | non dichiarato | — | — | — | — | — | — | — | — | — | — | — | — | — | — |

Issue senza pull request principale unita: 0.8.1-PT4 (chiusa), 0.8.1-PT5 (chiusa), 0.8.1-PT39 (aperta).

### 7.2 Comando

Dalla radice del repository:

```sh
scripts/misura_pacchetti.py --tappe 0.8.1 --dati misura-0.8.1 --scarica > /dev/null
```

Poi le due correzioni della sezione 1 nella cartella `misura-0.8.1/`: in
`pulls.json`, il titolo della #1049 senza `[0.8.1-PT19] ` e la sua descrizione
vuota, e la riga `Closes #974` in fondo alla descrizione della #1027; in
`pull-details.json`, i dati delle pull request #1027 e #1049, letti come fa lo
script (`repos/Matleyx78/tecnigrafo/pulls/<numero>` e `…/files`). Infine,
senza rete:

```sh
scripts/misura_pacchetti.py --tappe 0.8.1 --dati misura-0.8.1 > misura-0.8.1.md
```

I conteggi delle sezioni 2.8 e 4 si rifanno leggendo, per ogni pull request
principale, `repos/Matleyx78/tecnigrafo/issues/<numero>/comments` con le
regole della sezione 1, insieme ai commenti delle issue che lo script salva in
`misura-0.8.1/commenti.json`.
