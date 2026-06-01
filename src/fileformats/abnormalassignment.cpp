#include "abnormalassignment.h"

#include <algorithm>
#include <stdexcept>
#include <string>

#include <tournament/tournament.h>
#include <utility/uintstringconversion.h>

#include "types.h"

namespace fileformats
{
  namespace
  {
    /** Trimmed contents of [start, start+len) of line, or empty. */
    std::u32string fieldToken(
      const std::u32string &line,
      const std::u32string::size_type start,
      const std::u32string::size_type len)
    {
      if (start >= line.size())
      {
        return U"";
      }
      const std::u32string sub = line.substr(start, len);
      const auto first = sub.find_first_not_of(U" \t");
      if (first == std::u32string::npos)
      {
        return U"";
      }
      return sub.substr(first, sub.find_last_not_of(U" \t") - first + 1u);
    }

    /** Parse a signed points value ("[-]11.5") and return it in tenths. */
    long readSignedTenths(const std::u32string &token)
    {
      const bool negative = !token.empty() && token[0] == U'-';
      const std::u32string magnitude = negative ? token.substr(1) : token;
      try
      {
        const long value =
          static_cast<long>(
            utility::uintstringconversion
              ::parse<tournament::points>(magnitude, 1));
        return negative ? -value : value;
      }
      catch (const std::invalid_argument &)
      {
        throw InvalidLineException();
      }
      catch (const std::out_of_range &)
      {
        throw InvalidLineException();
      }
    }

    /** Whether every character of token is '0' (a "000"/"0000" = "all" field). */
    bool isAllZero(const std::u32string &token)
    {
      return token.find_first_not_of(U'0') == std::u32string::npos;
    }
  }

  tournament::AbnormalAssignment readAbnormalAssignment(
    const std::u32string &line)
  {
    if (line.size() < 5)
    {
      throw InvalidLineException();
    }
    tournament::AbnormalAssignment result{ };
    result.type = line[4];  // position 5
    switch (result.type)
    {
    case U'W': case U'D': case U'L': case U'F': case U'H':
    case U'Z': case U'+': case U'-': case U' ':
      break;
    default:
      throw InvalidLineException("unknown abnormal-assignment type");
    }

    const std::u32string matchPointsToken = fieldToken(line, 7u, 4u);  // 8-11
    if (!matchPointsToken.empty())
    {
      result.hasMatchPoints = true;
      result.matchPoints = readSignedTenths(matchPointsToken);
    }
    const std::u32string otherPointsToken = fieldToken(line, 13u, 4u);  // 14-17
    if (!otherPointsToken.empty())
    {
      result.hasOtherPoints = true;
      result.otherPoints = readSignedTenths(otherPointsToken);
    }

    const std::u32string roundToken = fieldToken(line, 19u, 3u);  // 20-22
    if (roundToken.empty() || isAllZero(roundToken))
    {
      result.allRounds = true;
    }
    else
    {
      try
      {
        result.allRounds = false;
        result.round =
          utility::uintstringconversion
              ::parse<tournament::round_index>(roundToken)
            - 1u;  // zero-based
      }
      catch (const std::invalid_argument &)
      {
        throw InvalidLineException();
      }
      catch (const std::out_of_range &)
      {
        throw InvalidLineException();
      }
    }

    // Target team/player numbers at 24-27, 29-32, ... (empty/000 => all).
    for (std::u32string::size_type col = 23u; col < line.size(); col += 5u)
    {
      const std::u32string target = fieldToken(line, col, 4u);
      if (target.empty())
      {
        break;
      }
      if (isAllZero(target))
      {
        continue;  // "all", so add no explicit target
      }
      try
      {
        result.targets.push_back(
          utility::uintstringconversion
              ::parse<tournament::player_index>(target)
            - 1u);
      }
      catch (const std::invalid_argument &)
      {
        throw InvalidLineException();
      }
      catch (const std::out_of_range &)
      {
        throw InvalidLineException();
      }
    }
    return result;
  }

  long individualAbnormalAdjustment(
    const tournament::Player &player,
    const tournament::Tournament &tournament)
  {
    long adjustment = 0;
    for (
      const tournament::AbnormalAssignment &a : tournament.abnormalAssignments)
    {
      if (a.type != U' ' || !a.hasOtherPoints)
      {
        continue;
      }
      if (!a.targets.empty()
            && std::find(a.targets.begin(), a.targets.end(), player.id)
                 == a.targets.end())
      {
        continue;
      }
      if (a.allRounds)
      {
        adjustment +=
          a.otherPoints * static_cast<long>(tournament.playedRounds);
      }
      else if (a.round < tournament.playedRounds)
      {
        adjustment += a.otherPoints;
      }
    }
    return adjustment;
  }
}
