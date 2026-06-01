// C.04.6 aggregation + §2.1.1 [C1] + §4.3.8. After R1 (T1 beat T2 2-0, T3 drew T4):
// match points T1=2, T3=T4=1, T2=0. Prior meetings {1,2},{3,4} forbid a rematch, so the
// two same-score teams (3,4) cannot pair; the identifier-minimal legal pairing is
// {1,3},{2,4}. Colours: both teams in each pair have CD ties and no preference (1 game),
// so §4.3.8 alternates the first-team last colour: T1 played White -> Black ("3 1");
// T4 (first by score) played Black -> White ("4 2").
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
