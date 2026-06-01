// TRF-2026 record 299, forfeit-loss override (type "-"). "299 -  1.0" rescores every team
// forfeit loss to 1.0 match point instead of the default TL=0. R1: 1>2 OTB, team 3 wins by
// forfeit over team 4, team 5 gets the PAB. Without the override t4=0 and the round-2 bye
// (lowest score, largest TPN) would be t4; the override makes t4=1, so t2 is the unique
// lowest and byes instead. Pairs {3,1},{5,4}; bye t2.
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
