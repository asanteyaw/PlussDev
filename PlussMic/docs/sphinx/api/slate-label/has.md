<!-- pluss-hand-authored -->

# SlateLabelAccessor::has

[Back to SlateLabelAccessor API Reference](../../slate_label_accessor_api_reference)

## Signatures

```cpp
bool SlateLabelAccessor::has() const
Slate SlateLabelAccessor::has(const Slate& values) const
```

## What It Does

Two unrelated questions share this name:

- **`has()`** (no arguments) answers a structural question: have case labels
  ever been explicitly attached to this slate (via [`set()`](set.md) or a
  derived operation), as opposed to still sitting at the default integer
  range? A direct forward to `Slate::has_caselbl()`.
- **`has(values)`** answers an elementwise membership question over the
  **case labels themselves** (not the slate's own data): for each row, is
  its case label equal to *any* of `values`? The result keeps the same case
  labels as `*this`, `display_name` `"has"`.

## Parameters

**values** (`const Slate&`, one overload)

A collection of values; each row's case label is tested for membership
against all of them.

## Returns

`bool`
    (No-argument overload) Whether case labels have been explicitly set.

`Slate`
    (`values`-taking overload) A `Bool` slate, one entry per row, with the
    same case labels as `*this`.

## Examples

Example 1. Check structural state before and after attaching labels.

```cpp
using namespace pluss::table;

auto sales = Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}));
std::cout << sales.label().has();

auto tagged = sales.label().set(
    Slate(pluss::make_note({"east", "west", "east", "north"})));
std::cout << tagged.label().has();
```

### Output

```text
false
true
```

`sales` still sits at the default integer range — no case labels attached
yet. `tagged` had `set()` called on it, so `has()` becomes `true`.

Example 2. Elementwise membership against several values, testing the case
labels rather than `tagged`'s own numeric data.

```cpp
auto in_set = tagged.label().has(Slate(pluss::make_note({"west", "north"})));
std::cout << in_set;
```

### Output

```text
east   false
west   true
east   false
north  true

ptype: Series
```

Rows whose case label is `"west"` or `"north"` flag `true` — the check runs
against the labels shown on the left, not against `tagged`'s own `sales`
values.

## Notes

- The no-argument overload is purely structural — it says nothing about
  whether any particular label value is present.
- `has(values)` tests the case labels, not the slate's own data — for a
  membership test over the data itself, use `Slate::has()` directly.
- Identical to calling `Slate::has_caselbl()` (no-argument overload)
  directly.

## See Also

- [`SlateLabelAccessor::set`](set.md): the operation that turns `has()` from
  `false` to `true`.
- [`SlateLabelAccessor::pos`](pos.md),
  [`SlateLabelAccessor::find`](find.md): locate *where* a label value
  occurs, rather than just whether it does.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="freq.html">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="intersect.html">
    <span class="label">Next</span>
    <span class="title">intersect</span>
  </a>
</div>
