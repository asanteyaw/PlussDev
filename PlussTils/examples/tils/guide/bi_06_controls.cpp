#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <iostream>
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
  auto& board = studio.board("overview");
  board.title("Sales overview").grid(1, 2);

  // A control and a view are both tiles on the board. The control tile is marked as one.
  board.control(bi::category("region_picker", "region", "Region"));
  board.view(bi::table("rows").query(bi::from("sales")).place(0, 1, 1, 1));
  std::cout << "tiles on the board: " << board.tiles().size() << '\n';

  const auto rendered = studio.render().boards.front();
  std::cout << "first tile is a control: " << rendered.tiles[0].is_control << '\n';
  std::cout << "second tile is a control: " << rendered.tiles[1].is_control << '\n';
  std::cout << "view tile has a table: " << (rendered.tiles[1].table != nullptr) << '\n';
}
