// Upper bound of the team edge weight, without any SCS record. 44 one-board teams after
// round 1 (1-23, 2-24, ... 22-44), where every top won except 21, beaten by 43.
//
// The 2-point group is 1-20, 22 and 43, 22 teams, so it pairs without upfloaters: 1-12, 2-13,
// ... 9-20, 10-22, 11-43. The 0-point group pairs the same way: 21-33, 23-34, ... 32-44.
//
// A bracket of 16 to 30 teams whose largest TPN is 43 made the weight bound overflow, so the
// first bracket could not be paired by itself and the engine moved 21 and 44 up into it.
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
