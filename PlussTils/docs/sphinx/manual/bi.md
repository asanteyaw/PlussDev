# PlussBI: A Guide

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussBI API Reference](../api/bi.md)

PlussBI describes an analytical experience in C++. You name the tables a
dashboard reads from, choose how each presentation shows them, attach controls
that change what is shown, and export the result as a document someone else can
open. The dashboard is data. You can build it in code, save it, load it again
against the same data, and render it as a standalone HTML file, all without a
browser window or a server.

The family has a few moving parts, and this guide introduces them in the order
you meet them. A **session** holds the choices a reader has made, such as a
filter or a parameter. A **source** is the set of named tables a studio reads
from. A **studio** owns boards. A **board** lays out **tiles**, and each tile is
either a **view** that presents data or a **control** that changes the session.
Every program in this guide is complete, and every printed line is copied from
the program's real output.

PlussBI is optional. It does not depend on the loader or the seed families, and
you can read this guide without them.

## The chapters

This landing page introduces the family with its first programs. Each chapter below covers one part of the
family in full, with an example for each overload and each toggle it teaches.

- [Sessions and parameters](bi-sessions.md) -- the versioned choices a reader makes, and the named values views read.
- [Filters](bi-filters.md) -- every filter factory, its payload types, and what a refusal looks like.
- [Views](bi-views.md) -- the eight constructors, every view member, and the view kinds.
- [Boards, tiles, and layout](bi-boards.md) -- boards, tiles, grids, and placement.
- [Controls](bi-controls.md) -- category, range, and time-range controls.
- [Studios](bi-studios.md) -- rendering and the cache, saving and restoring, exports, serving, and showing.

## The mental model

Read the family as a pipeline with one direction of flow. The source supplies
tables. The session supplies choices. The studio combines the two with its
boards, and rendering produces, for each board, the rows each view should show
under the current choices.

Two properties follow from that shape, and most of the guide checks them.

- **Choices are state, and state is versioned.** Every change to the session
  bumps a version number. Setting a value, clearing a value, and even clearing a
  value that was never set all count as changes.
- **Rendering reads, it does not decide.** A view shows whatever the session
  allows. Changing the session changes the next render, and nothing else. A
  board's definition does not change because a reader filtered it.

## Example 1: A session records choices and their types

The session is the simplest part of the family, and it is headless by nature.
The first program builds one, sets a text parameter and a numeric one, reads them
back with their types, and clears a parameter. It also shows the version number
moving with each change.

```cpp
#include <pluss/bi/session.h>
#include <pluss/bi/specs.h>

#include <iostream>
#include <variant>

int main() {
  std::cout << std::boolalpha;
  pluss::bi::Session session;
  std::cout << "starts empty: " << session.empty() << '\n';
  std::cout << "starts at version: " << session.version() << '\n';

  // Each change bumps the version, so a view can tell that the session moved.
  session.set_param("region", std::string("north"));
  std::cout << "after one change, version: " << session.version() << '\n';

  // Parameters hold either a number or text. Read them back with their stored type.
  session.set_param("threshold", 2.5);
  const auto threshold = session.param("threshold");
  std::cout << "threshold is numeric: " << std::holds_alternative<double>(*threshold) << '\n';
  std::cout << "threshold value: " << std::get<double>(*threshold) << '\n';
  std::cout << "region is text: " << std::holds_alternative<std::string>(*session.param("region")) << '\n';

  // Clearing a key removes it. Clearing an absent key still counts as a change.
  session.clear_param("region");
  std::cout << "region still set: " << session.has_param("region") << '\n';
  std::cout << "version after clear: " << session.version() << '\n';
  session.clear_param("never_set");
  std::cout << "version after clearing an absent key: " << session.version() << '\n';
}
```

Output

```text
starts empty: true
starts at version: 0
after one change, version: 1
threshold is numeric: true
threshold value: 2.5
region is text: true
region still set: false
version after clear: 3
version after clearing an absent key: 4
```

The session starts empty at version zero. Each change bumps the version, so the
version after one change is one, and after the two sets and one clear it is
three. Clearing a key that was never set still bumps the version to four, which
is the behavior to rely on when you watch the session for changes. A parameter
keeps its type. The numeric threshold reads back as a number, and the regional
name reads back as text, so you read each one with the type it was stored as.

## Example 2: A table over a named source

A studio needs tables before it can show anything. The second program registers
a dataframe under the name `sales` in a `Nexus`, attaches the nexus to a studio,
and places a table view on a one-row, two-column grid of a board. It then
renders the board to HTML under a private directory.

