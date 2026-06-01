#include <algorithm>
#include <cassert>
#include <cstdint>
#include <list>
#include <ostream>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <tournament/tournament.h>
#include <utility/typesizes.h>
#include <utility/uinttypes.h>

#include "common.h"
#include "teampairing.h"

#ifndef OMIT_TEAM
namespace swisssystems
{
  namespace teampairing
  {
    /*
     * Implementation of the FIDE Swiss Team Pairing System (Handbook C.04.6,
     * effective 1 February 2026). The locked, article-by-article reference is
     * docs/team-pairing.md. Article tags ("§1.7.1") and criterion tags ("[C5]")
     * map directly to that document and the handbook.
     *
     * A team is a tournament::Player. Per §1.6.1 a team's colour in a round is
     * its board-1 player's scheduled colour, counted only if the match was
     * actually played; the team-TRF reader populates each team-Player's matches
     * with those board-1 colours and the aggregated primary score
     * (scoreWithoutAcceleration) and secondary score (secondaryScore).
     *
     * Design: unlike the individual Dutch engine (which iterates brackets), the
     * team criteria are all additive / edge-local, and [C6] (next-bracket
     * viability) is automatic when the whole round is paired by one global
     * maximum-weight matching. So this reduces the round-pairing to a single
     * matching over all unpaired teams, with a lexicographically-layered edge
     * weight encoding the quality criteria in priority order. See
     * docs/team-pairing.md §4.
     */
    namespace
    {
      typedef matching_computer::edge_weight edge_weight;
      typedef tournament::player_index score_group_shift;

      /** The configuration carried on the tournament (set by the reader). */
      TeamConfig getConfig(const tournament::Tournament &tournament)
      {
        return tournament.teamConfig;
      }

      tournament::points scoreOf(
        const tournament::Player &team,
        const tournament::Tournament &tournament)
      {
        return team.scoreWithAcceleration(tournament);
      }

      // ----- §1.6 Colour Difference -------------------------------------------

      /**
       * §1.6.2 CD = (matches the team had White) - (matches it had Black),
       * counting only actually-played matches (§1.6.1).
       */
      int colourDifference(const tournament::Player &team)
      {
        int cd = 0;
        for (const tournament::Match &match : team.matches)
        {
          if (!match.gameWasPlayed)
          {
            continue;
          }
          if (match.color == tournament::COLOR_WHITE)
          {
            ++cd;
          }
          else if (match.color == tournament::COLOR_BLACK)
          {
            --cd;
          }
        }
        return cd;
      }

      /**
       * Whether the team had the given colour in each of its last `count`
       * played matches. False if it has played fewer than `count` matches.
       */
      bool lastPlayedColoursAre(
        const tournament::Player &team,
        const tournament::Color colour,
        const unsigned int count)
      {
        unsigned int seen = 0;
        for (
          auto it = team.matches.rbegin();
          it != team.matches.rend() && seen < count;
          ++it)
        {
          if (!it->gameWasPlayed)
          {
            continue;
          }
          if (it->color != colour)
          {
            return false;
          }
          ++seen;
        }
        return seen == count;
      }

      bool hasPlayedAnyMatch(const tournament::Player &team)
      {
        for (const tournament::Match &match : team.matches)
        {
          if (match.gameWasPlayed)
          {
            return true;
          }
        }
        return false;
      }

      /**
       * §1.5 A team floated in the round `roundsBack` before the current one if
       * its opponent there had a different score. Byes count as floats too (an
       * unplayed scoring round behaves like a down/up float for these purposes,
       * matching the individual engine's getFloat). Used by [C7] and [C10].
       */
      bool wasFloater(
        const tournament::Player &team,
        const tournament::round_index roundsBack,
        const tournament::Tournament &tournament)
      {
        if (tournament.playedRounds < roundsBack)
        {
          return false;
        }
        const tournament::Match &match =
          team.matches[tournament.playedRounds - roundsBack];
        if (!match.gameWasPlayed)
        {
          // A scoring unplayed round (e.g. PAB) is treated as a float.
          return tournament.getPoints(team, match) > tournament.pointsForLoss;
        }
        const tournament::points own =
          team.scoreWithAcceleration(tournament, roundsBack);
        const tournament::points opp =
          tournament.players[match.opponent]
            .scoreWithAcceleration(tournament, roundsBack);
        return own != opp;
      }

