// C.04.6 [C4] is per bracket, not per round: art. 3.5.2 fixes the number of upfloaters at the
// fewest the bracket's parity allows, and [C5] then prefers the highest-scoring candidates.
// Scores after R1 (1>2, 3=4, 5=6): t1=2, {t3,t4,t5,t6}=1, t2=0. Prior meetings {1,2},{3,4},{5,6}.
// Bracket 1 is t1 alone, so it takes one upfloater: [C5] prefers the 1-pointers to t2, and
// art. 3.5.4 orders the sets {3}<{4}<{5}<{6}. {3} is legal (t1 has met only t2) and leaves
// {4,5,6,2} pairable with [C6] satisfied, so t1 meets t3. Bracket 2 is {4,5,6} with t2 as its
// upfloater, and its identifier-minimal pairing (art. 3.6.2) is {4,6},{5,2}. [C7] is inert in
// the last two rounds. Colours: t1 and t3 are both CD +1 with no Type A preference, so
// art. 4.3.8 alternates the first-team t1 from its White -> 3 W; t4/t6 likewise -> 4 W;
// t2 (-1) against t5 (+1) by art. 4.3.5, lower CD -> 2 W.
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
