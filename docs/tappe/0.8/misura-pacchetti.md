# Misura dei pacchetti della 0.8 (#442)

- **Pacchetto:** [0.8-PT47](PT47-misura-pacchetti.md), issue #747
- **Autore:** Claude (Claude Code), modello Claude Opus 5.5, impegno medio
- **Data:** 2026-11-19
- **Dati:** API REST di GitHub, lette il 2026-11-19 alle 17:40 UTC, e `main` al
  commit `fac434848b89c61fab3d24458e5f53aeb651dd97` (solid-space al commit
  fissato `d84a40a6255da5fd01ca2918fc645aa77b8d5131`)
- **Stato:** misura per il responsabile, con la proposta di decisione
  definitiva sulle condizioni della #442 (sezione 6). Nessuna regola cambia
  con questo documento: la decisione la prende il responsabile.

È la seconda delle due misure decise il 2026-11-10
([DECISIONI.md](../../DECISIONI.md), «Pacchetti medi: le condizioni
dell'analisi della #442 e le due misure»), e un criterio di chiusura della
[scheda della 0.8](../0.8-quote-retini-blocchi.md). Rifà con lo stesso script
la [misura della 0.7.5](../0.7.5/misura-pacchetti.md) e la confronta con
essa e con l'[analisi della #442](../0.7.5/analisi-pacchetti-medi.md). I
numeri delle sezioni 2 e 7 vengono da `scripts/misura_pacchetti.py`; quelli
della sezione 2.8 (giri di revisione, fermate per una decisione, correzioni
delle schede) da un conteggio sui commenti, descritto nella sezione 1. Dove il
testo stima, lo dice.

## In breve

Lo script trova 79 pull request principali unite. Con le correzioni a mano
della sezione 1 sono 48 pacchetti di lavoro (39 medi, 4 facili, 5
impegnativi, tre dei quali sono documenti) e 30 pacchetti di riferimenti, più
RT20 unito a mano; PT29 è ancora in revisione (#907) e non è contato. Cinque cose:

1. **Gli impegnativi di un tempo, fatti in fascia media, sono andati come i
   medi: senza correzioni dopo l'unione.** Nove pacchetti che la prova sulla
   carta dell'analisi dava come impegnativi (scheletro, impaginazione del
   testo multiriga, blocchi, quote lineari e nel DXF, unione con distanza,
   tracciamento, righe dei motivi, contorni dei retini) sono stati fatti da
   Claude in fascia media. Nessuna pull request ne ha corretto il codice dopo
   l'unione; sei su nove hanno avuto almeno una run fallita prima del verde
   (gli impegnativi delle tappe 0.4–0.7: 22 %; i medi: 65 %), quattro su otto
   un secondo giro di revisione. Il costo vero sono state le **fermate per una
   decisione**: 20, in otto pacchetti su nove (sezione 4).
2. **Ma la fascia media di Claude è stata Opus 5.5 con impegno alto.** I
   pacchetti medi di Claude che il piano dava con impegno alto sono stati
   avviati con `[impegno: alto]` e lo dichiarano: 18 pacchetti di Claude su
   26. Nella tabella di [LAVORO-IN-PARALLELO.md](../../LAVORO-IN-PARALLELO.md),
   «Modello e impegno», Opus 5.5 alto è la riga della fascia forte. Il
   risparmio della sezione 3.9 dell'analisi (da alto a medio sullo stesso
   modello) **non è stato provato**; lo sono stati la divisione e lo
   spostamento delle decisioni prima del codice.
3. **I fermi sono tornati quelli della 0.7.** 249,6 ore di fermi in tutto
   (0.7.5: 5; 0.7: 286), e 64 commenti «Serve una decisione del responsabile»
   (55 nei pacchetti di lavoro). Vengono da buchi delle schede trovati a rate,
   da strumenti di verifica arrivati dopo i riferimenti e da risposte condivise
   fra pacchetti: le famiglie che il responsabile ha già raccolto nella #764 e
   deciso il 2026-11-19.
4. **I riferimenti forti non hanno avuto meno correzioni.** 11 correzioni per
   11 pacchetti di riferimenti in fascia forte, 18 per 20 in fascia leggera
   (1,0 e 0,9 per pacchetto, come nelle tappe 0.4–0.7). Almeno quattro
   correzioni non vengono da un errore di chi li ha scritti, ma da un altro
   pacchetto che ha cambiato una risposta (le righe degli stili di quota di
   PT22).
5. **I giri di CI non crescono con la misura.** Sui medi Spearman 0,05 tra
   righe e run fallite (0,01 con le righe di codice), come nelle tappe 0.4–0.7
   (−0,07); la correlazione della 0.7.5 (0,51) non si ripete. I pacchetti fuori
   da `app` hanno avuto un giro nel 59 % dei casi (0.7.5: 19 %).

La proposta (sezione 6): **le condizioni 1–7 diventano definitive**, con
quattro precisazioni; la più importante dice che la condizione 3 vale come è
stata provata (per Claude, Opus 5.5 con l'impegno della scheda) e che Opus
medio sugli impegnativi di un tempo si prova nella 0.8.1 su pochi pacchetti
scelti dal piano.

## 1. Metodo e limiti

Il metodo è quello della [misura della 0.7.5](../0.7.5/misura-pacchetti.md),
sezione 1, con lo stesso script e le stesse definizioni, senza modifiche:
`scripts/misura_pacchetti.py` conosce già i prefissi `PT`/`RT`/`CT` della 0.8
e le lettere dei blocchi. L'impegno non viene più dalla dichiarazione del
responsabile (che valeva solo per la 0.7.5): è quello scritto nella pull
request, o «non noto» / «non dichiarato».

**Correzioni a mano.** Lo script prende come principale la prima pull request
unita, aperta dopo la issue, che porta il codice nel titolo. Nella 0.8 due
correzioni di scheda fatte durante il lavoro portano il codice del pacchetto
nel titolo e sono state unite prima del codice:

- **PT21:** la principale vera è la #905 (826 righe di codice, una run
  fallita, un giro di revisione), che lo script conta come «correzione del
  codice»; la #900 è la correzione della scheda («-INSERT scale and prompts
  as AutoCAD»). Il testo usa la #905; le tabelle delle sezioni 2 e 7 sono
  quelle dello script e mostrano la #900.
- **PT29:** la #897 è la correzione della scheda (dipendenze da PT21 e PT26);
  la pull request del codice, la #907, è ancora aperta. Il testo non conta
  PT29 fra i pacchetti uniti; le tabelle dello script lo contano con la #897
  (29 righe, nessuna run fallita), quindi i medi delle tabelle sono 40 invece
  di 39.

Con queste due correzioni la «correzione del codice» che lo script trova (la
#905) sparisce: **nessuna pull request ha corretto il codice di un pacchetto
della 0.8 dopo la sua unione**. L'ho controllato anche a mano sull'elenco delle
pull request unite dalla #750 che lo script non attribuisce: sono documenti,
CI, script fuori tappa (F8, F9, F12) e la #883, che aggiorna dei casi (righe
degli stili di quota) dopo che `main` era diventato rosso, senza toccare il
codice.

Altri casi particolari:

- **RT20** non ha una pull request unita: la #806 è stata unita su `main` a
  mano (c198508) dopo un errore 502 di GitHub, e risulta chiusa. Lo script la
  conta fra i tentativi non uniti.
