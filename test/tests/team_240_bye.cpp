// TRF-2026 record 240: a team-requested bye (full/half/zero-point). For teams F and H byes
// are mandatory in 240. Here team 5 requests a half-point bye for round 1, so it is excluded
// from the round-1 pairing: the remaining 4 teams pair split-half {1,3},{2,4} and team 5 does
// not appear (a declared bye, not a pairing-allocated bye). Without 240 handling the field
// would be odd and team 5 would be wrongly pulled into the pairing / given the PAB. (240 is
// stripped from the member stream so it is not misapplied to a member player.)
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
