// A team that follows the default order of its 310 roster with a hole needs no 300 record
// (TRF-2026: "1 3 5 6 follows the default order"). Team1 registers Player01, Player02 and
// Player03 for two boards. In round 1 Player01 rests ("0000 - Z"), so Player02 sits on board 1
// with White and Player03 on board 2. Team1 beats Team3 and Team2 (White) beats Team4, both 2-0.
//
// Both top teams played White and want Black. They are level on match and game points, so Team1
// is the first-team by TPN (§4.2.3) and gets its preference (§4.3.7): 2-1. Team3 played Black
// and Team4 Black too (Player08 on board 1), both want White, and Team3 by TPN: 3-4.
//
// Reading Player01, who did not play, as board 1 left Team1 without a colour, so Team2's
// preference was granted (§4.3.2): 1-2.
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