      // ----- §1.7 Colour Preference -------------------------------------------

      struct Preference
      {
        tournament::Color colour = tournament::COLOR_NONE;
        bool strong = false; // meaningful only for Type B
      };

      /** §1.7.1 Type A (simple) colour preference. */
      Preference typeAPreference(const tournament::Player &team)
      {
        const int cd = colourDifference(team);
        if (
          cd < -1
            || ((cd == 0 || cd == -1)
                  && lastPlayedColoursAre(team, tournament::COLOR_BLACK, 2)))
        {
          return { tournament::COLOR_WHITE, false };
        }
        if (
          cd > 1
            || ((cd == 0 || cd == 1)
                  && lastPlayedColoursAre(team, tournament::COLOR_WHITE, 2)))
        {
          return { tournament::COLOR_BLACK, false };
        }
        return { tournament::COLOR_NONE, false };
      }

      /** §1.7.2 Type B (strong / mild) colour preference. */
      Preference typeBPreference(
        const tournament::Player &team,
        const bool isLastRound)
      {
        const int cd = colourDifference(team);

        if (!hasPlayedAnyMatch(team) || (cd == 0 && isLastRound))
        {
          return { tournament::COLOR_NONE, false };
        }

        if (
          cd < -1
            || ((cd == 0 || cd == -1)
                  && lastPlayedColoursAre(team, tournament::COLOR_BLACK, 2)))
        {
          return { tournament::COLOR_WHITE, true };
        }
        if (
          cd > 1
            || ((cd == 0 || cd == 1)
                  && lastPlayedColoursAre(team, tournament::COLOR_WHITE, 2)))
        {
          return { tournament::COLOR_BLACK, true };
        }

        if (cd == -1
              || (cd == 0
                    && lastPlayedColoursAre(team, tournament::COLOR_BLACK, 1)))
        {
          return { tournament::COLOR_WHITE, false };
        }
        if (cd == 1
              || (cd == 0
                    && lastPlayedColoursAre(team, tournament::COLOR_WHITE, 1)))
        {
          return { tournament::COLOR_BLACK, false };
        }
        return { tournament::COLOR_NONE, false };
      }

      Preference colourPreference(
        const tournament::Player &team,
        const TeamConfig &config,
        const bool isLastRound)
      {
        switch (config.colourType)
        {
        case ColourType::A:
          return typeAPreference(team);
        case ColourType::B:
          return typeBPreference(team, isLastRound);
        default: // ColourType::NONE
          return { tournament::COLOR_NONE, false };
        }
      }

      // ----- Edge weight ------------------------------------------------------

      /**
       * Left-shift edgeWeight by `shift`, growing the representation when sizing
       * (max == true). Mirrors dutch::shiftEdgeWeight.
       */
      template <bool max, typename Shift>
      void shiftEdgeWeight(edge_weight &edgeWeight, const Shift shift)
      {
        if (max)
        {
          edgeWeight.shiftGrow(shift);
        }
        else
        {
          edgeWeight <<= shift;
        }
      }

