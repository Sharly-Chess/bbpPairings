// TRF-2026 record 250 team acceleration: fictitious match/game points for a team range over
// a round range. Teams 1-2 get +2.0 fictitious MATCH points in round 1 (MP is the primary
// score). So for pairing round 1 their accelerated score is 2 while teams 3-4 are 0, forming
// scoregroups {1,2} and {3,4} -> pairs {1,2},{3,4}, instead of the unaccelerated split-half
// {1,3},{2,4}. The primary fictitious points go into each team accelerations vector (used by
// scoreWithAcceleration for scoregroups). 250 is stripped from the member stream.
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
