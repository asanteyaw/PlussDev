// Reconcile two measurements of the same stations: give each column its station names as labels, align one to the other by name,
// and see what is missing on either side.
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::AlignBy;
using pluss::table::Shelf;

static void show(const char *label, const Shelf &s) { std::cout << label << "\n" << s.to_string() << "\n"; }

int main() {
  // Today's readings, in the order the stations reported.
  Shelf today(pluss::make_series({14.2, 11.8, 15.0, 9.5}));
  auto today_by_name = today.label().set(Shelf(pluss::make_note({"north", "east", "south", "hill"})));
  // Yesterday's readings, in another order, and without "hill" but with a station that has since closed.
  Shelf yesterday(pluss::make_series({15.1, 13.0, 12.2, 10.0}));
  auto yesterday_by_name = yesterday.label().set(Shelf(pluss::make_note({"south", "north", "east", "dock"})));
  show("today", today_by_name);
  show("yesterday", yesterday_by_name);

  // 1. Align yesterday to today's stations by NAME, not position. Stations that did not exist yesterday become NULL.
  pluss::table::AlignOptions by_name;
  by_name.align = AlignBy::Caselbl;
  auto aligned = yesterday_by_name.label().align(today_by_name.label().values(), by_name);
  show("yesterday, in today's order", aligned);

  // 2. The change per station is a plain subtraction now that the rows correspond; NULL propagates.
  auto change = today_by_name - aligned;
  show("change since yesterday", change);

  // 3. Which stations have no previous reading, and which of the old stations are gone?
  std::cout << "no previous reading: " << change.isnull().sum().to_dbl() << " station(s)\n";
  auto gone = yesterday_by_name.label().find(Shelf(pluss::make_note({"dock"})));
  show("position of the closed station in yesterday's list", gone);

  // 4. Order by station name for a report.
  show("report, sorted by name", change.label().sort());
}
