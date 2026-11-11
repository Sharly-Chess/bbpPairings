// Upfloater search with a forbidden clique over the top scoregroup. Round 3 of a synthetic
// 56-team, 4-board cup with SCS records for same-club teams, and the FFE cup rule as one 260
// record for round 3: the 12 teams that won both of their first matches may not meet. The
// top bracket cannot pair any of its residents together, so it needs 12 upfloaters: the 4
// teams on 3 points and the 8 lowest-numbered of the 22 on 2 points. Each of the 12 meets one
// of them and no two of the 12 meet.
//
// The search went through every number of upfloaters from 0 up, and every set of each, before
// reaching 12: about C(44, 10) sets for 10 alone. It never finished on the real event this
// reproduces. It now starts at the fewest upfloaters that can meet all the residents. The
// expected output is the one the engine gives when only that starting point is changed, which
// skips nothing but numbers with no legal set.
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
