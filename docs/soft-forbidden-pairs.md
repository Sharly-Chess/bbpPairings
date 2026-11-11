# Soft forbidden pairs (SCS record)

Some regulations ask the arbiter to avoid certain pairings "as much as possible", typically
teams or players of the same club, without letting that override any pairing rule. The FFE
team cups are an example: the arbiter keeps same-club teams apart only when the Swiss criteria
are not harmed.

A `260` record cannot express this, since it makes the pairing absolute. The `SCS` record
(Sharly Chess, soft) lists pairs that the engine avoids as its lowest-priority criterion.

## Format

```
SCS RRF RRL PPPP PPPP ...
```

The layout is the same as a `260` record: the first and last rounds the record applies to,
then the pairing numbers of the players (or, in team mode, of the teams) that should not meet.
Every pair within the record is soft forbidden.

`SCS` uses its own prefix so it can never collide with a FIDE (numeric), JaVaFo (`XX?`) or
bbpPairings (`BB?`) code. Readers that do not know it, including upstream bbpPairings, ignore
it.

## Behaviour

A soft forbidden pair ranks below every FIDE quality criterion. Among the pairings that satisfy
the criteria equally, the engine picks the one with the fewest soft forbidden pairs, ahead of
the order in which the rules generate the candidates.

- **Dutch:** an extra edge-weight field counts the pairs in the current bracket that are not
  soft forbidden. It sits below the last float criterion and above the fields that enforce the
  order of transpositions and exchanges.
- **Team:** the same field sits below `[C10]` and above the §3.6 identifier, and the choice of
  upfloater set compares soft forbidden pairs after `[C6]` and `[C7]`, ahead of the order of
  §3.5.4.

The field only exists when an `SCS` record applies to the round, so files without one pair
exactly as before.

## Tests

`dutch_scs_order`, `dutch_scs_colour`, `team_scs_order` and `team_scs_colour`: the soft pairs
change which equally good pairing is chosen, and colour preferences still outrank them.

`team_scs_28_teams` and `team_scs_bracket_width`: the soft field makes the team edge weight
wider, and the weight bound must keep room for the matching at every width.
