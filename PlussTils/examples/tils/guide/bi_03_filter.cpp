#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  std::cout << std::boolalpha;
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  namespace bi = pluss::bi;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}, {3}, cfg))},
  });
  const pluss::bundle::Nexus nexus({{"sales", sales}});

  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").title("Sales overview").grid(1, 2);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).title("Rows").place(0, 0, 1, 2));

  // Rendering with no filter shows every row.
  const auto before = studio.render();
  std::cout << "rows with no filter: " << before.boards.front().tiles.front().table->height() << '\n';

  // A filter on the session narrows the rows that the view shows.
  studio.set_filter(bi::Filter{
      .key = "region_is_north",
      .feature = "region",
      .op = bi::FilterOp::Eq,
      .payload = bi::FilterScalar{std::string("north")},
  });
  const auto after = studio.render();
  std::cout << "rows with north filter: " << after.boards.front().tiles.front().table->height() << '\n';
  std::cout << "session filter count: " << studio.session().filters().size() << '\n';

  // Clearing the filter restores the full table.
  studio.clear_filter("region_is_north");
  std::cout << "rows after clearing: " << studio.render().boards.front().tiles.front().table->height() << '\n';
}
