// C.04.6 §3.4 pairing-allocated bye. Round 1, 7 one-board teams, all equal score.
// PAB rule: §3.4.2 lowest score (all tied) -> §3.4.3 most matches played (all 0) ->
// §3.4.4 largest TPN -> team 7 gets the bye. Remaining 6 pair split-half as in
// team_split6.
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
