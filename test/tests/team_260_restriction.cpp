// TRF-2026 record 260: teams that cannot meet (e.g. same-club teams) in a round range.
// "260 001 001 0001 0003" forbids teams 1 and 3 from meeting in round 1. Round 1, 4 teams,
// all score 0: the unrestricted split-half pairing would be {1,3},{2,4}; the restriction
// makes 1-3 illegal, so the identifier-minimal legal pairing is {1,4},{2,3}. Confirms 260
// restrictions are applied to the team tournament (parsed as team pairing numbers).
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
