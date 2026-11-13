<!-- pluss-hand-authored -->

# Nexus::defined

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
bool Nexus::defined() const noexcept
```

## What It Does

Reports whether the `Nexus` handle currently owns a usable backing workspace at all —
not whether it holds zero tables. Every ordinary constructor (the `vector<pair<...>>`
form and the brace-init form) produces a defined handle; the main way to end up with
an undefined one is moving from an existing `Nexus`, since `Nexus` has no public
default constructor of its own (unlike [`DataFrame`](../dataframe/defined.md), which
does).

```{important} defined() is about handle existence, not table count
A `Nexus` built from an empty table list is still `defined() == true` — it owns a
real, usable workspace that just happens to hold zero tables. For "does this
workspace hold any tables?", use [`empty()`](empty.md) instead.
```

## Parameters

`None`

## Returns

`bool`
    `true` if the handle owns a usable backing workspace; `false` for a moved-from
    handle.

## Examples

Example 1. An ordinarily constructed `Nexus` is defined.

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

std::cout << std::boolalpha << biz.defined();
```

### Output

```text
true
```

Example 2. A moved-from handle is not.

```cpp
auto moved_out = std::move(biz);
std::cout << std::boolalpha << biz.defined();
```

### Output

```text
false
```

`biz` no longer owns the underlying workspace — `moved_out` does. `defined()` itself
is safe to call on the moved-from handle (it's `noexcept`, just a pointer check), but
almost everything else on `biz` is not, as the next example shows.

Example 3. Calling an ordinary method on a moved-from handle throws.

```cpp
try {
  auto n = biz.ntables();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Nexus: undefined handle
```

`ntables()` (like nearly every other `Nexus` method) needs a real backing workspace
to read from. Checking `defined()` first, as this page recommends, lets calling code
sidestep this throw entirely — useful after a move, or around any code path that
might have handed the workspace off elsewhere.

## Notes

- `defined()` is `noexcept` and cheap — it's a pointer check, not a workspace scan.
- Unlike [`DataFrame`](../dataframe/defined.md), `Nexus` has no public default
  constructor — you cannot write `Nexus nexus;` and get an undefined handle directly.
  Moving from an existing `Nexus` is the ordinary way to observe `defined() == false`.
- A `Nexus` constructed from an empty table list is defined but empty — see
  [`empty()`](empty.md) for that distinction.

## See Also

- [`Nexus::unique`](unique.md): a related but different existence question — whether
  this handle is the *sole* owner of its workspace, not whether it owns one at all.
- [`Nexus::empty`](empty.md): whether a defined workspace holds zero tables.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="deduce.html">
    <span class="label">Previous</span>
    <span class="title">deduce</span>
  </a>
  <a class="next-link" href="detect_duplicates.html">
    <span class="label">Next</span>
    <span class="title">detect_duplicates</span>
  </a>
</div>
