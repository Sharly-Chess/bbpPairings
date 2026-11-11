// Upfloater search with a forbidden clique over the top scoregroup and part of the next. Round 2
// of a synthetic 33-team, 4-board event: one 260 record for round 2 over the 14 teams on 2
// points and 2 of the 5 on 1 point. The 14 need 14 upfloaters, each meeting one of them, so no
// profile that takes either of the 2 can be legal. The best profile that can takes the other
// 3 teams on 1 point and 11 on 0 points, and every set of it leaves [C6] unmet: the 2 are all
// that is left of their scoregroup and may not meet. The set chosen is the first in §3.5.4
// order that is legal, and team 32 gets the bye.
//
// sc8 took about 40 s here, pairing every set of the profiles that cannot be legal and every
// set of that profile in search of one that meets [C6]. The search now drops a partial choice
// as soon as no completion of it can be legal or beat the best set found. The expected output
// is sc8's.
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
