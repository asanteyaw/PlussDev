<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 273d577926cc -->

# Registry::show

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
void Registry::show(std::ostream& os) const
void Registry::show() const
```

## What It Does

The `std::ostream&` overload writes the registry's overview — one row per field, with what its
slot holds (`Series`, `Note`, …, or `Mixed`), how many states it has, and a preview of them —
the same text `operator<<` writes. [`to_string()`](to_string.md) is different: it is the full
JSON dump.

The no-argument overload opens the registry in the PlussView window as a **ragged grid**, the
window a [`Catalog`](../catalog/show.md) opens: the fields sit side by side, each exactly as
long as its own slot, and where a field has no state at a row nothing is drawn (which is not
a `NULL`). Inside a PlussII notebook it hands the registry to the notebook as the same grid
as an HTML table instead of opening a window.

## Parameters

**os** (`std::ostream&`, stream overload)

The destination stream.

## Returns

`void`
    Writes to `os` (or opens the window) as a side effect; produces no return value.

## Examples

Example 1. Print a registry's overview.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});

rg.show(std::cout);
```

### Output

```text
========================================================================================================================
scribble::Registry  fields=2
states=2
------------------------------------------------------------------------------------------------------------------------
field | ptype  | states | preview
------------------------------------------------------------------------------------------------------------------------
id    | Series | 2      | [1, 2]
city  | Note   | 2      | [Accra, Tema]
========================================================================================================================
```

Example 2. `show(os)` writes the same text as `operator<<`.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
});

std::ostringstream via_show;
rg.show(via_show);

std::ostringstream via_operator;
via_operator << rg;

std::cout << std::boolalpha << (via_show.str() == via_operator.str());
```

### Output

```text
true
```

## Notes

- The overview shows only the rows that fit, so it stays short for a registry with many fields
  and costs the same however long its slots are.
- A cell with no value is shown as `NULL`.

## See Also

- [`Registry::to_string`](to_string.md): the full JSON dump of the registry.
- [`Catalog::show`](../catalog/show.md): the equivalent method on the kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shape.html">
    <span class="label">Previous</span>
    <span class="title">shape</span>
  </a>
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
