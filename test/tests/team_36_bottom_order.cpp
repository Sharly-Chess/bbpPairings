// C.04.6 art. 3.6.2 identifies a pairing by the pairing numbers of its top members in ascending
// order followed by the bottom member of each corresponding pair, and art. 3.6.3 sorts the
// identifiers lexicographically. The bottom of the smallest top therefore decides before the
// bottom of the next.
//
// 15 teams, Type B on game points, the second round to pair. The bracket on the second score
// level is Teams 2, 3, 4, 6, 7 and 9, whose colour preferences leave every candidate pairing with
// one preference unfulfilled, so [C8] cannot separate them and the identifier decides. Of the two
// the matching weighs against each other,
//
//     2-7 3-6 4-9   ->  2 3 4 7 6 9
//     2-6 3-9 4-7   ->  2 3 4 6 9 7
//
// the second is the smaller and is the one art. 3.6.4 takes.
//
// Ordering the bottoms by the largest sum of topRank times botRank instead picks the first: that
// sum is greatest for the assignment pairing sorted tops with sorted bottoms, which is the
// smallest identifier only while nothing rules that assignment out, and here [C8] rules it out.
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
