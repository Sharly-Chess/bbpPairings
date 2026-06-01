#ifndef TEAMPAIRING_H
#define TEAMPAIRING_H

#include <list>
#include <ostream>
#include <utility>

#include <matching/computer.h>
#include <tournament/tournament.h>
#include <utility/dynamicuint.h>

#include "common.h"

#ifndef OMIT_TEAM
namespace tournament
{
  struct Tournament;
}

namespace swisssystems
{
  namespace teampairing
  {
    // The colour-preference model and configuration types live on the
    // Tournament (defined in tournament.h, swisssystems namespace) so the
    // reader can populate them; alias them here for brevity.
    using swisssystems::ColourType;
    using swisssystems::ScoreChoice;
    using swisssystems::TeamConfig;

    // Same dynamic-width edge weight machinery used by the Dutch engine; reused
    // for the lexicographic bracket selection of §3.6 (see option (a) in
    // docs/team-pairing.md §4).
    typedef matching::Computer<utility::uinttypes::DynamicUint>
      matching_computer;

    std::list<Pairing> computeMatching(
      tournament::Tournament &&,
      std::ostream *const = nullptr);

    struct TeamInfo final : public Info
    {
      std::list<Pairing> computeMatching(
        tournament::Tournament &&tournament,
        std::ostream *const ostream
      ) const override
      {
        return teampairing::computeMatching(std::move(tournament), ostream);
      }
    };
  }
}
#endif

#endif
