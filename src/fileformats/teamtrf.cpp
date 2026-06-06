#include <algorithm>
#include <cmath>
#include <codecvt>
#include <cstddef>
#include <istream>
#include <locale>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <tournament/tournament.h>

#include "teamtrf.h"
#include "trf.h"
#include "types.h"

#ifndef OMIT_TEAM
namespace fileformats
{
  namespace teamtrf
  {
    namespace
    {
      // A team's roster: its TPN (1-based) and member StartingRank numbers
      // converted to internal (0-based) player ids.
      struct Roster
      {
        unsigned long tpn;
        std::vector<tournament::player_index> members;
      };

      // A TRF-2026 record 330 (team forfeit): used when a whole team did not show
      // up, so the match is not captured in the 001 records. white/black are the
      // teams' pairing numbers minus one; whiteWon/blackWon give the result.
      struct TeamForfeit
      {
        tournament::round_index round;  // zero-based
        tournament::player_index white;
        tournament::player_index black;
        bool whiteWon;
        bool blackWon;
      };

      // A TRF-2026 record 240 team bye (full/half/zero-point). For teams, F and H
      // byes are mandatory in 240 (they are not fully derivable from 001).
      struct TeamBye
      {
        tournament::round_index round;  // zero-based
        tournament::player_index team;  // team pairing number minus one
        char32_t type;                  // U'F' / U'H' / U'Z'
      };

      // A TRF-2026 record 250 team acceleration: fictitious match/game points
      // assigned to a range of teams over a range of rounds.
      struct TeamAccel
      {
        tournament::points matchPoints;
        tournament::points gamePoints;
        tournament::round_index roundStart;  // zero-based, inclusive
        tournament::round_index roundEnd;     // zero-based, inclusive
        tournament::player_index firstTeam;   // team pairing number minus one
        tournament::player_index lastTeam;
      };

      std::string trim(const std::string &s)
      {
        const auto begin = s.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos)
        {
          return "";
        }
        const auto end = s.find_last_not_of(" \t\r\n");
        return s.substr(begin, end - begin + 1u);
      }

      // Extract the substring at 1-based columns [start, start+width).
      std::string field(const std::string &line, std::size_t start1, std::size_t width)
      {
        if (start1 == 0u || start1 - 1u >= line.size())
        {
          return "";
        }
        return line.substr(start1 - 1u, width);
      }

      // Reduce a UTF-8 line to one byte per CHARACTER (non-ASCII characters
      // become '?') so byte positions equal the spec's column positions. The
      // data fields parsed here are pure ASCII; only free-text names (e.g.
      // accented team names in 310/013) contain multi-byte characters, and
      // without this they would shift the columns of every field after them.
      // Decoding and validation use the same mechanism as trf::readFile.
      std::string asciiColumns(const std::string &line)
      {
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
        const std::u32string decoded = convert.from_bytes(line);
        if (convert.converted() < line.size())
        {
          throw FileFormatException("The file is not legal UTF-8.");
        }
        std::string out;
        out.reserve(decoded.size());
        for (const char32_t c : decoded)
        {
          out.push_back(c < 0x80u ? static_cast<char>(c) : '?');
        }
        return out;
      }

      // Parse a points value such as "2.0" or "11.5" into tenths (20, 115).
      bool parsePoints(const std::string &raw, tournament::points &out)
      {
        const std::string s = trim(raw);
        if (s.empty())
        {
          return false;
        }
        try
        {
          const double value = std::stod(s);
          out =
            static_cast<tournament::points>(std::lround(value * 10.0));
          return true;
        }
        catch (...)
        {
          return false;
        }
      }

