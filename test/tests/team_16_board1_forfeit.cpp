// C.04.6 art. 1.6.1: "a team is said to have (had) a colour in a match if the match was actually
// played and the player on the first board was scheduled to play with that colour". Both halves
// matter. The match is the unit -- one board going ahead makes it played, however many others
// were forfeited -- and what counts for the team is the colour board 1 was *scheduled*, whether
// or not that board's own game happened.
//
// 4 teams, 2 boards, round 1 played. Team1 (White) meets Team2: board 1 is forfeited on both
// sides (Team1's player absent, "3 w -" against "1 b +") while board 2 is played, so the match
// was played and Team1 had White, Team2 Black. Team3 (White) meets Team4 with both boards drawn.
// Every match is drawn 1-1, so all four teams stand on 1.0 and form a single bracket in round 2,
// with no upfloaters for art. 3.5 to choose. Colour differences are Team1 +1, Team2 -1,
// Team3 +1, Team4 -1, and after one played match none of those is a Type A preference
// (art. 1.7.1 wants |CD| > 1, or the same colour in the last two played matches), so [C8] cannot
// separate the candidates and art. 3.6.2's identifier decides: tops {1,2} either way, and
// sorted tops against sorted bottoms gives {1,3},{2,4}.
//
// Colours then fall to the end of art. 4.3. In each pair the two teams have equal CD, so
// art. 4.3.5 cannot choose; neither round has one of them White and the other Black, so
// art. 4.3.6 cannot; neither has a preference, so art. 4.3.7 cannot. art. 4.3.8 alternates the
// first-team's last played colour: the pairs are level on match points and on game points, so
// art. 4.2.3 makes the smaller TPN the first-team -- Team1 had White, so Team3 takes it; Team2
// had Black, so Team2 takes White.
//
// Counting board 1's own game instead of the match would leave Team1 and Team2 with no played
// match and a colour difference of 0, which changes both the pairs and the colours.
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
