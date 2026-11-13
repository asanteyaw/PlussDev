<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ff54d475b13b -->

# Slate::name

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
std::string Slate::name() const
Slate Slate::name(std::string value) const
```

## What It Does

Two overloads, chosen by whether an argument is passed: the zero-argument form is a
**getter** that reads the slate's currently attached name; the one-argument form is a
**setter** that returns a new slate with a name attached, leaving the original
unmodified. This name is presentation metadata used for labeling and identification (for
example, [`SlatePlotAccessor`](../../slate_plot_api_reference)'s chart methods use it to
label axes and legends when wrapping a `Slate` into a chart) — it is not schema truth the
way a `DataFrame` feature name is, and it never affects the slate's own values.

```{important} The getter throws if no name is set — check named() first
Unlike many "get the current X" accessors that return a default or empty value when
nothing has been set, `name()` (the getter) throws `"Slate::name: name is not set"` if
the slate has no name attached. Always check [`named()`](named.md) first (or catch the
exception) before calling the getter on a slate that might be unnamed.
```

## Parameters

**value** (`std::string`, setter overload only)
    The name to attach. Must be non-empty — passing an empty string throws
    `"Slate::name(value): name must not be empty"`.

## Returns

For the getter: `std::string`
    The slate's currently attached name.

For the setter: `Slate`
    A new slate with `value` attached as its name; the values and any other metadata
    (validity mask, row labels) are carried over unchanged.

## Examples

Example 1. Attach a name, then read it back.

```cpp
using namespace pluss::table;

auto balance = Slate(pluss::make_series({100.0, 250.5, 75.25}));
auto named_balance = balance.name("balance");

std::cout << named_balance.name();
```

### Output

```text
balance
```

`balance` itself is left unmodified — `name(...)` is non-mutating, so `named_balance` is a
distinct copy carrying the new name.

Example 2. Calling the getter on an unnamed slate throws.

```cpp
try {
  auto bad = balance.name();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::name: name is not set
```

`balance` (the original, still unnamed) throws because no name was ever attached to it —
see [`named()`](named.md) for how to check first.

Example 3. An empty name throws immediately.

```cpp
try {
  auto bad = balance.name("");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::name(value): name must not be empty
```

## Notes

- The getter throws `"Slate::name: name is not set"` on an unnamed slate — check
  [`named()`](named.md) first, or catch the exception.
- The setter throws `"Slate::name(value): name must not be empty"` on an empty string.
- Both overloads are non-mutating; the setter always returns a new `Slate`.
- This is presentation metadata, not schema — it never affects the slate's own values or
  comparisons between slates.

## See Also

- [`Slate::named`](named.md): check whether a name is attached without risking the
  getter's throw.
- [`Slate::unname`](unname.md): remove an attached name.
- [`Slate::plot`](plot.md): chart methods use a slate's name to label axes and legends
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
