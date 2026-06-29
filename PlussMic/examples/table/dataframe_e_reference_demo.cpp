#include <iostream>

#include <pluss/pluss.h>

int main() {
  using pluss::make_note;
  using pluss::make_series;
  using pluss::table::DataFrame;

  auto scores = DataFrame({
      {"quiz", make_series({84.0, 90.0, 84.0})},
      {"final", make_series({84.0, 88.0, 72.0})},
  }, make_note({"s1", "s2", "s3"}));

  auto scalar_mask = scores.eq(84.0);

  auto official = DataFrame({
      {"quiz", make_series({84.0, 90.0, 84.0})},
      {"final", make_series({84.0, 88.0, 72.0})},
  }, make_note({"s1", "s2", "s3"}));

  auto scratch_same = DataFrame({
      {"quiz", make_series({84.0, 90.0, 84.0})},
      {"final", make_series({84.0, 88.0, 72.0})},
  }, make_note({"s1", "s2", "s3"}));

  auto scratch_changed = DataFrame({
      {"quiz", make_series({84.0, 90.0, 83.0})},
      {"final", make_series({84.0, 88.0, 72.0})},
  }, make_note({"s1", "s2", "s3"}));

  auto frame_mask = official.eq(scratch_changed);

  std::cout << "EQ_SCALAR\n";
  std::cout << scalar_mask.to_string() << "\n\n";

  std::cout << "EQ_FRAME\n";
  std::cout << frame_mask.to_string() << "\n\n";

  std::cout << "EQUALS_SAME\n";
  std::cout << (official.equals(scratch_same) ? "true" : "false") << "\n\n";

  std::cout << "EQUALS_CHANGED\n";
  std::cout << (official.equals(scratch_changed) ? "true" : "false") << "\n";

  return 0;
}
