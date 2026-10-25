# ADR 0026: Descrizione dei comandi

- **Stato:** Proposto
- **Data:** 2026-10-25

## Contesto

Un comando oggi è un nome, degli alias e una fabbrica nel `CommandRegistry`
(ADR 0005). Le sue domande esistono solo mentre gira. Per elencare i comandi a
un agente (`tecnigrafo-mcp`, ADR 0027), per il manuale e, più avanti, per
l'assistente IA dell'utente (tappa 0.8) serve una **descrizione leggibile da
una macchina**: che cosa chiede il comando, in che ordine, con quali opzioni e
quale tipo di valore si aspetta. Conviene farla ora che i comandi sono una
decina (decisione del 2026-10-25, idea #224), e tenerla vera con un controllo
automatico, non con la disciplina.

## Decisione

### 1. La descrizione

In `cmd`, senza dipendenze nuove:

```cpp
enum class ValueType : std::uint8_t {
    Point, Distance, Angle, Integer, Real, Text, Name, Color, Keyword,
    FileOpen, FileSave
};

struct PromptDescription {
    std::string id;             ///< stable, English: "next-point"
    std::string text;           ///< as shown, with {default} for the value in < >
    ValueType value;            ///< what a typed answer is; Keyword if only options
    std::vector<Keyword> keywords;
    bool acceptsEmpty{false};   ///< Enter alone is an answer (ends, or takes the default)
};

struct CommandDescription {
    std::string name;                  ///< "PLINE"
    std::vector<std::string> aliases;  ///< {"PL"}
    std::string summary;               ///< one English sentence
    bool selectsFirst{false};          ///< asks "Select objects:" before starting
    std::vector<PromptDescription> prompts;  ///< in the usual order of a run
};
```

- `prompts` elenca **tutte** le domande che il comando può porre, una volta
  ciascuna, nell'ordine in cui compaiono nell'uso normale; le varianti (per
  esempio quelle di un'opzione) seguono la domanda che le apre. Non si
  descrive il grafo dei passaggi: l'ordine e le parole chiave bastano a un
  agente, e il grafo diventerebbe un secondo programma da tenere allineato.
- Il testo è quello dell'ADR 0005, intero e in inglese; `{default}` sostituisce
  il valore che cambia (`Specify radius of circle or [Diameter] <{default}>:`).
- Le domande del runner (`Select objects:`) non si ripetono nei comandi: le
  indica `selectsFirst`.

### 2. Il registro

- `CommandRegistry::add` riceve una `CommandDescription` e la fabbrica; nome e
  alias vengono dalla descrizione. Un comando senza descrizione non si
  registra.
- `descriptions()` le restituisce in ordine alfabetico di nome;
  `describe(nameOrAlias)` una sola.
- I comandi propri del programma (`LAYER`, `PROPERTIES`, `QSELECT`, `RIBBON`,
  `RIBBONCLOSE`) si registrano allo stesso modo, nell'applicazione.

### 3. Il controllo

- `CommandRunner` ha un **controllo delle descrizioni**: ogni volta che un
  comando pone una domanda, il runner verifica che corrisponda a una
  `PromptDescription` del comando (testo con `{default}` al posto del valore,
  parole chiave uguali, tipo di valore compatibile con `PromptKind`). Una
  domanda non descritta è un errore che fa fallire il test o il caso.
- Il controllo è acceso nei test di `cmd` e `script`, in `tecnigrafo-script`
  (quindi in tutti i casi di verifica, ai due livelli) e in `tecnigrafo-mcp`;
  è spento nel programma distribuito.
- Un test verifica che ogni comando registrato abbia sintesi non vuota e
  almeno una domanda, e che le parole chiave di ogni domanda compaiano tra
  parentesi quadre nel suo testo.

### 4. Uso

- `tecnigrafo-mcp` espone le descrizioni in JSON (ADR 0027).
- `tecnigrafo-script describe [nome]` le stampa in testo, una riga per campo,
  per il manuale e per chi lavora senza MCP.
- I testi passano per il sistema di traduzione della tappa 0.9; `id` e
  `ValueType` non si traducono.

## Alternative considerate

- **Descrizione scritta a parte** (un file JSON o YAML): si scollerebbe dal
  codice al primo cambiamento; legarla alla registrazione e controllarla a
  ogni esecuzione la tiene vera.
- **Descrizione ricavata dai comandi in esecuzione**: darebbe solo le domande
  dei percorsi provati, e nessuna sintesi.
- **Grafo completo dei passaggi**: più informazione, ma un secondo modello del
  comando da mantenere; l'ordine e il controllo bastano oggi.

## Conseguenze

- Ogni comando nuovo nasce con la sua descrizione: la scheda del pacchetto la
  scrive per intero, insieme alle domande.
- Un cambio di testo di una domanda si fa in due punti, comando e
  descrizione, e il controllo segnala subito se ne manca uno.
- La tappa 0.8 riusa le descrizioni per il canale dell'assistente IA
  dell'utente.
