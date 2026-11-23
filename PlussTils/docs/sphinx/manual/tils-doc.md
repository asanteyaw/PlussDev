# Reading the documentation from your program

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussTils Core Manual](core.md)

When you are in the middle of a program and want to know what a call does, the page is one call away. `pluss::tils::doc` prints the documentation page of a value's type, of one of its members, or of anything by name, in the terminal where the program runs. It can also give you the page as a file that your editor opens in a tab.

## How it works

The names are the titles of the pages on the website, so `Series::sum`, `DataFrame::show` and `Series` (the page of the type) all work, and case and `.` instead of `::` do not matter: `series.SUM` finds `Series::sum`. When a name matches nothing, the closest pages are listed.

| Call | What it does |
|---|---|
| `doc(value)` | prints the page of the type of `value` |
| `doc(value, "member")` | prints the page of `Type::member` |
| `doc<Type>("member")` | the same without a value |
| `doc("Series::sum")` | prints a page by name |
| `doc_link(...)` | prints where the page is: a file with a line number, and its address on the website |
| `doc_open(...)` | opens the file in your editor |
| `doc_find(name)`, `doc_search(name)` | the page, or the closest pages, as data |

`doc_link` prints the file as `path:1`. In the terminal of most editors that is a link: click it and the file opens in a tab. `doc_open` does it for you: from a VS Code terminal it runs `code -r` so the page opens in the editor you are already in; elsewhere it uses the system's opener.

A page is read from the first of these places that has it:

1. the folder named by `PLUSS_DOCS_DIR`, laid out like the documentation sources, which is how you read pages while writing them;
2. the documentation bundled in the PlussII download, in `share/pluss/docs`, found from the folder CMake reports for `Pluss::PlussII` (or the `PLUSS_PREFIX` variable), so no network is needed;
3. a copy kept in your cache folder;
4. the website, downloaded once with `curl` and kept in the cache.

## Signatures

```cpp
void pluss::tils::doc(std::string_view name, std::ostream& out = std::cout)
void pluss::tils::doc(const T& value, std::string_view member = {}, std::ostream& out = std::cout)
void pluss::tils::doc<T>(std::string_view member = {}, std::ostream& out = std::cout)
void pluss::tils::doc_link(std::string_view name, std::ostream& out = std::cout)
bool pluss::tils::doc_open(std::string_view name, std::ostream& out = std::cout)
std::optional<pluss::tils::DocPage> pluss::tils::doc_find(std::string_view name)
std::vector<pluss::tils::DocPage> pluss::tils::doc_search(std::string_view name, std::size_t limit = 8)
```

`doc_link` and `doc_open` also take a value and a member, as `doc` does. The header is `<pluss/tils/doc.h>`. A `DocPage` has the `title`, the `slug`, the `source` path and the `url`.

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run.

## Example 1: The page of a member

`doc(x, "sum")` takes the type of `x`, `Series`, and the member, `sum`, and prints the page of `Series::sum`. Only the start of the page is shown here; the program prints all of it.

```cpp
// Read the documentation from the program that uses it.
#include <pluss/pluss.h>
#include <pluss/tils/doc.h>

#include <iostream>

int main() {
  const auto x = pluss::make_series({1.0, 2.0, 3.0});

  // The page of a member of x's type.
  pluss::tils::doc(x, "sum");
}
```

Output

```text
Series::sum  -  https://plussible.org/docs/qit/api/series/sum

# Series::sum

## Signature

```cpp
Series Series::sum() const
Series Series::sum(std::int64_t dim) const
Series Series::sum(std::int64_t dim, bool keepdim) const
```

## What It Does

`sum()` adds values together, either across the whole `Series` or along one
chosen axis.

`sum()` is the most direct reduction in the `Series` surface, but there are
three details that matter in practice:

- full reduction returns a scalar `Series`
- `sum(dim, keepdim)` removes or preserves the reduced axis depending on
...
```

The first line is the page's title and its address on the website. Below it is the page itself, with the navigation and layout of the website removed so it reads well in a terminal.

## Example 2: A link, and a near miss

`doc_find` returns the page as data. `doc_link` says where its file is. A name with a typo lists the closest pages instead.

```cpp
// Finding a page, a link to it, and what a near miss says.
#include <pluss/tils/doc.h>

#include <iostream>

int main() {
  const auto page = pluss::tils::doc_find("series.SUM");  // case and '.' do not matter
  std::cout << page->title << "\n" << page->url << "\n\n";

  pluss::tils::doc_link("DataFrame::show");
  std::cout << "\n";
  pluss::tils::doc("Series::sun");
}
```

Output

```text
Series::sum
https://plussible.org/docs/qit/api/series/sum

DataFrame::show
  file: ~/Library/Caches/pluss/docs/PlussMic/docs/sphinx/api/dataframe/show.md:1
  web:  https://plussible.org/docs/mic/api/dataframe/show

No documentation page is called 'Series::sun'.
The closest pages:
  Series::sgn
  Series::sin
  Series::sub
  Series::sum
  Series::min
  Series::mul
```

The file is in your cache folder, so its path depends on your machine; the line shown here is from a Mac, where the cache is in `~/Library/Caches`. Click the `file:` line in an editor's terminal to open the page. `Series::sun` is not a page, so the program lists the ones nearest to it, and `Series::sum` is among them.

## Pitfalls

- **A value's type must be a PlussII type.** `doc(x)` looks for a page named after the type of `x`. For your own types it prints the list of closest pages.
- **A free function has no value to pass.** Use its name: `doc("make_series")`.
- **A first read with no bundled documentation needs the network.** `curl` must be installed. After that the page is in the cache and works offline.
