// Sharly Chess SCS record in a team tournament. Round 1, 6 teams, A1, A2 and A3 share a club,
// as do B1 and B2. The split-half pairing is 1-4, 2-5, 3-6; the soft pairs pick the clash-free
// pairing that comes first in the order of §3.6: 1-4, 2-6, 3-5.
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
