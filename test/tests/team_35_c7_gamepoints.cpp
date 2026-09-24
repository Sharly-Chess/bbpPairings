// C.04.6 [C7] (art. 2.3.4) minimises the upfloaters that floated in the previous round, and art.
// 1.5 makes a floater a team that played an opponent with a different score. Deciding it needs
// each team's score as it stood *before* that round, and a team's total is in whichever basis the
// competition made primary -- so the rounds have to be summed, not subtracted from the total,
// because the points of a match are match points whatever the primary score is.
//
// 16 teams, 6 boards, game points the primary score, the fourth round to pair. Team3 stands alone
// on 9.0 and takes one upfloater from the three teams on 8.5: Teams 7, 9 and 14. In round 3
// Team7 met Team15 with 3.5 against 3.0 and floated; Team14 met Team16 with 4.0 against 4.0 and
// did not. [C7] therefore takes Team14, although art. 3.5.4 would otherwise prefer Team7 for its
// smaller pairing number.
//
// Recovering the earlier scores by subtracting match points from a game-point total makes almost
// every pairing look like a float, [C7] stops discriminating, and the bracket takes Team7.
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
