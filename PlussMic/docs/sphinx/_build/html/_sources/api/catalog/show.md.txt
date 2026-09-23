<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 86826e14ef4e -->

# Catalog::show

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::show(std::ostream& os) const -> void
```

## What It Does

Renders the catalog's ASCII-table view directly to `os` — the same rendering
`operator<<`, [`display()`](display.md), and [`info()`](info.md) all use internally.
Unlike `Thread::show()`, `Catalog::show()` **requires** an explicit stream argument; there
is no no-argument overload, so there's no risk of it silently opening a GUI viewer window
the way some other `show()`-named methods in this codebase can.

```{important} display(), info(), and show() all render identically — there is no separate metadata summary
Unlike Thread::info(), which prints a name/kind/dtype/null-count summary distinct from
Thread::show()'s row rendering, Catalog::info() and Catalog::display() both call the exact
same underlying render as show() and operator<<. All four produce byte-identical output;
there is currently no metadata-only summary view for Catalog.
```

## Parameters

**os** (`std::ostream&`)
    The destination stream.

## Returns

`void`
    Writes to `os` as a side effect; produces no return value.

## Examples

Example 1. Render to an explicit stream.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

ct.show(std::cout);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [1, 2, 3]                                        
========================================================================================================================
```

Example 2. Capture the same rendering into a `std::ostringstream` instead of the console.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

std::ostringstream captured;
ct.show(captured);
std::cout << "captured " << captured.str().size() << " characters";
```

### Output

```text
captured 761 characters
```

## Notes

- Requires an explicit stream — there is no no-argument overload, unlike some other
  `show()`-named methods in this codebase.
- Identical output to [`display()`](display.md), [`info()`](info.md), and `operator<<` —
  see the important callout above.

## See Also

- [`Catalog::display`](display.md) / [`Catalog::info`](info.md): equivalent renders that
  write straight to `std::cout` instead of an explicit stream.
- [`Catalog::to_string`](to_string.md): the same rendering, returned as a `std::string`
  instead of written to a stream.

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
