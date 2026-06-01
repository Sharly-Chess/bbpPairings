# Team Swiss Pairing — Design & Locked Spec

Source of truth: FIDE Handbook **C.04.6 Swiss Team Pairing System** (effective 1 Feb 2026),
plus TRF-2026 records and the Tournament-Type Code Table (192).

This document is the locked reference for the implementation. Article numbers below are
verbatim from C.04.6. Criteria are `[C1]`..`[C10]`.

---

## 1. Relationship to the individual Dutch engine

A **team is a pairing node** and reuses `tournament::Player`. The matching engine
(`matching/detail`, Edmonds blossom) and most of `swisssystems/common` are reused.

Key structural differences from C.04.3 (Dutch, individuals):

| Aspect | Dutch (C.04.3) | Team (C.04.6) |
| --- | --- | --- |
| Node | player | team |
| Score | one | primary (MP or GP) + secondary (for colour) |
| PAB value | win | **draw** (MP and GP) |
| Absolute criteria | C1, C2, C3 (colour clash) | **C1, C2 only** (no colour clash) |
| Basic-rules 6 & 7 (colour ±2, no 3-in-row) | apply | **never apply** |
| Quality criteria | C6..C21 (downfloater-centric) | C4..C10 (upfloater-centric) |
| Colour preference | absolute / strong / mild | none / Type A / Type B (never absolute) |
| Topscorers | special colour handling | none |
| Bracket pairing | edge-weight max-weight matching | enumerate → lexicographic sort → first compliant (§3.6) |
| Float direction | downfloaters pushed down | upfloaters pulled up (§3.5) |

---

## 2. Definitions (C.04.6 §1)

- **§1.1 TPN** — each team has a unique Tournament Pairing Number `1..numTeams`,
  assigned externally (competition rules / Chief Arbiter). Engine trusts it; maps to the
  existing pairing-number / `Player::id`. Initial-order rules (GHR 2.1–2.3) deliberately omitted.
- **§1.2 Score** — competition designates **primary** = "match points" or "game points";
  the other is the **secondary** score, optionally used for colour allocation (§4.2.2).
  Default: primary = MP, secondary = GP (used for colour).
- **§1.4 PAB** — odd team count → one team gets PAB: no opponent, no colour, **draw** MP and
  draw GP (unless regs say otherwise; same value for all PABs).
- **§1.5 Floater** — a team that plays an opponent with a **different score**.
- **§1.6 Colour Difference (CD)**
  - §1.6.1 A team "had" a colour in a match iff the match was **actually played** and the
    **board-1 player** was scheduled that colour.
  - §1.6.2 `CD = (#matches had White) − (#matches had Black)`.
- **§1.7 Colour Preference** — Type A by default; Type B or "no preference" per regs.
  - **§1.7.1 Type A (simple):**
    - White if `CD < -1`, OR (`CD ∈ {0,-1}` AND Black in the last two **played** matches).
    - Black if `CD > +1`, OR (`CD ∈ {0,+1}` AND White in the last two **played** matches).
    - else none.
  - **§1.7.2 Type B (strong / mild):**
    - strong White: `CD < -1`, OR (`CD ∈ {0,-1}` AND Black last two played).
    - strong Black: `CD > +1`, OR (`CD ∈ {0,+1}` AND White last two played).
    - mild White: `CD = -1`, OR (`CD = 0` AND not last round AND Black last played match).
    - mild Black: `CD = +1`, OR (`CD = 0` AND not last round AND White last played match).
    - none: no match played yet, OR `CD = 0` when pairing the last round.
  - **No-preference mode:** every team has no preference; only §4.3.1/4.3.5–4.3.9 apply.

---

## 3. Criteria (C.04.6 §2)

**Absolute (§2.1)** — no pairing may violate:
- `[C1]` (§2.1.1) Two teams shall not play each other more than once.
- `[C2]` (§2.1.2) A team that already received a PAB, won a match by forfeit, or got a
  FIDE-deprecated full-point bye, shall not receive the PAB.

