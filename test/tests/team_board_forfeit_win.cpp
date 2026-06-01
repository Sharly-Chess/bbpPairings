// §1.4 / aggregation: a board won by forfeit against an absent opponent ("0000 +") inside a
// real team match counts as a board WIN (worth the game-point win value), NOT as a
// pairing-allocated bye (PAB).
//
// The "0000" undefined opponent collapses to the board player's own id at the Match level, so a
// board forfeit-win ("0000 +") is indistinguishable from a team PAB ("0000 U") by Match fields
// alone -- both are WIN, not-played, opponent==self, participated. The aggregator must
// disambiguate by CONTEXT: if the team has a real opponent that round (found from any board), the
// "0000 +" board is a board forfeit-win scored at the win value; only a team with no real
// opponent at all gets the PAB game points.
//
// This fixture forces the distinction to be observable. It is GP-primary (192 FIDE_TEAM_GP_MP),
// so a team's scoregroup is its game points, and "162  P 0.0" sets the per-board PAB worth ZERO
// game points -- the value the buggy code would wrongly award a forfeit-win board. Round 1
// (3-board teams):
//   TeamA vs TeamB -- A loses boards 1 and 2 but wins board 3 by forfeit ("0000 +", B's board-3
//                     player was a no-show "0000 - Z"). Correct A game points = 0 + 0 + 1.0 = 1.0.
//   TeamC vs TeamD -- C wins, wins, draws = 2.5 ; D = 0.5.
//   TeamE          -- takes the PAB (320 sets it worth 3.0 game points; this also makes TeamE
//                     bye-INELIGIBLE in round 2, so it cannot take a second bye).
//
// Pairing round 2: five teams, so the lowest-scoring bye-eligible team takes the PAB (§3.4.2).
// Correct standings: TeamE 3.0 (ineligible), TeamC 2.5, TeamB 2.0, TeamA 1.0, TeamD 0.5. The
// genuine tail-ender is TeamD (0.5), so TeamD byes -> output ends "4 0".
//
// If the board forfeit-win were mis-scored as a 0.0-point PAB, TeamA would crash from 1.0 to 0.0,
// become the lowest scorer, and wrongly take the bye ("1 0") -- a completely different pairing.
// So the bye line is sensitive to the fix.
void TEST_FUNCTION(const testing::Context &context)
{
  auto output_filename = STRINGIFY(TEST_ID) ".output";
  testing::run(
    context.exe_path.string()
    + " --team "
    + (context.data_folder_path / STRINGIFY(TEST_ID) ".input").string()
    + " -p "
    + output_filename);
  testing::assert_file_content_matches(
    context.data_folder_path / output_filename,
    context.data_folder_path / STRINGIFY(TEST_ID) ".output.expected");
}
