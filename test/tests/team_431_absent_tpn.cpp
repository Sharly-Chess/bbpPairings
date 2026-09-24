// C.04.6 art. 4.3.1 gives the first-team the initial-colour when its TPN is odd, and a team
// keeps its TPN whoever is absent (art. 1.1).
//
// Five teams, round 1, initial colour White; team 1 takes a half-point bye. Teams 2, 3, 4 and
// 5 are paired 2-4 and 3-5. The first-team of each pair is the smaller TPN: team 2 is even and
// takes Black, team 3 is odd and takes White, giving 4-2 and 3-5. Numbering the four teams
// present 1 to 4 would invert both.
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
