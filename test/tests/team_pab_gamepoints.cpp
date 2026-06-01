// §1.4 PAB game points are applied. GP-primary tournament (192 FIDE_TEAM_GP_MP), so a team scoregroup
// is its game points. R1: 1>2, 3>4, team 5 takes the PAB. A 320 record sets the PAB worth 3.0
// game points, so team 5 is the sole top scoregroup; team 4 byes and the pairing is {1,3},{2,5}
// with team 5 listed at the top. If the PAB game points were dropped (treated as 0) team 5 would
// be a bottom team and the output ordering would differ -- so this is sensitive to the PAB game
// points being honoured. (The partner set is identical either way: [C4] keeps the two 2-pointers
// 1-3 together and floats team 5 to the lone 0; the PAB score shifts only its standing.)
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
