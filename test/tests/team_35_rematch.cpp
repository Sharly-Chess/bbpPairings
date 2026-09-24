// Article 3.5 of C.04.2, the General Handling Rules: "two paired participants, who did not play
// their game or match, may be paired together in a future round". [C1] (art. 2.1.1) bars two
// teams that *played* from meeting again, and a match they were paired for and forfeited outright
// is not that -- which is how the individual engine has always read it.
//
// Two teams, one board, two rounds. Round 1 pairs them and the match is forfeited on both sides
// ("2 w +" against "1 b -"), so no board was played. Round 2 has nobody else to offer either of
// them, so either they meet again or the round cannot be paired at all.
//
// Counting the forfeited round as a meeting leaves the bracket with no legal pairing and the
// engine refuses the round.
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
