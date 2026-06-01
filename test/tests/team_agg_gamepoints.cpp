// C.04.6 §1.2/§362 aggregation: team match result is decided by board game points.
// R1: T1 vs T2 -> 1.5-0.5 (board1 win, board2 draw) => T1 WINS the match (TW), T2 loss.
// T3 vs T4 -> 1-1 (two draws) => match draw (TD each). So MP: T1=2, T3=T4=1, T2=0.
// R2 (3-4 met): pair 1-3, 2-4 (identifier). Colours §4.3.8 (1 game each, CD ties):
// T1 last W -> Black so 3 W; T4 last B -> White.
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
