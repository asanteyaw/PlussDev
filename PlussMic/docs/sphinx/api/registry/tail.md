<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 74a49a69c0ce -->

# Registry::tail

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::tail(std::int64_t n = 5) const
```

## What It Does

Returns a new registry keeping only the **last `n` values of every field**,
applying [`Slot::tail(n)`](../slot/tail.md) independently to each field — the
mirror image of [`head()`](head.md).

```{important} Each field is clipped to its own length, independently
Same caveat as [`head()`](head.md): `Registry` fields are not required to share a
common length. `tail(n)` never throws for `n` larger than a field's own size — that
field's `Slot::tail()` clamps internally, so a shorter field just returns
everything it has, taken from its own end.
```

## Parameters

**n** (`std::int64_t`, default `5`)

The number of trailing values to keep per field. Values less than `0` are
clamped to `0`.

## Returns

`Registry`
    A new registry with each field's `Slot` truncated to its own last `n` values
    (or fewer, if the field itself is shorter).

## Examples

Example 1. Keep the last two rows of a uniform registry.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3), pluss::make_series(4)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi"), pluss::make_note("Ho")})},
});

auto out = rg.tail(2);
auto ids = out.fields("id");
std::cout << "[";
for (std::int64_t i = 0; i < ids.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(ids.at(i)).host<std::int64_t>()[0];
}
std::cout << "]";
```

### Output

```text
[3, 4]
```

Example 2. Call `tail()` with no argument on a registry whose `id` field has seven cells and whose `city` field has three; `n` defaults to 5.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3), pluss::make_series(4),
                                  pluss::make_series(5), pluss::make_series(6), pluss::make_series(7)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.tail();
std::cout << out.fields("id") << "\n" << out.fields("city");
```

### Output

```text
0  3
1  4
2  5
3  6
4  7
ptype: Series

0  Accra
1  Tema
2  Kumasi
ptype: Note
```

Each field is cut on its own: `id` keeps its last five cells, 3 to 7, while `city` has only three and keeps them all.

## Notes

- Applies [`Slot::tail()`](../slot/tail.md) independently per field — a ragged
  registry can end up with fields of different sizes, same as [`head()`](head.md).
- `n < 0` is clamped to `0`, returning an all-fields-empty registry rather than
  throwing.
- Requires a defined registry — throws `"Registry: undefined handle"` on an
  undefined one.

## See Also

- [`Registry::head`](head.md): the mirror-image, first-`n`-values selection.
- [`Registry::slice`](slice.md): an arbitrary `[start, stop)` window, of which
  `tail(n)` is a specialization (`slice(size() - n, size())` per field).
- [`Catalog::tail`](../catalog/tail.md): the equivalent operation on the kindle
  model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tabelize.html">
    <span class="label">Previous</span>
    <span class="title">tabelize</span>
  </a>
  <a class="next-link" href="take.html">
    <span class="label">Next</span>
    <span class="title">take</span>
  </a>
</div>
