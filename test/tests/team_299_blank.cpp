// TRF-2026 record 299, blank type = free penalty/bonus points. A "299" with blank type,
// +3.0 match points, round 1, target team 4 adds 3.0 MP to team 4 in round 1. After R1
// (1>2, 3>4, 5 PAB-bye) the un-adjusted scores are t1=2,t3=2,t5=1,t2=0,t4=0; with the bonus
// t4=3, so the unique lowest score is t2 and the round-2 bye moves to t2 (it would otherwise
// go to t4, the larger-TPN of the two 0-pointers). Pairs {3,1},{4,5}; bye t2.
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
