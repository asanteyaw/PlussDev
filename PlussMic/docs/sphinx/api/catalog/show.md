<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 273d577926cc -->

# Catalog::show

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
void Catalog::show(std::ostream& os) const
void Catalog::show() const
```

## What It Does

Two ways to look at a catalog:

- **`show(os)`** writes the catalog's ASCII-table view to the stream you give it — the same
  rendering `operator<<`, [`display()`](display.md), and [`info()`](info.md) all use.
- **`show()`** opens the catalog in the viewer window as **one card per key**. Each card carries the
  key's name, its dtype and its own length, with the values listed underneath, so keys of different
  lengths sit side by side without being padded to a common height. The window's Theme menu works as
  it does for a DataFrame. When no window can be opened (a terminal-only session), `show()` prints the
  ASCII view to `std::cout` instead. Calling it on an undefined catalog throws.

```{important} display(), info(), and show(os) all render identically — there is no separate metadata summary
Unlike Shelf::info(), which prints a name/kind/dtype/null-count summary distinct from
Shelf::to_string()'s row rendering, Catalog::info() and Catalog::display() both call the exact
same underlying render as show(os) and operator<<. All four produce byte-identical output;
there is currently no metadata-only summary view for Catalog.
```

## Parameters

**os** (`std::ostream&`)

The destination stream, for `show(os)`. `show()` takes no parameters.

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

Example 3. Open the catalog in the viewer, one card per key.

```cpp
auto ct = pluss::kindle::Catalog({
    "names"_k = pluss::kindle::Stream(pluss::note::make({"John", "Pele", "Mensah"})),
    "scores"_k = pluss::kindle::Stream(pluss::make_series({1.5, 2.5, 3.5, 4.5, 5.5})),
});

ct.show();
```

The window shows a `names` card with three values and a `scores` card with five, each at its own
length. Nothing is written to the console when a window opens.

## Notes

- `show()` opens a window, so there is no printed output to capture; use `show(os)` or
  [`to_string()`](to_string.md) when you need the text.
- `show(os)` gives identical output to [`display()`](display.md), [`info()`](info.md), and `operator<<` —
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
  <a class="next-link" href="similarity.html">
    <span class="label">Next</span>
    <span class="title">similarity</span>
  </a>
</div>