```cpp
#include <pluss/mic.h>
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

int main() {
  std::cout << std::boolalpha;
  namespace fs = std::filesystem;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}, {3}, cfg))},
  });
  const pluss::bundle::Nexus nexus({{"sales", sales}});
  std::cout << "table registered: " << nexus.has("sales") << '\n';

  pluss::bi::Studio studio("sales_studio");
  studio.source(nexus);
  pluss::bi::Board& board = studio.board("overview");
  board.title("Sales overview").grid(1, 2);
  board.view(pluss::bi::table("rows").query(pluss::bi::from("sales")).title("Rows").place(0, 0, 1, 2));
  std::cout << "board tiles: " << board.tiles().size() << '\n';

  const fs::path dir = fs::temp_directory_path() / "pluss_bi_board_example";
  fs::remove_all(dir);
  fs::create_directories(dir);
  const fs::path target = dir / "overview.html";
  studio.export_html(target.string(), "Sales overview", 900, 600);
  std::cout << "html written: " << fs::exists(target) << '\n';

  std::ifstream in(target);
  const std::string html((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::cout << "html is a document: " << (html.find("<html") != std::string::npos) << '\n';
  fs::remove_all(dir);
}
```

Output

```text
table registered: true
board tiles: 1
html written: true
html is a document: true
```

The nexus registers the table, and the board holds one tile. Rendering writes a
standalone HTML document, and the file contains an html root. The table name is the link between the view and the data. The
view refers to `sales` by name, and the nexus resolves that name when the studio
renders. Every view refers to its data by name, so renaming a table means updating every
view that names it.

## Example 3: A filter narrows what a view shows

A dashboard becomes useful when a reader can narrow it. The third program renders
the table three times. It renders once with no filter, once with a filter on the
region set to north, and once after clearing that filter.

```cpp
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
```

Output

```text
rows with no filter: 3
rows with north filter: 2
session filter count: 1
rows after clearing: 3
```

The unfiltered table holds three rows. With the north filter, it holds two, and
the session records one filter. Clearing the filter restores all three rows. The
filter did not change the board or the table. It changed only what the next
render showed. Filters live in the session, so a reader's filter belongs to the
session you save, and two sessions over the same board can show different rows.

## Example 4: Saving and loading a studio

