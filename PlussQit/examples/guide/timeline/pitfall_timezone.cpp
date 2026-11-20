// A wall-clock time is not an instant until it has a zone, and only some zones are available. Both mistakes are reported, not guessed.
#include <pluss/pluss.h>

#include <iostream>

int main() {
  auto local = pluss::Timeline::from_ymdhms(pluss::make_series(2026), pluss::make_series(3), pluss::make_series(29), pluss::make_series(1),
                                           pluss::make_series(30), pluss::make_series(0));
  // 1. 01:30 on 2026-03-29 never happens in London: the clocks jump from 01:00 to 02:00. The default is to refuse.
  try {
    std::cout << pluss::note::print(local.tz_localize("Europe/London").strftime("%H:%M")) << "\n";
  } catch (const pluss::Error &e) {
    std::cout << "error: " << e.message() << "\n";
  }
  // 2. Say what to do instead: shift forward past the gap.
  std::cout << "with nonexistent=\"shift_forward\": " << pluss::note::print(local.tz_localize("Europe/London", 0, "shift_forward").strftime("%H:%M")) << "\n";
  // 3. A zone this build does not know is named in the error.
  try {
    (void)local.tz_localize("Asia/Tokyo");
  } catch (const pluss::Error &e) {
    std::cout << "error: " << e.message() << "\n";
  }
}