**Completion (§2.2)**
- `[C3]` (§2.2.1) A legal pairing (complying with absolute criteria) shall always exist for
  all teams not yet paired.

**Quality (§2.3)** — descending priority, "comply as much as possible":
- `[C4]` (§2.3.1) Minimise the number of upfloaters.
- `[C5]` (§2.3.2) Minimise the score differences (descending order) in pairs involving
  upfloaters, i.e. maximise upfloater scores (ascending order).
- `[C6]` (§2.3.3) Unless the following scoregroup is now empty, choose upfloaters so that
  `[C1]`, `[C3]`, `[C4]` are satisfiable in the bracket where that scoregroup is paired.
- `[C7]` (§2.3.4) Except in the last two rounds, minimise the number of upfloaters that were
  floaters in the previous round.
- `[C8]` (§2.3.5) Minimise the number of teams whose colour preference (if any) is unfulfilled.
- `[C9]` (§2.3.6) (Type B only) Minimise teams whose **strong** colour preference is unfulfilled.
- `[C10]` (§2.3.7) Except in the last two rounds, minimise the number of upfloaters'
  **opponents** that were floaters in the previous round.

---

## 4. Pairing process (C.04.6 §3)

- **§3.1 Legal pairing** = `[C1]` ∧ `[C2]`; during pairing also `[C3]`.
- **§3.2 Top-scoregroup** = highest-score teams among those not yet paired.
- **§3.3.1 Complete** when all teams paired except at most one (PAB) and `[C1]`,`[C2]` hold.
- **§3.3.2 Steps:**
  1. Assign PAB if needed (§3.4).
  2. Combine top-scoregroup with a set of upfloaters (§3.5) → bracket.
  3. Pair the bracket (§3.6).
  4. Repeat until complete.
  5. Assign colours (§4).
- **§3.3.3** If a round-pairing is impossible, the Chief Arbiter decides
  (engine → `NoValidPairingException`).

**§3.4 PAB assignment** — among teams, prefer the one that (first applicable):
1. §3.4.1 leaves a legal pairing for all teams;
2. §3.4.2 has the lowest score;
3. §3.4.3 has played the highest number of matches;
4. §3.4.4 has the largest TPN.

**§3.5 Upfloater selection**
- §3.5.1 All teams with a lower score than the resident top-scoregroup teams are potential upfloaters.
- §3.5.2 Consider all sets complying with `[C4]` and `[C5]`.
- §3.5.3 Within a set, sort upfloaters by descending score, then ascending TPN.
- §3.5.4 Sort sets among themselves by lexicographic order of their TPNs.
- §3.5.5 Choose the **first** set that (with the top-scoregroup) gives a legal pairing also
  complying with `[C6]` and `[C7]`.

**§3.6 Bracket pairing**
- §3.6.1 A pairing covers all bracket teams; in each pair the **smaller TPN is the top member**,
  the larger TPN the bottom member.
- §3.6.2 Identifier = top-member TPNs (ascending) followed by the corresponding bottom-member TPNs.
- §3.6.3 Pairings sorted by lexicographic order of identifier.
- §3.6.4 Choose the **first** pairing also complying with `[C1]`, `[C8]`, `[C9]`, `[C10]`.