      /**
       * Compute the edge weight for pairing teams a and b (or, when max==true,
       * an upper bound used to size the dynamic integer). The weight packs the
       * quality criteria as lexicographic fields, most significant first:
       *
       *   completion : prefer matching every team (maximise pairs)
       *   [C4]       : minimise upfloaters  (prefer same-scoregroup pairs)
       *   [C5]       : maximise upfloater scores (prefer the smallest score gap)
       *   [C7]       : minimise upfloaters that floated last round
       *   [C8]       : minimise unfulfilled colour preferences
       *   [C9]       : (Type B) minimise unfulfilled strong colour preferences
       *   [C10]      : minimise upfloaters' opponents that floated last round
       *   identifier : §3.6 lexicographic tie-break (prefer low TPNs)
       *
       * Each count field is `fieldBits` wide so the matching's per-field sums
       * cannot carry into a higher-priority field; the [C5] score field is
       * `scoreGroupsShift` wide for the same reason.
       */
      template <bool max = false>
      edge_weight computeEdgeWeight(
        const tournament::Player &a,
        const tournament::Player &b,
        const tournament::player_index maxRank,
        const TeamConfig &config,
        const bool isLastRound,
        const bool skipFloatHistory,
        const tournament::Tournament &tournament,
        const std::vector<std::unordered_set<tournament::player_index>>
          &forbiddenPairs,
        const unsigned int fieldBits,
        const score_group_shift scoreGroupsShift,
        const std::unordered_map<tournament::points, score_group_shift>
          &scoreGroupShifts,
        const unsigned int topFieldBits,
        const unsigned int prodFieldBits,
        edge_weight &maxEdgeWeight)
      {
        typename
            std::conditional<max, decltype(maxEdgeWeight), edge_weight>::type
          result{ maxEdgeWeight };
        result &= 0u;

        // [C1] absolute: never pair teams that have already met. (No colour
        // absolute criterion exists for teams.)
        if (!max && forbiddenPairs[a.id].count(b.id))
        {
          return result;
        }

        const tournament::points scoreA = scoreOf(a, tournament);
        const tournament::points scoreB = scoreOf(b, tournament);
        const bool sameScoreGroup = scoreA == scoreB;
        const tournament::points upfloaterScore =
          scoreA < scoreB ? scoreA : scoreB;
        const tournament::Player &upfloater = scoreA < scoreB ? a : b;
        const tournament::Player &resident = scoreA < scoreB ? b : a;

        // completion: every legal pair contributes 1 here, so maximising the
        // matched count is the top priority (drives [C3]/§3.3.1).
        result |= max ? 1u : 1u;

        // [C4] minimise upfloaters == maximise same-scoregroup pairs.
        shiftEdgeWeight<max>(result, fieldBits);
        result |= max ? 1u : (sameScoreGroup ? 1u : 0u);

        // [C5] maximise upfloater scores: for a cross-scoregroup pair, add a bit
        // at the upfloater's scoregroup offset, so higher upfloater scores
        // dominate (smaller score gap preferred).
        shiftEdgeWeight<max>(result, scoreGroupsShift);
        if (max)
        {
          // Upper bound: all bits in this field set.
        }
        else if (!sameScoreGroup)
        {
          result +=
            ((result & 0u) | 1u)
              << scoreGroupShifts.find(upfloaterScore)->second;
        }

        // [C7] minimise upfloaters that were floaters the previous round
        // (inactive in the last two rounds). Bonus when the upfloater did NOT
        // float last round (or the pair is intra-scoregroup, i.e. no upfloater).
        shiftEdgeWeight<max>(result, fieldBits);
        result |=
          max
            ? 1u
            : (sameScoreGroup || skipFloatHistory
                  || !wasFloater(upfloater, 1u, tournament))
                ? 1u
                : 0u;

        // [C8] minimise unfulfilled colour preferences. A pair leaves a
        // preference unfulfilled exactly when both teams want the same colour;
        // grant a bonus when their preferences are compatible.
        const Preference prefA = colourPreference(a, config, isLastRound);
        const Preference prefB = colourPreference(b, config, isLastRound);
        shiftEdgeWeight<max>(result, fieldBits);
        result |=
          max
            ? 1u
            : colorPreferencesAreCompatible(prefA.colour, prefB.colour) ? 1u
              : 0u;

        // [C9] (Type B only) minimise unfulfilled strong colour preferences.
        shiftEdgeWeight<max>(result, fieldBits);
        {
          const bool aStrong =
            prefA.strong && prefA.colour != tournament::COLOR_NONE;
          const bool bStrong =
            prefB.strong && prefB.colour != tournament::COLOR_NONE;
          const bool strongConflict =
            config.colourType == ColourType::B
              && aStrong && bStrong
              && prefA.colour == prefB.colour;
          result |= max ? 1u : strongConflict ? 0u : 1u;
        }

        // [C10] minimise upfloaters' opponents that were floaters the previous
        // round (inactive in the last two rounds). The upfloater's opponent is
        // the resident team.
        shiftEdgeWeight<max>(result, fieldBits);
        result |=
          max
            ? 1u
            : (sameScoreGroup || skipFloatHistory
                  || !wasFloater(resident, 1u, tournament))
                ? 1u
                : 0u;

        // §3.6 identifier tie-break, level 1: the lexicographically smallest
        // identifier puts the smallest-TPN teams as pair "tops" (§3.6.1-2; the
        // top member of a pair is the one with the smaller TPN). Encoded as a
        // bitmask: each pair contributes 2^(maxRank-topRank), summed over the
        // distinct tops, so maximising it selects the smallest tops in
        // lexicographic order. TPN == rankIndex + 1, so rankIndex gives the
        // ordering directly.
        const tournament::player_index topRank =
          a.rankIndex < b.rankIndex ? a.rankIndex : b.rankIndex;
        const tournament::player_index botRank =
          a.rankIndex < b.rankIndex ? b.rankIndex : a.rankIndex;
        shiftEdgeWeight<max>(result, topFieldBits);
        if (!max)
        {
          result += ((result & 0u) | 1u) << (maxRank - topRank);
        }

        // level 2: among pairings with the same tops, the smallest identifier
        // pairs sorted tops with sorted bottoms in order (b_1 < b_2 < ...). By
        // the rearrangement inequality this is the assignment maximising
        // Σ topRank·botRank.
        shiftEdgeWeight<max>(result, prodFieldBits);
        if (!max)
        {
          result += topRank * botRank;
        }

        if (max)
        {
          // Leave two bits of headroom for the matching subroutine.
          result.shiftGrow(2u);
          result >>= 1u;
          result -= 1u;
        }
        return result;
      }

