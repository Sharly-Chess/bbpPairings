// Record 300 names board 1 of a past round. Four teams of two boards after round 1: 1-3 (Team1
// White on board 1, so Player01 White and Player02 Black) and 4-2 (Team4 White), won 2-0 by
// Team1 and Team2. The 310 roster of Team1 lists Player02 first, as Sharly Chess writes it when
// the lineup of the round to pair has changed, but the 300 record of round 1 gives Player01 on
// board 1.
//
// Team1 played White and wants Black, Team2 played Black and wants White, so the top bracket is
// 2-1 (§4.3.3). Team3 played Black and Team4 White, so the bottom one is 3-4.
//
// Reading board 1 from the roster took Player02's Black for Team1, so both top teams wanted
// White and the engine gave 1-2.
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
