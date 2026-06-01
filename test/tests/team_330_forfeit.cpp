// TRF-2026 record 330: whole-team forfeit. Team B (2) does not show up in round 1, so team
// A (1) records a forfeit win against an undefined "0000" opponent and team B has only an
// absence marker -- the 001 data alone cannot link A to B. The "330 +- 001 001 002" record
// supplies that link and the result. With it, A is scored as a forfeit WIN (TW=2, not a PAB
// bye), so A shares the top scoregroup with C and round 2 pairs {1,3},{2,4}; it also records
// the A-B meeting so they are not re-paired.
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
