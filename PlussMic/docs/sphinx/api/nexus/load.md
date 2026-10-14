<!-- pluss-hand-authored -->

# Nexus::load

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
static Nexus::load(std::string_view directory, const LoadOptions& options = {}) -> Nexus
```

## What It Does

Loads a raw table directory into a `Nexus` workbench.

This is the canonical “MySQL-workbench-style” entry point for in-memory table coordination:

- scan a directory
- discover supported tabular files
- load them in parallel batches
- return one `Nexus` where each file becomes one named table

`load()` does **not** restore a previously saved Nexus store. That persistence-side inverse of
[`save()`](save.md) now lives on [`read()`](read.md).

## Supported Formats

By default, `load()` discovers these file types directly from extension:

- `.csv`
- `.txt`
- `.text`
- `.tsv`
- `.json`
- `.parquet`
- `.pq`

Excel is intentionally excluded from directory loading. A workbook is not naturally “one file =
one table” because sheet selection becomes part of the contract, so it stays a `DataFrame`
concern instead of the default Nexus bulk-loader path.

## Parameters

**directory** (`std::string_view`)
    Directory to scan for raw tabular files.

**options** (`const LoadOptions&`, default `{}`)
    Controls discovery, filtering, and dispatch.

## LoadOptions

```cpp
struct LoadOptions {
  std::vector<std::string> load{};
  std::vector<std::string> ignore{};
  std::vector<std::string> formats{"csv", "txt", "json", "parquet"};
  bool recursive{false};
  bool keep_extension{false};
  bool parallel{true};
  std::optional<std::size_t> max_workers{};
  pluss::loader::Config config{};
};
```

### Field Semantics

**load**
    Explicit allow-list. If non-empty, only those tables/files are loaded.

**ignore**
    Explicit deny-list. If non-empty, everything else is loaded except those tables/files.

```{important} `load` and `ignore` are mutually exclusive
Use one or the other. Supplying both is rejected.
```

**formats**
    Allowed source types for discovery. This filters by normalized format tokens, not only by
    literal extension spelling. For example, `"parquet"` covers both `.parquet` and `.pq`.

**recursive**
    If `true`, descend into subdirectories. Otherwise only the top level is scanned.

**keep_extension**
    Controls how file names become Nexus table keys.

    - `false` (default): `orders.csv` -> `orders`
    - `true`: `orders.csv` -> `orders.csv`

**parallel**
    Enables concurrent file loading across the directory. In other words, Nexus can load
    `account.csv`, `loan.csv`, and `trans.json` at the same time instead of loading one file,
    then the next, then the next. The heavy parsing still delegates to
    `DataFrame::load(...)` for each individual file.

**max_workers**
    Caps concurrent file loads. Leave empty to let Nexus choose a bounded worker count from
    hardware concurrency.

**config**
    Passed through to the underlying `DataFrame::load(source_type, filepath, config)` calls.
    This is how you tune delimiter/header-style behavior for text-like formats while keeping
    Nexus as the coordinator rather than the parser.

## Returns

`Nexus`
    A workbench whose table keys come from the discovered file names.

## Naming Rules

By default, each file’s stem becomes the table name:

- `account.csv` -> `account`
- `district.json` -> `district`
- `loan.parquet` -> `loan`

If two discovered files collapse to the same key, `load()` fails rather than silently picking
one. Use `keep_extension=true` or narrower `load`/`ignore` filters when you need to disambiguate.

## Examples

Every example below writes its own real files to a scratch directory first
(via `DataFrame::save_csv`/`save_json`/`save_parquet`), so each one is a
complete, self-contained, copy-pasteable unit rather than assuming
files already exist somewhere. Each example uses its own directory, so
running them in any order (or more than once) never leaves stale files from
one example interfering with another.

Example 1. Load every supported top-level file.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

std::filesystem::remove_all("/tmp/pluss_nexus_load_ex1");
std::filesystem::create_directories("/tmp/pluss_nexus_load_ex1");

DataFrame({
    {"account_id", Slate(pluss::make_series({1.0, 2.0, 3.0}))},
    {"balance",    Slate(pluss::make_series({100.0, 250.0, 75.0}))},
}).save_csv("/tmp/pluss_nexus_load_ex1/account.csv");

DataFrame({
    {"district_id", Slate(pluss::make_series({10.0, 20.0}))},
    {"name",        Slate(pluss::make_note({"North", "South"}))},
}).save_json("/tmp/pluss_nexus_load_ex1/district.json");

DataFrame({
    {"trans_id",   Slate(pluss::make_series({1000.0, 1001.0}))},
    {"account_id", Slate(pluss::make_series({1.0, 2.0}))},
    {"amount",     Slate(pluss::make_series({50.0, -20.0}))},
}).save_csv("/tmp/pluss_nexus_load_ex1/trans.csv");

auto nx = Nexus::load("/tmp/pluss_nexus_load_ex1");
std::cout << nx.ntables() << "\n";
std::cout << std::boolalpha << nx.has("account") << " "
          << nx.has("district") << " "
          << nx.has("trans") << "\n";
```

### Output

```text
3
true true true
```

Example 2. Load only the files you name. `options.load` entries can name
either the resulting table key (`"account"`) or the literal file name
(`"loan.json"`) — both spellings are checked when matching a discovered
file.

