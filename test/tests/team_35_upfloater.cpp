// C.04.6 art. 3.5 chooses the upfloaters before art. 3.6 pairs the bracket, and the two orders
// are independent: the set comes first in the lexicographic order of its TPNs (art. 3.5.4), and
// only then is the bracket arranged by its identifier (art. 3.6.2). 22 teams, 3 boards, round 1
// played; teams 3 and 18 are out of round 2 (240 records). Eleven teams stand on 2.0, team 8
// alone on 1.0 having taken the PAB (art. 1.4, a draw), ten on 0.0. Team 8's bracket therefore
// takes one upfloater out of the nine 0.0 teams still available. Under Type A none of them has a
// colour preference yet -- after one played match the CD is +-1 with a one-letter colour
// sequence, which art. 1.7.1 leaves without one -- so [C8] cannot choose, and neither can the
// identifier of the bracket the upfloater has not yet joined. art. 3.5.4 takes {12}, the
// smallest TPN that is legal: team 8 has met nobody, the eight teams left on 0.0 have met none
// of each other, so [C6] holds, and nothing floated in round 1, so [C7] is nil. The eight then
// form their own bracket, whose identifier-minimal pairing is {13,19},{14,20},{16,21},{17,22}.
// The field is too large to work the colours through by hand; the expected output is the one
// Gacrux's pairingchecker computes for this round, pair for pair and colour for colour.
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
