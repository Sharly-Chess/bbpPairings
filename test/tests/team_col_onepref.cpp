// C.04.6 §4.3.2 grant the only preference (Type A). Top pair {1,2}: t1 played W,W
// (CD+2 -> Black preference, §1.7.1); t2 played W,B (CD0 -> no Type A preference). Only t1
// has a preference -> grant it: t1 Black, t2 White ("2 1"). Bottom {3,4}: only t4 (B,B,
// CD-2) has a preference (White) -> t4 White ("4 3").
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
