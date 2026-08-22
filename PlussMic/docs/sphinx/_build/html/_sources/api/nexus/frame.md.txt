<!-- pluss-hand-authored -->

# Nexus::frame

[Back to Nexus API Reference](../../nexus_api_reference)

## Signatures

```cpp
Nexus::frame(std::string_view key) const -> DataFrame
Nexus::frame(std::int64_t index) const -> DataFrame
```

## What It Does

Fetches one table out of the workspace, either by name (`key`) or by position
(`index`) in [`keys()`](keys.md)/[`frames()`](frames.md) order. Both overloads throw
if the table doesn't exist — for a non-throwing alternative, use
[`get(key)`](get.md) (name-based only).

The positional overload supports negative indices Python-style: `-1` means the last
table, `-2` the second-to-last, and so on.

## Parameters

**key** (`std::string_view`)

The table name to fetch. Throws if unknown.

**index** (`std::int64_t`)

The zero-based position to fetch. Negative values count back from the end. Throws if
out of bounds.

## Returns

`DataFrame`
    The requested table.

## Examples

Example 1. Fetch by name.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", pluss::make_series({1.0, 2.0, 3.0})},
    {"name",        pluss::make_note({"Ada", "Bola", "Chen"})},
});

auto orders = DataFrame({
    {"order_id",    pluss::make_series({100.0, 101.0, 102.0})},
    {"customer_id", pluss::make_series({1.0, 2.0, 2.0})},
    {"amount",      pluss::make_series({50.0, 75.0, 20.0})},
});

auto biz = Nexus({
    {"customers", customers},
    {"orders", orders},
});

std::cout << biz.frame("orders");
```

### Output

```text
=====================================
      | order_id  customer_id  amount
-------------------------------------
0     | 100       1            50    
1     | 101       2            75    
2     | 102       2            20    
-------------------------------------
[3 cs x 3 feat]
```

Example 2. Fetch by position — `0` is `"customers"`, `1` is `"orders"`, matching
`keys()` order.

```cpp
std::cout << biz.frame(0).ncases() << " " << biz.frame(1).ncases();
```

### Output

```text
3 3
```

Example 3. A negative index counts from the end — `-1` is the last table,
equivalent to `1` here.

```cpp
auto last_by_index = biz.frame(1);
auto last_by_negative = biz.frame(-1);
std::cout << (last_by_index.equals(last_by_negative));
```

### Output

```text
true
```

Example 4. Both overloads throw on a bad argument.

```cpp
try {
  auto missing = biz.frame("products");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  auto out_of_range = biz.frame(5);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Nexus::frame: unknown key 'products'
Nexus: table index out of bounds
```

## Notes

- [`dloc(index)`](dloc.md) is an exact alias for the positional overload — the two
  overloads of `frame()` don't share one name by accident, but `dloc()` exists
  separately for readers coming from [`Atlas::dloc`](../../atlas_api_reference.md),
  which follows the same positional-access convention.
- [`get(key)`](get.md) is the non-throwing counterpart for the name-based overload
  only; there is no non-throwing positional lookup.
- Each call returns a copy of the `DataFrame` handle (cheap — `DataFrame` itself is
  reference-counted), not a fresh deep copy of the table data.

## See Also

- [`Nexus::get`](get.md): non-throwing name-based fetch.
- [`Nexus::dloc`](dloc.md): alias for the positional overload.
- [`Nexus::has`](has.md): check presence before fetching, if you'd rather avoid the
  `try`/`catch`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="flow.html">
    <span class="label">Previous</span>
    <span class="title">flow</span>
  </a>
  <a class="next-link" href="frames.html">
    <span class="label">Next</span>
    <span class="title">frames</span>
  </a>
</div>
