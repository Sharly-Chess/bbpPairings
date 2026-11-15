// TRF-2026 record 299, blank type = free penalty/bonus points. Its game points count towards
// the team's score in the match. R1, two boards: team 1 beats team 2 on board 1 and loses on
// board 2, 1-1 over the board, while team 3 beats team 4 2-0. A "299" with blank type, -1.0
// game point, round 1, target team 1 makes team 1's score 0, so team 2 wins the match (2.0 MP)
// and team 1 loses it (0.0 MP); counted as a draw, both would have 1.0. The 2-pointers {2,3}
// and the 0-pointers {1,4} are paired among themselves. Colours: t2 (B) against t3 (W) and
// t4 (B) against t1 (W) follow their preferences -> 2 W, 4 W.
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