      // ----- §4 Colour Allocation ---------------------------------------------

      /**
       * §4.2 first-team: higher primary score; else higher secondary score (if
       * used); else smaller TPN. Returns true if `a` is the first-team.
       */
      bool isFirstTeam(
        const tournament::Player &a,
        const tournament::Player &b,
        const TeamConfig &config,
        const tournament::Tournament &tournament)
      {
        const tournament::points sa = scoreOf(a, tournament);
        const tournament::points sb = scoreOf(b, tournament);
        if (sa != sb)
        {
          return sa > sb; // §4.2.1
        }
        if (config.useSecondaryForColour && a.secondaryScore != b.secondaryScore)
        {
          return a.secondaryScore > b.secondaryScore; // §4.2.2
        }
        return a.rankIndex < b.rankIndex; // §4.2.3 smaller TPN
      }

      tournament::Color lastPlayedColour(const tournament::Player &team)
      {
        for (auto it = team.matches.rbegin(); it != team.matches.rend(); ++it)
        {
          if (it->gameWasPlayed)
          {
            return it->color;
          }
        }
        return tournament::COLOR_NONE;
      }

      /**
       * §4.3 Decide the colour for the first-team of a pair. `first` is the
       * first-team (§4.2); `second` the other team.
       */
      tournament::Color colourForFirstTeam(
        const tournament::Player &first,
        const tournament::Player &second,
        const TeamConfig &config,
        const bool isLastRound,
        const tournament::Tournament &tournament)
      {
        const Preference pf = colourPreference(first, config, isLastRound);
        const Preference ps = colourPreference(second, config, isLastRound);
        const bool firstPlayed = hasPlayedAnyMatch(first);
        const bool secondPlayed = hasPlayedAnyMatch(second);

        // §4.3.1 both teams have yet to play.
        if (!firstPlayed && !secondPlayed)
        {
          const bool firstTpnOdd = ((first.rankIndex + 1u) & 1u) != 0u;
          return
            firstTpnOdd
              ? tournament.initialColor
              : invert(tournament.initialColor);
        }

        const bool fHas = pf.colour != tournament::COLOR_NONE;
        const bool sHas = ps.colour != tournament::COLOR_NONE;

        // §4.3.2 only one team has a preference -> grant it.
        if (fHas && !sHas)
        {
          return pf.colour;
        }
        if (!fHas && sHas)
        {
          return invert(ps.colour);
        }
        // §4.3.3 opposite preferences -> grant both.
        if (fHas && sHas && pf.colour != ps.colour)
        {
          return pf.colour;
        }

        // §4.3.4 (Type B) only one strong preference -> grant it.
        if (config.colourType == ColourType::B)
        {
          const bool fStrong = fHas && pf.strong;
          const bool sStrong = sHas && ps.strong;
          if (fStrong && !sStrong)
          {
            return pf.colour;
          }
          if (!fStrong && sStrong)
          {
            return invert(ps.colour);
          }
        }

        // §4.3.5 give White to the team with the lower colour difference.
        const int cdFirst = colourDifference(first);
        const int cdSecond = colourDifference(second);
        if (cdFirst != cdSecond)
        {
          return cdFirst < cdSecond
            ? tournament::COLOR_WHITE
            : tournament::COLOR_BLACK;
        }

        // §4.3.6 alternate to the most recent round where one had White and the
        // other Black.
        const tournament::Color alternated =
          choosePlayerNeutralColor(first, second);
        if (alternated != tournament::COLOR_NONE)
        {
          return alternated;
        }

        // §4.3.7 grant the first-team's preference.
        if (fHas)
        {
          return pf.colour;
        }

        // §4.3.8 alternate the first-team's colour from its last played round.
        const tournament::Color firstLast = lastPlayedColour(first);
        if (firstLast != tournament::COLOR_NONE)
        {
          return invert(firstLast);
        }

        // §4.3.9 alternate the other team's colour from its last played round.
        const tournament::Color secondLast = lastPlayedColour(second);
        if (secondLast != tournament::COLOR_NONE)
        {
          return secondLast;
        }

        // Fallback: TPN parity against the initial colour (as §4.3.1).
        return ((first.rankIndex + 1u) & 1u)
          ? tournament.initialColor
          : invert(tournament.initialColor);
      }

