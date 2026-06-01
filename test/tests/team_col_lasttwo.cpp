// C.04.6 §1.7.1 Type A "last two played" colour trigger. 6 teams/3 rounds; t1,t2 sweep.
// t1 = W,B,B: CD-1 with the last two played Black -> Type A WHITE preference (a plain CD-1
// gives no preference, so this isolates the trigger). t2 = B,W,B: CD-1, last two W,B -> none.
// Pair {1,2}: only t1 has a preference -> grant White ("1 2"). {3,5}: t3 (B,W,W, last two
// White -> Black pref) vs t5 (none) -> grant t3 Black ("5 3"). {4,6}: both no pref ->
// §4.3.5 lower CD, t6(-1)<t4(+1) -> t6 White ("6 4").
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