A dashboard should outlive the program that built it. The fourth program saves a
studio to a file, loads it back against the same data, and checks what came back.
The board, its title, and the filter all survive, and the restored board shows
the same two rows as the original did under its filter.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int main() {
  std::cout << std::boolalpha;
  namespace fs = std::filesystem;
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
  studio.set_filter(bi::Filter{
      .key = "region_is_north",
      .feature = "region",
      .op = bi::FilterOp::Eq,
      .payload = bi::FilterScalar{std::string("north")},
  });

  // Save the studio to a private file, then load it back against the same data.
  const fs::path dir = fs::temp_directory_path() / "pluss_bi_save_example";
  fs::remove_all(dir);
  fs::create_directories(dir);
  const fs::path saved = dir / "studio.bi";
  studio.save(saved.string());
  const bi::Studio restored = bi::Studio::load(saved.string(), nexus);

  std::cout << "boards restored: " << restored.boards().size() << '\n';
  std::cout << "board title restored: " << (restored.boards().front().title() == "Sales overview") << '\n';
  std::cout << "filter restored: " << restored.session().has_filter("region_is_north") << '\n';
  std::cout << "restored rows under the filter: "
            << restored.render().boards.front().tiles.front().table->height() << '\n';
  fs::remove_all(dir);
}
```

Output

```text
boards restored: 1
board title restored: true
filter restored: true
restored rows under the filter: 2
```

The restored studio has its one board with the same title, and the filter is
still in its session. Rendering the restored board under that filter gives two
rows, which matches the original. Load needs the nexus again, because the file
records the dashboard and the choices, not the data. Keep the data where your
loader can find it, and pass the same nexus when you load.

## Example 5: Controls are tiles, and they are marked as such

A reader changes a dashboard through controls, and a control is also a tile on the
board. The fifth program places a category control beside a table view, then reads
the rendered board to see which tile is which.

```cpp
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
```

Output

```text
tiles on the board: 2
first tile is a control: true
second tile is a control: false
view tile has a table: true
```

The board holds two tiles, the control first and the table second. The rendered
tiles carry the marking, so the first tile reports as a control and the second
does not. The view tile holds a table of its result. Read the marking when you build a front end
that must treat controls differently from presentations.

## Example 6: The view constructors name their presentations

The family offers a small set of presentations, and each constructor records which
one it creates. The sixth program builds a KPI, a line, a bar, and a table, then
checks the kind each one reports. It also reads back the key a view was built
with, because a board refers to a view by that key.

```cpp
#include <pluss/bi/bi.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace bi = pluss::bi;

  // Each constructor names a presentation. The view records which one it is.
  const bi::View kpi = bi::kpi("headline");
  const bi::View line = bi::line("trend");
  const bi::View bar = bi::bar("by_region");
  const bi::View table = bi::table("rows");

  std::cout << "kpi is a kpi: " << (kpi.bind().kind == bi::ViewKind::Kpi) << '\n';
  std::cout << "line is a line: " << (line.bind().kind == bi::ViewKind::Line) << '\n';
  std::cout << "bar is a bar: " << (bar.bind().kind == bi::ViewKind::Bar) << '\n';
  std::cout << "table is a detail table: " << (table.bind().kind == bi::ViewKind::DetailTable) << '\n';

  // The key identifies the view. It is what a board uses to refer to it.
  std::cout << "bar key: " << bar.key() << '\n';
}
```

Output

```text
kpi is a kpi: true
line is a line: true
bar is a bar: true
table is a detail table: true
bar key: by_region
```

Each constructor produces the presentation its name promises. A table is a detail
table. The bar view's key is the name you gave it, and that name is what a board
uses to refer to the view. Choose the
constructor for the question the reader asks: a single number for a KPI, a
trend for a line, a comparison for a bar, and the raw rows for a table.

## Example 7: Exporting a standalone document

A finished dashboard usually goes to someone who has no access to your program.
The seventh program exports a board to an HTML file with a chosen title and size,
then checks the document. It confirms that the file exists, that it has an html
root, that it carries the title, and that it is not trivially empty.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

int main() {
  std::cout << std::boolalpha;
  namespace fs = std::filesystem;
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
  studio.board("overview").title("Quarterly sales").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0, 1, 1));

  // Export a standalone HTML document. The file is written under a private directory.
  const fs::path dir = fs::temp_directory_path() / "pluss_bi_html_example";
  fs::remove_all(dir);
  fs::create_directories(dir);
  const fs::path target = dir / "quarterly.html";
  studio.export_html(target.string(), "Quarterly sales", 900, 600);

  std::ifstream in(target);
  const std::string html((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::cout << "file exists: " << fs::exists(target) << '\n';
  std::cout << "document has an html root: " << (html.find("<html") != std::string::npos) << '\n';
  std::cout << "document carries the export title: " << (html.find("Quarterly sales") != std::string::npos) << '\n';
  std::cout << "document is not empty: " << (html.size() > 1000) << '\n';
  fs::remove_all(dir);
}
```

Output

```text
file exists: true
document has an html root: true
document carries the export title: true
document is not empty: true
```

The export writes a file, and the file is a complete html document carrying the
title you passed. Check the document rather than trusting the export call alone, because the
document is what the reader receives.

## Example 8: Boards, keys, and rejected exports

Some mistakes in a dashboard do not show up until someone opens it. The eighth
program covers two of them. Asking for a board by a key that already exists
returns that board, and the board count does not grow. An export with a zero width
is refused, and no file is written.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>
#include <vector>

