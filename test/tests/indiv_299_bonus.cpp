// TRF-2026 record 299, blank type, for an INDIVIDUAL tournament (--dutch). A blank-type 299
// with +2.0 points, round 1, target player 4 is a standings bonus. The 001 Points field for
// player 4 lists 2.0 (= 0 game points + 2.0 bonus); validateScores must accept this (it would
// previously throw "score does not match the game results"). The bonus puts player 4 in the
// score-2 group, so after R1 (1>2, 3>4, 5 PAB) the unique lowest is player 2 and the round-2
// bye goes to p2 (it would otherwise go to p4, the larger-TPN 0-pointer).
void TEST_FUNCTION(const testing::Context &context)
{
  auto output_filename = STRINGIFY(TEST_ID) ".output";
  testing::run(
    context.exe_path.string()
    + " --dutch "
    + (context.data_folder_path / STRINGIFY(TEST_ID) ".input").string()
    + " -p "
    + output_filename);
  testing::assert_file_content_matches(
    context.data_folder_path / output_filename,
    context.data_folder_path / STRINGIFY(TEST_ID) ".output.expected");
}
