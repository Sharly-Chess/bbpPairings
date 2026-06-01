#ifndef TEAMTRF_H
#define TEAMTRF_H

#include <istream>

#include <tournament/tournament.h>

#ifndef OMIT_TEAM
namespace fileformats
{
  namespace teamtrf
  {
    /**
     * Read a TRF-2026 team tournament and return a tournament whose "players"
     * are the teams, ready for the team-pairing engine.
     *
     * The team-only records (310/013 rosters, the FIDE_TEAM_* 192 code, 362
     * match-point system, 352 board colours, 320 team PAB, 299) are extracted
     * here; everything else (the member 001 records and their game results, 162
     * game-point system, 152 initial colour, 142 rounds, ...) is delegated to
     * the existing individual reader fileformats::trf::readFile, then the member
     * results are aggregated per team. See docs/team-pairing.md §6.
     */
    tournament::Tournament readFile(std::istream &, bool useRank);
  }
}
#endif

#endif
