<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 68116af436a6 -->

# DataFrameFeatureAxisAccessor::match

[Back to DataFrameFeatureAxisAccessor API Reference](../../dataframe_feataxis_accessor_api_reference)

## Signature

```cpp
DataFrameFeatureAxisAccessor::match(std::string_view pattern) const -> DataFrame
```

## What It Does

Selects every feature whose **name** matches `pattern`, an ECMAScript regular
expression, searched (not required to match the whole name) against each
feature name. Case labels and row order are preserved unchanged.

## Parameters

**pattern** (`std::string_view`)

An ECMAScript-flavored regular expression, checked against each feature name
with `std::regex_search` — a partial match anywhere in the name is enough
unless you anchor with `^`/`$` yourself.

## Returns

`DataFrame`
    A new frame with only the matching features; same case labels and rows as
    the source. Throws if `pattern` isn't a valid regular expression.

## Examples

Example 1. Select every feature whose name starts with `"sales_"`.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"sales_units", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
     {"sales_price", Thread(pluss::make_series({1.5, 2.0, 1.8}))},
     {"city",        Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});
std::cout << orders;

auto out = orders.feataxis().match("^sales_");
std::cout << out;
```

### Output (before)

```text
=======================================
     | sales_units  sales_price  city  
---------------------------------------
0    | 100          1.5          Accra 
1    | 120          2            Tema  
2    | 90           1.8          Kumasi
---------------------------------------
[3 cs x 3 feat]
```

### Output (after)

```text
===============================
     | sales_units  sales_price
-------------------------------
0    | 100          1.5        
1    | 120          2          
2    | 90           1.8        
-------------------------------
[3 cs x 2 feat]
```

The `^sales_` anchor requires the name to *start* with `"sales_"` — both
`"sales_units"` and `"sales_price"` qualify; `"city"` doesn't.

Example 2. Match a name *ending* in a given character, something `like()`'s
plain substring search can't express on its own.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"sales_units", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
     {"sales_price", Thread(pluss::make_series({1.5, 2.0, 1.8}))},
     {"city",        Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = orders.feataxis().match("e$");
std::cout << out;
```

### Output

```text
==================
     | sales_price
------------------
0    | 1.5        
1    | 2          
2    | 1.8        
------------------
[3 cs x 1 feat]
```

Only `"sales_price"` ends in `"e"`; `"sales_units"` ends in `"s"` and `"city"`
ends in `"y"`. `$` anchors the match to the end of the name, the same way
`^` anchored it to the start in Example 1 — neither anchor is implied unless
you write it yourself, since matching uses `std::regex_search`.

Example 3. Pass an invalid regular expression to see the call throw.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"sales_units", Thread(pluss::make_series({100.0, 120.0, 90.0}))}});

try {
  auto out = orders.feataxis().match("[");
} catch (const std::exception& e) {
  std::cout << std::string(e.what()).substr(0, 46) << "...\n";
}
```

### Output

```text
DataFrame::feataxis().match: invalid regex '['...
```

`"["` is an unterminated character class, not a valid ECMAScript regex, so
construction fails before any feature name is even checked. The full message
also includes the C++ standard library's own regex-error text after this
prefix, which varies by standard library implementation.

## Notes

- Uses `std::regex_search`, not `std::regex_match` — the pattern only needs to
  be found somewhere in the name unless you anchor it yourself with `^`/`$`.
- An invalid regular expression throws immediately, before any matching is
  attempted.
- A `pattern` that matches nothing returns a frame with zero features but the
  same case labels and row count.

## See Also

- [`DataFrameFeatureAxisAccessor::like`](like.md): a simpler substring-only
  search, for when a full regex is more than you need.
- [`DataFrame::features`](../dataframe/features.md): select by exact name(s).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="like.html">
    <span class="label">Previous</span>
    <span class="title">like</span>
  </a>
  <a class="next-link" href="note.html">
    <span class="label">Next</span>
    <span class="title">note</span>
  </a>
</div>
