#include <algorithm>
#include <cassert>
#include <cstdint>
#include <deque>
#include <list>
#include <ostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <tournament/tournament.h>
#include <utility/typesizes.h>
#include <utility/uintstringconversion.h>
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
     * Design: the round is paired bracket by bracket, as §3.3.2 sets it out.
     * Each bracket is the top scoregroup of what is left plus the set of
     * upfloaters §3.5 selects for it, and the two stages have orders of their
     * own that do not reduce to one another. §3.5 ranks the *sets*: [C4] and
     * [C5] as outer loops, then the lexicographic order of their TPNs (§3.5.4),
     * with legality, [C6] and [C7] deciding which is taken (§3.5.5). §3.6 then
     * ranks the *pairings* of the bracket so settled, under [C1], [C8], [C9],
     * [C10] and the identifier of §3.6.2. Only the second of those is edge-local
     * enough for a matching, which is what pairBracket uses; the first is an
     * enumeration, because the set that wins it can lose on every criterion the
     * matching knows about. See docs/team-pairing.md §4.
     */
    namespace
    {
      typedef matching_computer::edge_weight edge_weight;

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
       * A team's primary score as it stood `roundsBack` rounds before the end of
       * what has been played. The rounds are summed from what each contributed,
       * because the total is in the competition's primary basis while getPoints
       * answers in match points: subtracting one from the other would only
       * agree when match points are the primary score.
       */
      tournament::points scoreBefore(
        const tournament::Player &team,
        const tournament::round_index roundsBack,
        const tournament::Tournament &tournament)
      {
        if (team.primaryScoreByRound.empty())
        {
          return team.scoreWithAcceleration(tournament, roundsBack);
        }
        tournament::points total{ };
        for (
          tournament::round_index r = 0;
          r + roundsBack < tournament.playedRounds
            && r < team.primaryScoreByRound.size();
          ++r)
        {
          total += team.primaryScoreByRound[r];
        }
        return total;
      }

      /**
       * §1.5 A team floated in the round `roundsBack` before the current one if
       * it played there against an opponent with a different score. A team that
       * took a bye, or whose match was decided by forfeit, played no opponent
       * and so did not float, however the round scored. Used by [C7] and [C10].
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
          return false;
        }
        const tournament::points own = scoreBefore(team, roundsBack, tournament);
        const tournament::points opp =
          scoreBefore(tournament.players[match.opponent], roundsBack, tournament);
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

        if (!hasPlayedAnyMatch(team))
        {
          return { tournament::COLOR_NONE, false };
        }

        // §1.7.2's first two paragraphs, the strong preferences: worded as the
        // Type A ones are, and not qualified by the round. What the fifth
        // paragraph takes away in the last round is the mild preference a
        // colour difference of zero would otherwise give, which is why the
        // third and fourth paragraphs state that condition themselves.
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
                    && !isLastRound
                    && lastPlayedColoursAre(team, tournament::COLOR_BLACK, 1)))
        {
          return { tournament::COLOR_WHITE, false };
        }
        if (cd == 1
              || (cd == 0
                    && !isLastRound
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
       * Compute the edge weight for pairing teams a and b within one bracket
       * (or, when max==true, an upper bound used to size the dynamic integer).
       * §3.6.4 pairs a bracket under [C1], [C8], [C9], [C10] and the identifier
       * of §3.6.2, and those are the only criteria here: the bracket's own
       * membership is already settled by §3.5, which is what [C4], [C5], [C6]
       * and [C7] govern. The weight packs them as lexicographic fields, most
       * significant first:
       *
       *   completion : prefer matching every team (maximise pairs)
       *   [C8]       : minimise unfulfilled colour preferences
       *   [C9]       : (Type B) minimise unfulfilled strong colour preferences
       *   [C10]      : minimise upfloaters' opponents that floated last round
       *   identifier : §3.6 lexicographic tie-break (prefer low TPNs)
       *
       * Each count field is `fieldBits` wide so the matching's per-field sums
       * cannot carry into a higher-priority field.
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
        const unsigned int topFieldBits,
        const unsigned int bottomFieldBits,
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
        const tournament::Player &resident = scoreA < scoreB ? b : a;

        // completion: every legal pair contributes 1 here, so maximising the
        // matched count is the top priority (drives [C3]/§3.3.1).
        result |= max ? 1u : 1u;

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

        // level 2: among the pairings that share those tops, §3.6.2 compares the
        // bottom members in the order of their tops, so the bottom of the
        // smallest top decides first. One field per top rank holds that pair's
        // bottom, counted down from maxRank so that maximising the weight
        // minimises the rank, and the fields run from the smallest top rank
        // (most significant) to the largest. A rank that is nobody's top leaves
        // its field empty in every candidate with these tops, so it cannot
        // affect the comparison.
        //
        // Summing topRank·botRank instead only finds the assignment that sorts
        // tops against sorted bottoms, which is the answer when nothing rules
        // it out and not otherwise.
        for (
          tournament::player_index rank = 0;
          rank <= maxRank;
          ++rank)
        {
          shiftEdgeWeight<max>(result, bottomFieldBits);
          if (!max && rank == topRank)
          {
            result += maxRank - botRank;
          }
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
       * The secondary score as it stood before the round being paired. The
       * total the file gives is the whole tournament's, which is what the
       * pairing of the next round wants; the checker re-pairs earlier rounds
       * and must see only what had been scored by then.
       */
      tournament::points secondaryScoreSoFar(
        const tournament::Player &team,
        const tournament::Tournament &tournament)
      {
        if (team.secondaryScoreByRound.empty())
        {
          return team.secondaryScore;
        }
        tournament::points total{ };
        for (
          tournament::round_index r = 0;
          r < tournament.playedRounds && r < team.secondaryScoreByRound.size();
          ++r)
        {
          total += team.secondaryScoreByRound[r];
        }
        return total;
      }

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
        if (config.useSecondaryForColour)
        {
          const tournament::points sa2 = secondaryScoreSoFar(a, tournament);
          const tournament::points sb2 = secondaryScoreSoFar(b, tournament);
          if (sa2 != sb2)
          {
            return sa2 > sb2; // §4.2.2
          }
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
          // A team keeps its TPN whoever is absent (art. 1.1): the team index
          // is the TPN order, the rank index only the order among those present.
          const bool firstTpnOdd = ((first.id + 1u) & 1u) != 0u;
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
        // other Black. The preferences have had their say above.
        tournament::Color firstThen;
        tournament::Color secondThen;
        findFirstColorDifference(first, second, firstThen, secondThen);
        if (firstThen != tournament::COLOR_NONE
              && secondThen != tournament::COLOR_NONE)
        {
          return secondThen;
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
        return ((first.id + 1u) & 1u)
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

      // ----- §3.5 Upfloater Selection, §3.6 Bracket Pairing -------------------

      typedef std::vector<const tournament::Player *> TeamList;
      typedef
        std::vector<
          std::pair<const tournament::Player *, const tournament::Player *>>
        PairList;

      /** §1.2 order: descending score, then ascending TPN. */
      void sortTeams(
        TeamList &teams,
        const tournament::Tournament &tournament)
      {
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
      }

      /**
       * §3.6: pair one bracket. The identifier fields of the edge weight make
       * the maximum-weight matching the first pairing in the order of §3.6.2-3
       * that complies with [C1], [C8], [C9] and [C10]. Returns false when the
       * bracket admits no pairing covering all of its teams (§3.6.1), which is
       * what makes §3.5.5 move on to the next set of upfloaters.
       *
       * `bracket` must be in §1.2 order.
       */
      bool pairBracket(
        const TeamList &bracket,
        const TeamConfig &config,
        const bool isLastRound,
        const bool skipFloatHistory,
        const tournament::Tournament &tournament,
        const std::vector<std::unordered_set<tournament::player_index>>
          &forbiddenPairs,
        PairList *const out)
      {
        if (out)
        {
          out->clear();
        }
        const tournament::player_index teamCount = bracket.size();
        if (teamCount & 1u)
        {
          return false;
        }
        if (!teamCount)
        {
          return true;
        }

        const unsigned int fieldBits =
          utility::typesizes::bitsToRepresent<unsigned int>(teamCount);
        tournament::player_index maxRank{ };
        for (const tournament::Player *const team : bracket)
        {
          if (team->rankIndex > maxRank)
          {
            maxRank = team->rankIndex;
          }
        }
        // Level-1 identifier field: a bitmask over the pair tops, one bit per
        // rank up to maxRank.
        const unsigned int topFieldBits = maxRank + 2u;
        // Level-2 identifier fields: one per top rank, each holding that
        // pair's bottom counted down from maxRank. Only one pair writes into
        // any of them, so a rank's own width is all each needs.
        const unsigned int bottomFieldBits =
          utility::typesizes::bitsToRepresent<unsigned int>(maxRank + 1u);

        edge_weight maxEdgeWeight{ 0u };
        computeEdgeWeight<true>(
          *bracket.front(),
          *bracket.front(),
          maxRank,
          config,
          isLastRound,
          skipFloatHistory,
          tournament,
          forbiddenPairs,
          fieldBits,
          topFieldBits,
          bottomFieldBits,
          maxEdgeWeight);

        matching_computer computer(teamCount, maxEdgeWeight);
        for (tournament::player_index vertex = 0; vertex < teamCount; ++vertex)
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
                *bracket[i],
                *bracket[j],
                maxRank,
                config,
                isLastRound,
                skipFloatHistory,
                tournament,
                forbiddenPairs,
                fieldBits,
                topFieldBits,
                bottomFieldBits,
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
            return false;
          }
          if (matching[i] < i)
          {
            continue; // pair already emitted
          }
          // A pair the teams have already played weighs nothing, so the
          // matching gains as much by leaving both unmatched; make sure one was
          // not taken anyway.
          if (forbiddenPairs[bracket[i]->id].count(bracket[matching[i]]->id))
          {
            return false;
          }
          if (out)
          {
            out->emplace_back(bracket[i], bracket[matching[i]]);
          }
        }
        return true;
      }

      /**
       * [C7] §2.3.4: the number of upfloaters that were floaters in the
       * previous round, nil when pairing the last two rounds.
       */
      std::size_t countC7(
        const TeamList &upfloaters,
        const bool skipFloatHistory,
        const tournament::Tournament &tournament)
      {
        if (skipFloatHistory)
        {
          return 0u;
        }
        std::size_t count{ };
        for (const tournament::Player *const team : upfloaters)
        {
          if (wasFloater(*team, 1u, tournament))
          {
            ++count;
          }
        }
        return count;
      }

      /**
       * [C6] §2.3.3: unless the following scoregroup is now empty — every one of
       * its teams having been taken as an upfloater — what is left of it must
       * still be pairable ([C1], [C3]) with the fewest upfloaters of its own
       * ([C4]), which is none when it is even and one when it is odd. Only that
       * one scoregroup is involved, whatever lower group the upfloaters came
       * from.
       */
      bool checkC6(
        const TeamList &rest,
        const tournament::points followingScore,
        const bool hasFollowing,
        const tournament::Tournament &tournament,
        const std::vector<std::unordered_set<tournament::player_index>>
          &forbiddenPairs)
      {
        if (!hasFollowing)
        {
          return true;
        }
        TeamList following;
        TeamList below;
        for (const tournament::Player *const team : rest)
        {
          if (scoreOf(*team, tournament) == followingScore)
          {
            following.push_back(team);
          }
          else if (scoreOf(*team, tournament) < followingScore)
          {
            below.push_back(team);
          }
        }
        if (following.empty())
        {
          return true; // §2.3.3, the scoregroup is now empty
        }
        if (!(following.size() & 1u))
        {
          return feasibleComplete(following, forbiddenPairs)
            && feasibleComplete(below, forbiddenPairs);
        }
        for (std::size_t taken = 0; taken < below.size(); ++taken)
        {
          TeamList bracket = following;
          bracket.push_back(below[taken]);
          if (!feasibleComplete(bracket, forbiddenPairs))
          {
            continue;
          }
          TeamList remainder;
          remainder.reserve(below.size() - 1u);
          for (std::size_t index = 0; index < below.size(); ++index)
          {
            if (index != taken)
            {
              remainder.push_back(below[index]);
            }
          }
          if (feasibleComplete(remainder, forbiddenPairs))
          {
            return true;
          }
        }
        return false;
      }

      /** Every choice of `k` of `n` indices, in lexicographic order. */
      void indexCombinations(
        const std::size_t n,
        const std::size_t k,
        std::vector<std::vector<std::size_t>> &combos)
      {
        if (k > n)
        {
          return;
        }
        std::vector<std::size_t> indices(k);
        for (std::size_t index = 0; index < k; ++index)
        {
          indices[index] = index;
        }
        for (;;)
        {
          combos.push_back(indices);
          if (!k)
          {
            return;
          }
          std::size_t position = k;
          for (;;)
          {
            --position;
            if (indices[position] != position + n - k)
            {
              ++indices[position];
              for (std::size_t next = position + 1u; next < k; ++next)
              {
                indices[next] = indices[next - 1u] + 1u;
              }
              break;
            }
            if (!position)
            {
              return;
            }
          }
        }
      }

      /** The profiles of `numup` upfloaters, each a descending score list. */
      void buildProfiles(
        const std::vector<tournament::points> &scores,
        const std::vector<std::size_t> &counts,
        const std::size_t numup,
        const std::size_t from,
        std::vector<tournament::points> &current,
        std::vector<std::vector<tournament::points>> &profiles)
      {
        if (current.size() == numup)
        {
          profiles.push_back(current);
          return;
        }
        for (std::size_t level = from; level < scores.size(); ++level)
        {
          for (
            std::size_t take = 1u;
            take <= counts[level] && current.size() + take <= numup;
            ++take)
          {
            current.insert(current.end(), take, scores[level]);
            buildProfiles(
              scores, counts, numup, level + 1u, current, profiles);
            current.resize(current.size() - take);
          }
        }
      }

      /**
       * §3.5.2 with [C5] (§2.3.2): the score profiles of a set of `numup`
       * upfloaters, best first. [C5] minimises the score differences taken in
       * descending order, which is to maximise the upfloaters' scores taken in
       * ascending order, so the profiles compare as their ascending score lists
       * and the largest wins.
       */
      std::vector<std::vector<tournament::points>> upfloaterProfiles(
        const TeamList &lower,
        const std::size_t numup,
        const tournament::Tournament &tournament)
      {
        std::vector<tournament::points> scores;
        std::vector<std::size_t> counts;
        for (const tournament::Player *const team : lower)
        {
          const tournament::points score = scoreOf(*team, tournament);
          if (scores.empty() || score != scores.back())
          {
            scores.push_back(score);
            counts.push_back(1u);
          }
          else
          {
            ++counts.back();
          }
        }
        std::vector<std::vector<tournament::points>> profiles;
        std::vector<tournament::points> current;
        buildProfiles(scores, counts, numup, 0u, current, profiles);
        std::sort(
          profiles.begin(),
          profiles.end(),
          [](
            const std::vector<tournament::points> &x,
            const std::vector<tournament::points> &y)
          {
            const std::vector<tournament::points> ascendingX(
              x.rbegin(), x.rend());
            const std::vector<tournament::points> ascendingY(
              y.rbegin(), y.rend());
            return ascendingX > ascendingY;
          });
        return profiles;
      }

      /**
       * Every set of upfloaters with the given score profile, in the order of
       * §3.5.3 (within a set: descending score, then ascending TPN) and §3.5.4
       * (among the sets: the lexicographic order of their TPNs).
       */
      std::vector<TeamList> upfloaterSets(
        const TeamList &lower,
        const std::vector<tournament::points> &profile,
        const tournament::Tournament &tournament)
      {
        std::vector<TeamList> sets{ TeamList{} };
        std::size_t start = 0;
        while (start < profile.size())
        {
          std::size_t end = start;
          while (end < profile.size() && profile[end] == profile[start])
          {
            ++end;
          }
          TeamList group;
          for (const tournament::Player *const team : lower)
          {
            if (scoreOf(*team, tournament) == profile[start])
            {
              group.push_back(team);
            }
          }
          std::vector<std::vector<std::size_t>> combos;
          indexCombinations(group.size(), end - start, combos);
          std::vector<TeamList> extended;
          for (const TeamList &prefix : sets)
          {
            for (const std::vector<std::size_t> &combo : combos)
            {
              TeamList grown = prefix;
              for (const std::size_t index : combo)
              {
                grown.push_back(group[index]);
              }
              extended.push_back(std::move(grown));
            }
          }
          sets = std::move(extended);
          start = end;
        }
        std::sort(
          sets.begin(),
          sets.end(),
          [](const TeamList &x, const TeamList &y)
          {
            for (std::size_t index = 0; index < x.size() && index < y.size();
                 ++index)
            {
              if (x[index]->rankIndex != y[index]->rankIndex)
              {
                return x[index]->rankIndex < y[index]->rankIndex;
              }
            }
            return x.size() < y.size();
          });
        return sets;
      }

      /**
       * §3.5: choose the upfloaters that join `residents`, and pair the bracket
       * they form. [C4] (§2.3.1, the fewest upfloaters) and [C5] are the outer
       * loops: the number grows from the fewest the parity of the residents
       * allows, and for each number the score profiles are tried best first. A
       * number or a profile no set of which gives a legal pairing is no
       * candidate at all, since [C3] (§2.2.1) asks that all the teams not yet
       * paired can be, so the search falls through to the next one.
       *
       * Within one profile the sets come in the order of §3.5.4, and §3.5.5
       * takes the first that is legal and complies with [C6] and [C7] — read,
       * as §2.3 asks ("comply as much as possible ... in descending priority"),
       * as the smallest value of [C7] a legal set of this profile can reach.
       *
       * `remaining` is the residents and everything still to be paired below
       * them, in §1.2 order.
       */
      bool selectUpfloaters(
        const TeamList &residents,
        const TeamList &remaining,
        const TeamConfig &config,
        const bool isLastRound,
        const bool skipFloatHistory,
        const tournament::Tournament &tournament,
        const std::vector<std::unordered_set<tournament::player_index>>
          &forbiddenPairs,
        TeamList *const chosen,
        PairList *const pairs)
      {
        const tournament::points residentScore =
          scoreOf(*residents.front(), tournament);
        TeamList lower;
        for (const tournament::Player *const team : remaining)
        {
          if (scoreOf(*team, tournament) < residentScore)
          {
            lower.push_back(team); // §3.5.1
          }
        }
        const bool hasFollowing = !lower.empty();
        const tournament::points followingScore =
          hasFollowing ? scoreOf(*lower.front(), tournament) : residentScore;

        for (
          std::size_t numup = residents.size() & 1u;
          numup <= lower.size();
          numup += 2u)
        {
          for (
            const std::vector<tournament::points> &profile :
              upfloaterProfiles(lower, numup, tournament))
          {
            bool found = false;
            std::size_t bestC6{ };
            std::size_t bestC7{ };
            for (
              const TeamList &set : upfloaterSets(lower, profile, tournament))
            {
              TeamList bracket = residents;
              bracket.insert(bracket.end(), set.begin(), set.end());
              sortTeams(bracket, tournament);
              PairList bracketPairs;
              if (
                !pairBracket(
                  bracket,
                  config,
                  isLastRound,
                  skipFloatHistory,
                  tournament,
                  forbiddenPairs,
                  &bracketPairs))
              {
                continue;
              }
              std::unordered_set<const tournament::Player *> inBracket(
                bracket.begin(), bracket.end());
              TeamList rest;
              for (const tournament::Player *const team : remaining)
              {
                if (!inBracket.count(team))
                {
                  rest.push_back(team);
                }
              }
              if (!feasibleComplete(rest, forbiddenPairs))
              {
                continue; // [C3] §2.2.1
              }
              const std::size_t c6 =
                checkC6(
                  rest,
                  followingScore,
                  hasFollowing,
                  tournament,
                  forbiddenPairs)
                  ? 0u
                  : 1u;
              const std::size_t c7 =
                countC7(set, skipFloatHistory, tournament);
              if (!found || c6 < bestC6 || (c6 == bestC6 && c7 < bestC7))
              {
                found = true;
                bestC6 = c6;
                bestC7 = c7;
                *chosen = set;
                *pairs = bracketPairs;
              }
              if (!bestC6 && !bestC7)
              {
                break; // §3.5.5, the first such set
              }
            }
            if (found)
            {
              return true;
            }
          }
        }
        return false;
      }

      /**
       * Print the checklist for the round being paired: one row per team, in
       * the order the pairing considered them. The shared printer supplies
       * the ID, score, colour history and colour preference columns; the
       * specialty columns carry what belongs to the team system alone — the
       * secondary score colour allocation reads (§4.2.2), bye eligibility
       * ([C2]) and the match the team was given.
       *
       * *pairings* is null when no legal pairing was found, so the rows show
       * the inputs the search worked from without a resulting match.
       */
      void printTeamChecklist(
        std::ostream &ostream,
        const tournament::Tournament &tournament,
        const std::vector<const tournament::Player *> &orderedTeams,
        const TeamConfig &config,
        const std::list<Pairing> *const pairings)
      {
        std::unordered_map<tournament::player_index, std::string> assignment;
        if (pairings)
        {
          for (const Pairing &pairing : *pairings)
          {
            // A bye is emitted as a team paired with itself.
            if (pairing.white == pairing.black)
            {
              assignment[pairing.white] = "(bye)";
              continue;
            }
            assignment[pairing.white] =
              '('
                + utility::uintstringconversion::toString(pairing.black + 1u)
                + "W)";
            assignment[pairing.black] =
              '('
                + utility::uintstringconversion::toString(pairing.white + 1u)
                + "B)";
          }
        }
        swisssystems::printChecklist(
          ostream,
          std::deque<std::string>{"2nd", "Bye", "Cur"},
          [&assignment, &config, &tournament]
            (const tournament::Player &team)
          {
            const auto assigned = assignment.find(team.id);
            return std::deque<std::string>{
              config.useSecondaryForColour
                ? utility::uintstringconversion::toString(team.secondaryScore, 1)
                : std::string{"-"},
              swisssystems::eligibleForBye(team, tournament) ? "Y" : "N",
              assigned == assignment.end() ? std::string{} : assigned->second
            };
          },
          tournament,
          orderedTeams);
      }
    }

    /**
     * Compute the team pairings for the next round (C.04.6 §3.3.2).
     */
    std::list<Pairing> computeMatching(
      tournament::Tournament &&tournament,
      std::ostream *const checklistStream)
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
          // Two teams that played must not meet again (§2.1.1 [C1]). A match
          // they were paired for and did not play leaves them free to be paired
          // again: art. 3.5 of the General Handling Rules, "two paired
          // participants, who did not play their game or match, may be paired
          // together in a future round". A real opponent is indicated by
          // match.opponent != team.id (a bye uses the team's own id).
          if (match.gameWasPlayed && match.opponent != team.id)
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
        if (checklistStream)
        {
          printTeamChecklist(
            *checklistStream, tournament, teams, config, &result);
        }
        return result;
      }

      // §3.3.2: combine the top scoregroup with the set of upfloaters §3.5
      // selects for it, pair that bracket (§3.6), and repeat with what is left.
      TeamList remaining = pairTeams;
      while (!remaining.empty())
      {
        const tournament::points topScore =
          scoreOf(*remaining.front(), tournament);
        TeamList residents;
        for (const tournament::Player *const team : remaining)
        {
          if (scoreOf(*team, tournament) == topScore)
          {
            residents.push_back(team);
          }
        }

        TeamList chosen;
        PairList bracketPairs;
        if (
          !selectUpfloaters(
            residents,
            remaining,
            config,
            isLastRound,
            skipFloatHistory,
            tournament,
            forbiddenPairs,
            &chosen,
            &bracketPairs))
        {
          if (checklistStream)
          {
            printTeamChecklist(
              *checklistStream, tournament, teams, config, nullptr);
          }
          throw NoValidPairingException(
            "No set of upfloaters gives the score bracket a legal pairing "
            "(C.04.6 art. 3.3.3).");
        }

        for (const auto &pair : bracketPairs)
        {
          const tournament::Player &x = *pair.first;
          const tournament::Player &y = *pair.second;
          const tournament::Player &first =
            isFirstTeam(x, y, config, tournament) ? x : y;
          const tournament::Player &second = &first == &x ? y : x;
          const tournament::Color firstColour =
            colourForFirstTeam(first, second, config, isLastRound, tournament);
          result.emplace_back(first.id, second.id, firstColour);
        }

        std::unordered_set<const tournament::Player *> paired(
          residents.begin(), residents.end());
        paired.insert(chosen.begin(), chosen.end());
        TeamList rest;
        for (const tournament::Player *const team : remaining)
        {
          if (!paired.count(team))
          {
            rest.push_back(team);
          }
        }
        remaining = std::move(rest);
      }

      if (checklistStream)
      {
        printTeamChecklist(
          *checklistStream, tournament, teams, config, &result);
      }
      return result;
    }
  }
}
#endif
