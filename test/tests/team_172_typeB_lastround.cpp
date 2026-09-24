// C.04.6 art. 1.7.2: a team has a *strong* Type B preference for Black if its colour difference
// is more than +1, or, being it 0 or +1, it had White in the last two played matches. The fifth
// paragraph gives a team no preference "when its CD is zero when pairing for the last round" --
// which is about the mild preferences, and is why the third and fourth paragraphs carry the
// condition themselves ("if it is zero and it is not the last round"). The strong ones are worded
// as the Type A preferences and are not qualified by the round.
//
// 18 teams, Type B on game points, the seventh and last round to pair. Team4 has a colour
// difference of 0 and played White in each of its last two matches, so its preference for Black
// is strong; Team5's difference of +1 makes its own preference for Black mild. Art. 4.3.4 grants
// the only strong preference of a pair, so Team4 takes Black and Team5 White.
//
// Applying the fifth paragraph ahead of the first two leaves Team4 with no preference at all, and
// the pair comes out the other way up.
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
