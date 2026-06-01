// C.04.6 §4.3.7 grant the first-team preference. Pair {1,2}: both played W,W
// (CD+2 -> both prefer Black, same direction). §4.3.3 (opposite) does not apply; §4.3.5
// (CD equal); §4.3.6 (no W/B split, both W,W) -> none; §4.3.7 grants first-team t1 Black,
// so t2 White ("2 1"). Bottom {3,4}: both B,B -> both prefer White -> §4.3.7 t3 White ("3 4").
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
