// C.04.6 art. 2.1.2 [C2]: a team that won a match by forfeit shall not receive the PAB. R1: team1
// wins by forfeit vs team2; 3=4 draw; 5 takes the PAB. Two 299 records then drop team1 to the
// lowest score and lift team2, so team1 (a forfeit winner) is the unique lowest -- the natural
// PAB candidate. C2 must skip it: the bye goes to team4, the larger TPN of the 1-pointers that
// played. Without the forfeit-win exclusion the bye would wrongly go to team1. (Same shared
// eligibleForBye rule applies to individuals: a win scored in an unplayed round blocks the PAB.)
// team1 is paired instead: bracket 1 is team2 alone and takes one upfloater, which [C5] draws
// from the 1-pointers and art. 3.5.4 settles at {3}, leaving {5,1}. Colours: t2 (-1) against
// t3 (+1) by art. 4.3.5 -> 2 W; neither t5 (PAB) nor t1 (a forfeit, so an unplayed match) has a
// colour to count under art. 1.6.1, so art. 4.3.1 gives the first-team t5 the initial colour on
// its odd TPN -> 5 W.
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
