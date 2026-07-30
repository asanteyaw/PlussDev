<!-- pluss-hand-authored -->

# Archive::loc

[Back to Archive API Reference](../../archive_api_reference)

## Signatures

```cpp
Archive::loc(std::string_view label) const -> Catalog
Archive::loc(std::int64_t label) const -> Catalog
Archive::loc(const pluss::Timeline& label) const -> Catalog
```

## What It Does

Fetches the catalog whose **depth label** matches `label` — the label-based counterpart to
[`catalog()`](catalog.md)/[`dloc()`](dloc.md)'s positional access. The overload is chosen
by `label`'s C++ type, and must match the depth labels' own payload kind: the
`std::string_view` overload for `Note`-kind depth labels, `std::int64_t` for numeric
(`Series`) depth labels, `pluss::Timeline` for `Timeline`-kind depth labels.

```{important} loc(int64_t) silently mismatches on Note-kind depth labels — confirmed bug
`loc(std::int64_t)` internally casts the depth-label stream to `Series[int64]` before
comparing. If the depth labels are actually `Note`-kind (text, e.g. `"train"`/`"test"`),
that cast can't parse them — each unparseable label silently becomes `0` (a failed
Note→int64 parse, with the validity mask dropped before comparison), **not** a thrown
error. The practical, verified effect: on a `Note`-labeled archive, `loc(int64_t{0})`
always matches the *first* catalog regardless of its actual text label, and every other
integer throws `"Archive::loc(int64): label not found"` — even though none of the depth
labels are really the number `0`. This reproduces on the current build; treat
`loc(int64_t)` as only meaningful for a genuinely numeric-labeled archive (verify with
[`label().values()`](label.md)'s `kind` first), not as a safe no-op on a text-labeled one.
```

## Parameters

**label** (`std::string_view` / `std::int64_t` / `const pluss::Timeline&`)
    The depth label to search for. Its C++ type must match the depth labels' own payload
    kind — see the important callout above for what happens when it doesn't.

## Returns

`Catalog`
    The catalog whose depth label equals `label`.

## Examples

Example 1. `loc(std::string_view)` on a `Note`-kind (text) depth-labeled archive.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})) });
auto c1 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({2.0})) });
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

std::cout << archive.loc("test");
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 1       | [2]                                              
========================================================================================================================
```

Example 2. `loc(std::int64_t)` on a genuinely numeric-labeled archive works correctly.

```cpp
auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})) });
auto c1 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({2.0})) });
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_series({100.0, 200.0})));

std::cout << archive.loc(std::int64_t(200));
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 1       | [2]                                              
========================================================================================================================
```

Example 3. The confirmed `loc(int64_t)` mismatch on a `Note`-labeled archive — see the
important callout above.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})) });
auto c1 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({2.0})) });
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

std::cout << "loc(0):\n" << archive.loc(std::int64_t(0));
try {
  archive.loc(std::int64_t(1));
} catch (const std::exception& e) {
  std::cout << "loc(1) threw: " << e.what();
}
```

### Output

```text
loc(0):
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 1       | [1]                                              
========================================================================================================================
loc(1) threw: Archive::loc(int64): label not found
```

`loc(0)` matched the catalog labeled `"train"` — not because `"0"` is a real label, but
because of the parse-failure quirk described above.

Example 4. `loc(const pluss::Timeline&)` on a `Timeline`-kind depth-labeled archive.

```cpp
auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})) });
auto c1 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({2.0})) });
auto archive = pluss::kindle::Archive({c0, c1},
    pluss::kindle::Stream(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-02-01T00:00:00Z"})));

auto key = pluss::make_timeline({"2024-02-01T00:00:00Z"});
std::cout << archive.loc(key);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 1       | [2]                                              
========================================================================================================================
```

## Notes

- Requires depth labels to be set at all; throws `"Archive::loc(<type>): depth labels are
  not set"` otherwise (each overload names itself in the message).
- A genuinely unmatched label (correct kind, but no such value) throws
  `"Archive::loc(<type>): label not found"`.
- See the important callout above for `loc(std::int64_t)`'s confirmed silent-mismatch
  behavior on non-numeric depth labels — check the depth-label kind before relying on this
  overload.
- The `Timeline` overload additionally requires `label` to be a defined, single-element
  `Timeline`; a multi-element or undefined `Timeline` throws before any search happens.

## See Also

- [`Archive::catalog`](catalog.md) / [`Archive::dloc`](dloc.md): positional access by
  depth-index instead of label.
- [`Archive::label`](label.md): inspect, set, or check the kind of the depth labels this
  method searches.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="label.html">
    <span class="label">Previous</span>
    <span class="title">label</span>
  </a>
  <a class="next-link" href="map_catalogs.html">
    <span class="label">Next</span>
    <span class="title">map_catalogs</span>
  </a>
</div>
