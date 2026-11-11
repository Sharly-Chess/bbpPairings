// Record 300 with board 1 vacant (TRF-2026: the record is required then, with 0000 for the
// board). Four teams of three boards (352 WBW). In round 1 Team1 has White against Team3 and
// leaves board 1 vacant: Player02 sits on board 2 with Black and Player03 on board 3 with White,
// and Team1 wins 2-1 (board 1 lost by forfeit). Team2 wins 2-1 against Team4 with Black.
//
// Team1's colour that round is board 1's, White, which is the other colour from the first
// occupied board (board 2) by record 352. Team1 and Team2 are level on match and game points,
// so Team1 is the first-team by TPN (§4.2.3). Team1 wants Black and Team2 White, so 2-1
// (§4.3.3). Team3 played Black and Team4 White, so 3-4.
//
// Taking board 2's Black for the team made both top teams want White, and the first-team got it:
// 1-2.
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