namespace {

// Returns true if the callable throws. The message is not printed, so the output
// does not depend on library wording.
template <class Fn>
bool throws(Fn fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

} // namespace

int main() {
  std::cout << std::boolalpha;
  namespace fs = std::filesystem;
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  namespace bi = pluss::bi;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10.0, 20.0}, {2}, cfg))},
  });
  const pluss::bundle::Nexus nexus({{"sales", sales}});

  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0, 1, 1));

  // Asking for an existing board key returns that board. It does not add a second one.
  const auto boards_before = studio.boards().size();
  studio.board("overview");
  std::cout << "boards before repeating the key: " << boards_before << '\n';
  std::cout << "boards after repeating the key: " << studio.boards().size() << '\n';

  // An export needs a positive size. A zero width is refused and writes nothing.
  const fs::path dir = fs::temp_directory_path() / "pluss_bi_pitfalls_example";
  fs::remove_all(dir);
  fs::create_directories(dir);
  const fs::path target = dir / "rejected.html";
  std::cout << "zero width export rejected: "
            << throws([&] { studio.export_html(target.string(), "Sales", 0, 600); }) << '\n';
  std::cout << "no file written for rejected export: " << !fs::exists(target) << '\n';
  fs::remove_all(dir);
}
```

Output

```text
boards before repeating the key: 1
boards after repeating the key: 1
zero width export rejected: true
no file written for rejected export: true
```

Repeating a board key does not create a second board, so you can refer to a board
by name in several places without worrying about duplicates. A zero-width export is
rejected, and the rejection leaves no file behind, so a refused export left no partial
document for someone to open by mistake in this case. Validate sizes before you export
in a batch job, and treat a refusal as a bug in the caller, not a warning to ignore.

## Example 9: Layout places tiles on a grid

A dashboard is a grid, and each tile says where it sits. The ninth program builds
a board with two rows and two columns. The first tile covers two columns of the top
row, the second sits below it, and the third sits beside the second with a minimum
height. It prints the grid, each tile's cell and spans, and the minimum height.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;
namespace fs = std::filesystem;

// Six sales rows: regions north, south, north, east, south, north; months jan, jan, feb, feb, mar, mar;
// amounts 10 to 60; units 1 to 6; and one date per row from January to June 2024.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east", "south", "north"}))},
      {"month", Slate(pluss::make_note({"jan", "jan", "feb", "feb", "mar", "mar"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {6}, cfg))},
      {"units", Slate(pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {6}, cfg))},
      {"placed", Slate(pluss::make_timeline({"2024-01-05", "2024-02-10", "2024-03-15", "2024-04-20", "2024-05-25", "2024-06-30"}))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// The text of the page a studio exports, for checks on what is shown.
std::string page_text(const bi::Studio& studio) {
  const fs::path dir = fs::temp_directory_path() / "pluss_bi_guide_page";
  fs::remove_all(dir);
  fs::create_directories(dir);
  studio.export_html((dir / "page.html").string(), "T", 900, 600, bi::DashboardChrome::None);
  std::ifstream in((dir / "page.html").string());
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  fs::remove_all(dir);
  return text;
}


int main() {
  // The grid sets how many rows and columns the board has. Each tile names its cell and its spans.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").title("Sales overview").grid(2, 2);
  studio.board("overview").view(bi::table("wide").query(bi::from("sales")).title("Wide").place(0, 0, 1, 2));
  studio.board("overview").view(bi::kpi("headline").feature("amount").query(bi::from("sales")).title("Headline").place(1, 0));
  studio.board("overview").view(bi::table("tall").query(bi::from("sales")).title("Tall").place(1, 1).min_height(320));

  const auto render = studio.render();
  const auto& board = render.boards.front();
  std::cout << "grid rows and columns: " << board.rows << " " << board.cols << '\n';
  for (const auto& tile : board.tiles) {
    std::cout << tile.key << " at " << tile.layout.row << "," << tile.layout.col
              << " spans " << tile.layout.row_span << "x" << tile.layout.col_span << '\n';
  }
  std::cout << "tall minimum height: " << board.tiles.back().layout.min_height.value_or(0.0) << '\n';
  return 0;
}
```

Output

```text
grid rows and columns: 2 2
wide at 0,0 spans 1x2
headline at 1,0 spans 1x1
tall at 1,1 spans 1x1
tall minimum height: 320
```

The grid has two rows and two columns. A tile's cell is its top-left corner, so the
wide table starts at row zero and column zero, and its spans say it covers one row and
two columns. The minimum height is a floor the dashboard keeps when the screen is
small, so the tall table reports 320. A tile that sets no minimum height reports zero
here only because the program asks for a default of zero.

## Example 10: A kpi counts, sums, and averages

A headline number is the most common tile, and it is easy to read the wrong value from
it. The tenth program renders four kpi tiles: a count, a sum, a mean with its default
formatting, and a mean with two decimals. It also builds a kpi with no feature to show the
refusal.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;
namespace fs = std::filesystem;

