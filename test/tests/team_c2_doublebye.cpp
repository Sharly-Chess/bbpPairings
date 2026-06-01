// C.04.6 §2.1.2 [C2]: a team that already received a pairing-allocated bye cannot get
// another. 5 one-board teams, 3 rounds. R1: t5 byes; R2: t4 byes. Entering R3 the two
// lowest-scored teams (t4,t5, score 1) have BOTH already had a bye, so §3.4 must skip them
// and assign the bye one scoregroup up: §3.4.4 largest TPN among the eligible lowest (the
// score-2 teams t2,t3) -> t3 byes ("3 0"). The remaining {1,2,4,5} have a unique legal
// pairing {1,5},{2,4}: t1 (W,W) Black pref granted -> "5 1"; {2,4} no prefs, t4 CD-1 <
// t2 CD0 -> t4 White ("4 2").
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