```cpp
std::filesystem::remove_all("/tmp/pluss_nexus_load_ex2");
std::filesystem::create_directories("/tmp/pluss_nexus_load_ex2");

DataFrame({
    {"account_id", Slate(pluss::make_series({1.0, 2.0}))},
}).save_csv("/tmp/pluss_nexus_load_ex2/account.csv");

DataFrame({
    {"district_id", Slate(pluss::make_series({10.0}))},
}).save_json("/tmp/pluss_nexus_load_ex2/district.json");

DataFrame({
    {"loan_id",    Slate(pluss::make_series({500.0}))},
    {"account_id", Slate(pluss::make_series({1.0}))},
}).save_json("/tmp/pluss_nexus_load_ex2/loan.json");

LoadOptions options;
options.load = {"account", "loan.json"};
options.formats = {"csv", "json"};

auto nx = Nexus::load("/tmp/pluss_nexus_load_ex2", options);
std::cout << nx.ntables() << "\n";
std::cout << nx.keys().at(0) << " " << nx.keys().at(1) << "\n";
```

### Output

```text
2
account loan
```

`district.json` is discovered (it matches `options.formats`) but excluded, since
neither `"account"` nor `"loan.json"` matches it. Table keys come back sorted
alphabetically, so `"account"` precedes `"loan"` regardless of the files'
directory-listing order.

Example 3. Ignore a few tables but load the rest.

```cpp
std::filesystem::remove_all("/tmp/pluss_nexus_load_ex3");
std::filesystem::create_directories("/tmp/pluss_nexus_load_ex3");

DataFrame({
    {"account_id", Slate(pluss::make_series({1.0, 2.0}))},
}).save_csv("/tmp/pluss_nexus_load_ex3/account.csv");

DataFrame({
    {"trans_id", Slate(pluss::make_series({1.0}))},
}).save_csv("/tmp/pluss_nexus_load_ex3/trans.csv");

DataFrame({
    {"order_id", Slate(pluss::make_series({1.0}))},
}).save_csv("/tmp/pluss_nexus_load_ex3/order.csv");

LoadOptions options;
options.ignore = {"trans", "order"};

auto nx = Nexus::load("/tmp/pluss_nexus_load_ex3", options);
std::cout << nx.ntables() << "\n";
std::cout << std::boolalpha << nx.has("trans") << " "
          << nx.has("account") << "\n";
```

### Output

```text
1
false true
```

Example 4. Recursive scan plus stable file-name keys. The files live one
level down, in a `raw/` subdirectory — `options.recursive = true` is what
makes them discoverable at all; without it, only the top-level directory
(empty here) would be scanned.

```cpp
std::filesystem::remove_all("/tmp/pluss_nexus_load_ex4");
std::filesystem::create_directories("/tmp/pluss_nexus_load_ex4/raw");

DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben"}))},
}).save_csv("/tmp/pluss_nexus_load_ex4/raw/customers.csv");

DataFrame({
    {"event_id",    Slate(pluss::make_series({1.0, 2.0, 3.0}))},
    {"customer_id", Slate(pluss::make_series({1.0, 1.0, 2.0}))},
}).save_parquet("/tmp/pluss_nexus_load_ex4/raw/events.parquet");

LoadOptions options;
options.recursive = true;
options.keep_extension = true;

auto nx = Nexus::load("/tmp/pluss_nexus_load_ex4", options);
std::cout << std::boolalpha << nx.has("customers.csv") << " "
          << nx.has("events.parquet") << "\n";
```

### Output

```text
true true
```

`keep_extension = true` is what makes `"customers.csv"`/`"events.parquet"`
the table keys here — without it, they'd collapse to `"customers"`/`"events"`
the same as every other example on this page.

Example 5. Invalid mixed selection mode.

```cpp
std::filesystem::remove_all("/tmp/pluss_nexus_load_ex5");
std::filesystem::create_directories("/tmp/pluss_nexus_load_ex5");
DataFrame({
    {"account_id", Slate(pluss::make_series({1.0}))},
}).save_csv("/tmp/pluss_nexus_load_ex5/account.csv");

LoadOptions options;
options.load = {"account"};
options.ignore = {"loan"};

try {
  auto nx = Nexus::load("/tmp/pluss_nexus_load_ex5", options);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Nexus::load: choose either LoadOptions.load or LoadOptions.ignore, not both
```

This check runs after confirming `directory` exists and is actually a
directory, but before any file inside it is scanned or parsed — so a
mixed `load`/`ignore` request fails fast once the directory itself is
valid, regardless of what files it actually contains.

## Notes

- `load()` is for raw file ingestion into an in-memory Nexus workbench.
- [`read()`](read.md) is the persisted-store restore path.
- `parallel=true` means outer file-level concurrency across the discovered tables, not
  “parallel parsing inside one csv file”.
- Actual per-file parsing still remains delegated to `DataFrame::load(...)`.
- The `config` field lets you stay format-flexible without exposing Nexus parser-specific knobs.
- Validation order: `directory` must exist and be an actual directory first;
  only then is the `load`/`ignore` mutual-exclusivity check applied; only
  after both pass does the directory actually get scanned.
- Discovered candidates are sorted by resulting table key before loading, so
  the final `Nexus`'s table order is always alphabetical by key — never
  directory-listing order, which the OS/filesystem doesn't guarantee.

## See Also

- [`Nexus::read`](read.md): restore a saved Nexus store.
- [`Nexus::save`](save.md): persist a Nexus store that `read()` can reconstruct.
- [`Nexus::pull`](pull.md): once the tables are loaded, lift root-grain features safely.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="links.html">
    <span class="label">Previous</span>
    <span class="title">links</span>
  </a>
  <a class="next-link" href="map_frames.html">
    <span class="label">Next</span>
    <span class="title">map_frames</span>
  </a>
</div>
