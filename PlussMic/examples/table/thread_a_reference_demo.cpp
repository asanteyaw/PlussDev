#include <iostream>
#include <vector>

#include <pluss/pluss.h>

int main() {
  using pluss::ScalarType;
  using pluss::make_note;
  using pluss::make_series;
  using pluss::make_timeline;
  using pluss::table::Thread;

  auto scores = Thread(make_series({-2.5, 0.0, 3.5}));
  auto flags = Thread(make_series(std::vector<bool>{true, false, true}));
  auto names = Thread(make_note({" Ada ", "BOLA", " chen "}));
  auto stamp_values = make_timeline({
      "2024-01-01T09:10:00Z",
      "2024-02-01T11:00:00Z",
      "2024-03-05T15:30:00Z",
  });
  auto stamps = Thread(stamp_values);
  auto ticks = Thread(make_timeline({
      "2024-01-01T00:10:00Z",
      "2024-01-01T00:20:00Z",
      "2024-01-01T01:10:00Z",
      "2024-01-02T00:15:00Z",
  }));
  auto ints = Thread(make_series({1.0, 0.0, 1.0}));

  std::cout << "ABS\n" << scores.abs().to_string() << "\n\n";
  std::cout << "ALL\n" << flags.all().to_str() << "\n\n";
  std::cout << "ANY\n" << flags.any().to_str() << "\n\n";

  std::cout << "AS_NOTE_LOWER\n"
            << Thread(names.as_note().strip().lower()).to_string() << "\n\n";
  std::cout << "AS_SERIES_SHIFT\n"
            << Thread(scores.as_series() + 5.0).to_string() << "\n\n";
  std::cout << "AS_TIMELINE_YEAR\n"
            << Thread(stamps.as_timeline().year()).to_string() << "\n\n";
  std::cout << "AS_TIMELINE_INDEX_RESAMPLE_COUNT\n"
            << Thread(ticks.as_timeline().index(true).resample_count("D", 1)).to_string() << "\n\n";
  std::cout << "ASTYPE_BOOL\n" << ints.astype(ScalarType::Bool).to_string() << "\n";

  return 0;
}
