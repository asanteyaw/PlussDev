<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: adfc335854d0 -->

# Archive::to_string

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::to_string() const -> std::string
```

## What It Does

Renders the archive's boxed-table summary as a `std::string` — one row per catalog,
showing depth-index, depth label (if any), key count, min/max record count across the
archive, and the key list. Same rendering `operator<<` and [`show()`](show.md) write to a
stream, minus the trailing newline those two append.

## Parameters

`None`

## Returns

`std::string`
    The rendered archive summary table, without a trailing newline.

## Examples

Example 1. Render an archive to a string.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})) });
auto archive = pluss::kindle::Archive({c0}, pluss::kindle::Stream(pluss::make_note({"jan"})));

std::cout << archive.to_string();
```

### Output

```text
========================================================================================================================
kindle::Archive  depth=1
depth_labels=yes
------------------------------------------------------------------------------------------------------------------------
frame  | label                  | keys     | min_cs | max_cs | keys                                                     
------------------------------------------------------------------------------------------------------------------------
0      | jan                    | 1        | 2      | 2      | [id]                                                     
========================================================================================================================
```

Example 2. `to_string()` is one byte shorter than the equivalent `show()` capture — the
missing trailing newline, same pattern as [`Catalog::to_string`](../catalog/to_string.md)/
[`Stream::to_string`](../stream/to_string.md).

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})) });
auto archive = pluss::kindle::Archive({c0});

auto str = archive.to_string();
std::ostringstream oss;
archive.show(oss);

std::cout << "to_string length: " << str.size() << "\n";
std::cout << "show length: " << oss.str().size();
```

### Output

```text
to_string length: 766
show length: 767
```

## Notes

- This is a summary view — one row per catalog — not the full per-catalog table rendering;
  use [`catalog(i)`](catalog.md)`.to_string()` for a specific catalog's own contents.
- One byte shorter than [`show()`](show.md)/`operator<<`'s output — see Example 2.

## See Also

- [`Archive::show`](show.md): the same rendering, written to a stream instead of returned
  as a string, with a trailing newline this method omits.
- [`Catalog::to_string`](../catalog/to_string.md): the per-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="unique.html">
    <span class="label">Next</span>
    <span class="title">unique</span>
  </a>
</div>
