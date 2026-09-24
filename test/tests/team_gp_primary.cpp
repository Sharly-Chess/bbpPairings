// C.04.6 art. 1.2 GP-primary configuration (192 FIDE_TEAM_GP_MP): the PRIMARY score is game
// points, match points the secondary. 6 teams, 2 boards, 2 rounds engineered so game-point
// totals are T1=3,T2=3,T6=2,T3=1.5,T5=1.5,T4=1 while match points differ (T1=4,T2=3,...).
// Under GP-primary, T1 and T2 share the top scoregroup (GP 3) and pair each other ("1 2");
// an MP-primary engine would have separated them. T6 is then a bracket of one and takes one
// upfloater: [C5] prefers the GP 1.5 pair to T4, and art. 3.5.4 takes {3}, so T6 meets T3 and
// {5,4} is left. Colours: T6 and T3 are both CD 0 with no Type A preference, so art. 4.3.8
// alternates the first-team T6 from its White -> 3 W; T4 (CD -2) and T5 (CD +2) want opposite
// colours, which art. 4.3.3 grants -> 4 W.
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