      // Map a FIDE_TEAM_* 192 code to a TeamConfig (see docs/team-pairing.md §7).
      swisssystems::TeamConfig parse192(const std::string &codeRaw)
      {
        std::string code = trim(codeRaw);
        swisssystems::TeamConfig config; // default: Type A, MP primary, GP secondary

        // Strip a trailing _BAKU (acceleration handled elsewhere; not v1).
        const std::string baku = "_BAKU";
        if (code.size() >= baku.size()
              && code.compare(code.size() - baku.size(), baku.size(), baku) == 0)
        {
          code.erase(code.size() - baku.size());
        }

        if (code == "FIDE_TEAM" || code.empty())
        {
          return config; // defaults
        }

        // Colour type.
        if (code.find("TYPEA") != std::string::npos)
        {
          config.colourType = swisssystems::ColourType::A;
        }
        else if (code.find("TYPEB") != std::string::npos)
        {
          config.colourType = swisssystems::ColourType::B;
        }
        else
        {
          config.colourType = swisssystems::ColourType::NONE;
        }

        // Score roles: the suffix is _MP_GP, _GP_MP, _MP, or _GP.
        const bool endsGpMp =
          code.size() >= 6u && code.compare(code.size() - 6u, 6u, "_GP_MP") == 0;
        const bool endsMpGp =
          code.size() >= 6u && code.compare(code.size() - 6u, 6u, "_MP_GP") == 0;
        const bool endsGp =
          code.size() >= 3u && code.compare(code.size() - 3u, 3u, "_GP") == 0;

        if (endsGpMp)
        {
          config.primaryScore = swisssystems::ScoreChoice::GAME_POINTS;
          config.secondaryForColour = swisssystems::ScoreChoice::MATCH_POINTS;
          config.useSecondaryForColour = true;
        }
        else if (endsMpGp)
        {
          config.primaryScore = swisssystems::ScoreChoice::MATCH_POINTS;
          config.secondaryForColour = swisssystems::ScoreChoice::GAME_POINTS;
          config.useSecondaryForColour = true;
        }
        else if (endsGp)
        {
          config.primaryScore = swisssystems::ScoreChoice::GAME_POINTS;
          config.useSecondaryForColour = false;
        }
        else // ends with _MP (or unrecognised) -> MP primary, no secondary
        {
          config.primaryScore = swisssystems::ScoreChoice::MATCH_POINTS;
          config.useSecondaryForColour = false;
        }
        return config;
      }
    }

