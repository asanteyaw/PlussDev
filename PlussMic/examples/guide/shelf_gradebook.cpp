// A gradebook as one column: scores with absences, rescue the missing ones, rank the class, find the cut points and assign grade bands.
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::Shelf;

int main() {
  const double absent = std::numeric_limits<double>::quiet_NaN();
  // Ten exam scores; two students did not sit the exam. A Shelf carries a validity mask next to its numbers: true = present.
  auto values = pluss::make_series({72.0, 85.5, absent, 91.0, 64.0, absent, 78.5, 55.0, 88.0, 69.5});
  Shelf raw(values, values.isnan().logical_not());
  std::cout << raw.to_string() << "\n";
  std::cout << "present " << raw.notnull().sum().to_dbl() << " of " << raw.size() << "\n";

  // 1. Summary of the observed scores only.
  std::cout << raw.describe().to_string() << "\n";

  // 2. Quartile cut points and a rank for each student (missing students stay missing).
  std::cout << raw.percentiles({0.25, 0.5, 0.75}).to_string() << "\n";
  std::cout << raw.rank().to_string() << "\n";

  // 3. Grade bands from explicit edges: F below 60, D 60-70, C 70-80, B 80-90, A 90 and above.
  auto bands = raw.bucketize({0.0, 60.0, 70.0, 80.0, 90.0, 100.0});
  std::cout << bands.to_string() << "\n";

  // 4. Absent students sit a make-up exam and score the class median; the table records who was filled.
  auto filled = raw.fillnull(raw.median().to_dbl());
  std::cout << filled.to_string() << "\n";
}
