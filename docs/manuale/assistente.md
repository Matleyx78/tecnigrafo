# Collegare un assistente IA

Tecnigrafo espone il disegno aperto a un assistente attraverso MCP. Il canale è
locale al computer, non apre porte di rete ed è **spento a ogni avvio**. Chi usa
il computer decide quando accenderlo e vede ogni operazione nella cronologia.

## Accendere il canale

Apri un disegno e attiva **AI Assistant** nel gruppo *View → Assistant* o con
l'interruttore **AI** della barra di stato. Il pulsante distingue canale spento,
in ascolto e assistente collegato. Il pannello *Assistant* mostra il comando da
copiare, le richieste e il loro esito. Un solo assistente può collegarsi alla
volta; spegni l'interruttore per interrompere il collegamento.

Il programma ponte è lo stesso eseguibile:

```sh
tecnigrafo --mcp
```

Resta avviato anche quando Tecnigrafo è chiuso o il canale è spento. In quel
caso una chiamata risponde che Tecnigrafo non è disponibile; dopo aver aperto il
programma e acceso il canale, la chiamata successiva riprova automaticamente.

## Claude Desktop

Aggiungi un server alla configurazione MCP di Claude Desktop, sostituendo il
percorso con quello dell'eseguibile installato:

```json
{
  "mcpServers": {
    "tecnigrafo": {
      "command": "/percorso/assoluto/tecnigrafo",
      "args": ["--mcp"]
    }
  }
}
```

Riavvia Claude Desktop dopo aver salvato la configurazione, poi accendi il
canale in Tecnigrafo.

## Claude Code

Registra il server locale nel progetto o nella configurazione utente:

```sh
claude mcp add tecnigrafo -- /percorso/assoluto/tecnigrafo --mcp
```

Avvia Claude Code e accendi il canale in Tecnigrafo. Se l'eseguibile è già nel
`PATH`, basta `tecnigrafo` al posto del percorso assoluto.

## Che cosa può fare

L'assistente dispone di sei strumenti:

- `list_commands` e `describe_command` scoprono i comandi e le loro richieste;
- `run` invia una sequenza di risposte alla sessione attiva;
- `report` legge un rapporto del disegno attivo;
- `open` apre un file in una nuova scheda;
- `save` salva il disegno in un percorso indicato.

Le richieste agiscono sulla scheda attiva e vengono eseguite una alla volta,
solo quando la riga di comando mostra `Command:`. Se l'utente sta usando un
comando, l'assistente attende fino a 30 secondi. Le righe che invia sono
precedute da `[AI]` nella cronologia. Ogni comando completato è un normale passo
di annullamento: usa `U` o `Ctrl+Z` per annullarlo.

L'assistente non può eseguire strumenti di test, interrogare direttamente il
modello, azzerare la sessione, aggirare le richieste dei comandi o lavorare in
parallelo con l'utente. Senza un disegno aperto può usare soltanto `open`.
Controlla sempre il risultato prima di salvare; spegnere il canale impedisce
nuove operazioni ma non annulla quelle già concluse.