    tournament::Tournament readFile(std::istream &stream, const bool useRank)
    {
      // ---- Pass 1: split lines; extract team-only records, keep the rest. ----
      std::vector<Roster> rosters;
      std::vector<TeamForfeit> forfeits;
      std::vector<TeamBye> teamByes;
      std::vector<TeamAccel> teamAccels;
      swisssystems::TeamConfig config;
      bool sawTeamCode = false;
      tournament::points pointsForTeamWin = 20u;  // 362 defaults: 2.0 / 1.0 / 0.0
      tournament::points pointsForTeamDraw = 10u;
      tournament::points pointsForTeamLoss = 0u;
      // §1.4: a PAB scores as many MATCH and GAME points as a draw, unless the
      // regulations state otherwise (a 320 record). When 320 is absent the
      // defaults are resolved below from the team draw value (match points) and
      // the board count (game points = a drawn match).
      tournament::points pabMatchPoints = 0u;
      tournament::points pabGamePoints = 0u;
      bool pab320Mp = false;
      bool pab320Gp = false;

      std::ostringstream kept;
      std::string line;
      while (std::getline(stream, line))
      {
        if (!line.empty() && line.back() == '\r')
        {
          line.pop_back();
        }
        // Column-based extraction must count CHARACTERS, not bytes (names may
        // contain multi-byte UTF-8). The original line is still forwarded
        // verbatim to the individual reader, which does its own decoding.
        const std::string cline = asciiColumns(line);
        const std::string code = cline.substr(0, 3);

        if (code == "310" || code == "013")
        {
          Roster roster{ };
          std::size_t memberStart;
          if (code == "310")
          {
            const std::string tpnStr = trim(field(cline, 5u, 3u));
            roster.tpn = tpnStr.empty() ? rosters.size() + 1u : std::stoul(tpnStr);
            memberStart = 74u; // §310: members at 74-77, 79-82, ...
          }
          else
          {
            roster.tpn = rosters.size() + 1u; // §013 has no TPN: use file order
            memberStart = 37u; // §013: members at 37-40, 42-45, ...
          }
          for (std::size_t col = memberStart; col - 1u < cline.size(); col += 5u)
          {
            const std::string idStr = trim(field(cline, col, 4u));
            if (idStr.empty())
            {
              continue;
            }
            const unsigned long fileId = std::stoul(idStr);
            if (fileId == 0u)
            {
              continue;
            }
            roster.members.push_back(
              static_cast<tournament::player_index>(fileId - 1u));
          }
          rosters.push_back(std::move(roster));
          continue; // drop from the member stream
        }

        if (code == "362")
        {
          // TW/TD/TL at (5-6, 7-10), (14-15, 16-19), (23-24, 25-28).
          const std::size_t symCols[] = { 5u, 14u, 23u };
          const std::size_t valCols[] = { 7u, 16u, 25u };
          for (int k = 0; k < 3; ++k)
          {
            const std::string sym = trim(field(cline, symCols[k], 2u));
            tournament::points value;
            if (sym.empty() || !parsePoints(field(cline, valCols[k], 4u), value))
            {
              continue;
            }
            if (sym == "TW") pointsForTeamWin = value;
            else if (sym == "TD") pointsForTeamDraw = value;
            else if (sym == "TL") pointsForTeamLoss = value;
          }
          continue;
        }

        if (code == "320")
        {
          tournament::points value;
          if (parsePoints(field(cline, 5u, 4u), value))
          {
            pabMatchPoints = value;
            pab320Mp = true;
          }
          if (parsePoints(field(cline, 10u, 4u), value))
          {
            pabGamePoints = value;
            pab320Gp = true;
          }
          continue;
        }

        if (code == "192")
        {
          const std::string arg = trim(cline.substr(std::min<std::size_t>(4u, cline.size())));
          if (arg.find("TEAM") != std::string::npos)
          {
            config = parse192(arg);
            sawTeamCode = true;
          }
          continue; // drop: trf::readFile rejects FIDE_TEAM_* codes
        }

        if (code == "250")
        {
          // Team acceleration: MP 5-8, GP 10-13, round range 15-17/19-21, team
          // range 23-26/28-31. Stripped so trf's individual parser (which
          // rejects a non-empty match-points field and resizes by player id)
          // does not choke on or misapply it.
          tournament::points mp = 0u;
          tournament::points gp = 0u;
          parsePoints(field(cline, 5u, 4u), mp);
          parsePoints(field(cline, 10u, 4u), gp);
          const std::string rsStr = trim(field(cline, 15u, 3u));
          const std::string reStr = trim(field(cline, 19u, 3u));
          const std::string f1Str = trim(field(cline, 23u, 4u));
          const std::string f2Str = trim(field(cline, 28u, 4u));
          if (!rsStr.empty() && !reStr.empty() && !f1Str.empty() && !f2Str.empty())
          {
            TeamAccel a{ };
            a.matchPoints = mp;
            a.gamePoints = gp;
            a.roundStart =
              static_cast<tournament::round_index>(std::stoul(rsStr) - 1u);
            a.roundEnd =
              static_cast<tournament::round_index>(std::stoul(reStr) - 1u);
            a.firstTeam =
              static_cast<tournament::player_index>(std::stoul(f1Str) - 1u);
            a.lastTeam =
              static_cast<tournament::player_index>(std::stoul(f2Str) - 1u);
            teamAccels.push_back(a);
          }
          continue;
        }

        if (code == "240")
        {
          // Team bye request. Type at 5 (F/H/Z), round 7-9, team IDs 11-14,
          // 16-19, ... Stripped from the member stream so trf does not misapply
          // it to a member player.
          const std::string type = trim(field(cline, 5u, 1u));
          const std::string roundStr = trim(field(cline, 7u, 3u));
          if (!type.empty() && !roundStr.empty())
          {
            const tournament::round_index rd =
              static_cast<tournament::round_index>(std::stoul(roundStr) - 1u);
            for (std::size_t col = 11u; col - 1u < cline.size(); col += 5u)
            {
              const std::string idStr = trim(field(cline, col, 4u));
              if (idStr.empty())
              {
                continue;
              }
              const unsigned long id = std::stoul(idStr);
              if (id == 0u)
              {
                continue;
              }
              teamByes.push_back(
                { rd,
                  static_cast<tournament::player_index>(id - 1u),
                  static_cast<char32_t>(type[0]) });
            }
          }
          continue;
        }

        if (code == "330")
        {
          // Team forfeit. Type at 5-6 (+-/-+/--), round 8-10, White team 12-14,
          // Black team 16-18.
          const std::string type = trim(field(cline, 5u, 2u));
          const std::string roundStr = trim(field(cline, 8u, 3u));
          const std::string whiteStr = trim(field(cline, 12u, 3u));
          const std::string blackStr = trim(field(cline, 16u, 3u));
          if (!roundStr.empty() && !whiteStr.empty() && !blackStr.empty()
                && type.size() == 2u)
          {
            TeamForfeit f{ };
            f.round =
              static_cast<tournament::round_index>(std::stoul(roundStr) - 1u);
            f.white =
              static_cast<tournament::player_index>(std::stoul(whiteStr) - 1u);
            f.black =
              static_cast<tournament::player_index>(std::stoul(blackStr) - 1u);
            f.whiteWon = type[0] == '+';  // "+-" => White wins
            f.blackWon = type[1] == '+';  // "-+" => Black wins; "--" => neither
            forfeits.push_back(f);
          }
          continue;
        }

        if (code == "352")
        {
          continue; // 352 board colours are implicit from the rosters
        }

        kept << line << '\n';
      }

      if (rosters.empty())
      {
        throw FileFormatException(
          "No team records (310/013) found in the team tournament file.");
      }
      (void)sawTeamCode;

      // ---- Pass 2: read the member tournament via the individual reader. ----
      // The member 001 Points field is informative in team competitions
      // (TRF-2026) and need not match game results, so skip score validation.
      std::istringstream memberStream(kept.str());
      tournament::Tournament member =
        trf::readFile(memberStream, useRank, nullptr, false);

      // ---- Map members to teams, indexed by TPN order. ----
      std::sort(
        rosters.begin(),
        rosters.end(),
        [](const Roster &a, const Roster &b) { return a.tpn < b.tpn; });
      const std::size_t teamCount = rosters.size();

      constexpr tournament::player_index noTeam =
        ~tournament::player_index{ };
      std::vector<tournament::player_index> memberToTeam(
        member.players.size(), noTeam);
      for (tournament::player_index t = 0; t < teamCount; ++t)
      {
        for (const tournament::player_index m : rosters[t].members)
        {
          if (m < memberToTeam.size())
          {
            memberToTeam[m] = t;
          }
        }
      }

      // Map a team's pairing-number index (TPN - 1) to its team index. Used to
      // resolve 330 forfeit records, which reference teams by pairing number.
      std::unordered_map<tournament::player_index, tournament::player_index>
        teamOfTpnIndex;
      for (tournament::player_index t = 0; t < teamCount; ++t)
      {
        teamOfTpnIndex[
          static_cast<tournament::player_index>(rosters[t].tpn - 1u)] = t;
      }

      const tournament::round_index rounds = member.playedRounds;

      // Pass 2a: per team, per round, the board-1 record and the game points.
      std::vector<std::vector<tournament::points>> gamePoints(
        teamCount, std::vector<tournament::points>(rounds, 0u));
      std::vector<std::vector<tournament::player_index>> oppTeam(
        teamCount, std::vector<tournament::player_index>(rounds, noTeam));
      std::vector<std::vector<tournament::Color>> board1Colour(
        teamCount, std::vector<tournament::Color>(rounds, tournament::COLOR_NONE));
      std::vector<std::vector<bool>> board1Played(
        teamCount, std::vector<bool>(rounds, false));
      std::vector<std::vector<bool>> board1Participated(
        teamCount, std::vector<bool>(rounds, false));
      std::vector<std::vector<tournament::MatchScore>> board1Score(
        teamCount,
        std::vector<tournament::MatchScore>(
          rounds, tournament::MATCH_SCORE_LOSS));

      for (tournament::player_index t = 0; t < teamCount; ++t)
      {
        const std::vector<tournament::player_index> &members = rosters[t].members;
        for (tournament::round_index r = 0; r < rounds; ++r)
        {
          // The opponent team comes from ANY board with a real opponent (not
          // just board 1, which may itself be a forfeit). All boards in a team
          // match face the same opponent team.
          for (const tournament::player_index m : members)
          {
            if (m >= member.players.size())
            {
              continue;
            }
            const tournament::Player &mp = member.players[m];
            if (r < mp.matches.size()
                  && mp.matches[r].opponent != m
                  && mp.matches[r].opponent < memberToTeam.size())
            {
              const tournament::player_index ot =
                memberToTeam[mp.matches[r].opponent];
              if (ot != noTeam)
              {
                oppTeam[t][r] = ot;
                break;
              }
            }
          }
          const bool hasOpponent = oppTeam[t][r] != noTeam;

          // Game points = sum of the boards. Within a real match, a board won by
          // forfeit against an undefined opponent ("0000 +") scores as a board
          // win, not as a pairing-allocated bye (the "0000" opponent otherwise
          // collapses to the member's own id, which getPoints scores as a PAB).
          tournament::points gp = 0u;
          for (const tournament::player_index m : members)
          {
            if (m >= member.players.size())
            {
              continue;
            }
            const tournament::Player &mp = member.players[m];
            if (r >= mp.matches.size())
            {
              continue;
            }
            const tournament::Match &mm = mp.matches[r];
            if (hasOpponent
                  && mm.matchScore == tournament::MATCH_SCORE_WIN
                  && !mm.gameWasPlayed
                  && mm.opponent == m
                  && mm.participatedInPairing)
            {
              gp += member.pointsForWin;
            }
            else
            {
              gp += member.getPoints(mp, mm);
            }
          }
          gamePoints[t][r] = gp;

          if (!members.empty() && members[0] < member.players.size())
          {
            const tournament::Player &b1 = member.players[members[0]];
            if (r < b1.matches.size())
            {
              const tournament::Match &m0 = b1.matches[r];
              board1Played[t][r] = m0.gameWasPlayed;
              board1Participated[t][r] = m0.participatedInPairing;
              board1Score[t][r] = m0.matchScore;
              board1Colour[t][r] = m0.gameWasPlayed
                ? m0.color
                : tournament::COLOR_NONE;
            }
          }
        }
      }

      // Resolve the PAB match points: the 320 value if given, else a draw (§1.4).
      const tournament::points pabMatchPointsResolved =
        pab320Mp ? pabMatchPoints : pointsForTeamDraw;

      // ---- Build the team tournament. ----
      tournament::Tournament teams;
      teams.playedRounds = rounds;
      teams.expectedRounds = member.expectedRounds;
      teams.initialColor = member.initialColor;
      teams.pointsForWin = pointsForTeamWin;
      teams.pointsForDraw = pointsForTeamDraw;
      teams.pointsForLoss = pointsForTeamLoss;
      teams.pointsForForfeitLoss = pointsForTeamLoss;
      teams.pointsForZeroPointBye = 0u;
      teams.pointsForPairingAllocatedBye = pabMatchPointsResolved;
      teams.swissSystem = swisssystems::TEAM;
      teams.teamConfig = config;
      teams.defaultAcceleration = false;
      // Pairing restrictions (260 "teams that cannot meet", and XXP) are parsed
      // by the member reader as team-pairing-number entries; they apply directly
      // to the team tournament (a team's index equals its TPN minus one).
      teams.forbiddenPairs = member.forbiddenPairs;

      const std::vector<tournament::AbnormalAssignment> &abnormals =
        member.abnormalAssignments;

      for (tournament::player_index t = 0; t < teamCount; ++t)
      {
        // The team's identifier for 299 targeting is its TPN minus one.
        const tournament::player_index teamTpnIndex =
          static_cast<tournament::player_index>(rosters[t].tpn - 1u);

        std::vector<tournament::Match> matches;
        matches.reserve(rounds);
        long matchPointTotal = 0;
        long gamePointTotal = 0;

        for (tournament::round_index r = 0; r < rounds; ++r)
        {
          const tournament::player_index opp = oppTeam[t][r];
          const long rawGp = static_cast<long>(gamePoints[t][r]);

          // Determine the round's abnormal-assignment type, the opponent stored
          // in the team's history, the team's match result, and the DEFAULT
          // match/game points (before any 299 override).
          char32_t type;
          tournament::player_index matchOpp = opp;
          tournament::MatchScore matchScore;
          bool played = board1Played[t][r];
          long defaultMp;
          long defaultGp = rawGp;

          // A 330 team-forfeit record overrides the 001-derived data: it is the
          // only source when a whole team did not show up (the present team's 001
          // records a forfeit win against an undefined "0000" opponent, and the
          // absent team has no 001 entry at all).
          bool forfeitApplies = false;
          for (const TeamForfeit &f : forfeits)
          {
            if (f.round != r)
            {
              continue;
            }
            if (f.white == teamTpnIndex)
            {
              forfeitApplies = true;
              matchScore =
                f.whiteWon ? tournament::MATCH_SCORE_WIN
                  : tournament::MATCH_SCORE_LOSS;
              const auto it = teamOfTpnIndex.find(f.black);
              matchOpp = it == teamOfTpnIndex.end() ? t : it->second;
              break;
            }
            if (f.black == teamTpnIndex)
            {
              forfeitApplies = true;
              matchScore =
                f.blackWon ? tournament::MATCH_SCORE_WIN
                  : tournament::MATCH_SCORE_LOSS;
              const auto it = teamOfTpnIndex.find(f.white);
              matchOpp = it == teamOfTpnIndex.end() ? t : it->second;
              break;
            }
          }

          // A 240 team bye (full/half/zero-point) for a played round overrides
          // the 001-derived data; it is the authoritative source for team byes.
          char32_t declaredBye = 0;
          for (const TeamBye &b : teamByes)
          {
            if (b.round == r && b.team == teamTpnIndex)
            {
              declaredBye = b.type;
              break;
            }
          }

          if (declaredBye)
          {
            matchOpp = t;
            played = false;
            type = declaredBye;  // U'F' / U'H' / U'Z'
            matchScore =
              declaredBye == U'F' ? tournament::MATCH_SCORE_WIN
                : declaredBye == U'H' ? tournament::MATCH_SCORE_DRAW
                : tournament::MATCH_SCORE_LOSS;
            defaultMp =
              declaredBye == U'F' ? static_cast<long>(pointsForTeamWin)
                : declaredBye == U'H' ? static_cast<long>(pointsForTeamDraw)
                : static_cast<long>(pointsForTeamLoss);
            defaultGp = rawGp;
          }
          else if (forfeitApplies)
          {
            played = false;
            if (matchScore == tournament::MATCH_SCORE_WIN)
            {
              type = U'+';
              defaultMp = static_cast<long>(pointsForTeamWin);
              // A whole-team forfeit win scores every board as a forfeit win
              // (consistent with a board-level forfeit win inside a played
              // match), not as a pairing-allocated bye. This only diverges from
              // the 001-derived rawGp when the per-board PAB value differs from
              // the win value (e.g. a "162 P" override).
              defaultGp =
                static_cast<long>(rosters[t].members.size())
                  * static_cast<long>(member.pointsForWin);
            }
            else
            {
              type = U'-';
              defaultMp = static_cast<long>(pointsForTeamLoss);
              defaultGp = rawGp;
            }
          }
          else if (opp == noTeam)
          {
            // Bye: opponent is the team itself; the bye type is recovered from
            // board 1's stored result.
            matchOpp = t;
            played = false;
            if (board1Score[t][r] == tournament::MATCH_SCORE_WIN
                  && board1Participated[t][r])
            {
              // U: pairing-allocated bye (not a 299 type; scored as a draw/§1.4).
              // Game points default to a drawn match (boards x game-draw points)
              // unless a 320 record specifies them.
              type = U'P';
              matchScore = tournament::MATCH_SCORE_WIN;
              defaultMp = static_cast<long>(pabMatchPointsResolved);
              defaultGp = pab320Gp
                ? static_cast<long>(pabGamePoints)
                : static_cast<long>(rosters[t].members.size())
                    * static_cast<long>(member.pointsForDraw);
            }
            else if (board1Score[t][r] == tournament::MATCH_SCORE_WIN)
            {
              type = U'F';  // full-point bye
              matchScore = tournament::MATCH_SCORE_WIN;
              defaultMp = static_cast<long>(pointsForTeamWin);
            }
            else if (board1Score[t][r] == tournament::MATCH_SCORE_DRAW)
            {
              type = U'H';  // half-point bye
              matchScore = tournament::MATCH_SCORE_DRAW;
              defaultMp = static_cast<long>(pointsForTeamDraw);
            }
            else
            {
              type = U'Z';  // zero-point bye
              matchScore = tournament::MATCH_SCORE_LOSS;
              defaultMp = static_cast<long>(pointsForTeamLoss);
            }
          }
          else if (!played)
          {
            // Unplayed match against a real opponent: a team forfeit.
            if (board1Score[t][r] == tournament::MATCH_SCORE_WIN)
            {
              type = U'+';
              matchScore = tournament::MATCH_SCORE_WIN;
              defaultMp = static_cast<long>(pointsForTeamWin);
            }
            else
            {
              type = U'-';
              matchScore = tournament::MATCH_SCORE_LOSS;
              defaultMp = static_cast<long>(pointsForTeamLoss);
            }
          }
          else
          {
            // Over-the-board match: result by game-point comparison.
            const long gpO = static_cast<long>(gamePoints[opp][r]);
            if (rawGp > gpO)
            {
              type = U'W';
              matchScore = tournament::MATCH_SCORE_WIN;
              defaultMp = static_cast<long>(pointsForTeamWin);
            }
            else if (rawGp < gpO)
            {
              type = U'L';
              matchScore = tournament::MATCH_SCORE_LOSS;
              defaultMp = static_cast<long>(pointsForTeamLoss);
            }
            else
            {
              type = U'D';
              matchScore = tournament::MATCH_SCORE_DRAW;
              defaultMp = static_cast<long>(pointsForTeamDraw);
            }
          }

          long mp = defaultMp;
          long gp = defaultGp;

          // Apply the most specific matching non-blank 299 override (explicit
          // target beats all-targets; explicit round beats all-rounds; later
          // records win ties).
          int bestSpecificity = -1;
          for (const tournament::AbnormalAssignment &a : abnormals)
          {
            if (a.type != type)
            {
              continue;
            }
            if (!a.allRounds && a.round != r)
            {
              continue;
            }
            const bool targeted =
              a.targets.empty()
                || std::find(a.targets.begin(), a.targets.end(), teamTpnIndex)
                     != a.targets.end();
            if (!targeted)
            {
              continue;
            }
            const int specificity =
              (a.targets.empty() ? 0 : 2) + (a.allRounds ? 0 : 1);
            if (specificity >= bestSpecificity)
            {
              bestSpecificity = specificity;
              if (a.hasMatchPoints)
              {
                mp = a.matchPoints;
              }
              if (a.hasOtherPoints)
              {
                gp = a.otherPoints;
              }
            }
          }

          // Apply blank-type 299 records as additive penalties/bonuses.
          for (const tournament::AbnormalAssignment &a : abnormals)
          {
            if (a.type != U' ')
            {
              continue;
            }
            if (!a.allRounds && a.round != r)
            {
              continue;
            }
            const bool targeted =
              a.targets.empty()
                || std::find(a.targets.begin(), a.targets.end(), teamTpnIndex)
                     != a.targets.end();
            if (!targeted)
            {
              continue;
            }
            if (a.hasMatchPoints)
            {
              mp += a.matchPoints;
            }
            if (a.hasOtherPoints)
            {
              gp += a.otherPoints;
            }
          }

          // participatedInPairing drives bye eligibility (§2.1.2 [C2]): a PAB and
          // a full-point bye block a further PAB; half/zero-point byes do not.
          // PAB is marked participated; F blocks via its win-valued getPoints, so
          // F/H/Z are left unparticipated; all real games/forfeits participated.
          const bool participated =
            type == U'P' ? true
              : (type == U'F' || type == U'H' || type == U'Z') ? false
              : true;

          matches.emplace_back(
            matchOpp,
            played ? board1Colour[t][r] : tournament::COLOR_NONE,
            matchScore,
            played,
            participated);

          matchPointTotal += mp;
          gamePointTotal += gp;
        }

        // If the team requested a bye (240) for the round being paired, append a
        // placeholder so it is excluded from this round's pairing.
        for (const TeamBye &b : teamByes)
        {
          if (b.round == rounds && b.team == teamTpnIndex)
          {
            matches.emplace_back(t);
            break;
          }
        }

        if (matchPointTotal < 0)
        {
          matchPointTotal = 0;
        }
        if (gamePointTotal < 0)
        {
          gamePointTotal = 0;
        }
        const tournament::points matchPoints =
          static_cast<tournament::points>(matchPointTotal);
        const tournament::points gamePointsTotal =
          static_cast<tournament::points>(gamePointTotal);
        const tournament::points primary =
          config.primaryScore == swisssystems::ScoreChoice::MATCH_POINTS
            ? matchPoints
            : gamePointsTotal;
        const tournament::points secondary =
          config.primaryScore == swisssystems::ScoreChoice::MATCH_POINTS
            ? gamePointsTotal
            : matchPoints;

        tournament::Player team(t, primary, 0u, std::move(matches));
        team.secondaryScore =
          config.useSecondaryForColour ? secondary : 0u;
        teams.players.push_back(std::move(team));
        teams.playersByRank.push_back(t);
      }

      // Apply 250 team accelerations: the primary-score fictitious points go into
      // each team's round-indexed acceleration vector (so scoreWithAcceleration
      // boosts the score used for scoregroups); the secondary-score fictitious
      // points are added to the secondary score when the round being paired is in
      // range (it affects only colour, §4.2.2).
      for (const TeamAccel &a : teamAccels)
      {
        const tournament::points primaryAccel =
          config.primaryScore == swisssystems::ScoreChoice::MATCH_POINTS
            ? a.matchPoints
            : a.gamePoints;
        const tournament::points secondaryAccel =
          config.primaryScore == swisssystems::ScoreChoice::MATCH_POINTS
            ? a.gamePoints
            : a.matchPoints;
        for (
          tournament::player_index tpn = a.firstTeam;
          tpn <= a.lastTeam;
          ++tpn)
        {
          const auto it = teamOfTpnIndex.find(tpn);
          if (it == teamOfTpnIndex.end())
          {
            continue;
          }
          tournament::Player &team = teams.players[it->second];
          if (primaryAccel)
          {
            if (team.accelerations.size() <= a.roundEnd)
            {
              team.accelerations.resize(a.roundEnd + 1u, 0u);
            }
            for (
              tournament::round_index r = a.roundStart;
              r <= a.roundEnd;
              ++r)
            {
              team.accelerations[r] = primaryAccel;
            }
          }
          if (config.useSecondaryForColour
                && a.roundStart <= rounds && rounds <= a.roundEnd)
          {
            team.secondaryScore += secondaryAccel;
          }
        }
      }

      return teams;
    }
  }
}
#endif
