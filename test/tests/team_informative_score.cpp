// TRF-2026: in team competitions the 001 Points field is informative ("over-the-board
// points") and need NOT equal the game results, so the team reader must skip the individual
// score-validation that the individual reader applies. Here every member carries a bogus 9.0
// Points value with no games played (real team score 0). A non-team read would reject this
// ("score does not match the game results"); the team read pairs normally -> split-half.
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
