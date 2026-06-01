// C.04.6 §4.3.3 opposite preferences granted (Type A, §1.7.1 absolute CD>1).
// t1 W,W -> CD+2 -> prefers Black; t2 B,B -> CD-2 -> prefers White. Opposite -> both
// granted: t1 Black, t2 White. Pair {3,4}: both CD0 (no pref) -> §4.3.6 alternate the
// most recent W/B split (R2: t3 W, t4 B) -> t3 Black, t4 White.
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
