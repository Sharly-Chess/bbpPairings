// C.04.6 art. 4.3.6 alternates the colours to the most recent round in which one team had
// White and the other Black; the colour preferences are for 4.3.2 to 4.3.4 and 4.3.7 alone.
//
// Round 4, 192 FIDE_TEAM_GP (no colour preference type). Teams 3 and 4 are paired, both on a
// colour difference of +1: team 3 played one match, with White, between two byes; team 4 had
// Black, White and White. No round has one with White and the other Black, so 4.3.6 decides
// nothing, neither team has a preference for 4.3.7, and 4.3.8 alternates the first-team's colour
// from its last played round: team 3, first on 7 game points to 6, had White, so it takes Black.
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
