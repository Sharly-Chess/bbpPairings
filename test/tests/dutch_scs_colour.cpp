// Sharly Chess SCS record: colour criteria outrank soft pairs. Round 2, 4 players on 0.5,
// 1-3 and 2-4 drew in round 1, so P1 and P2 are due black and P3 and P4 white. The only other
// pairing, 1-2 and 3-4, avoids the soft pairs 1-4 and 2-3 but fails both colour preferences,
// so the pairing stays 4-1, 3-2.
void TEST_FUNCTION(const testing::Context &context)
{
  auto output_filename = STRINGIFY(TEST_ID) ".output";
  testing::run(
    context.exe_path.string()
    + " --dutch "
    + (context.data_folder_path / STRINGIFY(TEST_ID) ".input").string()
    + " -p "
    + output_filename);
  testing::assert_file_content_matches(
    context.data_folder_path / output_filename,
    context.data_folder_path / STRINGIFY(TEST_ID) ".output.expected");
}
