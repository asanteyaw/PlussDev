<!-- pluss-hand-authored -->

# Nexus::duplicates

[Back to Nexus API Reference](../../nexus_api_reference)

## Signatures

```cpp
Nexus::duplicates(std::string_view table) const -> DataFrame

Nexus::duplicates(std::string_view table, std::initializer_list<std::string_view> keys) const -> DataFrame
```

## What It Does

An exact alias for [`detect_duplicates()`](detect_duplicates.md) — both overloads call
straight through with no behavioral difference. `duplicates` is the shorter, more
conversational spelling.

## Parameters

See [`detect_duplicates()`](detect_duplicates.md) — identical here.

## Returns

Identical to [`detect_duplicates()`](detect_duplicates.md).

## Examples

Example 1. `duplicates()` and `detect_duplicates()` return identical results.

```cpp
using namespace pluss::table;

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0, 3.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy", "Cy2"}))},
});

auto biz = Nexus({{"customers", customers}});

auto dups = biz.duplicates("customers", {"customer_id"});
std::cout << dups.ncases();
```

### Output

```text
2
```

## Notes

- Purely a naming alternative — see [`detect_duplicates()`](detect_duplicates.md) for the
  full documentation.

## See Also

- [`Nexus::detect_duplicates`](detect_duplicates.md): the canonical name this aliases.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="drop.html">
    <span class="label">Previous</span>
    <span class="title">drop</span>
  </a>
  <a class="next-link" href="empty.html">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>
