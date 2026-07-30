<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c0fb031f78a6 -->

# DataFrame::info_string

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::info_string(const display::DisplayOptions& options = {}) const -> std::string
```

## What It Does

Exactly [`info()`](info.md)'s two-part health-check report — the summary block plus
the feature inventory — returned as a `std::string` instead of printed to `stdout`.
Both functions compute the identical content via the same underlying renderer; the
only difference is where the text ends up.

Reach for `info_string()` when the report itself is the thing you need to work with
programmatically: embedding it in a log message, attaching it to a test failure,
writing it into a file alongside other diagnostics, or comparing it against an
expected string in a test.

## Parameters

**options** (`const display::DisplayOptions&`, default `{}`)

The same rendering options `info()` accepts — see [`info()`](info.md)'s Parameters for
the full breakdown (`terminal_width`, `terminal_height`, `mode`, and so on).

## Returns

`std::string`
    The rendered report, identical in content to what `info()` prints. `info_string()`
    itself does **not** append a trailing newline; `info()` adds exactly one `'\n'`
    after the same text when it prints. `std::cout << df.info_string() << '\n';`
    reproduces `info()`'s exact terminal output.

## Examples

Example 1. Capture the report as a string instead of printing it directly.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Thread(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"city",       Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Kumasi", "Kumasi"}))},
    {"product",    Thread(pluss::make_note({"Laptop", "Headphones", "Laptop", "Monitor", "Headphones", "Monitor"}))},
    {"units",      Thread(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Thread(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Thread(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

std::string report = orders.info_string();
std::cout << report << '\n';
```

### Output

```text
========================================
Field          | Value                  
----------------------------------------
Class          | pluss::table::DataFrame
Cases          | 6                      
Features       | 6                      
Case Range     | 0 -> 5                 
Null Case Rows | 2                      
Dtypes         | Note(2), Series(4)     
Memory         | 316.00 B               
----------------------------------------
=======================================
#  | Feature     Non-Null Cases  Dtype 
---------------------------------------
0  | order_id    6               Series
1  | city        6               Note  
2  | product     6               Note  
3  | units       6               Series
4  | unit_price  5               Series
5  | discount    5               Series
---------------------------------------
```

This is byte-for-byte the same report [`info()`](info.md) prints for the identical
`orders` frame — only the mechanism (returning a string versus writing to `stdout`)
differs.

Example 2. Append the report into a larger diagnostic message.

```cpp
std::string diagnostic = "Frame check failed for 'orders':\n" + orders.info_string();
```

Because `info_string()` returns a plain `std::string`, it composes naturally with
ordinary string concatenation — useful for building a single diagnostic message that
combines the report with your own context, rather than printing the report and your
own message as two separate `std::cout` calls.

## Notes

- `info_string()` and `info()` are backed by the exact same renderer
  (`display::render_info`); there is no drift between what one prints and what the
  other returns.
- The only textual difference from `info()`'s printed output is the trailing newline:
  `info()` always appends one after the report, `info_string()` never does.
- See [`info()`](info.md)'s own Notes for the meaning of `Null Case Rows`, the `Memory`
  estimate, and how the inventory table truncates on very wide frames — all of that
  applies identically here.

## See Also

- [`DataFrame::info`](info.md): the printed form of this exact report, and the full
  explanation of what each field means.
- [`DataFrame::to_string`](to_string.md): the plain frame renderer's string form, for
  the actual row/column contents rather than this structural summary.
- [`DataFrame::describe`](describe.md): value-level statistics, complementary to this
  method's structural summary.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="info.html">
    <span class="label">Previous</span>
    <span class="title">info</span>
  </a>
  <a class="next-link" href="insert.html">
    <span class="label">Next</span>
    <span class="title">insert</span>
  </a>
</div>
