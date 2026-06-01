// C.04.6 §1.7.2 Type B + §4.3.4 grant the only strong preference. 6 teams, 3 rounds;
// t1,t2 sweep wins (never meet) and form the R4 top pair. Colour histories:
//   t1 W,W,W  -> CD+3 -> STRONG Black;   t2 W,W,B -> CD+1 -> mild Black.
// Pair {1,2}: both prefer Black (not opposite); only t1 is strong -> §4.3.4 grants t1
// Black, t2 White ("2 1"). Pair {3,5}: t3 (B,W,W CD+1 mild Black) vs t5 (W,B,B CD-1 mild
// White) are opposite -> §4.3.3 both granted -> t5 White ("5 3"). Pair {4,6}: t4 (B,B,B
// CD-3 STRONG White) vs t6 (B,B,W CD-1 mild White), both White, only t4 strong -> §4.3.4
// grants t4 White ("4 6"). Confirms a strong pref overrides §4.3.5 lower-CD.
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
