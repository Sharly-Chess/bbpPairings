// TRF-2026 record 299, blank type = free penalty/bonus points. A "299" with blank type,
// +3.0 match points, round 1, target team 4 adds 3.0 MP to team 4 in round 1. After R1
// (1>2, 3>4, 5 PAB-bye) the un-adjusted scores are t1=2,t3=2,t5=1,t2=0,t4=0; with the bonus
// t4=3, so the unique lowest score is t2 and the round-2 bye moves to t2 (it would otherwise
// go to t4, the larger-TPN of the two 0-pointers). t4 is then a bracket of one and takes one
// upfloater, which [C5] draws from the 2-pointers and art. 3.5.4 settles at {1}, leaving {3,5}:
// the pairs are {4,1},{5,3}. Colours: t4 (-1) against t1 (+1) by art. 4.3.5, lower CD -> 4 W;
// t5 (PAB, CD 0) against t3 (+1) likewise -> 5 W.
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
