// TRF-2026 record 299, blank type, NEGATIVE penalty (individual --dutch). A "299" with
// points -0.5, round 1, target player 2 deducts half a point. After R1 (all draws + p5 PAB),
// game scores are p5=1, p1=p2=p3=p4=0.5; the penalty makes player 2 0.0 (listed), which the
// reader parses (signed value) and validateScores accepts (0.5 game - 0.5 = 0.0). Player 2 is
// then the unique lowest score and takes the round-2 bye (it would otherwise go to p4, the
// largest-TPN 0.5-pointer). Exercises negative signed parsing + deduction end-to-end.
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