// Six sales rows: regions north, south, north, east, south, north; months jan, jan, feb, feb, mar, mar;
// amounts 10 to 60; units 1 to 6; and one date per row from January to June 2024.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east", "south", "north"}))},
      {"month", Slate(pluss::make_note({"jan", "jan", "feb", "feb", "mar", "mar"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {6}, cfg))},
      {"units", Slate(pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {6}, cfg))},
      {"placed", Slate(pluss::make_timeline({"2024-01-05", "2024-02-10", "2024-03-15", "2024-04-20", "2024-05-25", "2024-06-30"}))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// The text of the page a studio exports, for checks on what is shown.
std::string page_text(const bi::Studio& studio) {
  const fs::path dir = fs::temp_directory_path() / "pluss_bi_guide_page";
  fs::remove_all(dir);
  fs::create_directories(dir);
  studio.export_html((dir / "page.html").string(), "T", 900, 600, bi::DashboardChrome::None);
  std::ifstream in((dir / "page.html").string());
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  fs::remove_all(dir);
  return text;
}


// The text of the first kpi value on the page.
std::string kpi_value(const bi::Studio& studio) {
  const std::string page = page_text(studio);
  const std::string key = "class=\"bi-kpi-value\">";
  const auto at = page.find(key);
  if (at == std::string::npos) return "none";
  const auto end = page.find('<', at + key.size());
  return page.substr(at + key.size(), end - at - key.size());
}

int main() {
  // A kpi reduces one feature with the operation on the view. Its value prints in its shortest form unless you set decimals.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::kpi("count").feature("amount").query(bi::from("sales")).place(0, 0));
  std::cout << "count of amounts: " << kpi_value(studio) << '\n';

  bi::Studio sums("sum_studio");
  sums.source(nexus);
  sums.board("overview").grid(1, 1);
  sums.board("overview").view(bi::kpi("sum").feature("amount").op(pluss::table::GroupOp::Sum).query(bi::from("sales")).place(0, 0));
  std::cout << "sum of amounts: " << kpi_value(sums) << '\n';

  bi::Studio means("mean_studio");
  means.source(nexus);
  means.board("overview").grid(1, 1);
  means.board("overview").view(bi::kpi("mean").feature("units").op(pluss::table::GroupOp::Mean).query(bi::from("sales")).place(0, 0));
  std::cout << "mean of units, shortest: " << kpi_value(means) << '\n';

  bi::Studio fixed("fixed_studio");
  fixed.source(nexus);
  fixed.board("overview").grid(1, 1);
  fixed.board("overview").view(bi::kpi("mean").feature("units").op(pluss::table::GroupOp::Mean).decimals(2).query(bi::from("sales")).place(0, 0));
  std::cout << "mean of units, two decimals: " << kpi_value(fixed) << '\n';

  // A kpi with no feature has nothing to reduce, so the studio refuses it when it renders.
  bi::Studio refused("refused_studio");
  refused.source(nexus);
  refused.board("overview").grid(1, 1);
  refused.board("overview").view(bi::kpi("headline").query(bi::from("sales")).place(0, 0));
  bool was_refused = false;
  try {
    (void)refused.render();
  } catch (const std::exception&) {
    was_refused = true;
  }
  std::cout << "kpi with no feature refused: " << was_refused << '\n';
  return 0;
}
```

Output

```text
count of amounts: 6
sum of amounts: 210
mean of units, shortest: 3.5
mean of units, two decimals: 3.50
kpi with no feature refused: 1
```

The count of the six amounts is six, and the sum is 210. The mean of units is 3.5, and it
prints in its shortest form, with no padding to six places. Setting `decimals(2)` fixes
the places, so the same mean prints as 3.50. A kpi with no feature has nothing to reduce,
and the studio refuses it when it renders, not when you build it.

## Example 11: A time filter keeps the dates in a window

A timeline column is filtered with bounds, not with a single value. The eleventh program
renders the table with no filter, then with a window from February 1 to April 30, then
with the same two bounds given in reverse order.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;
namespace fs = std::filesystem;

// Six sales rows: regions north, south, north, east, south, north; months jan, jan, feb, feb, mar, mar;
// amounts 10 to 60; units 1 to 6; and one date per row from January to June 2024.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east", "south", "north"}))},
      {"month", Slate(pluss::make_note({"jan", "jan", "feb", "feb", "mar", "mar"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {6}, cfg))},
      {"units", Slate(pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {6}, cfg))},
      {"placed", Slate(pluss::make_timeline({"2024-01-05", "2024-02-10", "2024-03-15", "2024-04-20", "2024-05-25", "2024-06-30"}))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// The text of the page a studio exports, for checks on what is shown.
std::string page_text(const bi::Studio& studio) {
  const fs::path dir = fs::temp_directory_path() / "pluss_bi_guide_page";
  fs::remove_all(dir);
  fs::create_directories(dir);
  studio.export_html((dir / "page.html").string(), "T", 900, 600, bi::DashboardChrome::None);
  std::ifstream in((dir / "page.html").string());
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  fs::remove_all(dir);
  return text;
}


int main() {
  // A time filter keeps rows whose timeline value falls between the two bounds, both included.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));

  const auto rows = [&studio] { return studio.render().boards.front().tiles.front().table->height(); };
  std::cout << "rows with no filter: " << rows() << '\n';

  studio.set_filter(bi::Filter::between_time("placed", pluss::make_timeline({"2024-02-01"}), pluss::make_timeline({"2024-04-30"})));
  std::cout << "rows from February 1 to April 30: " << rows() << '\n';

  // The bounds are ordered for you, so swapping them selects the same rows.
  studio.clear_filter("placed");
  studio.set_filter(bi::Filter::between_time("placed", pluss::make_timeline({"2024-04-30"}), pluss::make_timeline({"2024-02-01"})));
  std::cout << "rows with the bounds swapped: " << rows() << '\n';
  return 0;
}
```

Output

```text
rows with no filter: 6
rows from February 1 to April 30: 3
rows with the bounds swapped: 3
```

The unfiltered table holds six rows. The window holds three, because the rows placed on
February 10, March 15, and April 20 fall inside it, and both bounds are included. Swapping
the bounds selects the same three rows, so you can give the bounds in either order.

## Example 12: A pivot shows a table, and a correlation shows a plot

The kind of a view decides what it holds once it renders. The twelfth program builds a
pivot over region, month, and amount, and a correlation over amount and units. It reports
which of the two results each tile holds.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;
namespace fs = std::filesystem;

// Six sales rows: regions north, south, north, east, south, north; months jan, jan, feb, feb, mar, mar;
// amounts 10 to 60; units 1 to 6; and one date per row from January to June 2024.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east", "south", "north"}))},
      {"month", Slate(pluss::make_note({"jan", "jan", "feb", "feb", "mar", "mar"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {6}, cfg))},
      {"units", Slate(pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {6}, cfg))},
      {"placed", Slate(pluss::make_timeline({"2024-01-05", "2024-02-10", "2024-03-15", "2024-04-20", "2024-05-25", "2024-06-30"}))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// The text of the page a studio exports, for checks on what is shown.
std::string page_text(const bi::Studio& studio) {
  const fs::path dir = fs::temp_directory_path() / "pluss_bi_guide_page";
  fs::remove_all(dir);
  fs::create_directories(dir);
  studio.export_html((dir / "page.html").string(), "T", 900, 600, bi::DashboardChrome::None);
  std::ifstream in((dir / "page.html").string());
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  fs::remove_all(dir);
  return text;
}


int main() {
  // A pivot shows a table and a correlation shows a plot. The tile records which one it holds.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 2);
  studio.board("overview").view(bi::pivot("by_region_month")
      .pivot_index({"region"}).pivot_columns({"month"}).pivot_values({"amount"})
      .query(bi::from("sales")).title("Pivot").place(0, 0));
  studio.board("overview").view(bi::corr("relations").features({"amount", "units"})
      .query(bi::from("sales")).title("Correlation").place(0, 1));

  const auto render = studio.render();
  const auto& tiles = render.boards.front().tiles;
  std::cout << "pivot holds a table: " << (tiles[0].table != nullptr) << '\n';
  std::cout << "pivot holds a plot: " << (tiles[0].plot != nullptr) << '\n';
  std::cout << "correlation holds a plot: " << (tiles[1].plot != nullptr) << '\n';
  return 0;
}
```

Output

```text
pivot holds a table: 1
pivot holds a plot: 0
correlation holds a plot: 1
```

The pivot holds a table and no plot. The correlation holds a plot. A tile that shows a
table has no plot to read, so check the kind before you go looking for one.

## Example 13: Range and time-range controls name their feature

Controls filter the other tiles, and each kind of control offers a different choice. The
thirteenth program builds a numeric range, a time range, and a category control on a
feature the source does not have. It checks the page for each label and for the
placeholder.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;
namespace fs = std::filesystem;

// Six sales rows: regions north, south, north, east, south, north; months jan, jan, feb, feb, mar, mar;
// amounts 10 to 60; units 1 to 6; and one date per row from January to June 2024.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east", "south", "north"}))},
      {"month", Slate(pluss::make_note({"jan", "jan", "feb", "feb", "mar", "mar"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {6}, cfg))},
      {"units", Slate(pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {6}, cfg))},
      {"placed", Slate(pluss::make_timeline({"2024-01-05", "2024-02-10", "2024-03-15", "2024-04-20", "2024-05-25", "2024-06-30"}))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// The text of the page a studio exports, for checks on what is shown.
std::string page_text(const bi::Studio& studio) {
  const fs::path dir = fs::temp_directory_path() / "pluss_bi_guide_page";
  fs::remove_all(dir);
  fs::create_directories(dir);
  studio.export_html((dir / "page.html").string(), "T", 900, 600, bi::DashboardChrome::None);
  std::ifstream in((dir / "page.html").string());
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  fs::remove_all(dir);
  return text;
}


int main() {
  // Each control kind names the feature it filters on. A control whose feature is missing says so on the page.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 3);
  studio.board("overview").control(bi::range("size", "amount", "Amount range").place(0, 0));
  studio.board("overview").control(bi::time_range("when", "placed", "Placed range").place(0, 1));
  studio.board("overview").control(bi::category("pick", "nope", "Missing feature").place(0, 2));

  const std::string page = page_text(studio);
  std::cout << "page shows the amount range: " << (page.find("Amount range") != std::string::npos) << '\n';
  std::cout << "page shows the placed range: " << (page.find("Placed range") != std::string::npos) << '\n';
  std::cout << "page shows the missing-feature placeholder: "
            << (page.find("No source feature matched this control yet.") != std::string::npos) << '\n';
  return 0;
}
```

Output

```text
page shows the amount range: 1
page shows the placed range: 1
page shows the missing-feature placeholder: 1
```

The numeric range and the time range each show their label on the page. The category
control names a feature the source lacks, so the page shows the placeholder instead of
failing the render. A missing feature is a visible mistake on the page, not an error that
stops the dashboard.

## Example 14: Serving and showing a studio

A studio can run as a local page, or be written to a file. The fourteenth program starts a
server on an ephemeral port, checks that it runs, stops it, and then calls `show` with
launch set to false, which writes the page and opens nothing. The launch path opens a
browser, so this guide does not run it.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bi/serve.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;
namespace fs = std::filesystem;

// Six sales rows: regions north, south, north, east, south, north; months jan, jan, feb, feb, mar, mar;
// amounts 10 to 60; units 1 to 6; and one date per row from January to June 2024.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east", "south", "north"}))},
      {"month", Slate(pluss::make_note({"jan", "jan", "feb", "feb", "mar", "mar"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {6}, cfg))},
      {"units", Slate(pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {6}, cfg))},
      {"placed", Slate(pluss::make_timeline({"2024-01-05", "2024-02-10", "2024-03-15", "2024-04-20", "2024-05-25", "2024-06-30"}))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// The text of the page a studio exports, for checks on what is shown.
std::string page_text(const bi::Studio& studio) {
  const fs::path dir = fs::temp_directory_path() / "pluss_bi_guide_page";
  fs::remove_all(dir);
  fs::create_directories(dir);
  studio.export_html((dir / "page.html").string(), "T", 900, 600, bi::DashboardChrome::None);
  std::ifstream in((dir / "page.html").string());
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  fs::remove_all(dir);
  return text;
}


int main() {
  // start_serving runs the dashboard in the background and returns a handle. show writes the page and opens nothing when launch is false.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));

  auto handle = studio.start_serving("Sales", 900, 600, 0, false, bi::DashboardChrome::None);
  std::cout << "running before stop: " << handle.running() << '\n';
  handle.stop();
  handle.wait();
  std::cout << "running after stop: " << handle.running() << '\n';

  std::cout << "show without launch succeeds: " << studio.show("Sales overview", 900, 600, false, bi::DashboardChrome::None) << '\n';
  return 0;
}
```

Output

```text
running before stop: 1
running after stop: 0
show without launch succeeds: 1
```

The server reports running before the stop and not running after it. `show` with launch
set to false succeeds. Keep the handle until you are done with the server, and call `stop`
followed by `wait` so the server has finished before the program exits.

## Example 15: Cache statistics show what a render reused

A render reports how many tiles it computed (misses) and how many it reused (hits). The
fifteenth program renders the same studio twice, then changes the session and renders
again.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;
namespace fs = std::filesystem;

// Six sales rows: regions north, south, north, east, south, north; months jan, jan, feb, feb, mar, mar;
// amounts 10 to 60; units 1 to 6; and one date per row from January to June 2024.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east", "south", "north"}))},
      {"month", Slate(pluss::make_note({"jan", "jan", "feb", "feb", "mar", "mar"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {6}, cfg))},
      {"units", Slate(pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {6}, cfg))},
      {"placed", Slate(pluss::make_timeline({"2024-01-05", "2024-02-10", "2024-03-15", "2024-04-20", "2024-05-25", "2024-06-30"}))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// The text of the page a studio exports, for checks on what is shown.
std::string page_text(const bi::Studio& studio) {
  const fs::path dir = fs::temp_directory_path() / "pluss_bi_guide_page";
  fs::remove_all(dir);
  fs::create_directories(dir);
  studio.export_html((dir / "page.html").string(), "T", 900, 600, bi::DashboardChrome::None);
  std::ifstream in((dir / "page.html").string());
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  fs::remove_all(dir);
  return text;
}


int main() {
  // A render reports how many tiles it computed fresh (misses) and how many it reused (hits).
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));

  (void)studio.render();
  const auto steady = studio.render().cache;
  std::cout << "steady render tile misses: " << steady.tile_misses << '\n';
  std::cout << "steady render tile hits: " << steady.tile_hits << '\n';

  studio.set_filter(bi::Filter::eq("region", std::string("north")));
  const auto changed = studio.render().cache;
  std::cout << "render after a filter tile misses: " << changed.tile_misses << '\n';
  return 0;
}
```

Output

```text
steady render tile misses: 0
steady render tile hits: 1
render after a filter tile misses: 1
```

The second render computes no tile again, so its misses are zero, and it reuses the one
tile, so its hits are one. After a filter changes the session, that tile is computed
again, so the miss count goes back to one. A session change invalidates the cached tiles,
and an unchanged session does not.

## Decisions

Most dashboard work comes down to a few choices, made in this order.

| You want | Use | Notes |
|---|---|---|
| A table to read named data | `bi::table` over `bi::from` | The name must match a table in the source |
| A single headline number | `bi::kpi` | One value per view |
| A comparison across categories | `bi::bar` | One view per comparison |
| A trend | `bi::line` | One view per trend |
| A reader to choose a category | `bi::category` as a control | Shows as its own tile |
| A narrowed view | `Studio::set_filter` | Lives in the session, not the board |
| To keep the dashboard | `Studio::save` and `Studio::load` | Load against the same nexus |
| A document for someone else | `Studio::export_html` | Pass a positive size |
| A grid of tiles | `Board::grid` with `View::place` | Each tile names its cell and spans |
| A kpi with a fixed number of places | `View::decimals` | Without it, the value prints in its shortest form |
| A kpi of an average | `View::op` with `GroupOp::Mean` | The default operation is a count |
| A window of dates | `Filter::between_time` | Both bounds are included, and their order does not matter |
| A correlation | `bi::corr` with `View::features` | Renders a plot |
| A pivot table | `bi::pivot` with its three role lists | Renders a table, not a plot |
| A local page that stays up | `Studio::start_serving` | Returns a handle, so stop it when you are done |
| To know what a render reused | `StudioRender::cache` | A steady session reuses its tiles |

Two decisions carry the most consequence. First, decide which state belongs to the
session and which belongs to the board. A reader's filter is session state, so a
saved dashboard keeps it only when you save the session with it. Second, keep table
names stable, because every view refers to its data by name, so renaming a table
means updating each view that names it.

## Pitfalls

**Expecting a filter to change the board.** Example 3 shows that a filter changes
the next render, not the board. The board is the same before and after.

**Loading a saved studio without its data.** Example 4 shows that load needs the
nexus again. The file records the dashboard, not the tables.

**Creating duplicate boards by accident.** Example 8 shows that repeating a key
returns the existing board. Refer to a board by its key rather than building a new
one.

**Ignoring a refused export.** Example 8 shows that a zero-width export is refused
and writes nothing. Treat the refusal as a failure in the caller.

**Assuming a control is a presentation.** Example 5 shows that a control is a tile
that carries no table. Check the marking before you read a tile's contents.

**Expecting six decimals from a kpi.** Example 10 shows the mean of units as 3.5,
not 3.500000. The value prints in its shortest form until you set `decimals`.

**Expecting a pivot to draw a plot.** Example 12 shows that a pivot holds a table and no
plot. Look for the table on a pivot tile, and for the plot on a correlation tile.

## Where next

- [Sessions and parameters](bi-sessions.md), [Filters](bi-filters.md), [Views](bi-views.md),
  [Boards, tiles, and layout](bi-boards.md), [Controls](bi-controls.md), and [Studios](bi-studios.md).
- [PlussBI API Reference](../api/bi.md) -- the canonical BI surface, one entry per
  type and operation.
- [PlussView: A Guide](view.md) -- the built-in renderer that a Studio's
  `.show()` opens a window through.
- [Core Utilities Manual](core.md) -- the shelf of small utilities that the BI
  examples do not require.
