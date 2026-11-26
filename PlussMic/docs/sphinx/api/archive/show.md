<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 273d577926cc -->

# Archive::show

[Back to Archive API Reference](../../archive_api_reference)

## Signatures

```cpp
void Archive::show(std::ostream& os) const
void Archive::show() const
```

## What It Does

The `std::ostream&` overload writes the archive's summary to `os` (a short list of facts; see
[`to_string()`](to_string.md)) — the same rendering `operator<<` writes, and the same content
`to_string()` returns as a string, plus a trailing newline `to_string()` omits.

The no-argument overload opens the archive in the PlussView window: one catalog at a time, as
the ragged grid a [`Catalog`](../catalog/show.md) opens, with a **Level** menu (First,
Previous, Next, Last, Go to…) to move between catalogs. Inside a PlussII notebook it hands the
archive to the notebook as the same summary instead of opening a window.

## Parameters

**os** (`std::ostream&`)

The destination stream to write to (e.g. `std::cout`, an `std::ostringstream`).

## Returns

`void`
    Writes to `os` as a side effect; produces no return value.

## Examples

Example 1. Write directly to `std::cout`.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})) });
auto archive = pluss::kindle::Archive({c0}, pluss::kindle::Stream(pluss::make_note({"jan"})));

archive.show(std::cout);
```

### Output

```text
========================================================================================================================
Archive
------------------------------------------------------------------------------------------------------------------------
fact            | value
------------------------------------------------------------------------------------------------------------------------
depth           | 1 catalogs
keys            | 1 each
entries per key | 2 each
key names       | the same in every catalog
ptypes          | Series × 1
depth labels    | jan to jan
========================================================================================================================
```

Example 2. `show()` and `operator<<` produce byte-identical output.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})) });
auto archive = pluss::kindle::Archive({c0});

std::ostringstream via_show;
archive.show(via_show);

std::ostringstream via_operator;
via_operator << archive;

std::cout << "equal: " << (via_show.str() == via_operator.str());
```

### Output

```text
equal: 1
```

## Notes

- Byte-identical to `operator<<`'s output — see Example 2.
- One byte longer than [`to_string()`](to_string.md): the trailing newline `show()`
  appends that `to_string()` omits.
- This is a summary view — one row per catalog — not the full per-catalog table rendering.

## See Also

- [`Archive::to_string`](to_string.md): the same rendering, returned as a string (minus the
  trailing newline) instead of written to a stream.
- [`Catalog::show`](../catalog/show.md): the per-catalog equivalent.

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
