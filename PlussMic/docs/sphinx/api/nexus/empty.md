<!-- pluss-hand-authored -->

# Nexus::empty

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::empty() const -> bool
```

## What It Does

Reports whether the workspace holds zero tables — `true` if either the handle is
undefined at all, or it's defined but was built from (or reduced to) an empty table
list. This is a strict superset of [`defined()`](defined.md): every undefined handle
is also empty, but a defined handle can be empty too.

## Parameters

`None`

## Returns

`bool`
    `true` if the handle is undefined, or defined with zero tables; `false` if it
    holds at least one table.

## Examples

Example 1. A workspace with tables in it is not empty.

```cpp
using namespace pluss::table;

auto customers = DataFrame({
    {"customer_id", pluss::make_series({1.0, 2.0, 3.0})},
    {"name",        pluss::make_note({"Ada", "Bola", "Chen"})},
});

auto biz = Nexus({{"customers", customers}});
std::cout << std::boolalpha << biz.empty();
```

### Output

```text
false
```

Example 2. A `Nexus` built from an empty table list is defined but empty — the
"defined, holds nothing" case.

```cpp
auto blank = Nexus(std::vector<std::pair<std::string, DataFrame>>{});
std::cout << std::boolalpha << blank.defined() << " " << blank.empty();
```

### Output

```text
true true
```

`blank` owns a real, usable workspace (`defined() == true`) — it's just a workspace
with nothing in it. Calling a method like [`keys()`](keys.md) on it works fine and
returns an empty vector, unlike calling a method on an undefined handle, which
throws.

Example 3. A moved-from (undefined) handle is also empty, via the other branch of
the `||`.

```cpp
auto moved_out = std::move(biz);
std::cout << std::boolalpha << biz.defined() << " " << biz.empty();
```

### Output

```text
false true
```

`biz` is undefined after the move, so `empty()` reports `true` without even checking
a table count — `defined() == false` alone is enough to satisfy `empty()`.

## Notes

- `empty()` is `!defined() || ntables() == 0` — either branch makes it `true`.
- Contrast with [`defined()`](defined.md), which only answers the handle-existence
  half of this question, and [`ntables()`](ntables.md), which gives the exact count
  instead of a boolean.

## See Also

- [`Nexus::defined`](defined.md): the handle-existence half of what `empty()` checks.
- [`Nexus::ntables`](ntables.md): the exact table count, when a boolean isn't enough.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="duplicates.html">
    <span class="label">Previous</span>
    <span class="title">duplicates</span>
  </a>
  <a class="next-link" href="fingerprint.html">
    <span class="label">Next</span>
    <span class="title">fingerprint</span>
  </a>
</div>
