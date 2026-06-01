// TRF-2026 record 299 with NO target (applies to all teams of the type). The forfeit-loss
// override "299 -  1.0" rescores every team forfeit loss to 1.0 MP. Same R1 as
// team_299_target (T1 ff-beats T2, T3 ff-beats T4, T5 PAB), so both t2 and t4 become 1 MP;
// with no 0-point team, the round-2 bye goes to t4 (largest TPN among the eligible lowest,
// t5 being [C2]-ineligible). Pairs {1,3},{5,2}; bye t4. Demonstrates all-teams vs the
// explicit-target case in team_299_target.
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
