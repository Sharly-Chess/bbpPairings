// C.04.6 art. 1.3: the result of a match is the comparison of the game points its boards
// scored. A board decided by forfeit still scores as a board, so a match in which every board
// was forfeited -- one of them the other way -- is not a team forfeit, and its result is still
// the game points. Only a match every board of which went the same way without being played is
// the team forfeit of art. 2.1.2.
//
// 4 teams, 3 boards, round 1. Team1 meets Team2 and nothing is played over the board: Team1
// forfeit-wins board 1 and forfeit-loses boards 2 and 3, so the game points are 1-2 and Team1
// loses the match. Team3 beats Team4 3-0 over the board. Match points are therefore Team2 2,
// Team3 2, Team1 0, Team4 0, and round 2 pairs the two winners and the two losers: Team2 has
// met only Team1 and Team3 only Team4, so {2,3} and {1,4} are legal.
//
// Colours: Team1 and Team2 played no board, so art. 1.6.1 leaves them without a colour for that
// round and their CD at 0, while Team3 had White and Team4 Black. In each pair the two are level
// on match points, neither has a Type A preference after one round, and art. 4.3.5 gives White
// to the lower colour difference -- Team2 (0) against Team3 (+1), and Team4 (-1) against Team1.
//
// Reading the match result off board 1 instead would make Team1 the winner of a match it lost,
// putting it in the top scoregroup and pairing {1,3} and {2,4}.
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
