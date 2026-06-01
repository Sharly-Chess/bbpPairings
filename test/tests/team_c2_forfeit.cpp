// C.04.6 §2.1.2 [C2]: a team that won a match by forfeit shall not receive the PAB. R1: team1
// wins by forfeit vs team2; 3=4 draw; 5 takes the PAB. Two 299 records then drop team1 to the
// lowest score and lift team2, so team1 (a forfeit winner) is the unique lowest -- the natural
// PAB candidate. C2 must skip it: the bye goes to team4, and team1 is instead paired (1-3).
// Without the forfeit-win exclusion the bye would wrongly go to team1. (Same shared
// eligibleForBye rule applies to individuals: a win scored in an unplayed round blocks the PAB.)
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
