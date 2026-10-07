// Sharly Chess SCS record in a team tournament: colour criteria outrank soft pairs. Type B,
// round 2, 4 teams on 1 match point; Alpha and Bravo had white and Charlie and Delta black. The
// only other pairing, Alpha-Bravo and Charlie-Delta, avoids the soft pairs 1-4 and 2-3 but
// leaves both colour preferences unfulfilled ([C8]), so the pairing stays 4-1, 3-2.
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
