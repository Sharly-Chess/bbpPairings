// C.04.6 [C4] global minimise-upfloaters. Scores after R1 (1>2, 3=4, 5=6):
// t1=2, {t3,t4,t5,t6}=1, t2=0. Prior meetings {1,2},{3,4},{5,6}.
// t1 cannot meet t2 (rematch), so t1 floats to a 1-pointer and t2 floats to a
// 1-pointer => 2 upfloaters are forced (minimum). The identifier-minimal legal
// pairing (tops {1,2,3}) is {1,4},{2,5},{3,6}. Colours by §4.3.5 (lower CD->White,
// only 1 game played each): 1(+1)/4(-1)->4 W; 2(-1)/5(+1)->2 W; 3(+1)/6(-1)->6 W.
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
