#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <cstdint>
#include <iostream>
#include <vector>

namespace {

using ExtractSpec = pluss::codem::ExtractSpec;
using ExtractSpecView = pluss::msharp::RangeView::Of<ExtractSpec>;

ExtractSpecView as_specs(const std::vector<ExtractSpec>& specs) {
  return ExtractSpecView(specs);
}

} // namespace

int main() {
  let x = pluss::make_series(
      std::vector<double>{
          10, 11, 12, 13,
          20, 21, 22, 23,
          30, 31, 32, 33},
      {3, 4});
  std::cout << "x:\n" << x << "\n\n";

  const std::vector<ExtractSpec> every_other_col{
      pluss::Slicing::SliceAll(),
      pluss::Slicing::SliceRange(0, 4, 2)};
  std::cout << "traverse_slice(all, 0:4:2):\n"
            << x.traverse_slice(as_specs(every_other_col)) << "\n\n";

  auto match_ge = x.traverse.find_ge(22.0);
  std::cout << "traverse.find_ge(22).coords:\n" << match_ge.coords() << "\n\n";
  std::cout << "traverse.find_ge(22).linear_indices:\n" << match_ge.linear_indices() << "\n\n";
  std::cout << "traverse.find_ge(22).select_from(x):\n" << match_ge.select_from(x) << "\n\n";

  auto mask = x.ge(21.0).logical_and(x.le(31.0));
  auto match_mask = x.traverse.find_all(mask);
  std::cout << "traverse.find_all(mask[21..31]).coords:\n" << match_mask.coords() << "\n\n";
  std::cout << "traverse.find_first(mask[21..31]).coords:\n"
            << x.traverse.find_first(mask).coords() << "\n\n";

  const std::vector<ExtractSpec> tail_cols{
      pluss::Slicing::SliceAll(),
      pluss::Slicing::SliceFrom(1)};
  auto x_tail = x.traverse_slice(as_specs(tail_cols));
  auto tail_match = x.traverse(as_specs(tail_cols)).find_btw(21.0, 32.0, true);
  std::cout << "traverse(on=all,1:).find_btw(21,32).coords:\n"
            << tail_match.coords() << "\n\n";
  std::cout << "traverse(on=all,1:).select_from(x_tail):\n"
            << tail_match.select_from(x_tail) << "\n\n";

  auto writable = x.clone();
  const std::vector<ExtractSpec> middle_cols{
      pluss::Slicing::SliceAll(),
      pluss::Slicing::SliceRange(1, 3, 1)};
  writable.traverse_put_(as_specs(middle_cols), pluss::full({3, 2}, -1.0));
  std::cout << "traverse_put_(all, 1:3) = -1:\n" << writable << "\n\n";

  std::cout << "slice(dim=1, start=1, end=4, step=2):\n"
            << x.slice(1, 1, 4, 2) << "\n\n";
  std::cout << "cube().slice(0, 0, 1):\n"
            << x.cube().slice(0, 0, 1) << "\n";

  auto cube3d = pluss::make_series(
      std::vector<double>{
          0, 1, 2, 3, 4, 5, 6, 7,
          8, 9, 10, 11, 12, 13, 14, 15,
          16, 17, 18, 19, 20, 21, 22, 23},
      {3, 4, 2});
  auto cs_idx = pluss::make_series(std::vector<std::int64_t>{0, 1, 2, 1}, {4});
  auto bdtm_idx = pluss::make_series(std::vector<std::int64_t>{1, 3, 0, 2}, {4});
  const std::vector<ExtractSpec> adv_plus_all{
      cs_idx.to(pluss::ScalarType::Int64),
      bdtm_idx.to(pluss::ScalarType::Int64),
      pluss::Slicing::SliceAll()};
  const auto pair_pick = cube3d.index(as_specs(adv_plus_all));
  std::cout << "\nindex({cs_idx, bdtm_idx, SliceAll}) values:\n"
            << pair_pick << "\n";

  return 0;
}
