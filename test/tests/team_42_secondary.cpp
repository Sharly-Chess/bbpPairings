// C.04.6 §4.2.2 first-team determined by the secondary (game points) score when match
// points tie. 4 teams, 2 boards, 2 rounds. t1: 2-0 then 1.5-0.5 -> MP 4, GP 3.5. t2: 2-0
// then 2-0 -> MP 4, GP 4. Equal MP; t2 has higher GP AND the larger TPN, so §4.2.2 makes t2
// the first-team (overriding §4.2.3 which would pick t1). Both board-1 White,White -> CD+2
// strong Black; §4.3.7 grants the first-team (t2) Black -> t1 White ("1 2"). {3,4}: both CD-2
// strong White; t4 GP0.5 > t3 GP0 -> first-team t4 -> §4.3.7 t4 White ("4 3").
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
