# ADR 0053: Catena degli agenti avviata con un commento

- **Stato:** Accettato
- **Data:** 2026-11-14

## Contesto

Fino alla 0.8 ogni passaggio di un lavoro passa dal responsabile, chat per
chat: l'avvio di un pacchetto, la revisione, le correzioni, i giri successivi,
la ripresa dopo una decisione, la ripartenza dopo un merge. Con due agenti e
il giro a tre (riferimenti, lavoro, revisione;
[LAVORO-IN-PARALLELO.md](../LAVORO-IN-PARALLELO.md), "Chi fa cosa") il tempo
del responsabile se ne va in messaggi da copiare, e le sessioni aperte si
accumulano.

Il 10 e il 14 novembre 2026 la proposta della
[#764](https://github.com/Matleyx78/tecnigrafo/issues/764) («[0.8] Proposte di
metodo») è stata provata nel repository usa e getta
[Matleyx78/prova-orchestratore](https://github.com/Matleyx78/prova-orchestratore)
con tutti i tipi di lavoro: pacchetto eseguito da Codex e da Claude con
correzione e secondo giro, pacchetto avviato dalla chat di Codex, riferimenti
che al merge avviano il pacchetto, riferimento che scopre un errore del
programma, contratto annidato con tre giri, revisione delle interfacce, e la
sentinella su una catena interrotta. In tutte gli interventi del responsabile
si sono ridotti a decisioni e merge.

## Decisione

1. **Avvio.** Il responsabile avvia un lavoro con **un solo commento** nella
   issue, che menziona l'agente della sua etichetta `agente:…`; per Claude il
   commento sceglie modello e impegno (`[modello: …]`, `[impegno: …]`). Codex
   si avvia anche dalla sua chat, quando serve un modello preciso. Solo il
   responsabile avvia gli agenti: il workflow di Claude parte solo per i suoi
   commenti, e Codex risponde solo a lui. Non ci sono limiti di quota
   nell'automazione: ogni catena comincia da un suo avvio.
2. **Ruoli.** L'**esecutore** è l'agente dell'etichetta della issue, il
   **revisore** l'altro. Vale per pacchetti, riferimenti e revisioni delle
   interfacce.
3. **Passaggi solo con gli script.** `scripts/passa.sh` (revisione, correzioni,
   analisi di una CI rossa) e `scripts/avvia.sh` scrivono la menzione a nome
   del responsabile, con un token personale a grana fine (secret
   `CATENA_PAT`) quando scrive Claude da GitHub Actions. Nessun agente scrive
   a mano la chiocciola davanti al nome di un agente: ogni menzione avvia una
   sessione. Le richieste di revisione e di analisi per Codex vanno nella
   issue collegata, perché su una pull request attiverebbero la sua revisione
   automatica.
4. **Revisione a giri.** Il verbale porta «Giro di revisione: N» e classi
   **[bloccante]**, **[decisione]** (i buchi visibili) e **[facoltativa]**.
   Le [bloccante] dei giri 1 e 2 tornano all'esecutore; una [decisione], o il
   giro 3, fermano; altrimenti l'etichetta `pronta`. Dal giro 2 si rivedono
   solo i commit di correzione. Che cosa blocca resta quello di
   [revisione.md](../agenti/revisione.md).
5. **Decisioni.** Chi si ferma mette `decisione` sulla issue e sulla pull
   request e lascia la riga da copiare, senza chiocciola, con il nome
   dell'agente e la issue in cui scriverla. Il responsabile decide **sempre in
   una issue**. Un revisore riprende con «Ripresa dopo la decisione (giro N)»,
   che non conta come giro.
6. **Dopo il merge.** Il workflow *Dopo il merge* legge le righe
   `Avvia dopo il merge: #N` della pull request unita e avvia la issue N con
   il suo agente, se le issue delle sue righe "Dipende da" e "Riferimenti"
   sono chiuse. Le scrivono i riferimenti (per il loro pacchetto) e le
   correzioni delle schede o dei riferimenti fatte dopo una decisione (per il
   pacchetto fermo).
7. **Sentinella.** Ogni 30 minuti un workflow mette `ferma` sulle catene
   avviate e senza movimento da almeno **60 minuti** (la CI di Tecnigrafo è
   lenta), comprese le decisioni scritte e mai riprese, e segnala
   `AA-Presa in carico` rimasta. Lascia stare le pull request con la CI in
   corso.
8. **Niente lotti.** Con la catena ogni lavoro si avvia con il suo commento,
   anche molti nello stesso momento: il lavoro a lotti di riferimenti e
   revisioni non c'è più (`docs/agenti/lotti.md` è tolto).
9. **Ambienti.** Gli ambienti in cui gli agenti lavorano nella catena (il
   workflow di Claude, gli ambienti di Codex e i loro script di
   preparazione) li decide il responsabile
   ([comandi.md](../agenti/comandi.md), "Gli ambienti della catena").
10. **Dove sono le regole.** Le regole per gli agenti sono in
    [docs/agenti/catena.md](../agenti/catena.md), con due frasi nel nucleo di
    AGENTS.md (la chiocciola e il passaggio prima di terminare) e una riga
    nella sua tabella; i file del lavoro (pacchetto, riferimenti, contratto,
    revisione) rimandano a catena.md dove la catena cambia il modo di
    fermarsi. Le frasi e la preparazione per il responsabile sono nella
    [guida](../GUIDA-RESPONSABILE.md), A2a.
11. **Quando vale.** Dal merge della pull request che la introduce, **a tappa
    aperta** (la 0.8), in via eccezionale rispetto alla regola che il metodo
    cambia solo nella pausa fra le tappe ([DECISIONI.md](../DECISIONI.md),
    2026-11-03 e 2026-11-07): il responsabile la considera molto più veloce
    del metodo attuale. Ogni agente legge l'AGENTS.md del suo ramo, quindi una
    pull request nata prima va aggiornata su `main` prima di entrare nella
    catena. In solid-space arriva con una pull request sua, con le stesse
    voci.

## Alternative considerate

- **Proposta nella pausa dopo la 0.8**, come chiedono le regole di metodo: era
  la scelta iniziale. Scartata dopo il giro di prove generale, perché il
  guadagno di tempo vale già per il resto della 0.8.
- **Un orchestratore che ogni dieci minuti guarda lo stato e avvia da solo le
  issue pronte** (l'idea del 2026-11-10): scartato, perché solo il
  responsabile avvia gli agenti; resta automatico solo ciò che segue un suo
  avvio o un suo merge.
- **Una GitHub App come identità degli script**: gli agenti non rispondono ai
  bot, e Codex nemmeno a un'app; serve un token del responsabile.
- **Revisioni di Codex chieste sulla pull request**: attivano la sua revisione
  automatica, che nelle prove è stata incostante (sullo stesso commit una
  volta due [bloccante], la volta dopo nessuna).

## Conseguenze

- Il responsabile interviene solo per avvii, decisioni, lettura finale e
  merge; le etichette `pronta`, `decisione` e `ferma` gli dicono dove.
- I rallentamenti veri sono la quota di Codex (cinque ore e settimanale),
  coperta dalla sentinella, e la coda di GitHub Actions, di uno o due minuti
  per passaggio.
- Codex avviato da una menzione usa il suo modello predefinito, con impegno
  non noto: per un modello preciso si avvia dalla chat, e la catena prosegue
  lo stesso.
- Il workflow di Claude gira fino a cinque ore, perché aspetta la CI di
  Tecnigrafo; consuma il piano Max, non l'API.
- Serve una preparazione una volta per repository: app di Claude, secret
  `CLAUDE_CODE_OAUTH_TOKEN` e `CATENA_PAT`, un ambiente legacy di Codex per le
  menzioni (guida, A2a).
