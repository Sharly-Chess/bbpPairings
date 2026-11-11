// Upper bound of the team edge weight. Round 1 of 28 one-board teams, with an SCS record for
// teams 1 and 2, which are both tops and cannot meet anyway. The single bracket is the plain
// split-half pairing 1-15, 16-2, 3-17, ... 14-28, as without the record.
//
// With 28 teams, the soft field put the top bit of the weight bound at the end of a 64-bit word,
// so the matching's shift of the bound by two bits overflowed. The matching then left teams
// unpaired and the engine reported that no pairing existed.
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