> **Implementation note.** §3.6 is defined as enumerate-sort-pick, not max-weight matching.
> Two implementation options: (a) reuse the blossom matcher with lexicographic edge weights
> (hard `[C1]`; maximise `[C8]`/`[C9]`/`[C10]` compliance; then prefer lexicographically-smallest
> identifier via decreasing-significance per-edge weights — the same technique Dutch uses to
> encode JaVaFo's sequential order); (b) direct enumeration of bracket perfect matchings
> (brackets are small). Default to (a) for robustness on large scoregroups; fall back to (b) if
> the weight encoding for the lexicographic tiebreak proves awkward.

---

## 5. Colour allocation (C.04.6 §4)

- **§4.1 initial-colour** — drawn by lots before round 1 (engine: supplied / inferred, as today).
- **§4.2 first-team** (first applicable): §4.2.1 higher primary score; §4.2.2 higher secondary
  score (unless regs say not to use it); §4.2.3 smaller TPN.
- **§4.3 per pair**, descending priority:
  1. §4.3.1 both teams yet to play: first-team odd TPN → initial-colour; else opposite.
  2. §4.3.2 only one team has a preference → grant it.
  3. §4.3.3 opposite preferences → grant both.
  4. §4.3.4 (Type B only) only one **strong** preference → grant it.
  5. §4.3.5 lower CD → White. (Note: −2 < −1; +1 < +2.)
  6. §4.3.6 alternate to the most recent time one team had White and the other Black.
  7. §4.3.7 grant the first-team's preference.
  8. §4.3.8 alternate the first-team's colour from its last played round.
  9. §4.3.9 alternate the other team's colour from its last played round.

---

## 6. TRF-2026 records (team mode)

| Code | Use |
| --- | --- |
| `001` | member players + games (reuse existing reader; aggregate into teams) |
| `310` (new) / `013` (legacy) | team roster: TPN, name, nickname, board members (StartingRank ids) |
| `192` | team config — see §7 |
| `162` | game-point system (individual games = team game points) |
| `352` | board colour sequence + implicit board count |
| `362` | match-point values TW/TD/TL — default **2.0 / 1.0 / 0.0**, overridable |
| `320` | team PAB match/game points + which team got the PAB each round |
| `299` | abnormal team assignment points — **deferred** (rare) |
| `300`/`320`(game)/`330`/`801`/`802` | not parsed (alternatives to `001`) |

**Aggregation (member tournament → team tournament), per round per team:**
1. game points = Σ member board results (1/=/0, W/D/L unrated, forfeits ±).
2. team colour = board-1 player's scheduled colour (only if the match was played; §1.6.1).
3. opponent team = team of any member's individual opponent.
4. match points = compare team GP vs opponent GP → W/D/L → TW/TD/TL (§362).
5. record opponent (forbidden pair), colour history, both scores; mark PAB/forfeit/bye for `[C2]`.

---

## 7. 192 tournament-type codes → config

`TeamConfig { colourType ∈ {None, A, B}; primaryScore ∈ {MP, GP}; secondaryForColour ∈ {GP, MP, none}; baku }`

| 192 code | colourType | primary | secondary (colour) |
| --- | --- | --- | --- |
| `FIDE_TEAM_TYPEA_MP_GP` | A | MP | GP |
| `FIDE_TEAM_TYPEA_GP_MP` | A | GP | MP |
| `FIDE_TEAM_TYPEA_MP` | A | MP | none |
| `FIDE_TEAM_TYPEA_GP` | A | GP | none |
| `FIDE_TEAM_TYPEB_MP_GP` / `_GP_MP` / `_MP` / `_GP` | B | … | … |
| `FIDE_TEAM_MP_GP` / `_GP_MP` / `_MP` / `_GP` | None | … | … |
| `FIDE_TEAM` | → `FIDE_TEAM_TYPEA_MP_GP` | | |
| `*_BAKU` variants | + Baku acceleration | | |
| `CUSTOM_TEAM_SWISS[_MP|_GP]` | (treated as MP/GP primary, config via flag) | | |

A CLI flag may override the colour type. 192 is the authoritative source.

---

## 8. Output

`-p` team mode emits a **team pairing list**: each pair as (white TPN, black TPN) plus the PAB
team. Board-level expansion is the caller's responsibility. (Proper `801`/`802` writing deferred.)

---

## 9. Merge-safety

- New files only for logic: `swisssystems/teampairing.{cpp,h}`, `fileformats/teamtrf.{cpp,h}`,
  this doc.
- Hot-file edits are small, localized, and guarded by `#ifndef OMIT_TEAM`:
  `tournament.h` (enum `TEAM`, `Player::secondaryScore`, `TeamConfig` on `Tournament`),
  `swisssystems/common.{h,cpp}` (`getInfo`), `main.cpp` (CLI flag + team read path),
  `fileformats/trf.cpp` (route `310`/`013` to roster capture instead of throwing),
  `Makefile` + `test/test-includes.h`.
