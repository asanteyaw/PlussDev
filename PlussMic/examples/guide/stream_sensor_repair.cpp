// Repair one sensor stream: carry the last good value over gaps, clip impossible readings, keep a trail of what was done in the
// stream's metadata, and keep only the readings you trust.
#include <pluss/pluss.h>

#include <iostream>

using pluss::kindle::Stream;

static void show(const char *label, const Stream &s) { std::cout << label << ": " << pluss::series::print(s.as_series()) << "\n"; }

int main() {
  // Eight temperature readings with two dropouts and one impossible spike. The mask says which readings are present.
  auto values = pluss::make_series({20.5, 20.7, 0.0, 21.0, 450.0, 21.3, 0.0, 21.6});
  auto present = pluss::make_series({true, true, false, true, true, true, false, true});
  Stream raw(values, present);
  raw = raw.with_meta("sensor", "roof-3").with_meta("unit", "celsius");
  std::cout << "raw: " << raw.nrecords() << " records, " << raw.isnull().as_series().sum().item<double>() << " missing, meta sensor=" << raw.meta("sensor").value_or("?") << "\n";

  // 1. Cap impossible values first: a room reading above 60 is a fault, so clip it to the limit.
  auto capped = raw.clip(0.0, 60.0);
  show("clipped", capped);

  // 2. Fill the dropouts with the last good value.
  auto filled = capped.ffill();
  show("forward-filled", filled);

  // 3. Metadata travels with the values: every step above kept the sensor and the unit. Add a note of what was done, so the repaired
  //    stream explains itself later. A flag derived from the stream (a comparison, isnull) is a different quantity and does not inherit it.
  std::cout << "metadata after repair: sensor=" << filled.meta("sensor").value_or("(missing)") << ", unit=" << filled.meta("unit").value_or("(missing)") << "\n";
  auto documented = filled.with_meta("repair", "clip 0-60, ffill");
  std::cout << "with the note: sensor=" << documented.meta("sensor").value_or("?") << ", unit=" << documented.meta("unit").value_or("?") << ", repair=" << documented.meta("repair").value_or("?") << "\n";
  std::cout << "a comparison carries no unit: " << (!(raw > 30.0).meta("unit").has_value() ? "yes" : "no") << "\n";

  // 4. Keep only the trusted readings: those that were present and not above the limit.
  auto trusted_mask = raw.notnull().as_series().logical_and(raw.as_series().lt(60.0));
  auto trusted = raw.filter(trusted_mask);
  show("trusted readings only", trusted);
  std::cout << "trusted " << trusted.nrecords() << " of " << raw.nrecords() << "; mean " << trusted.mean().as_series().items<double>()[0] << "\n";
}