      // ----- Feasibility (used for §3.4 PAB selection) ------------------------

      /**
       * Whether the given teams admit a complete legal pairing (every team
       * matched, [C1] respected). Uses a plain 0/1 maximum matching.
       */
      bool feasibleComplete(
        const std::vector<const tournament::Player *> &teams,
        const std::vector<std::unordered_set<tournament::player_index>>
          &forbiddenPairs)
      {
        const std::size_t n = teams.size();
        if (n & 1u)
        {
          return false;
        }
        if (n == 0u)
        {
          return true;
        }
        edge_weight maxWeight{ 1u };
        matching_computer computer(n, maxWeight);
        for (std::size_t i = 0; i < n; ++i)
        {
          computer.addVertex();
        }
        for (std::size_t i = 0; i < n; ++i)
        {
          for (std::size_t j = 0; j < i; ++j)
          {
            edge_weight w{ maxWeight };
            w &= 0u;
            if (!forbiddenPairs[teams[i]->id].count(teams[j]->id))
            {
              w |= 1u;
            }
            computer.setEdgeWeight(i, j, w);
          }
        }
        computer.computeMatching();
        const std::vector<tournament::player_index> matching =
          computer.getMatching();
        for (std::size_t i = 0; i < n; ++i)
        {
          if (matching[i] == i)
          {
            return false; // unmatched
          }
        }
        return true;
      }
    }

