<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ff54d475b13b -->

# Shelf::name

[Back to Shelf API Reference](../../shelf_api_reference)

## Signatures

```cpp
std::string Shelf::name() const
Shelf Shelf::name(std::string value) const
```

## What It Does

Two overloads, chosen by whether an argument is passed: the zero-argument form is a
**getter** that reads the shelf's currently attached name; the one-argument form is a
**setter** that returns a new shelf with a name attached, leaving the original
unmodified. This name is presentation metadata used for labeling and identification (for
example, [`ShelfPlotAccessor`](../../shelf_plot_api_reference)'s chart methods use it to
label axes and legends when wrapping a `Shelf` into a chart) — it is not schema truth the
way a `DataFrame` feature name is, and it never affects the shelf's own values.

```{important} The getter throws if no name is set — check named() first
Unlike many "get the current X" accessors that return a default or empty value when
nothing has been set, `name()` (the getter) throws `"Shelf::name: name is not set"` if
the shelf has no name attached. Always check [`named()`](named.md) first (or catch the
exception) before calling the getter on a shelf that might be unnamed.
```

## Parameters

**value** (`std::string`, setter overload only)

The name to attach. Must be non-empty — passing an empty string throws
`"Shelf::name(value): name must not be empty"`.

## Returns

For the getter: `std::string`
    The shelf's currently attached name.

For the setter: `Shelf`
    A new shelf with `value` attached as its name; the values and any other metadata
    (validity mask, row labels) are carried over unchanged.

## Examples

Example 1. Attach a name, then read it back.

```cpp
using namespace pluss::table;

auto balance = Shelf(pluss::make_series({100.0, 250.5, 75.25}));
auto named_balance = balance.name("balance");

std::cout << named_balance.name();
```

### Output

```text
balance
```

`balance` itself is left unmodified — `name(...)` is non-mutating, so `named_balance` is a
distinct copy carrying the new name.

Example 2. Calling the getter on an unnamed shelf throws.

```cpp
try {
  auto bad = balance.name();
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
Shelf::name: name is not set
```

`balance` (the original, still unnamed) throws because no name was ever attached to it —
see [`named()`](named.md) for how to check first.

Example 3. An empty name throws immediately.

```cpp
try {
  auto bad = balance.name("");
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
Shelf::name(value): name must not be empty
```

## Notes

- The getter throws `"Shelf::name: name is not set"` on an unnamed shelf — check
  [`named()`](named.md) first, or catch the exception.
- The setter throws `"Shelf::name(value): name must not be empty"` on an empty string.
- Both overloads are non-mutating; the setter always returns a new `Shelf`.
- This is presentation metadata, not schema — it never affects the shelf's own values or
  comparisons between shelves.

## See Also

- [`Shelf::named`](named.md): check whether a name is attached without risking the
  getter's throw.
- [`Shelf::unname`](unname.md): remove an attached name.
- [`Shelf::plot`](plot.md): chart methods use a shelf's name to label axes and legends
  when one is set.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mul.html">
    <span class="label">Previous</span>
    <span class="title">mul</span>
  </a>
  <a class="next-link" href="named.html">
    <span class="label">Next</span>
    <span class="title">named</span>
  </a>
</div>
