// art. 1.4 PAB game points are applied. GP-primary tournament (192 FIDE_TEAM_GP_MP), so a team
// scoregroup is its game points. R1: 1>2, 3>4, team 5 takes the PAB. A 320 record sets the PAB
// worth 3.0 game points, so team 5 is the sole top scoregroup; team 4 byes. Team 5's bracket
// takes one upfloater, which [C5] draws from the GP 1.0 pair rather than from team 2, and
// art. 3.5.4 settles at {1}: the pairing is {5,1},{2,3}. If the PAB game points were dropped
// (treated as 0) team 5 would be a bottom team, {1,3} would pair each other and team 5 would
// meet team 2 -- so this is sensitive to the PAB game points being honoured. Colours: team 5 has
// no played match, so art. 4.3.1 gives the first-team its initial colour on an odd TPN -> 5 W;
// t2 (-1) against t3 (+1) by art. 4.3.5 -> 2 W.
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
