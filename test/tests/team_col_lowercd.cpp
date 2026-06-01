// C.04.6 §4.3.5 lower CD gets White, isolated via no-colour mode (192 FIDE_TEAM_MP).
// t1,t2 win R1+R2 (top group R3). Colours: t1 W,W (CD+2); t2 W,B (CD0);
// t3 B,W (CD0); t4 B,B (CD-2). Pair {1,2}: t2 (CD0) < t1 (CD+2) -> t2 White.
// Pair {3,4}: t4 (CD-2) < t3 (CD0) -> t4 White.
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
