// C.04.6 §3.4 PAB. After R1 (1>2, 3>4, 5 bye): t1=t3=2, t5=1(bye), t2=t4=0.
// Odd field -> bye to the lowest score (0): t2,t4 tie -> §3.4.4 largest TPN -> t4.
// Remaining {1,2,3,5}: pair the two 2-pointers 1-3 (not met), then 5-2.
// Colours §4.3.5: 1(+1)/3(+1) tie ->§4.3.8 alt t1 last W -> 3 W; 5(bye,CD0)/2(-1) -> 2 W.
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
