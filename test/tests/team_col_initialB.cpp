// C.04.6 §4.1 / §4.3.1 with a Black initial colour. Round 1, 4 teams, split-half
// {1,3},{2,4}. first-team odd TPN (t1) takes the initial colour Black -> t3 White ("3 1");
// even TPN (t2) takes the opposite (White) -> "2 4".
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
