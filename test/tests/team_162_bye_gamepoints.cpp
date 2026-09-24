// Record 162 of TRF26 is the "scoring point system for individuals", and says of itself that it
// is "valid also for game points in team competitions". Its defaults give a full-point bye the
// value of a win and a half-point bye that of a draw, per board; a zero-point bye is nil.
//
// A bye declared by a 240 record has no board entries to sum, so the round's game points have to
// come from those defaults. 7 teams, 2 boards, game points the primary score (192
// FIDE_TEAM_TYPEA_GP). Round 1: Team1, Team3 and Team5 each win 2-0, and Team7 takes a
// half-point bye, which is worth 2 x 0.5 = 1.0 game points. The standings are therefore
// 2.0 for Teams 1, 3 and 5, 1.0 for Team7, and 0.0 for Teams 2, 4 and 6.
//
// Round 2 byes Team6 (art. 3.4: the lowest score, then the most matches played, then the largest
// TPN). The three teams on 2.0 are an odd bracket and take one upfloater, and [C5] makes it the
// highest-scoring candidate, Team7 on 1.0 -- not one of the teams on 0.0. The bracket
// {1,3,5,7} pairs 1-5 and 3-7 by its identifier, and Teams 2 and 4 are left to each other.
//
// Scoring the bye at nothing would put Team7 on 0.0 among Teams 2 and 4, where it would be the
// upfloater instead of one of them and the round would come out 3-1, 2-5, 4-7.
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
