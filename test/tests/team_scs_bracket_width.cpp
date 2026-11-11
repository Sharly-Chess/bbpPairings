// Upper bound of the team edge weight, inside one bracket. 18 one-board teams after round 1
// (1-10, 2-11, ... 9-18), where every top won except 9, beaten by 18. An SCS record for teams
// 1 and 2 changes nothing, since no pairing of the top bracket puts them together.
//
// The 2-point group is 1-8 and 18, nine teams, so §3.5 takes one upfloater, the first in TPN
// order: 9 (it played 18 but not the others). §3.6 pairs {1..9, 18} as 1-6, 2-7, 3-8, 4-9,
// 5-18, and the 0-point group 10-17 as 10-14, 11-15, 12-16, 13-17.
//
// A bracket of 10, 12 or 14 teams whose largest TPN is 18 made the weight bound overflow once
// the soft field was there, so every set of 1, 3 and 5 upfloaters was rejected and the engine
// took seven (9-15).
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
