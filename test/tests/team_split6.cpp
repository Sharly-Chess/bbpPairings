// C.04.6 §3.6 + §4.3.1. Round 1, 6 one-board teams, one bracket. Lexicographically
// smallest identifier = split-half: tops {1,2,3}, bottoms {4,5,6}, monotonic ->
// {1,4},{2,5},{3,6}. Colours by TPN parity vs initial White: t1,t3 odd -> White;
// t2 even -> Black ("5 2").
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
