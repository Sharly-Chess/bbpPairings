// TRF-2026 record 162: the absence (zero-point-bye / forfeit-loss) result symbol is "A"
// (TRF(x)/JaVaFo used "Z"). This fixture has a 162 line written with the 2026 "A" symbol;
// it must parse (it previously threw "Invalid line") and pair normally. 4 teams, round 1,
// so the pairing is the split-half {1,3},{2,4} with §4.3.1 colours.
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
