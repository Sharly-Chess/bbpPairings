// C.04.6 §3.6 + §4.3.1. Round 1, 4 one-board teams, all score 0 (one bracket).
// The lexicographically-smallest pairing identifier (§3.6.2-3) has tops {1,2} and
// bottoms {3,4} paired monotonically -> {1,3},{2,4}.
// Colours (both teams unplayed, §4.3.1): first-team odd TPN gets the initial colour
// (White), even TPN gets the opposite. t1 odd -> White ("1 3"); t2 even -> Black ("4 2").
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
