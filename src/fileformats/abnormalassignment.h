#ifndef ABNORMALASSIGNMENT_H
#define ABNORMALASSIGNMENT_H

#include <string>

#include <tournament/tournament.h>

namespace fileformats
{
  /**
   * Parse a TRF-2026 record 299 (abnormal points assignment) line. Throws
   * fileformats::InvalidLineException on malformed input.
   */
  tournament::AbnormalAssignment readAbnormalAssignment(const std::u32string &);

  /**
   * The total blank-type (penalty/bonus) abnormal-assignment adjustment for an
   * individual player, in tenths (signed). A record with an explicit round
   * contributes once; an all-rounds record contributes once per played round
   * (mirroring the per-round handling in the team aggregator).
   */
  long individualAbnormalAdjustment(
    const tournament::Player &,
    const tournament::Tournament &);
}

#endif
