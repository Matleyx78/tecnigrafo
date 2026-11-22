# [0.8.1-PT19] `cmd`: `-ATTEDIT` uno per uno e globale

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8.1-RT19]` (#…), agente codex
- **Dipende da:** PT5, PT15, RT19
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0056](../../adr/0056-comandi-degli-attributi.md),
  punto 4; `cmd/src/commands/chprop_command.cpp` (colore e layer da riga di
  comando); la scheda [PT15](PT15-attributi-core.md) (`pickAttribute`); la
  cronologia `autocad-attributi.txt` ([dxf-di-prova.md](dxf-di-prova.md))

## File

- `cmd/include/tg/cmd/commands/attedit_command.hpp`,
  `cmd/src/commands/attedit_command.cpp` (nuovi)
- `cmd/src/name_list.hpp`, `cmd/src/name_list.cpp` (i caratteri jolly, se
  non ci sono già)
- `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt` (file comuni)
- `tests/cmd/attedit_command_test.cpp` (nuovo), `tests/CMakeLists.txt` (file
  comune)

## Cosa fare

Le richieste e i messaggi dell'ADR 0056, punto 4, nell'ordine della
cronologia:

1. La scelta degli attributi (`Select Attributes:`) prende **una** selezione,
   clic (`pickAttribute`), finestra o intersezione, e si chiude da sola, senza
   Invio, come `EXPLODE` ([dxf-di-prova.md](dxf-di-prova.md), «Come sono fatti
   gli script»); tiene solo gli attributi visibili che passano i tre filtri;
   `<n> found` e `<n> attributes selected.`. Con il clic i tre filtri vanno in
   `accept` di `pickAttribute` e valgono **prima** della scelta del più
   vicino, non dopo; gli attributi degli inserimenti annidati non si
   scelgono (PT15; verbale della revisione PT5, [bloccante] 5).
2. **Uno per uno:** per ciascun attributo, nell'ordine degli inserimenti e degli
   attributi, le opzioni; l'attributo corrente si segna (lo mostra il
   programma; nessun campo nel rapporto). *Value* con *Change* (`Enter string to
   change:`, `Enter new string:`, la prima occorrenza) e *Replace* (`Enter new
   attribute value:`); *Position*, *Height*, *Angle*, *Style*, *Layer*,
   *Color* con le loro richieste; *Next* passa al successivo.
3. **Globale:** sostituisce ogni occorrenza; con *No* agli attributi visibili
   prende anche gli invisibili e non chiede la selezione.
4. Un passo `Attedit` per tutto il comando.

## Regole della tappa che valgono qui

- I caratteri jolly come `-LAYER` e AutoCAD, senza maiuscole (ADR 0056,
  punto 4).

## Test richiesti

- Le quattro sequenze di `-ATTEDIT` di `autocad-attributi`; *Position* rifiutato
  per un attributo bloccato; nessun attributo scelto.
- Due attributi visibili sovrapposti, `SIGLA` e `CODICE`, con il filtro
  d'etichetta `CODICE`: il clic trova `CODICE` (`1 attributes selected.`);
  due inserimenti sovrapposti dei blocchi `A` e `B` con lo stesso `CODICE` e
  lo stesso valore, con il filtro del blocco `A`: il clic trova quello di `A`
  ([bloccante] 5; #992, [bloccante] 3).

## Fatto quando

- I casi di RT19 passano; la CI è verde.

## Prova a secco dei riferimenti

```text
# casi/attributi/attedit-uno-per-uno/script.tgs
OPEN
tre-componenti.dxf
-ATTEDIT
Y
COMPONENTE
CODICE
*
C
-1,-41
41,-19
V
R
K11
P
5,-27
H
3.5
N
```

`tre-componenti.dxf`: un DXF del caso con i tre inserimenti di
`autocad-attributi` prima di `-ATTEDIT`. Righe da controllare: l'`ATTRIB`
`CODICE` del primo inserimento con valore `K11`, posizione e altezza nuove; le
richieste in `messages:`. Un secondo script per il globale (`N`, `N`, filtri,
`cablare` → `verificare`).

- **Strumenti:** righe normali con la finestra scritta (`C` e due punti), su
  `main`.
- **Dipendenze:** PT15 (`pickAttribute`).
- **Fonte:** `autocad-attributi.dxf` e la sua cronologia.
- **Risposte condivise:** nessuna.
