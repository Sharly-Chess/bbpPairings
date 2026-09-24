// C.04.6 art. 1.3: the result of a match is the comparison of the game points its boards
// scored, and a draw needs a point on each side: a match neither team scored in is lost by both.
//
// 6 teams, 2 boards, round 1. Team1 and Team2 lose every board to each other (0-0 over the
// board), Team3 beats Team4 2-0, and Team5 and Team6 draw 1-1. Match points are therefore
// Team3 2, Team5 1, Team6 1, and Team1, Team2 and Team4 0. In round 2 Team3 floats down to
// Team5 and Team6, who have met; one of them meets Team3 and the other floats down to the teams
// on nothing: 5-3, 6-1 and 2-4.
//
// Scoring the 0-0 match as a draw would put Team1 and Team2 on 1 alongside Team5 and Team6,
// pairing 1-3, 2-5 and 6-4 instead.
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
