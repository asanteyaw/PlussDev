# Content And Moment Helpers

This page documents the high-level helpers that are available directly in `pluss::*`:

- `pluss::content(...)`
- `pluss::content_lines(...)`
- `pluss::content_words(...)`
- `pluss::moment(...)`

These are intentionally different from strict constructors:

- `pluss::make_note(...)` and `pluss::make_timeline(...)` are strict constructors.
- `pluss::content(...)` and `pluss::moment(...)` are convenience parsers/helpers.

## Content Helpers

`pluss::content(...)` builds `pluss::Note` with configurable ingestion behavior.

```cpp
#include <pluss/pluss.h>

auto note_default = pluss::content("  alpha   beta  "); // normalized by default
auto note_lines   = pluss::content_lines("a\n\nb\n", false, true); // ["a", "b"]
auto note_words   = pluss::content_words("alpha   beta\tgamma");   // ["alpha","beta","gamma"]
```

### Options

```cpp
pluss::ContentOptions options;
options.mode = pluss::ContentMode::Normalize; // Normalize | Lines | Words | Raw
options.trim = true;
options.collapse_whitespace = true;
options.drop_empty = true;
options.keep_line_endings = false;

auto note = pluss::content("  a   b  ", options);
```

## Moment Helpers

`pluss::moment(...)` resolves friendly time expressions into `pluss::Timeline`.

```cpp
#include <pluss/pluss.h>

auto t0 = pluss::moment("today");
auto t1 = pluss::moment("today-7d");
auto t2 = pluss::moment("now+4h");
auto t3 = pluss::moment("som"); // start of month
auto t4 = pluss::moment("eom"); // end of month
```

### Container Inputs (RangeView)

`RangeView` container views are the primary plural-input path for helper/strict constructor APIs.
Use `RangeView::Of<T>` for 1D/flat container arguments.

```cpp
#include <MSharp/RangeView.h>
#include <pluss/pluss.h>

std::vector<std::string_view> rows = {"a", "b", "c"};
auto rv = pluss::msharp::RangeView::Of<std::string_view>(rows);

auto note = pluss::content(rv);
auto ts   = pluss::moment(rv);
auto n2   = pluss::make_note(rv);
auto t2   = pluss::make_timeline(rv);
```

### Options

```cpp
pluss::MomentOptions options;
options.mode = pluss::MomentMode::Auto;      // Auto | ExpressionOnly | Strict
options.clock = pluss::MomentClock::UTC;     // UTC | Local

auto t = pluss::moment("tomorrow", options);
```

## Strict Constructors

Use strict constructors when you do not want expression or ingestion behavior:

```cpp
auto note = pluss::make_note("exact payload");
auto ts   = pluss::make_timeline("2026-04-02T00:00:00Z");
```

## Error Behavior

Input validation follows the repo-wide centralized error checks. Invalid helper inputs fail through the same centralized check/error path used across Pluss.
