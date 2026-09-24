# Team Pairing — Test Coverage Map (C.04.6)

Every team test lives in `test/tests/team_*` and runs `--team` on a hand-built TRF, diffing
the output against a `.output.expected` that was **derived from the C.04.6 spec by hand**,
or, where a test's whole point is a position too large to work through by hand, agreed with an
independent implementation and said so in the test. Each test's `.cpp` carries the derivation. Run with
`make test`. Fixtures use 1-board teams (team ≡ player) for criteria/colour logic and
multi-board teams for aggregation.

## Article / criterion → test

| C.04.6 reference | Test(s) |
| --- | --- |
| §1.2 score; GP as primary (192 `FIDE_TEAM_GP_MP`) | team_gp_primary |
| §1.3 match result by game points, boards forfeited each way included | team_13_board_forfeits |
| §1.3 a match neither team scored in (0-0) is lost by both | team_13_nil_match |
| §1.5 a team that took a bye did not float | team_35_upfloater |
| §1.7.2 a strong preference survives the last round | team_172_typeB_lastround |
| record 162 game points of a declared bye | team_162_bye_gamepoints |
| §1.4 PAB = draw points | team_bye_lowscore, team_c2_doublebye |
| §1.6.1 colour counted only for a played match, from board 1's scheduled colour | team_16_board1_forfeit |
| §1.6 colour difference (board-1 colour) | team_col_lowercd, team_agg_gamepoints, team_42_secondary |
| §1.7.1 Type A absolute (CD>1) | team_col_oppositepref, team_col_onepref, team_col_firstpref |
| §1.7.1 Type A "last two played" trigger | team_col_lasttwo |
| §1.7.2 Type B strong & mild | team_col_typeB_strong |
| §2.1.1 [C1] no rematch | team_round2, team_c4_bracket, team_c2_doublebye |
| §2.1.2 [C2] no second PAB | team_c2_doublebye |
| §2.3.1 [C4] minimise upfloaters (per bracket, §3.5.2) | team_c4_bracket |
| §2.3.2 [C5] maximise the upfloaters' scores | team_pab_gamepoints, team_299_blank, team_gp_primary |
| §3.3.2 bracket by bracket, top scoregroup first | team_35_upfloater, team_c4_bracket |
| §3.5.2/.3/.4 upfloater sets, lexicographic by TPN | team_35_upfloater, team_c2_forfeit |
| §3.5.5 the first legal set complying with [C6]/[C7] | team_35_upfloater |
| §2.3.4 [C7] on the primary score, not on match points | team_35_c7_gamepoints |
| §3.6.2 the bottoms ordered by their tops | team_36_bottom_order |
| C.04.2 §3.5 a rematch after a match nobody played | team_35_rematch |
| §3.4.1 PAB leaves a legal pairing | team_c2_doublebye |
| §3.4.2 PAB lowest score | team_bye_lowscore |
| §3.4.3/.4 PAB most-matches / largest-TPN | team_bye, team_bye_lowscore |
| §3.6 identifier / split-half / monotonic | team_round1, team_split6 |
| §4.1 initial colour (W / B) | (all) / team_col_initialB |
| §4.2.1 first-team by primary score | team_round2, team_c4_bracket |
| §4.2.2 first-team by secondary (game points) | team_42_secondary |
| §4.2.3 first-team by TPN | team_round1, team_split6 |
| §4.3.1 both unplayed → TPN parity | team_round1, team_split6, team_col_initialB |
| §4.3.2 grant the only preference | team_col_onepref, team_col_lasttwo |
| §4.3.3 opposite preferences granted | team_col_oppositepref |
| §4.3.4 (Type B) grant only strong preference | team_col_typeB_strong |
| §4.3.5 lower CD → White | team_col_lowercd, team_c4_bracket |
| §4.3.6 alternate W/B split | team_col_oppositepref |
| §4.3.6 decides only on a round with White against Black, preferences not re-read | team_436_alternation |
| §4.3.7 grant first-team preference | team_col_firstpref, team_42_secondary |
| §4.3.8 alternate first-team last colour | team_round2, team_436_alternation |
| no-colour mode (192 `FIDE_TEAM_MP`) | team_col_lowercd |
| aggregation (boards → match / game points) | team_agg_gamepoints, team_round2 |
| checker (`-c`) re-pairs each round on the primary score | team_checker_gamepoints |

## Exercised but not isolated

`[C6]` (next-scoregroup viability) and `[C10]` (the upfloater's opponent having floated) are
**structural** criteria that run on every multi-scoregroup pairing: §3.5.5 evaluates [C6] for each
candidate set, and the 6-team round-4 tests (team_col_lasttwo, team_col_typeB_strong,
team_gp_primary) produce upfloater pairs whose members floated in earlier rounds, so these code
paths execute and the resulting pairings were verified against the spec. They are hard to make
*decisive* in a small fixture because [C4] and [C5] settle the candidate set first, and they are
inert in the last two rounds — a three-round fixture never weighs them. Isolating them would
require longer multi-round fields with several equally-good legal sets differing only in float
history. Not yet built as dedicated decisive tests.

`[C7]` is now decisive in team_35_c7_gamepoints, where the bracket takes the team that did not
float over the one with the smaller pairing number.

## Not independently observable

- **§4.3.9** (alternate the *other* team's last colour) is the last fallback. It is reached
  only when the first-team has **no** played game while the other team does **and** both have
  equal colour difference — but a team with zero played games has only ever had byes, so its
  CD is 0 while the played opponent's CD is ±1, which §4.3.5 already resolves. So §4.3.9 is
  effectively unreachable in legal play.
- **§1.7.2 "no preference when CD = 0 on the last round"**: in the cases where it would apply,
  the alternative (a mild preference) points the same way as the §4.3.5 lower-CD / §4.3.8
  alternation fallback, so the colour assigned is identical either way — the clause is not
  behaviourally distinguishable through the pairing output.
