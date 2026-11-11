#include <algorithm>
#include <cassert>
#include <cstdint>
#include <deque>
#include <list>
#include <ostream>
#include <string>
#include <tuple>
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
       *   soft       : (only with SCS records) minimise soft forbidden pairs
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

        // Minimise soft forbidden pairs: below every FIDE criterion, ahead of
        // the §3.6 order.
        const auto &softForbiddenPairs = tournament.currentSoftForbiddenPairs;
        if (!softForbiddenPairs.empty())
        {
          shiftEdgeWeight<max>(result, fieldBits);
          result |=
            max ? 1u : softForbiddenPairs[a.id].count(b.id) ? 0u : 1u;
        }

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
          // The bound is twice the value built above, less one. No real weight
          // exceeds it: the count fields of that value are all 1, and the
          // identifier fields sum to less than its lowest count bit. The
          // matching subroutine shifts the bound left by two bits, so it needs
          // two bits of headroom above its top bit. Growing by three bits
          // before halving twice leaves them. (dutch.cpp grows by two, which
          // is enough there because its bound is a power of two. This one is
          // not, and growing by two overflowed whenever the top bit fell at
          // the end of a 64-bit word.)
          result.shiftGrow(3u);
          result >>= 2u;
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

      /**
       * The fewest upfloaters from `lower` with which every resident can be
       * paired ([C1]), or `lower.size() + 1` when no number will do. A pairing
       * of a bracket matches each resident either to another resident or to an
       * upfloater of its own, so a bracket with fewer upfloaters than this
       * cannot be paired at all. The count comes from one weighted matching
       * over the residents and `lower` that pairs as many residents as it can
       * and, among those matchings, as many of them with each other as it can.
       */
      std::size_t fewestUpfloaters(
        const TeamList &residents,
        const TeamList &lower,
        const std::vector<std::unordered_set<tournament::player_index>>
          &forbiddenPairs)
      {
        const std::size_t residentCount = residents.size();
        const std::size_t n = residentCount + lower.size();
        // Each resident covered is worth `cover`, and a pair of residents is
        // worth one more than the two it covers, which can never make up for
        // a resident left out.
        const std::size_t cover = residentCount + 1u;
        edge_weight maxWeight{ 2u * cover + 1u };
        matching_computer computer(n, maxWeight);
        for (std::size_t i = 0; i < n; ++i)
        {
          computer.addVertex();
        }
        const auto team =
          [&residents, &lower, residentCount](const std::size_t index)
          {
            return index < residentCount
              ? residents[index]
              : lower[index - residentCount];
          };
        for (std::size_t i = 0; i < residentCount; ++i)
        {
          for (std::size_t j = 0; j < n; ++j)
          {
            if (j == i || (j < residentCount && j < i))
            {
              continue;
            }
            edge_weight w{ maxWeight };
            w &= 0u;
            if (!forbiddenPairs[team(i)->id].count(team(j)->id))
            {
              w |= j < residentCount ? 2u * cover + 1u : cover;
            }
            computer.setEdgeWeight(i, j, w);
          }
        }
        computer.computeMatching();
        const std::vector<tournament::player_index> matching =
          computer.getMatching();
        std::size_t upfloaters{ };
        for (std::size_t i = 0; i < residentCount; ++i)
        {
          if (matching[i] == i)
          {
            return lower.size() + 1u;
          }
          if (matching[i] >= residentCount)
          {
            ++upfloaters;
          }
        }
        return upfloaters;
      }
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
       * The number of pairs that are soft forbidden.
       */
      std::size_t countSoftForbidden(
        const PairList &pairs,
        const tournament::Tournament &tournament)
      {
        const auto &softForbiddenPairs = tournament.currentSoftForbiddenPairs;
        if (softForbiddenPairs.empty())
        {
          return 0u;
        }
        std::size_t result{ };
        for (const auto &pair : pairs)
        {
          result += softForbiddenPairs[pair.first->id].count(pair.second->id);
        }
        return result;
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
       * The best set found so far for one profile, by [C6], [C7] and then the
       * soft forbidden pairs. A later set replaces it only when it does
       * strictly better, so §3.5.4 settles the ties.
       */
      struct BestSet
      {
        bool found = false;
        std::size_t c6{ };
        std::size_t c7{ };
        std::size_t soft{ };
      };

      /**
       * The sets of upfloaters with a given score profile that could still
       * be chosen, in the order of §3.5.3 (within a set: descending score,
       * then ascending TPN) and §3.5.4 (among the sets: the lexicographic
       * order of their TPNs). Only sets that cannot be legal, or cannot do
       * strictly better than `best`, are skipped, so the set selectUpfloaters
       * ends up with is the one a walk through every set would give.
       *
       * A set takes a fixed number of teams from each score level of the
       * profile, highest score first, and every level's teams are in
       * ascending TPN order in `lower`. Choosing the teams of a set one by
       * one in that order, each from the teams after the one chosen before it
       * at its level, walks the sets in §3.5.4 order, and a partial choice
       * every completion of which can be ruled out cuts them all off at once.
       * Two things rule a completion out:
       *
       * - It cannot be legal: the bracket and the teams left below must both
       *   be paired completely ([C1], [C3]). Then the two pairings together
       *   pair all of `remaining`, every resident and chosen team meeting
       *   another of them or a team that can still be chosen, and the teams
       *   that can still be chosen meet them no more often than there are
       *   places left in the set. canComplete asks one weighted matching for
       *   exactly that.
       * - It cannot beat `best`: lower bounds on [C6], [C7] and the soft
       *   forbidden pairs of every completion, taken together, are no better
       *   than what `best` already has.
       */
      class UpfloaterSearch
      {
      public:
        UpfloaterSearch(
          const TeamList &residents,
          const TeamList &remaining,
          const TeamList &lower,
          const std::vector<tournament::points> &profile,
          const bool hasFollowing,
          const tournament::points followingScore_,
          const bool skipFloatHistory,
          const tournament::Tournament &tournament_,
          const std::vector<std::unordered_set<tournament::player_index>>
            &forbiddenPairs_,
          const BestSet &best_)
          : teams(remaining),
            tournament(tournament_),
            forbiddenPairs(forbiddenPairs_),
            best(best_),
            state(remaining.size(), EXCLUDED),
            floater(remaining.size(), false),
            size(profile.size()),
            followingScore(followingScore_)
        {
          std::unordered_map<const tournament::Player *, std::size_t> position;
          for (std::size_t index = 0; index < teams.size(); ++index)
          {
            position[teams[index]] = index;
            floater[index] =
              !skipFloatHistory && wasFloater(*teams[index], 1u, tournament);
          }
          for (const tournament::Player *const team : residents)
          {
            state[position[team]] = CORE;
          }
          std::size_t start = 0;
          while (start < profile.size())
          {
            std::size_t end = start;
            while (end < profile.size() && profile[end] == profile[start])
            {
              ++end;
            }
            Level level;
            level.take = end - start;
            for (const tournament::Player *const team : lower)
            {
              if (scoreOf(*team, tournament) == profile[start])
              {
                level.group.push_back(position[team]);
                state[position[team]] = POOL;
              }
            }
            levels.push_back(std::move(level));
            start = end;
          }

          // [C6] looks at what is left of the following scoregroup. When the
          // profile takes nobody from it, that is the whole of it whatever
          // the set; otherwise it is known once the first level is chosen.
          followingLevel =
            hasFollowing && !profile.empty()
              && profile.front() == followingScore_;
          c6Bound = hasFollowing && !followingLevel ? leastC6() : 0u;
        }

        /**
         * Call `visit` with each set in turn until it returns false. Returns
         * false when `visit` stopped the search.
         */
        template <typename Visit>
        bool run(Visit &&visit)
        {
          for (const Level &level : levels)
          {
            if (level.take > level.group.size())
            {
              return true;
            }
          }
          TeamList set;
          return viable(0u) ? choose(0u, 0u, 0u, set, visit) : true;
        }

      private:
        enum State : unsigned char
        {
          CORE,     // a resident or a chosen upfloater
          POOL,     // a team that can still be chosen
          EXCLUDED  // a team that can no longer be chosen
        };

        struct Level
        {
          std::vector<std::size_t> group; // positions in `teams`
          std::size_t take{ };
        };

        const TeamList &teams;
        const tournament::Tournament &tournament;
        const std::vector<std::unordered_set<tournament::player_index>>
          &forbiddenPairs;
        const BestSet &best;
        std::vector<State> state;
        std::vector<bool> floater;
        const std::size_t size;
        const tournament::points followingScore;
        std::vector<Level> levels;
        std::size_t chosenCount{ };
        std::size_t chosenFloaters{ };
        bool followingLevel{ };
        std::size_t c6Bound{ };

        /**
         * Choose the team for slot `slot` of level `level`, from position
         * `from` of the level's group on.
         */
        template <typename Visit>
        bool choose(
          const std::size_t level,
          const std::size_t slot,
          const std::size_t from,
          TeamList &set,
          Visit &visit)
        {
          if (level == levels.size())
          {
            return visit(static_cast<const TeamList &>(set));
          }
          Level &current = levels[level];
          if (slot == current.take)
          {
            // Every team of the level not chosen by now is out.
            std::vector<std::size_t> dropped;
            for (std::size_t index = from; index < current.group.size(); ++index)
            {
              if (state[current.group[index]] == POOL)
              {
                state[current.group[index]] = EXCLUDED;
                dropped.push_back(current.group[index]);
              }
            }
            const std::size_t savedC6Bound = c6Bound;
            if (level == 0u && followingLevel)
            {
              c6Bound = leastC6();
            }
            const bool go =
              viable(level + 1u) ? choose(level + 1u, 0u, 0u, set, visit) : true;
            c6Bound = savedC6Bound;
            for (const std::size_t team : dropped)
            {
              state[team] = POOL;
            }
            return go;
          }
          bool go = true;
          std::size_t index = from;
          for (;
               go && index + (current.take - slot) <= current.group.size();
               ++index)
          {
            const std::size_t team = current.group[index];
            state[team] = CORE;
            ++chosenCount;
            chosenFloaters += floater[team];
            set.push_back(teams[team]);
            if (viable(level))
            {
              go = choose(level, slot + 1u, index + 1u, set, visit);
            }
            set.pop_back();
            chosenFloaters -= floater[team];
            --chosenCount;
            // Every later set passed over this team at this slot.
            state[team] = EXCLUDED;
          }
          for (std::size_t back = from; back < index; ++back)
          {
            state[current.group[back]] = POOL;
          }
          return go;
        }

        /**
         * Whether some completion of the current choice, whose levels before
         * `level` are settled, could be legal and do strictly better than
         * `best`.
         */
        bool viable(const std::size_t level) const
        {
          if (best.found)
          {
            const std::size_t c7Bound = leastC7(level);
            if (
              std::tie(c6Bound, c7Bound) > std::tie(best.c6, best.c7)
                || (std::tie(c6Bound, c7Bound) == std::tie(best.c6, best.c7)
                      && !best.soft))
            {
              return false;
            }
            if (
              std::tie(c6Bound, c7Bound) == std::tie(best.c6, best.c7)
                && !(leastSoft() < best.soft))
            {
              return false;
            }
          }
          return cost(0u, 1u) <= size - chosenCount;
        }

        /**
         * A lower bound on the soft forbidden pairs of the bracket of any
         * legal completion. Such a completion pairs all of `teams` within the
         * rules of cost(), with at most `places` resident or chosen teams
         * meeting a team that can still be chosen, and its bracket holds every
         * soft pair that touches a resident or chosen team. So the fewest such
         * pairs over every pairing within those rules is a bound, and so is,
         * by weak duality, the fewest of those pairs plus the number of
         * resident and chosen teams meeting a team still to be chosen, less
         * `places`. The first is the better one when places are plentiful,
         * the second when they are scarce.
         */
        std::size_t leastSoft() const
        {
          const std::size_t places = size - chosenCount;
          const std::size_t withPlaces = cost(1u, 1u);
          const std::size_t dual = withPlaces > places ? withPlaces - places : 0u;
          return dual >= best.soft ? dual : std::max(dual, cost(1u, 0u));
        }

        /**
         * [C7] counts the chosen teams that floated in the previous round.
         * Each level still to fill must take, beyond the teams that can still
         * be chosen there and did not float, as many that did as it still
         * needs.
         */
        std::size_t leastC7(const std::size_t level) const
        {
          std::size_t result = chosenFloaters;
          for (std::size_t index = level; index < levels.size(); ++index)
          {
            const Level &current = levels[index];
            std::size_t taken{ };
            std::size_t steady{ };
            for (const std::size_t team : current.group)
            {
              if (state[team] == CORE)
              {
                ++taken;
              }
              else if (state[team] == POOL && !floater[team])
              {
                ++steady;
              }
            }
            if (current.take - taken > steady)
            {
              result += current.take - taken - steady;
            }
          }
          return result;
        }

        /**
         * [C6] for every set that leaves the same teams of the following
         * scoregroup: those must pair among themselves, with one team from
         * below when they are odd ([C4] for that scoregroup), and the teams
         * below must pair too. When even the first part fails, whatever the
         * teams below, [C6] is not met.
         */
        std::size_t leastC6() const
        {
          TeamList following;
          TeamList below;
          for (std::size_t index = 0; index < teams.size(); ++index)
          {
            if (state[index] == CORE)
            {
              continue;
            }
            const tournament::points score = scoreOf(*teams[index], tournament);
            if (score == followingScore)
            {
              following.push_back(teams[index]);
            }
            else if (score < followingScore)
            {
              below.push_back(teams[index]);
            }
          }
          if (following.empty())
          {
            return 0u; // §2.3.3, the scoregroup is now empty
          }
          return fewestUpfloaters(following, below, forbiddenPairs) <= 1u
            ? 0u
            : 1u;
        }

        /**
         * The least total cost of a pairing of all of `teams` in which a
         * resident or chosen team meets only another of them or a team that
         * can still be chosen ([C1] throughout). A soft forbidden pair that
         * touches a resident or chosen team costs `softCost`, and a resident or
         * chosen team meeting a team still to be chosen costs `placeCost`.
         * Returns more than any count it is compared with when `teams` cannot
         * be paired that way at all.
         */
        std::size_t cost(
          const std::size_t softCost,
          const std::size_t placeCost) const
        {
          const std::size_t n = teams.size();
          const auto &softForbiddenPairs = tournament.currentSoftForbiddenPairs;
          // Each pair is worth `pair` less its cost, and the costs of a whole
          // pairing add up to less than `pair`, so no saving in cost can make
          // up for a pair lost.
          const std::size_t most = softCost + placeCost;
          const std::size_t pair = n * most + 1u;
          edge_weight maxWeight{ pair };
          matching_computer computer(n, maxWeight);
          for (std::size_t i = 0; i < n; ++i)
          {
            computer.addVertex();
          }
          const auto edgeCost =
            [&](const std::size_t i, const std::size_t j)
            {
              const bool iCore = state[i] == CORE;
              const bool jCore = state[j] == CORE;
              std::size_t result{ };
              if (
                (iCore || jCore)
                  && !softForbiddenPairs.empty()
                  && softForbiddenPairs[teams[i]->id].count(teams[j]->id))
              {
                result += softCost;
              }
              if (iCore != jCore)
              {
                result += placeCost;
              }
              return result;
            };
          for (std::size_t i = 0; i < n; ++i)
          {
            for (std::size_t j = 0; j < i; ++j)
            {
              const bool iCore = state[i] == CORE;
              const bool jCore = state[j] == CORE;
              if (
                (iCore && state[j] == EXCLUDED)
                  || (jCore && state[i] == EXCLUDED)
                  || forbiddenPairs[teams[i]->id].count(teams[j]->id))
              {
                continue;
              }
              edge_weight w{ maxWeight };
              w &= 0u;
              w |= pair - edgeCost(i, j);
              computer.setEdgeWeight(i, j, w);
            }
          }
          computer.computeMatching();
          const std::vector<tournament::player_index> matching =
            computer.getMatching();
          std::size_t total{ };
          for (std::size_t i = 0; i < n; ++i)
          {
            if (matching[i] == i)
            {
              return n * most + size + 1u;
            }
            if (matching[i] < i)
            {
              total += edgeCost(i, matching[i]);
            }
          }
          return total;
        }
      };

      /**
       * §3.5: choose the upfloaters that join `residents`, and pair the bracket
       * they form. [C4] (§2.3.1, the fewest upfloaters) and [C5] are the outer
       * loops: the number grows from the fewest the residents can be paired
       * with at all (fewestUpfloaters), and for each number the score profiles
       * are tried best first. A
       * number or a profile no set of which gives a legal pairing is no
       * candidate at all, since [C3] (§2.2.1) asks that all the teams not yet
       * paired can be, so the search falls through to the next one.
       *
       * Within one profile the sets come in the order of §3.5.4, and §3.5.5
       * takes the first that is legal and complies with [C6] and [C7] — read,
       * as §2.3 asks ("comply as much as possible ... in descending priority"),
       * as the smallest value of [C7] a legal set of this profile can reach.
       * UpfloaterSearch hands out the sets in that order, leaving out only
       * those that cannot be legal or cannot do better than the best so far.
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

        // [C4]: every number below the fewest the residents can be paired
        // with leaves every set unpairable, and the search would only find
        // that out set by set. The fewest has the parity of the residents.
        for (
          std::size_t numup =
            fewestUpfloaters(residents, lower, forbiddenPairs);
          numup <= lower.size();
          numup += 2u)
        {
          for (
            const std::vector<tournament::points> &profile :
              upfloaterProfiles(lower, numup, tournament))
          {
            BestSet best;
            UpfloaterSearch search(
              residents,
              remaining,
              lower,
              profile,
              hasFollowing,
              followingScore,
              skipFloatHistory,
              tournament,
              forbiddenPairs,
              best);
            search.run([&](const TeamList &set)
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
                return true;
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
                return true; // [C3] §2.2.1
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
              // Soft forbidden pairs rank below [C6] and [C7], ahead of the
              // order of §3.5.4.
              const std::size_t soft =
                countSoftForbidden(bracketPairs, tournament);
              if (
                !best.found
                  || std::tie(c6, c7, soft)
                       < std::tie(best.c6, best.c7, best.soft))
              {
                best.found = true;
                best.c6 = c6;
                best.c7 = c7;
                best.soft = soft;
                *chosen = set;
                *pairs = bracketPairs;
              }
              // §3.5.5, the first such set: none does better.
              return best.c6 || best.c7 || best.soft;
            });
            if (best.found)
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
