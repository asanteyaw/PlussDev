<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0292317d45e7 -->

# ArchiveLabelAccessor::has

[Back to ArchiveLabelAccessor API Reference](../../archive_label_accessor_api_reference)

## Signature

```cpp
bool ArchiveLabelAccessor::has() const
```

## What It Does

Reports whether the archive currently carries depth labels — `true` after a prior
[`set(...)`](set.md), `false` both before any label is set and after
[`reset()`](reset.md).

```{important} Unlike Stream::lbl()::has(), values() throws instead of falling back
[`StreamLabelAccessor::values()`](../stream-lbl/values.md) always succeeds, synthesizing
a positional range when unlabeled. `ArchiveLabelAccessor::values()` does not — it
throws outright if `has()` is `false`. `has()` is not just a "nice to check" accuracy
tool here; it's the required precondition before calling
[`values()`](values.md)/[`align()`](align.md) at all.
```

## Parameters

`None`

## Returns

`bool`
    `true` if depth labels are set, `false` otherwise.

## Examples

Example 1. `has()` toggles across the set/reset lifecycle.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({10.0})) });
auto c1 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({20.0})) });
auto archive = pluss::kindle::Archive({c0, c1});

std::cout << "initial: " << std::boolalpha << archive.label().has() << "\n";

auto labeled = archive.label().set(pluss::kindle::Stream(pluss::make_note({"q1", "q2"})));
std::cout << "after set: " << std::boolalpha << labeled.label().has() << "\n";

auto back = labeled.label().reset();
std::cout << "after reset: " << std::boolalpha << back.label().has();
```

### Output

```text
initial: false
after set: true
after reset: false
```

## Notes

- See the important callout above — `has()` is a required precondition here, not just
  an accuracy check.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.
- Every method on this accessor returns a *new* `Archive`, so `has()` must be
  re-checked on the result of `set()`/`reset()`, not on the original archive.

## See Also

- [`ArchiveLabelAccessor::values`](values.md): throws unless `has()` is `true`.
- [`ArchiveLabelAccessor::align`](align.md): also requires `has()` to be `true`.
- [`ArchiveLabelAccessor::set`](set.md) / [`reset()`](reset.md): the methods that
  actually change what `has()` reports.
- [`Archive::label`](../archive/label.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="align.html">
    <span class="label">Previous</span>
    <span class="title">align</span>
  </a>
  <a class="next-link" href="reset.html">
    <span class="label">Next</span>
    <span class="title">reset</span>
  </a>
</div>
