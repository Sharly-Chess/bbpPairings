// The checker re-pairs each round from the rounds before it, and a team's score is in the
// primary basis its competition chose. Here game points rank first (192 FIDE_TEAM_GP_MP): 5
// teams, 4 rounds, whose order on match points differs from their order on game points.
// Re-paired on game points, every round comes out as it was paired.
void TEST_FUNCTION(const testing::Context &context)
{
  auto output_filename = STRINGIFY(TEST_ID) ".output";
  testing::run(
    context.exe_path.string()
    + " --team "
    + (context.data_folder_path / STRINGIFY(TEST_ID) ".input").string()
    + " -c > "
    + (context.data_folder_path / output_filename).string());
  testing::assert_file_content_matches(
    context.data_folder_path / output_filename,
    context.data_folder_path / STRINGIFY(TEST_ID) ".output.expected");
}
