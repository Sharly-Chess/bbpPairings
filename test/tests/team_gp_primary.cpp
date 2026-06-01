// C.04.6 §1.2 GP-primary configuration (192 FIDE_TEAM_GP_MP): the PRIMARY score is game
// points, match points the secondary. 6 teams, 2 boards, 2 rounds engineered so game-point
// totals are T1=3,T2=3,T6=2,T3=1.5,T5=1.5,T4=1 while match points differ (T1=4,T2=3,...).
// Under GP-primary, T1 and T2 share the top scoregroup (GP 3) and pair each other ("1 2");
// an MP-primary engine would have separated them. Remaining: {3,5} same GP 1.5 -> t5 (CD+2)
// Black pref granted -> "3 5"; {4,6} -> t4 (CD-2) White pref -> "4 6".
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
