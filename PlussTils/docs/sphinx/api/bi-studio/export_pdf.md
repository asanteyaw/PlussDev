# `pluss::bi::Studio::export_pdf`

[Back to `Studio`](../bi-studio.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

void export_pdf(std::string_view path,
                 pluss::pl::PdfEngine engine = pluss::pl::PdfEngine::Auto) const;
```

`export_pdf` writes the studio's render as a PDF document. The engine you name is used to produce it. `Internal` and `Auto` produce the
same file size for the sample studio; `SvgConvert` produces a larger file.

## Overloads

| Engine | Value |
|---|---|
| `Auto` | The default. |
| `Internal` | The internal engine. |
| `SvgConvert` | The SVG conversion engine. |

## Parameters

| Parameter | Meaning |
|---|---|
| `path` | The file to write. |
| `engine` | `Auto` (default), `Internal`, or `SvgConvert`. |

## Returns

Nothing. The call writes the file.

## Errors

Errors from the chosen engine are reported when the call runs.

## Practical Examples

### Example 1

Example 1 exports with the default engine and checks the PDF signature.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bi/serve.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;
namespace fs = std::filesystem;

// Three sales rows: regions north, south, north; amounts 10, 20, 30.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30}, {3}, cfg))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// A studio with one board holding one table over the sales data.
bi::Studio overview_studio(const pluss::bundle::Nexus& nexus) {
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").title("Sales overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));
  return studio;
}

// The number of rows the overview table shows on a studio.
std::int64_t rows_shown(const bi::Studio& studio) {
  return studio.render().boards.front().tiles.front().table->height();
}

fs::path private_dir(const std::string& name) {
  const fs::path dir = fs::temp_directory_path() / name;
  fs::remove_all(dir);
  fs::create_directories(dir);
  return dir;
}

std::string read_file(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

template <class Fn>
bool throws(Fn fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

int main() {
  // Engine Auto produces a PDF document. The file begins with the PDF signature.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const fs::path dir = private_dir("pluss_studio_pdf_auto_example");
  studio.export_pdf((dir / "overview.pdf").string(), pluss::pl::PdfEngine::Auto);
  const std::string pdf = read_file(dir / "overview.pdf");
  std::cout << "PDF signature present: " << (pdf.compare(0, 4, "%PDF") == 0) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
PDF signature present: 1
```

### Example 2

Example 2 exports with the internal engine and checks the PDF signature.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bi/serve.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;
namespace fs = std::filesystem;

// Three sales rows: regions north, south, north; amounts 10, 20, 30.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30}, {3}, cfg))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// A studio with one board holding one table over the sales data.
bi::Studio overview_studio(const pluss::bundle::Nexus& nexus) {
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").title("Sales overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));
  return studio;
}

// The number of rows the overview table shows on a studio.
std::int64_t rows_shown(const bi::Studio& studio) {
  return studio.render().boards.front().tiles.front().table->height();
}

fs::path private_dir(const std::string& name) {
  const fs::path dir = fs::temp_directory_path() / name;
  fs::remove_all(dir);
  fs::create_directories(dir);
  return dir;
}

std::string read_file(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

template <class Fn>
bool throws(Fn fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

int main() {
  // Engine Internal produces a PDF document. The file begins with the PDF signature.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const fs::path dir = private_dir("pluss_studio_pdf_internal_example");
  studio.export_pdf((dir / "overview.pdf").string(), pluss::pl::PdfEngine::Internal);
  const std::string pdf = read_file(dir / "overview.pdf");
  std::cout << "PDF signature present: " << (pdf.compare(0, 4, "%PDF") == 0) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
PDF signature present: 1
```

### Example 3

Example 3 exports with the SVG conversion engine and checks the PDF signature.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bi/serve.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;
namespace fs = std::filesystem;

// Three sales rows: regions north, south, north; amounts 10, 20, 30.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30}, {3}, cfg))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// A studio with one board holding one table over the sales data.
bi::Studio overview_studio(const pluss::bundle::Nexus& nexus) {
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").title("Sales overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));
  return studio;
}

// The number of rows the overview table shows on a studio.
std::int64_t rows_shown(const bi::Studio& studio) {
  return studio.render().boards.front().tiles.front().table->height();
}

fs::path private_dir(const std::string& name) {
  const fs::path dir = fs::temp_directory_path() / name;
  fs::remove_all(dir);
  fs::create_directories(dir);
  return dir;
}

std::string read_file(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

template <class Fn>
bool throws(Fn fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

int main() {
  // Engine SvgConvert produces a PDF document. The file begins with the PDF signature.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const fs::path dir = private_dir("pluss_studio_pdf_svgconvert_example");
  studio.export_pdf((dir / "overview.pdf").string(), pluss::pl::PdfEngine::SvgConvert);
  const std::string pdf = read_file(dir / "overview.pdf");
  std::cout << "PDF signature present: " << (pdf.compare(0, 4, "%PDF") == 0) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
PDF signature present: 1
```

## See Also

- [`export_svg`](export_svg.md) and [`export_png`](export_png.md) -- the other formats.
