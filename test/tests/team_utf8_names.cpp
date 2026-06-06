// UTF-8 team names ("Liffré", "Pacé", "Évran") contain multi-byte characters. The TRF column
// positions are CHARACTER positions (the individual reader decodes UTF-8 before parsing), so the
// team reader must count characters too: a byte-based parser would read the 310 member columns
// (74-77, 79-82, ...) shifted by the extra bytes of each accent and corrupt the rosters --
// wrong member->team map => wrong opponents, phantom byes, wrong scores, garbage pairing.
// (Found in the wild: a 30-team event with "Liffré"/"Pacé"/"Vitré" teams paired 3-pointers
// against 1-pointers because ten rosters were misread.)
//
// This fixture is team_42_secondary with accented team names. Team names must not affect the
// pairing, so the expected output is identical to that test's (see team_42_secondary.cpp for
// the C.04.6 derivation). If column handling regresses to bytes, the rosters scramble and the
// output changes.
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