- **PT4 e PT5** hanno una sola pull request (#763), contata due volte. Come
  PT40 (#751), è la trascrizione nelle schede delle decisioni sui verbali di
  Codex, scritta da Claude: lo script le attribuisce a Claude (come 0.7.5-P23).

**Giri di revisione, fermate e correzioni delle schede (sezione 2.8).** Lo
script non li conta. Li ho contati sui commenti della pull request principale
e della issue, letti dall'API il 2026-11-19:

- *giri di revisione*: commenti della pull request principale che contengono
  «Giro di revisione:» (la regola di [catena.md](../../agenti/catena.md),
  «Revisione (revisore)»). Esistono dalla catena (#768, 2026-11-14 sera):
  PT1, PT2, PT4, PT5, PT6, PT36 e PT40 non ne hanno;
- *fermate per una decisione*: commenti della issue o della pull request
  principale che cominciano con «Serve una decisione del responsabile», tolti
  quelli della issue che rimandano alla pull request («vedi la pull request»).
  Contano anche le fermate prima della pull request (un caso che sembra
  sbagliato, un buco della scheda) e quelle dei revisori;
- *correzioni delle schede*: pull request unite durante la tappa che correggono
  la scheda di un pacchetto già in lavoro, attribuite dal titolo (per esempio
  la #825 a PT23); l'elenco è nella sezione 4.

**Limiti** (oltre a quelli della sezione 1 dell'analisi e della misura della
0.7.5, che valgono tutti):

- **Difetti dopo l'unione su una finestra corta.** I pacchetti sono stati uniti
  fra il 13 e il 19 novembre; la prova manuale della tappa (sezione 7 del
  piano) non è ancora fatta. «Nessuna correzione» vuol dire nessuna in pochi
  giorni e nessuna che la CI o un altro pacchetto abbia trovato.
- **Chi rivede cambia con l'autore.** Codex ha rivisto le pull request di
  Claude, Claude quelle di Codex: i giri di revisione confondono il modello di
  chi scrive con quello di chi rivede.
- **Revisione mirata in fascia media.** Il piano chiedeva per i medi di
  Claude con casi delicati una revisione di Codex in fascia forte; le
  revisioni dichiarano GPT-5.6 Sol (fascia media), perché dalla catena Codex
  usa il modello predefinito (#764).
- **Impegno di Codex quasi mai noto.** 10 delle 23 pull request di lavoro di
  Codex scrivono «impegno non noto» o niente.
- **Campioni piccoli.** Nove pacchetti nel gruppo che interessa (sezione 4),
  due impegnativi di codice in fascia forte.
- **Fermo e catena.** Il fermo va dalla prima presa in carico all'ultima prima
  della pull request. Con la catena ogni ripresa dopo una decisione è una presa
  nuova, e le decisioni prese di notte allungano il fermo di ore senza lavoro.

## 2. Dati

### 2.1 Tappa 0.8

Colonne come nella misura della 0.7.5, sezione 2.1.

| Tipo | Difficoltà | PR | Commit (med.) | Run CI (med.; tot.) | Run fallite (tot.) | Run annullate (tot.) | PR con ≥1 giro | Ore aperta→unita (med.) | Ore presa→unita (med.) | File (med.) | Righe (med.) | Righe di codice (med.) | Correzioni dei riferimenti (tot.) | Correzioni del codice (tot.) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| pacchetto | facile | 4 | 2 | 2; 12 | 1 | 5 | 1 | 1,7 | 5,1 | 9,5 | 171 | 38 | 0 | 0 |
| pacchetto | media | 40 | 4 | 4; 176 | 36 | 56 | 25 | 4,0 | 8,1 | 12 | 769 | 433 | 0 | 1 |
| pacchetto | impegnativa | 5 | 3 | 2; 26 | 7 | 5 | 2 | 1,4 | 20,5 | 19 | 517 | 0 | 0 | 0 |
| riferimenti | media | 30 | 1 | 1,5; 59 | 6 | 15 | 5 | 1,6 | 2,2 | 12,5 | 583 | 0 | 29 | 0 |

### 2.2 Per difficoltà e autore, pacchetti di lavoro

| Pacchetti di lavoro | PR | Run fallite | PR con ≥1 giro | Quota con ≥1 giro | Run (med.) | Righe (med.) | Run fallite per 1000 righe |
|---|---|---|---|---|---|---|---|
| facile, codex | 4 | 1 | 1 | 25 % | 2 | 171 | 1,18 |
| media, claude | 21 | 14 | 11 | 52 % | 2 | 1116 | 0,49 |
| media, codex | 19 | 22 | 14 | 74 % | 5 | 540 | 2,05 |
| impegnativa, claude | 5 | 7 | 2 | 40 % | 2 | 517 | 1,03 |

Pacchetti di lavoro che toccano `app/` o `tests/app/` (Qt): 8, con almeno un giro 4 (0.8-PT2, 0.8-PT34, 0.8-PT37, 0.8-PT38).
Pacchetti di lavoro che non toccano `app/` o `tests/app/` (Qt): 41, con almeno un giro 24 (0.8-PT1, 0.8-PT9, 0.8-PT3, 0.8-PT10, 0.8-PT48, 0.8-PT49, 0.8-PT11, 0.8-PT14, 0.8-PT15, 0.8-PT16, 0.8-PT18, 0.8-PT22, 0.8-PT21, 0.8-PT23, 0.8-PT25, 0.8-PT26, 0.8-PT27, 0.8-PT28, 0.8-PT31, 0.8-PT33, 0.8-PT36, 0.8-PT42, 0.8-PT43, 0.8-PT44).
Spearman tra righe e run fallite prima del verde, pacchetti di lavoro tutti (49): 0,28; con le righe di codice: 0,26.
Spearman tra righe e run fallite prima del verde, pacchetti di lavoro medi (40): 0,05; con le righe di codice: 0,01.

### 2.3 Per fascia e impegno, previsti e dichiarati

«Fascia prevista» e «Impegno previsto» sono le colonne «Modello» e «Impegno»
della tabella del piano; «Modello dichiarato» e «Impegno» vengono dalla pull
request. «Claude Opus 5.5 + Claude Opus» è lo stesso modello nominato due volte
nella sezione «Uso dell'IA».

**Previsti dal piano**

| Difficoltà | Fascia prevista | Impegno previsto | PR | PR con ≥1 giro | Run fallite (tot.) | Righe (med.) | Correzioni del codice (tot.) |
|---|---|---|---|---|---|---|---|
| facile | leggero | basso | 2 | 0 | 0 | 131 | 0 |
| facile | leggero | medio | 2 | 1 | 1 | 294 | 0 |
| impegnativa | forte | alto | 5 | 2 | 7 | 517 | 0 |
| media | medio | alto | 20 | 11 | 14 | 1160 | 1 |
| media | medio | medio | 20 | 14 | 22 | 542 | 0 |

**Dichiarati**

| Autore | Modello dichiarato | Impegno | Fonte dell'impegno | PR | PR con ≥1 giro | Run fallite (tot.) | Righe (med.) | Correzioni del codice (tot.) |
|---|---|---|---|---|---|---|---|---|
| claude | Claude Opus 5.5 | alto | pull request | 13 | 10 | 18 | 1840 | 0 |
| claude | Claude Opus 5.5 | medio | pull request | 2 | 1 | 1 | 50 | 1 |
| claude | Claude Opus 5.5 | non noto | non dichiarato | 5 | 1 | 1 | 517 | 0 |
| claude | Claude Opus 5.5 + Claude Opus | alto | pull request | 5 | 1 | 1 | 1048 | 0 |
| claude | Claude Opus 5.5 + Claude Opus | medio | pull request | 1 | 0 | 0 | 926 | 0 |
| codex | GPT-5.6 sol | basso | pull request | 1 | 0 | 0 | 203 | 0 |
| codex | GPT-5.6 sol | medio | pull request | 11 | 9 | 11 | 545 | 0 |
| codex | GPT-5.6 sol | non noto | non dichiarato | 4 | 1 | 1 | 378 | 0 |
| codex | GPT-5.6 sol + GPT-6 sol | medio | pull request | 1 | 1 | 5 | 747 | 0 |
| codex | GPT-6 | non noto | non dichiarato | 3 | 2 | 2 | 499 | 0 |
| codex | non dichiarato | non dichiarato | non dichiarato | 3 | 2 | 4 | 524 | 0 |

I 18 pacchetti di Claude con impegno alto dichiarato sono tutti medi
pianificati con impegno alto o impegnativi; i commenti di avvio della catena
portano `[impegno: alto]` (per esempio nelle issue #687, #693, #707, #725,
#738). I tre con impegno medio sono PT8, PT21 (la #900, di soli documenti) e
PT29 (la #897, idem).

### 2.4 Misura dei pacchetti (condizione 2)

Pacchetti di lavoro uniti: 49; con più di 15 file (test e documenti compresi): 15; con più di 600 righe di codice: 14; con meno di 150 righe di codice: 11; righe di codice, mediana 405.

Con la #905 al posto della #900 per PT21 e senza PT29: 48 pacchetti di lavoro,
15 oltre le 600 righe di codice e 9 sotto le 150.

Oltre le 600 righe di codice: PT6, PT9, PT13, PT16, PT18, PT19, PT21, PT22,
PT23, PT24, PT25, PT32, PT33, PT37 e PT48; dieci di loro hanno avuto almeno una
run fallita prima del verde (67 %), contro 18 su 33 degli altri (55 %). Sotto
le 150: i documenti (PT3, la cui parte grossa è in `tests/verifica/`, PT4,
PT5, PT40, PT46), le icone (PT39, PT45), PT35 e PT41.

### 2.5 Costi fissi

- Run verde, durata mediana: 16,2 minuti (181 run verdi delle pull request principali, di correzione e dei tentativi).
- Vita della pull request principale: apertura→ultimo verde 0,78 ore, verde→unione 0,76 ore (mediane su 79).
- Pull request principali unite: 79; correzioni dei riferimenti 29, del codice 1, di documenti 0; tentativi non uniti 3.
- Run delle pull request principali: 273, di cui 50 fallite e 81 annullate.

### 2.6 Tempi

«Fermo», «ultimo lavoro», «attesa» e percorso critico come nella misura della
0.7.5, sezione 2.6. L'attesa negativa di PT29 viene dalla dipendenza da PT21
aggiunta alla scheda (#897) dopo la sua prima presa.

Orari UTC.

| Pacchetto | Difficoltà | Ultima dipendenza unita | Prima presa | Ultima presa prima della PR | Unito | Fermo (h) | Ultimo lavoro (h) | Attesa da pronto a presa (h) |
|---|---|---|---|---|---|---|---|---|
| PT1 | facile | — | 12/11 16:48 | 14/11 12:21 | 14/11 14:59 | 43,5 | 2,6 | 9,2 |
| PT2 | media | — | 12/11 16:49 | 12/11 16:49 | 13/11 20:10 | 0 | 27,4 | 9,2 |
| PT4 | impegnativa | — | 12/11 16:49 | 12/11 16:49 | 14/11 07:04 | 0 | 38,2 | 9,2 |
| PT5 | impegnativa | — | 12/11 16:50 | 12/11 16:50 | 14/11 07:04 | 0 | 38,2 | 9,3 |
| PT6 | media | PT4 14/11 07:04 | 14/11 12:22 | 14/11 12:22 | 14/11 17:12 | 0 | 4,8 | 5,3 |
| PT7 | media | PT6 14/11 17:12 | 15/11 12:34 | 15/11 12:34 | 15/11 16:01 | 0 | 3,5 | 19,4 |
| PT8 | media | PT6 14/11 17:12 | 15/11 12:19 | 15/11 12:19 | 15/11 20:15 | 0 | 7,9 | 19,1 |
| PT9 | media | PT6 14/11 17:12 | 15/11 12:35 | 15/11 16:11 | 15/11 18:34 | 3,6 | 2,4 | 19,4 |
| PT3 | media | PT9 15/11 18:34 | 15/11 20:11 | 15/11 20:11 | 16/11 17:01 | 0 | 20,8 | 1,6 |
| PT10 | media | RT10 16/11 19:29 | 16/11 19:41 | 16/11 19:41 | 17/11 07:59 | 0 | 12,3 | 0,2 |
| PT48 | media | RT48 16/11 19:30 | 16/11 19:31 | 16/11 19:31 | 17/11 06:10 | 0 | 10,7 | 0 |
| PT49 | media | PT48 17/11 06:10 | 17/11 06:38 | 17/11 06:38 | 17/11 10:41 | 0 | 4,0 | 0,5 |
| PT11 | media | RT11 16/11 19:30 | 16/11 19:40 | 16/11 21:02 | 17/11 06:12 | 1,4 | 9,2 | 0,2 |
| PT12 | media | PT11 17/11 06:12 | 17/11 06:39 | 17/11 06:39 | 17/11 10:11 | 0 | 3,5 | 0,5 |
| PT13 | media | PT11 17/11 06:12 | 17/11 06:32 | 17/11 10:12 | 17/11 11:58 | 3,7 | 1,8 | 0,3 |
| PT14 | media | PT13 17/11 11:58 | 17/11 12:03 | 17/11 12:03 | 18/11 07:56 | 0 | 19,9 | 0,1 |
| PT15 | media | PT13 17/11 11:58 | 17/11 12:04 | 17/11 13:35 | 17/11 17:52 | 1,5 | 4,3 | 0,1 |
| PT16 | media | PT48 17/11 06:10 | 17/11 06:33 | 17/11 06:33 | 17/11 11:45 | 0 | 5,2 | 0,4 |
| PT17 | media | PT16 17/11 11:45 | 17/11 11:46 | 17/11 11:47 | 17/11 13:30 | 0,0 | 1,7 | 0 |
| PT18 | impegnativa | PT17 17/11 13:30 | 17/11 13:31 | 17/11 20:57 | 18/11 10:02 | 7,4 | 13,1 | 0 |
| PT22 | media | PT11 17/11 06:12 | 17/11 06:32 | 18/11 06:29 | 18/11 07:57 | 23,9 | 1,5 | 0,3 |
| PT19 | media | PT18 18/11 10:02 | 18/11 12:37 | 18/11 15:48 | 18/11 17:16 | 3,2 | 1,5 | 2,6 |
| PT20 | media | PT17 17/11 13:30 | 17/11 13:33 | 17/11 13:33 | 17/11 17:52 | 0 | 4,3 | 0,1 |
| PT21 | media | PT19 18/11 17:16 | 18/11 19:53 | 19/11 05:15 | 19/11 06:46 | 9,4 | 1,5 | 2,6 |
| PT23 | media | PT22 18/11 07:57 | 18/11 08:02 | 18/11 11:38 | 18/11 16:14 | 3,6 | 4,6 | 0,1 |
| PT24 | media | PT23 18/11 16:14 | 18/11 16:15 | 19/11 05:26 | 19/11 06:48 | 13,2 | 1,4 | 0 |
| PT25 | media | PT23 18/11 16:14 | 18/11 16:16 | 18/11 19:52 | 19/11 05:24 | 3,6 | 9,5 | 0 |
| PT26 | media | PT23 18/11 16:14 | 18/11 16:20 | 18/11 16:20 | 19/11 04:46 | 0 | 12,4 | 0,1 |
| PT27 | media | PT24 19/11 06:48 | 19/11 06:56 | 19/11 06:56 | 19/11 14:18 | 0 | 7,4 | 0,1 |
| PT28 | media | PT22 18/11 07:57 | 18/11 08:10 | 18/11 08:10 | 18/11 16:43 | 0 | 8,6 | 0,2 |
| PT29 | media | PT21 19/11 06:46 | 18/11 16:16 | 19/11 04:53 | 19/11 05:53 | 12,6 | 1 | -14,5 |
| PT30 | media | RT30 16/11 16:55 | 16/11 16:56 | 17/11 04:06 | 17/11 05:27 | 11,2 | 1,4 | 0 |
| PT31 | media | RT31 17/11 05:28 | 17/11 05:31 | 17/11 06:47 | 17/11 10:33 | 1,3 | 3,8 | 0,1 |
| PT32 | media | RT32 15/11 09:39 | 15/11 09:40 | 15/11 10:45 | 15/11 12:13 | 1,1 | 1,5 | 0 |
| PT33 | media | PT32 15/11 12:13 | 15/11 12:38 | 15/11 12:38 | 15/11 18:32 | 0 | 5,9 | 0,4 |
| PT34 | media | RT34 16/11 04:34 | 16/11 04:35 | 16/11 09:23 | 16/11 16:29 | 4,8 | 7,1 | 0 |
| PT35 | media | PT34 16/11 16:29 | 16/11 17:17 | 16/11 17:17 | 16/11 20:05 | 0 | 2,8 | 0,8 |
| PT36 | media | — | 13/11 11:47 | 13/11 11:47 | 13/11 20:03 | 0 | 8,3 | 28,2 |
| PT37 | impegnativa | PT35 16/11 20:05 | 17/11 04:38 | 17/11 04:38 | 17/11 06:22 | 0 | 1,8 | 8,5 |
| PT38 | media | PT37 17/11 06:22 | 17/11 06:40 | 17/11 06:40 | 17/11 11:57 | 0 | 5,3 | 0,3 |
| PT39 | media | PT27 19/11 14:18 | 19/11 14:21 | 19/11 14:21 | 19/11 16:08 | 0 | 1,8 | 0,1 |
| PT40 | impegnativa | — | 12/11 11:39 | 12/11 11:59 | 12/11 15:32 | 0,3 | 3,5 | 4,1 |
| PT41 | facile | PT1 14/11 14:59 | 14/11 21:35 | 14/11 22:34 | 15/11 06:55 | 1 | 8,3 | 6,6 |
| PT42 | media | RT42 17/11 11:59 | 17/11 12:00 | 17/11 17:51 | 18/11 07:56 | 5,8 | 14,1 | 0 |
| PT43 | media | RT43 17/11 17:51 | 17/11 17:52 | 17/11 20:58 | 18/11 11:37 | 3,1 | 14,7 | 0 |
| PT44 | media | PT43 18/11 11:37 | 18/11 12:39 | 18/11 12:39 | 18/11 19:50 | 0 | 7,2 | 1 |
| PT50 | media | RT50 17/11 11:59 | 17/11 12:01 | 17/11 17:52 | 17/11 20:58 | 5,9 | 3,1 | 0 |
| PT45 | facile | PT39 19/11 16:08 | 19/11 16:11 | 19/11 16:11 | 19/11 17:04 | 0 | 0,9 | 0,1 |
| PT46 | facile | PT45 19/11 17:04 | 19/11 17:14 | 19/11 17:14 | 19/11 17:33 | 0 | 0,3 | 0,2 |

Percorso critico realizzato: PT4 → PT6 → PT9 → PT3 → RT11 → PT11 → PT22 → PT23 → PT24 → PT27 → PT39 → PT45 → PT46; dall'unione di PT4 all'ultima unione 130,5 ore, ultimo lavoro dei nodi dopo il primo 57,3 ore; fermi di tutti i pacchetti 249,6 ore; attesa da pronto a presa, mediana 0,2 ore su 49 pacchetti di lavoro; dalla prima issue all'ultima unione 178 ore.

### 2.7 Riferimenti

Casi fini e ordinari contati con la regola della sezione 1 dell'analisi, su
tutte le cartelle di casi toccate da ciascun pacchetto, come sono oggi su
`main`.

| Riferimenti | Chi li ha scritti (modello dichiarato) | Domande (fini) | Script (fini) | Correzioni dei riferimenti | Righe |
|---|---|---|---|---|---|
| 0.8-RT10 | claude (Claude Opus 5.5) | 115 (2) | 0 (0) | — | 539 |
| 0.8-RT48 | codex (GPT-6) | 76 (12) | 0 (0) | — | 519 |
| 0.8-RT49 | claude (Claude Opus 5.5) | 0 (0) | 7 (5) | — | 603 |
| 0.8-RT11 | claude (Claude Opus 5.5) | 6 (0) | 2 (2) | #832 | 999 |
| 0.8-RT12 | claude (Claude Opus 5.5) | 0 (0) | 6 (6) | #849 | 624 |
| 0.8-RT13 | codex (GPT-6) | 98 (36) | 0 (0) | #851 | 256 |
| 0.8-RT14 | claude (Claude Opus 5.5 + Claude Opus) | 18 (1) | 0 (0) | — | 591 |
| 0.8-RT15 | claude (Claude Opus 5.5 + Claude Opus) | 0 (0) | 6 (2) | #863 | 521 |
| 0.8-RT16 | codex (GPT-6) | 80 (2) | 0 (0) | — | 444 |
| 0.8-RT17 | codex (GPT-6) | 50 (20) | 0 (0) | — | 164 |
| 0.8-RT18 | codex (GPT-6) | 116 (3) | 1 (1) | #872 | 3903 |
| 0.8-RT22 | codex (GPT-5.6 sol) | 0 (0) | 2 (0) | #846, #852, #864, #879 | 386 |
| 0.8-RT19 | codex (GPT-5.6 sol) | 20 (0) | 0 (0) | #889 | 321 |
| 0.8-RT21 | codex (non dichiarato) | 0 (0) | 4 (4) | #899, #903 | 1593 |
| 0.8-RT23 | codex (GPT-6) | 82 (51) | 0 (0) | #884 | 3628 |
| 0.8-RT24 | codex (GPT-6) | 224 (57) | 0 (0) | #898 | 1531 |
| 0.8-RT25 | codex (non dichiarato) | 23 (7) | 0 (0) | #893 | 125 |
| 0.8-RT26 | claude (Claude Opus 5.5) | 0 (0) | 7 (1) | #896 | 575 |
| 0.8-RT27 | claude (Claude Opus 5.5) | 0 (0) | 13 (11) | #904 | 911 |
| 0.8-RT28 | claude (Claude Opus 5.5) | 0 (0) | 6 (6) | — | 417 |
| 0.8-RT29 | codex (GPT-5.6 sol) | 0 (0) | 2 (1) | #894, #906 | 177 |
| 0.8-RT30 | codex (non dichiarato) | 12 (0) | 0 (0) | #829 | 34 |
| 0.8-RT31 | claude (Claude Opus 5.5) | 64 (2) | 0 (0) | — | 2187 |
| 0.8-RT32 | codex (GPT-5.6 sol) | 25 (1) | 0 (0) | #775, #777 | 104 |
| 0.8-RT33 | claude (Claude Opus 5.5) | 20 (2) | 15 (9) | — | 1149 |
| 0.8-RT34 | codex (GPT-5.6 sol) | 0 (0) | 5 (1) | #801, #804 | 803 |
| 0.8-RT42 | codex (GPT-6) | 46 (21) | 4 (4) | #866 | 5442 |
| 0.8-RT43 | codex (GPT-6) | 73 (6) | 0 (0) | #875 | 680 |
| 0.8-RT44 | claude (Claude Opus 5.5) | 0 (0) | 10 (4) | — | 703 |
| 0.8-RT50 | codex (GPT-6) | 160 (27) | 1 (1) | #865, #871, #887 | 542 |

Per fascia prevista e autore:

| Fascia prevista | Autore | Pacchetti | Correzioni | Per pacchetto |
|---|---|---|---|---|
| forte | codex (GPT-6) | 11 | 11 | 1,0 |
| leggero | claude (Opus 5.5, impegno medio) | 12 | 5 | 0,4 |
| leggero | codex (GPT-5.6 sol o non dichiarato) | 7 | 13 | 1,9 |
| leggero | RT20 (unito a mano) | 1 | 0 | 0 |

Dai titoli, almeno quattro correzioni vengono da un altro pacchetto e non da un
errore del caso: #852 e #879 (RT22, i casi esistenti con le righe di PT22),
#871 (RT50, lo stile di quota di PT22) e #875 (RT43, una dipendenza da PT23
tolta). La #887 aggiunge due casi chiesti da solid-space, non ne corregge.

### 2.8 Giri di revisione, fermate e correzioni delle schede

Gruppi della sezione 4. «Run fallite»: prima del verde; «giri di revisione»
con la regola della sezione 1; «presa→unione» e «fermi» dallo script, PT21
con i dati della #900 (circa 4,7 ore in meno della #905).

| Gruppo | Pacchetti | Con ≥1 run fallita | Run fallite | Giri di revisione (med.; tot.) | Con ≥2 giri di revisione | Fermate per una decisione (pacchetti) | Correzioni delle schede | Correzioni del codice dopo l'unione | Ore presa→unione (med.) | Fermi (h) | Righe di codice (med.) |
|---|---|---|---|---|---|---|---|---|---|---|---|
| A. Impegnativi di un tempo, in fascia media (Claude) | 9 | 6 | 7 | 1; 13 | 4 | 20 (8) | 7 | 0 | 8,2 | 25,7 | 752 |
| B. Altri medi di Claude con impegno alto | 10 | 5 | 7 | 1; 8 | 0 | 14 (8) | 6 | 0 | 9,8 | 70,3 | 696 |
| C. Medi con impegno medio (19 di Codex e PT8) | 20 | 14 | 22 | 2; 40 | 17 | 12 (8) | 5 | 0 | 6,5 | 4,1 | 364 |
| D. Impegnativi di codice in fascia forte (PT18, PT37) | 2 | 2 | 7 | 1,5; 3 | 1 | 4 (2) | 0 | 0 | 11,1 | 7,4 | 1980 |

A: PT6, PT13, PT16, PT23, PT25, PT32, PT34, PT42, PT43. B: PT9, PT17, PT19,
PT21 (#905), PT22, PT24, PT30, PT36, PT48, PT50. C: PT2, PT3, PT7, PT8, PT10,
PT11, PT12, PT14, PT15, PT20, PT26, PT27, PT28, PT31, PT33, PT35, PT38, PT39,
PT44, PT49.

## 3. Confronto con l'analisi e con la 0.7.5

Solo dove la differenza è grande, con la prudenza della sezione 1 dell'analisi
sui campioni piccoli. I numeri della 0.8 sono con le correzioni a mano della
sezione 1 (PT21 con la #905, senza PT29).

| Che cosa | 0.4–0.7 | 0.7 | 0.7.5 | 0.8 | Giudizio |
|---|---|---|---|---|---|
| Fermi di tutti i pacchetti | — | 286 h | 5 h | 250 h | grande: tornati ai livelli della 0.7 |
| Durata della tappa (prima issue → ultima unione) | — | circa 198 h | 71 h | 178 h (PT29 escluso) | con 79 pacchetti invece di 39 (0.7.5) e 48 (0.7) |
| Medi con almeno un giro di CI | 65 % (35 su 54) | 69 % (11 su 16) | 48 % (15 su 31) | 64 % (25 su 39) | la 0.7.5 era l'eccezione |
| Medi di Claude / di Codex con almeno un giro | 64 % / 65 % | — | 47 % / 50 % | 55 % (11 su 20) / 74 % (14 su 19) | Codex più alto, entro il caso (stima) |
| Lavoro con almeno un giro, che tocca `app` / gli altri | 60 % / 41 % | — | 59 % / 19 % | 50 % (4 su 8) / 59 % (24 su 40) | l'ipotesi di `app` della 0.7.5 non si ripete |
| Spearman righe di codice / giri, medi | 0,11 (tutti) | — | 0,38 | 0,01 (dallo script, con PT29) | nessuna relazione, come nell'analisi |
| Correzioni dei riferimenti per pacchetto di riferimenti | 0,9 | 1,0 | 0 (su 1) | 0,94 (29 su 31) | uguale |
| Correzioni del codice dopo l'unione | non registrate | — | 0 | 0 | uguale, su finestre corte |
| Durata mediana di una run verde | 14,9 min | — | 13,0 min | 16,2 min | piccola |
| Verde → unione, pull request principali (mediana) | — | — | 0,14 h | 0,76 h | la revisione della catena, fra verde e unione |

**La 0.7.5 era l'eccezione.** I medi della 0.8 hanno avuto giri di CI come
quelli delle tappe 0.4–0.7, non come la 0.7.5, che aveva medi di natura
diversa (CI, strumenti, interfaccia). Anche il calo dei giri fuori da `app`
della 0.7.5 (19 %) non si ripete: nella 0.8 il codice fuori da `app` è
geometria e DXF, e lì i giri sono tornati al 59 %.

**I fermi.** Tornano quelli della 0.7, per gli stessi motivi che l'analisi
aveva trovato (sezione 2.4): buchi delle schede, casi sbagliati, solid-space.
La revisione delle interfacce prima delle parti (PT4, PT5, PT40) li ha ridotti
ma non tolti: i buchi sono emersi a rate scrivendo il codice e i casi (#764,
proposta del 2026-11-18 sulla preparazione della tappa). In più ci sono le
risposte condivise (PT3, PT22) e gli strumenti di verifica arrivati dopo i
riferimenti (F8, F9, F11, F12).

**Le revisioni.** I medi di Codex rivisti da Claude hanno avuto quasi sempre
due o più giri (16 su 19); quelli di Claude rivisti da Codex di rado (5 su
17 con revisione). Non si separa il modello di chi scrive da quello di chi
rivede (sezione 1).

## 4. Gli impegnativi di un tempo in fascia media

La scheda chiede, per ogni pacchetto che il piano dava come impegnativo in
passato e che la 0.8 ha realizzato in fascia media, come è andato. La
corrispondenza è con la prova sulla carta dell'analisi (sezione 4), che
segnava 12 impegnativi: restano fuori dal gruppo la revisione (carta PT2,
oggi PT4, PT5, PT40), i blocchi nel DXF (carta PT20, oggi PT18) e la coda
comune del canale (carta PT33, oggi PT37), rimasti in fascia forte, e la
definizione modificata (carta PT24), che non è diventata un pacchetto a sé. Il
tracciamento (PT34) non era nella prova sulla carta: lo aggiunge la scheda.

| Pacchetto | Nella prova sulla carta | Riferimenti (fascia) | Impegno dichiarato | Righe di codice | Run fallite | Giri di revisione | Fermate per una decisione | Correzioni della scheda | Correzioni dei riferimenti | Ore presa→unione | Fermo (h) |
|---|---|---|---|---|---|---|---|---|---|---|---|
| PT6 scheletro | PT1, entità composte | non servono | non noto | 2849 | 0 | — | 0 | 0 | — | 4,8 | 0 |
| PT13 testo multiriga | PT6, impaginazione | RT13 (forte) | alto | 731 | 0 | 2 | 1 | 1 (#850) | 1 | 5,4 | 3,7 |
| PT16 blocchi | PT18, blocchi | RT16 (forte) | alto | 802 | 1 | 1 | 3 | 2 (#781, #785) | 0 | 5,2 | 0 |
| PT23 quote lineari | PT11, quota lineare | RT23 (forte) | alto | 1094 | 1 | 3 | 2 | 1 (#825) | 1 | 8,2 | 3,6 |
| PT25 quote nel DXF | PT12, quota nel DXF | RT25 (leggera) | alto | 1180 | 1 | 2 | 2 | 0 | 1 | 13,1 | 3,6 |
| PT32 unione con distanza | PT32, *Join* con tolleranza | RT32 (forte) | alto | 752 | 0 | 1 | 2 | 1 (#772) | 2 | 2,6 | 1,1 |
| PT34 tracciamento | — | RT34 (leggera) | alto | 576 | 1 | 1 | 3 | 2 (#759, #761) | 2 | 11,9 | 4,8 |
| PT42 righe dei motivi | PT26, riempimento con isole | RT42 (forte) | alto | 428 | 2 | 2 | 3 | 0 | 1 | 19,9 | 5,8 |
| PT43 contorni dei retini | PT27, rilevamento del contorno | RT43 (forte) | alto | 429 | 1 | 1 | 4 | 0 | 1 | 17,8 | 3,1 |

**Giri di correzione.** Sei su nove hanno avuto almeno una run fallita (7 run
in tutto), come i medi (64 %) e molto più degli impegnativi delle tappe
0.4–0.7 (22 %, 6 su 27), che però erano fatti in fascia forte e prima della
catena. Un giro di CI costa un quarto d'ora di macchina: è il costo che
l'analisi aveva previsto (sezione 3.9, circa 0,6 run in più per pacchetto),
qui circa 0,45 (0,78 per pacchetto contro 0,33).

**Buchi trovati in revisione.** Quattro su otto hanno avuto un secondo giro di
revisione, PT23 un terzo; le segnalazioni bloccanti riguardano casi che i
riferimenti non coprivano (per PT23: gli snap del testo della quota, il centro
di un testo su più righe). La revisione mirata, anche in fascia media, li ha
trovati prima dell'unione.

**Correzioni dopo il merge.** Nessuna, né del codice né delle schede dopo
l'unione. I riferimenti sono stati corretti 9 volte (su 7 pacchetti di
riferimenti), sempre prima dell'unione del pacchetto di lavoro.

**Tempo.** Mediana di 8,2 ore dalla presa all'unione (medi della 0.8: 8,1;
impegnativi delle tappe 0.4–0.7: 1,9, la 0.7 8,1). Il tempo è andato quasi
tutto nelle **fermate per una decisione** (20, in otto pacchetti su nove) e
nelle correzioni delle schede (7, in cinque pacchetti): buchi delle schede,
casi che sembravano sbagliati, dipendenze mancanti. È lo stesso costo che
hanno avuto i medi di Claude del gruppo B (14 fermate in otto pacchetti), e
non dipende dalla difficoltà di un tempo.

**Che cosa ne viene.** Il codice degli impegnativi di un tempo, con le
decisioni scritte prima e i riferimenti forti, è stato scritto senza difetti
visti dopo l'unione e con il costo di un medio. Ma due delle tre condizioni
del modello «la fascia forte decide e controlla» sono andate diversamente dal
previsto: le decisioni non erano tutte scritte prima (venti fermate), e il
controllo (revisione mirata) è stato in fascia media. E la fascia «media» di
Claude è stata Opus 5.5 con impegno alto.

## 5. Le condizioni della #442

| Condizione | Che cosa dice la 0.8 | Perché |
|---|---|---|
| 1. Si divide per un motivo che si scrive | confermata | Il piano scrive un motivo per ogni divisione (sezione 6, «Motivi»). Le divisioni per modulo e per tipo (modello, DXF, comando) hanno tenuto: nessun pacchetto ha avuto una parte «b» aggiunta durante la tappa. |
| 2. La misura conta il codice | confermata come indicazione, non come limite | 15 pacchetti su 48 superano le 600 righe di codice, e i giri non crescono con la misura (Spearman 0,01 sui medi). PT6 (2849 righe, scheletro) e PT19 (1100) non hanno avuto giri. |
| 3. La fascia segue le decisioni | confermata per Opus 5.5 alto; non provata per Opus medio | Sezione 4: nessun difetto dopo l'unione, costo di un medio. Ma 8 su 9 dichiarano impegno alto, la riga forte della tabella. |
| 4. Riferimenti delicati in fascia forte | non smentita, non confermata | 1,0 correzioni per pacchetto forte, 0,9 per leggero; almeno quattro correzioni non sono errori del caso (sezione 2.7). Le correzioni dei riferimenti leggeri di Claude (0,4) e di Codex (1,9) dicono più dell'autore che della fascia. |
| 5. Revisione delle interfacce prima delle parti | necessaria, non sufficiente | PT4, PT5 e PT40 hanno rivisto tutte le schede prima della presa, eppure 55 fermate nei pacchetti di lavoro. La revisione mirata c'è stata, in fascia media. |
| 6. Pacchetto iniziale di interfaccia solo se toglie file comuni | confermata | PT6 (con PT7–PT9) ha tolto le visite di `Entity` dalle catene dei tipi; nessun giro, nessun fermo. |
| 7. Si registra e si misura | in parte | Il modello è dichiarato da 45 pull request di lavoro su 48; l'impegno da 33 (le altre «non noto» o niente, quasi tutte di Codex). |

## 6. Proposta di decisione definitiva

Per il responsabile, da scrivere in [DECISIONI.md](../../DECISIONI.md) se la
approva:

> **Pacchetti medi: decisione definitiva sulle condizioni della #442.** Letta
> la misura della 0.8 (PT47), le condizioni 1–7 della sezione 5 dell'analisi
> della #442 e il modello «la fascia forte decide e controlla, la media
> realizza» diventano definitivi, con quattro precisazioni:
>
> 1. **Condizione 3, come è stata provata.** Il codice che traduce decisioni
>    scritte e coperte da riferimenti di fascia forte va in fascia media anche
>    se un tempo era impegnativo; per Claude con Opus 5.5 e l'impegno che la
>    scheda chiede (alto se ci sono casi limite), come nella 0.8. Opus 5.5
>    medio sugli impegnativi di un tempo non è provato: il piano della 0.8.1
>    lo prova su due o tre pacchetti scelti, scritti nella colonna «Impegno»,
>    e la chiusura della 0.8.1 li misura con lo stesso script.
> 2. **Condizione 2, come indicazione.** Le 600 righe di codice restano la
>    misura sopra la quale si cerca una divisione con un motivo (condizione
>    1); non sono un limite per un pacchetto che non si divide bene, perché
>    nella 0.8 i giri non crescono con la misura.
> 3. **Condizioni 4 e 5: le fermate vengono dalle schede, non dalla fascia.**
>    Restano come sono; la difesa contro le fermate è la preparazione della
>    tappa già decisa il 2026-11-19 (#764: revisione del piano, prova a secco
>    dei riferimenti, risposte condivise). Una pull request che corregge un
>    riferimento scrive la causa in una riga («errore del caso» o «risposta
>    cambiata da <pacchetto>»), così la misura separa i due casi.
> 4. **Condizione 7, l'impegno anche dalla catena.** Quando la pull request
>    non sa l'impegno, vale quello del commento di avvio (`[impegno: …]`);
>    `scripts/misura_pacchetti.py` lo legge dalla issue nella pausa dopo la
>    0.8.

**Che cosa le cifre non dicono.**

- Se Opus 5.5 **medio** basterebbe per il codice degli impegnativi di un
  tempo: nella 0.8 non è stato usato su nessuno di loro (precisazione 1).
- Se una revisione mirata in fascia **forte** avrebbe trovato di più: le
  revisioni sono state di GPT-5.6 Sol, e hanno comunque trovato le bloccanti
  della sezione 4.
- Se il codice regge nell'uso: la finestra dopo l'unione è di pochi giorni e
  la prova manuale non è ancora fatta.
- Quanto costino i modelli: i token non sono misurati (analisi, sezione 1).
- Quanto delle 64 fermate si sarebbe evitato con la preparazione decisa il
  2026-11-19: lo dirà la misura degli arresti alla chiusura della 0.8.1
  (#764).

## 7. Dati grezzi e comando

### 7.1 Per pacchetto

Come nella misura della 0.7.5, sezione 7.1. Per PT21 e PT29 vale la sezione 1
(la tabella mostra la #900 e la #897, correzioni delle schede).

| Codice | Issue | Tipo | Diff. | Previsti (agente, fascia, impegno) | Autore | Modello dichiarato | Impegno (fonte) | PR | Commit | File | Righe | Righe di codice | Run | Fallite | Annullate | Giri | Ore PR | Ore presa→unione | Correzioni riferimenti | Correzioni codice | Altre PR |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 0.8-PT1 | #667 | pacchetto | facile | codex, leggero, medio | codex | non dichiarato | non dichiarato | #765 | 6 | 31 | 448 | 209 | 6 | 1 | 4 | 1 | 2,5 | 46,2 | — | — | — |
| 0.8-PT2 | #668 | pacchetto | media | codex, medio, medio | codex | GPT-6 | non noto | #754 | 6 | 9 | 411 | 237 | 6 | 1 | 4 | 1 | 27,3 | 27,4 | — | — | — |
| 0.8-PT4 | #669 | pacchetto | impegnativa | codex, forte, alto | claude | Claude Opus 5.5 | non noto | #763 | 3 | 19 | 517 | 0 | 2 | 0 | 0 | 0 | 1,1 | 38,2 | — | — | — |
| 0.8-PT5 | #670 | pacchetto | impegnativa | codex, forte, alto | claude | Claude Opus 5.5 | non noto | #763 | 3 | 19 | 517 | 0 | 2 | 0 | 0 | 0 | 1,1 | 38,2 | — | — | — |
| 0.8-PT6 | #671 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | non noto | #766 | 4 | 44 | 3477 | 2849 | 4 | 0 | 0 | 0 | 3,9 | 4,8 | — | — | — |
| 0.8-PT7 | #672 | pacchetto | media | codex, medio, medio | codex | GPT-6 | non noto | #780 | 3 | 17 | 499 | 251 | 3 | 0 | 2 | 0 | 3,3 | 3,5 | — | — | — |
| 0.8-PT8 | #673 | pacchetto | media | claude, medio, medio | claude | Claude Opus 5.5 + Claude Opus | medio (pull request) | #779 | 2 | 15 | 926 | 539 | 2 | 0 | 0 | 0 | 7,7 | 7,9 | — | — | — |
| 0.8-PT9 | #674 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | alto (pull request) | #786 | 3 | 12 | 2705 | 1732 | 2 | 1 | 0 | 1 | 2,2 | 6,0 | — | — | — |
| 0.8-PT3 | #675 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol + GPT-6 sol | medio (pull request) | #794 | 18 | 6 | 747 | 0 | 20 | 5 | 9 | 5 | 20,5 | 20,8 | — | — | — |
| 0.8-RT10 | #676 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 | medio (pull request) | #822 | 1 | 8 | 539 | 0 | 1 | 0 | 0 | 0 | 2,1 | 2,2 | — | — | — |
| 0.8-PT10 | #677 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | medio (pull request) | #827 | 7 | 17 | 459 | 237 | 5 | 1 | 2 | 1 | 11,7 | 12,3 | — | — | — |
| 0.8-RT48 | #678 | riferimenti | media | codex, forte, alto | codex | GPT-6 | non noto | #823 | 2 | 10 | 519 | 0 | 3 | 0 | 2 | 0 | 2,1 | 2,2 | — | — | — |
| 0.8-PT48 | #679 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | alto (pull request) | #826 | 4 | 21 | 1363 | 648 | 4 | 2 | 0 | 2 | 10,1 | 10,7 | — | — | — |
| 0.8-RT49 | #680 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 | medio (pull request) | #792 | 1 | 15 | 603 | 0 | 1 | 0 | 0 | 0 | 0,5 | 0,6 | — | — | — |
| 0.8-PT49 | #681 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | non noto | #844 | 5 | 8 | 538 | 375 | 4 | 1 | 0 | 1 | 3,7 | 4,0 | — | — | — |
| 0.8-RT11 | #682 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 | medio (pull request) | #817 | 2 | 11 | 999 | 0 | 3 | 0 | 1 | 0 | 2,2 | 2,3 | #832 | — | — |
| 0.8-PT11 | #683 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | medio (pull request) | #831 | 3 | 14 | 576 | 362 | 4 | 1 | 1 | 1 | 8,5 | 10,5 | — | — | — |
| 0.8-RT12 | #684 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 | medio (pull request) | #793 | 1 | 13 | 624 | 0 | 1 | 0 | 0 | 0 | 0,5 | 0,6 | #849 | — | — |
| 0.8-PT12 | #685 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | medio (pull request) | #847 | 4 | 15 | 464 | 366 | 4 | 0 | 2 | 0 | 2,9 | 3,5 | — | — | — |
| 0.8-RT13 | #686 | riferimenti | media | codex, forte, alto | codex | GPT-6 | non noto | #805 | 1 | 9 | 256 | 0 | 2 | 0 | 0 | 0 | 1,0 | 1,1 | #851 | — | — |
| 0.8-PT13 | #687 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 + Claude Opus | alto (pull request) | #854 | 3 | 16 | 1417 | 731 | 1 | 0 | 0 | 0 | 1,5 | 5,4 | — | — | — |
| 0.8-RT14 | #688 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 + Claude Opus | medio (pull request) | #820 | 1 | 3 | 591 | 0 | 1 | 0 | 0 | 0 | 2,1 | 2,2 | — | — | — |
| 0.8-PT14 | #689 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | medio (pull request) | #862 | 9 | 10 | 452 | 277 | 8 | 1 | 4 | 1 | 19,2 | 19,9 | — | — | — |
| 0.8-RT15 | #690 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 + Claude Opus | medio (pull request) | #808 | 1 | 13 | 521 | 0 | 1 | 0 | 0 | 0 | 4,1 | 4,2 | #863 | — | — |
| 0.8-PT15 | #691 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | medio (pull request) | #867 | 4 | 9 | 545 | 388 | 4 | 1 | 1 | 1 | 4,1 | 5,8 | — | — | — |
| 0.8-RT16 | #692 | riferimenti | media | codex, forte, alto | codex | GPT-6 | non noto | #797 | 1 | 10 | 444 | 0 | 1 | 0 | 0 | 0 | 1,0 | 1,1 | — | — | — |
| 0.8-PT16 | #693 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | alto (pull request) | #845 | 2 | 23 | 1761 | 802 | 2 | 1 | 0 | 1 | 4,7 | 5,2 | — | — | — |
| 0.8-RT17 | #694 | riferimenti | media | codex, forte, alto | codex | GPT-6 | non noto | #796 | 1 | 8 | 164 | 0 | 2 | 0 | 0 | 0 | 1,1 | 1,1 | — | — | — |
| 0.8-PT17 | #695 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 + Claude Opus | alto (pull request) | #861 | 2 | 15 | 1048 | 596 | 2 | 0 | 0 | 0 | 1,3 | 1,7 | — | — | — |
| 0.8-RT18 | #696 | riferimenti | media | codex, forte, alto | codex | GPT-6 | non noto | #824 | 3 | 25 | 3903 | 0 | 4 | 1 | 1 | 1 | 3,5 | 3,6 | #872 | — | — |
| 0.8-PT18 | #697 | pacchetto | impegnativa | claude, forte, alto | claude | Claude Opus 5.5 | alto (pull request) | #877 | 11 | 32 | 3400 | 2458 | 12 | 6 | 3 | 6 | 12,9 | 20,5 | — | — | — |
| 0.8-RT22 | #698 | riferimenti | media | codex, leggero, medio | codex | GPT-5.6 sol | non noto | #818 | 1 | 7 | 386 | 0 | 1 | 0 | 0 | 0 | 2,1 | 2,2 | #846, #852, #864, #879 | — | — |
| 0.8-PT22 | #699 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | alto (pull request) | #881 | 8 | 18 | 2106 | 1390 | 4 | 2 | 0 | 2 | 1,3 | 25,4 | — | — | #878 (non unita) |
| 0.8-RT19 | #700 | riferimenti | media | codex, leggero, medio | codex | GPT-5.6 sol | non noto | #858 | 1 | 9 | 321 | 0 | 1 | 0 | 0 | 0 | 1,2 | 7,0 | #889 | — | — |
| 0.8-PT19 | #701 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | alto (pull request) | #890 | 2 | 12 | 1840 | 1100 | 2 | 0 | 0 | 0 | 1,2 | 4,7 | — | — | — |
| 0.8-RT20 | #702 | riferimenti | media | claude, leggero, medio | ? | non dichiarato | non dichiarato | — | — | — | — | — | — | — | — | — | — | — | — | — | #806 (non unita) |
| 0.8-PT20 | #703 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | non noto | #869 | 2 | 14 | 540 | 279 | 2 | 0 | 0 | 0 | 3,7 | 4,3 | — | — | — |
| 0.8-RT21 | #704 | riferimenti | media | codex, leggero, medio | codex | non dichiarato | non dichiarato | #791 | 2 | 17 | 1593 | 0 | 2 | 0 | 1 | 0 | 0,5 | 0,5 | #899, #903 | — | — |
| 0.8-PT21 | #705 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | medio (pull request) | #900 | 3 | 2 | 70 | 0 | 3 | 1 | 0 | 1 | 1,5 | 10,9 | — | #905 | — |
| 0.8-RT23 | #706 | riferimenti | media | codex, forte, alto | codex | GPT-6 | non noto | #830 | 3 | 18 | 3628 | 0 | 2 | 0 | 0 | 0 | 7,2 | 10,8 | #884 | — | — |
| 0.8-PT23 | #707 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | alto (pull request) | #885 | 4 | 17 | 1976 | 1094 | 6 | 1 | 2 | 1 | 4 | 8,2 | — | — | — |
| 0.8-RT24 | #708 | riferimenti | media | codex, forte, alto | codex | GPT-6 | non noto | #839 | 1 | 24 | 1531 | 0 | 2 | 0 | 0 | 0 | 0,5 | 1,9 | #898 | — | — |
| 0.8-PT24 | #709 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 + Claude Opus | alto (pull request) | #901 | 1 | 11 | 1204 | 744 | 1 | 0 | 0 | 0 | 1,0 | 14,5 | — | — | — |
| 0.8-RT25 | #710 | riferimenti | media | codex, leggero, medio | codex | non dichiarato | non dichiarato | #837 | 4 | 4 | 125 | 0 | 4 | 1 | 2 | 1 | 1,6 | 1,7 | #893 | — | — |
| 0.8-PT25 | #711 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | alto (pull request) | #895 | 3 | 14 | 1841 | 1180 | 3 | 1 | 0 | 1 | 9,4 | 13,1 | — | — | — |
| 0.8-RT26 | #712 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 | medio (pull request) | #809 | 1 | 15 | 575 | 0 | 1 | 0 | 0 | 0 | 4,0 | 4,1 | #896 | — | — |
| 0.8-PT26 | #713 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | medio (pull request) | #892 | 8 | 10 | 631 | 472 | 11 | 1 | 7 | 1 | 12,0 | 12,4 | — | — | — |
| 0.8-RT27 | #714 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 | medio (pull request) | #810 | 1 | 27 | 911 | 0 | 1 | 0 | 0 | 0 | 4,0 | 4,1 | #904 | — | — |
| 0.8-PT27 | #715 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | medio (pull request) | #902 | 6 | 8 | 771 | 558 | 5 | 1 | 2 | 1 | 7,0 | 7,4 | — | — | — |
| 0.8-RT28 | #716 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 | medio (pull request) | #815 | 1 | 13 | 417 | 0 | 1 | 0 | 0 | 0 | 0,7 | 5,1 | — | — | — |
| 0.8-PT28 | #717 | pacchetto | media | codex, medio, medio | codex | non dichiarato | non dichiarato | #882 | 9 | 12 | 626 | 521 | 8 | 3 | 2 | 3 | 7,9 | 8,6 | — | — | — |
| 0.8-RT29 | #718 | riferimenti | media | codex, leggero, medio | codex | GPT-5.6 sol | non noto | #834 | 4 | 5 | 177 | 0 | 4 | 2 | 1 | 2 | 1,6 | 1,7 | #894, #906 | — | — |
| 0.8-PT29 | #719 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | medio (pull request) | #897 | 1 | 3 | 29 | 0 | 1 | 0 | 0 | 0 | 1,0 | 13,6 | — | — | #907 (non unita) |
| 0.8-RT30 | #720 | riferimenti | media | codex, leggero, medio | codex | non dichiarato | non dichiarato | #814 | 1 | 2 | 34 | 0 | 1 | 0 | 0 | 0 | 0,3 | 6,3 | #829 | — | — |
| 0.8-PT30 | #721 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 + Claude Opus | alto (pull request) | #833 | 1 | 10 | 914 | 437 | 1 | 0 | 0 | 0 | 1,0 | 12,5 | — | — | — |
| 0.8-RT31 | #722 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 | medio (pull request) | #835 | 1 | 12 | 2187 | 0 | 1 | 0 | 0 | 0 | 0,7 | 0,8 | — | — | — |
| 0.8-PT31 | #723 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | medio (pull request) | #848 | 4 | 12 | 657 | 405 | 3 | 1 | 0 | 1 | 3,1 | 5,0 | — | — | — |
| 0.8-RT32 | #724 | riferimenti | media | codex, forte, alto | codex | GPT-5.6 sol | non noto | #773 | 1 | 5 | 104 | 0 | 1 | 0 | 0 | 0 | 2,6 | 5,3 | #775, #777 | — | — |
| 0.8-PT32 | #725 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | alto (pull request) | #778 | 1 | 14 | 1116 | 752 | 1 | 0 | 0 | 0 | 1,0 | 2,6 | — | — | — |
| 0.8-RT33 | #726 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 | non noto | #757 | 2 | 33 | 1149 | 0 | 2 | 0 | 1 | 0 | 4,6 | 4,8 | — | — | — |
| 0.8-PT33 | #727 | pacchetto | media | codex, medio, medio | codex | GPT-6 | non noto | #782 | 6 | 11 | 939 | 633 | 9 | 1 | 6 | 1 | 5,8 | 5,9 | — | — | — |
| 0.8-RT34 | #728 | riferimenti | media | codex, leggero, medio | codex | GPT-5.6 sol | medio (pull request) | #795 | 3 | 11 | 803 | 0 | 3 | 1 | 1 | 1 | 7,8 | 64,8 | #801, #804 | — | — |
| 0.8-PT34 | #729 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 + Claude Opus | alto (pull request) | #803 | 2 | 22 | 997 | 576 | 2 | 1 | 0 | 1 | 6,8 | 11,9 | — | — | — |
| 0.8-PT35 | #730 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | non noto | #821 | 2 | 10 | 217 | 116 | 5 | 0 | 3 | 0 | 2,7 | 2,8 | — | — | — |
| 0.8-PT36 | #731 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | non noto | #758 | 3 | 9 | 954 | 500 | 4 | 1 | 1 | 1 | 7,9 | 8,3 | — | — | — |
| 0.8-PT37 | #732 | pacchetto | impegnativa | claude, forte, alto | claude | Claude Opus 5.5 | alto (pull request) | #838 | 2 | 12 | 2041 | 1502 | 2 | 1 | 0 | 1 | 1,4 | 1,8 | — | — | — |
| 0.8-PT38 | #733 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | medio (pull request) | #843 | 8 | 16 | 453 | 256 | 9 | 1 | 5 | 1 | 5,2 | 5,3 | — | — | — |
| 0.8-PT39 | #734 | pacchetto | media | codex, medio, medio | codex | non dichiarato | non dichiarato | #908 | 4 | 30 | 524 | 121 | 4 | 0 | 1 | 0 | 1,7 | 1,8 | — | — | — |
| 0.8-PT40 | #735 | pacchetto | impegnativa | codex, forte, alto | claude | Claude Opus 5.5 | non noto | #751 | 6 | 10 | 315 | 0 | 8 | 0 | 2 | 0 | 3,5 | 3,9 | — | — | — |
| 0.8-PT41 | #736 | pacchetto | facile | codex, leggero, medio | codex | GPT-5.6 sol | medio (pull request) | #769 | 2 | 12 | 139 | 73 | 2 | 0 | 1 | 0 | 8,3 | 9,3 | — | — | — |
| 0.8-RT42 | #737 | riferimenti | media | codex, forte, alto | codex | GPT-6 | non noto | #856 | 2 | 29 | 5442 | 0 | 2 | 0 | 1 | 0 | 1,1 | 1,2 | #866 | — | — |
| 0.8-PT42 | #738 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | alto (pull request) | #874 | 5 | 12 | 975 | 428 | 5 | 2 | 0 | 2 | 13,7 | 19,9 | — | — | — |
| 0.8-RT43 | #739 | riferimenti | media | codex, forte, alto | codex | GPT-6 | non noto | #855 | 3 | 44 | 680 | 0 | 6 | 1 | 3 | 1 | 6,9 | 7,0 | #875 | — | — |
| 0.8-PT43 | #740 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | alto (pull request) | #876 | 7 | 11 | 913 | 429 | 5 | 1 | 1 | 1 | 14,5 | 17,8 | — | — | — |
| 0.8-RT44 | #741 | riferimenti | media | claude, leggero, medio | claude | Claude Opus 5.5 | medio (pull request) | #816 | 1 | 21 | 703 | 0 | 1 | 0 | 0 | 0 | 0,6 | 5,1 | — | — | — |
| 0.8-PT44 | #742 | pacchetto | media | codex, medio, medio | codex | GPT-5.6 sol | medio (pull request) | #886 | 7 | 10 | 657 | 404 | 6 | 3 | 1 | 3 | 6,8 | 7,2 | — | — | — |
| 0.8-RT50 | #743 | riferimenti | media | codex, forte, alto | codex | GPT-6 | non noto | #857 | 2 | 16 | 542 | 0 | 3 | 0 | 1 | 0 | 1,0 | 1,2 | #865, #871, #887 | — | — |
| 0.8-PT50 | #744 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | alto (pull request) | #873 | 2 | 14 | 767 | 376 | 1 | 0 | 0 | 0 | 2,9 | 9,0 | — | — | — |
| 0.8-PT45 | #745 | pacchetto | facile | codex, leggero, basso | codex | GPT-5.6 sol | non noto | #910 | 2 | 7 | 59 | 4 | 2 | 0 | 0 | 0 | 0,8 | 0,9 | — | — | — |
| 0.8-PT46 | #746 | pacchetto | facile | codex, leggero, basso | codex | GPT-5.6 sol | basso (pull request) | #911 | 2 | 4 | 203 | 0 | 2 | 0 | 0 | 0 | 0,2 | 0,3 | — | — | — |
| 0.8-PT47 | #747 | pacchetto | media | claude, medio, medio | ? | non dichiarato | non dichiarato | — | — | — | — | — | — | — | — | — | — | — | — | — | — |

Issue senza pull request principale unita: 0.8-RT20 (chiusa), 0.8-PT47 (aperta).

### 7.2 Comando

Dalla radice del repository:

```sh
scripts/misura_pacchetti.py --tappe 0.8 --dati misura-0.8 --scarica > misura-0.8.md
```

`--scarica` legge da GitHub e salva tutto nella cartella `misura-0.8/`; senza
`--scarica` lo script rifà i calcoli da quella cartella, senza rete. I
conteggi della sezione 2.8 si rifanno leggendo, per ogni pull request
principale, `repos/Matleyx78/tecnigrafo/issues/<numero>/comments` con le
regole della sezione 1, insieme ai commenti delle issue che lo script salva in
`misura-0.8/commenti.json`.
