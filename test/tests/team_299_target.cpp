// TRF-2026 record 299 with an explicit TARGET. Same setup as team_299_all but the
// forfeit-loss override "299 -  1.0  ... 0004" names only team 4. So team 4 (forfeit loss)
// is rescored to 1.0 MP while team 2 (also a forfeit loss) keeps the default TL=0. After R1
// (T1 ff-beats T2, T3 ff-beats T4, T5 PAB): t1=2,t3=2,t5=1,t4=1,t2=0 -> t2 is the unique
// lowest and gets the round-2 bye. Pairs {1,3},{5,4}; bye t2. Contrast with team_299_all,
// which omits the target and rescores BOTH forfeit losses (different bye).
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
