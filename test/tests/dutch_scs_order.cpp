// Sharly Chess SCS record: pairs to avoid as the lowest-priority criterion. Round 1, 6 players,
// P2 and P5 share a club, as do P3 and P6. The FIDE pairing is 1-4, 2-5, 3-6; every pairing of
// S1 against S2 satisfies the criteria equally, so the soft pairs pick the transposition that
// keeps both clubs apart: 1-4, 2-6, 3-5.
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