    /**
     * Compute the team pairings for the next round (C.04.6 §3.3.2).
     */
    std::list<Pairing> computeMatching(
      tournament::Tournament &&tournament,
      std::ostream *const /*checklistStream*/)
    {
      const TeamConfig config = getConfig(tournament);
      const bool isLastRound =
        tournament.playedRounds + 1u >= tournament.expectedRounds;
      // [C7]/[C10] are inactive when pairing the last two rounds.
      const bool skipFloatHistory =
        tournament.playedRounds + 2u >= tournament.expectedRounds;

      // Gather the teams to be paired (valid, not yet paired this round) and the
      // forbidden pairs from earlier meetings (§2.1.1 [C1]).
      std::vector<const tournament::Player *> teams;
      auto forbiddenPairs =
        tournament.resolveForbiddenPairs(tournament.playedRounds);
      for (tournament::Player &team : tournament.players)
      {
        if (!team.isValid)
        {
          continue;
        }
        if (team.matches.size() <= tournament.playedRounds)
        {
          teams.push_back(&team);
        }
        for (const tournament::Match &match : team.matches)
        {
          // Two teams that were paired must not meet again (§2.1.1 [C1]),
          // whether the match was played or decided by forfeit. A real opponent
          // is indicated by match.opponent != team.id (a bye uses the team's own
          // id).
          if (match.opponent != team.id)
          {
            forbiddenPairs[team.id].insert(match.opponent);
          }
        }
      }

      std::list<Pairing> result;
      if (teams.empty())
      {
        return result;
      }

      // Sort by descending primary score, then ascending TPN (§1.2 order).
      std::sort(
        teams.begin(),
        teams.end(),
        [&tournament](
          const tournament::Player *const x,
          const tournament::Player *const y)
        {
          const tournament::points sx = scoreOf(*x, tournament);
          const tournament::points sy = scoreOf(*y, tournament);
          if (sx != sy)
          {
            return sx > sy;
          }
          return x->rankIndex < y->rankIndex;
        });

      // §3.4 Pairing-Allocated-Bye assignment. Among bye-eligible teams ([C2]),
      // in order of lowest score, then most matches played, then largest TPN,
      // choose the first whose removal leaves a complete legal pairing (§3.4.1).
      const tournament::Player *byeTeam = nullptr;
      if (teams.size() & 1u)
      {
        std::vector<const tournament::Player *> candidates;
        for (const tournament::Player *const team : teams)
        {
          if (eligibleForBye(*team, tournament))
          {
            candidates.push_back(team);
          }
        }
        std::sort(
          candidates.begin(),
          candidates.end(),
          [&tournament](
            const tournament::Player *const x,
            const tournament::Player *const y)
          {
            const tournament::points sx = scoreOf(*x, tournament);
            const tournament::points sy = scoreOf(*y, tournament);
            if (sx != sy)
            {
              return sx < sy;                       // §3.4.2 lowest score
            }
            if (x->playedGames != y->playedGames)
            {
              return x->playedGames > y->playedGames; // §3.4.3 most matches
            }
            return x->rankIndex > y->rankIndex;     // §3.4.4 largest TPN
          });

        for (const tournament::Player *const candidate : candidates)
        {
          std::vector<const tournament::Player *> rest;
          rest.reserve(teams.size() - 1u);
          for (const tournament::Player *const team : teams)
          {
            if (team != candidate)
            {
              rest.push_back(team);
            }
          }
          if (feasibleComplete(rest, forbiddenPairs))
          {
            byeTeam = candidate;
            break;
          }
        }

        if (!byeTeam)
        {
          throw NoValidPairingException(
            "No team can receive the pairing-allocated bye while leaving a "
            "legal pairing.");
        }
      }

      // The teams actually paired (even count).
      std::vector<const tournament::Player *> pairTeams;
      pairTeams.reserve(teams.size());
      for (const tournament::Player *const team : teams)
      {
        if (team != byeTeam)
        {
          pairTeams.push_back(team);
        }
      }

      if (byeTeam)
      {
        result.emplace_back(byeTeam->id, byeTeam->id);
      }

      if (pairTeams.empty())
      {
        return result;
      }

      const tournament::player_index teamCount = pairTeams.size();

      // Compute scoregroup bit offsets (for [C5]) and field widths.
      score_group_shift scoreGroupsShift{ };
      std::unordered_map<tournament::points, score_group_shift> scoreGroupShifts;
      tournament::player_index repeated{ };
      for (auto it = pairTeams.rbegin(); it != pairTeams.rend(); )
      {
        const auto current = it++;
        ++repeated;
        const tournament::points score = scoreOf(**current, tournament);
        if (it == pairTeams.rend()
              || score < scoreOf(**it, tournament))
        {
          const unsigned int bits =
            utility::typesizes::bitsToRepresent<unsigned int>(repeated);
          scoreGroupShifts[score] = scoreGroupsShift;
          repeated = 0;
          scoreGroupsShift += bits;
        }
      }

      // Each count field must hold a sum of up to teamCount/2 contributions.
      const unsigned int fieldBits =
        utility::typesizes::bitsToRepresent<unsigned int>(teamCount);
      // The largest TPN-rank among the paired teams, used by the identifier
      // tie-break fields.
      tournament::player_index maxRank{ };
      for (const tournament::Player *const team : pairTeams)
      {
        if (team->rankIndex > maxRank)
        {
          maxRank = team->rankIndex;
        }
      }
      // Level-1 identifier field: a bitmask over tops (one bit per rank up to
      // maxRank).
      const unsigned int topFieldBits = maxRank + 2u;
      // Level-2 identifier field: Σ topRank·botRank, at most
      // (teamCount/2)·maxRank^2.
      const unsigned int prodFieldBits =
        utility::typesizes::bitsToRepresent<unsigned int>(
          teamCount ? teamCount * (maxRank + 1u) * (maxRank + 1u) : 1u);

      // Size the dynamic edge weight.
      edge_weight maxEdgeWeight{ 0u };
      computeEdgeWeight<true>(
        *pairTeams.front(),
        *pairTeams.front(),
        maxRank,
        config,
        isLastRound,
        skipFloatHistory,
        tournament,
        forbiddenPairs,
        fieldBits,
        scoreGroupsShift,
        scoreGroupShifts,
        topFieldBits,
        prodFieldBits,
        maxEdgeWeight);

      matching_computer computer(teamCount, maxEdgeWeight);
      for (tournament::player_index i = 0; i < teamCount; ++i)
      {
        computer.addVertex();
      }
      for (tournament::player_index i = 0; i < teamCount; ++i)
      {
        for (tournament::player_index j = 0; j < i; ++j)
        {
          computer.setEdgeWeight(
            i,
            j,
            computeEdgeWeight(
              *pairTeams[i],
              *pairTeams[j],
              maxRank,
              config,
              isLastRound,
              skipFloatHistory,
              tournament,
              forbiddenPairs,
              fieldBits,
              scoreGroupsShift,
              scoreGroupShifts,
              topFieldBits,
              prodFieldBits,
              maxEdgeWeight));
        }
      }

      computer.computeMatching();
      const std::vector<tournament::player_index> matching =
        computer.getMatching();

      for (tournament::player_index i = 0; i < teamCount; ++i)
      {
        if (matching[i] == i)
        {
          throw NoValidPairingException(
            "The teams could not be simultaneously paired while satisfying the "
            "absolute criteria.");
        }
        if (matching[i] < i)
        {
          continue; // pair already emitted
        }
        const tournament::Player &x = *pairTeams[i];
        const tournament::Player &y = *pairTeams[matching[i]];
        const tournament::Player &first =
          isFirstTeam(x, y, config, tournament) ? x : y;
        const tournament::Player &second = &first == &x ? y : x;
        const tournament::Color firstColour =
          colourForFirstTeam(first, second, config, isLastRound, tournament);
        result.emplace_back(first.id, second.id, firstColour);
      }

      return result;
    }
  }
}
#endif
