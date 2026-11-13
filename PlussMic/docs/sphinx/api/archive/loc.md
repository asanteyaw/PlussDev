<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ea90dbb3db25 -->

# Archive::loc

[Back to Archive API Reference](../../archive_api_reference)

## Signatures

```cpp
Catalog Archive::loc(std::string_view label) const
Catalog Archive::loc(std::int64_t label) const
Catalog Archive::loc(const pluss::Timeline& label) const
```

## What It Does

Fetches the catalog whose **depth label** matches `label` — the label-based counterpart to
[`catalog()`](catalog.md)/[`dloc()`](dloc.md)'s positional access. The overload is chosen
by `label`'s C++ type, and must match the depth labels' own payload kind: the
`std::string_view` overload for `Note`-kind depth labels, `std::int64_t` for numeric
(`Series`) depth labels, `pluss::Timeline` for `Timeline`-kind depth labels.

`loc(std::int64_t)` requires the depth labels to actually be `Series`-kind (numeric); it
throws immediately — `"Archive::loc(int64): depth labels must be numeric Series"` — if
they are `Note`, `Timeline`, or `TimeIndex` instead, rather than attempting to coerce them.
This was fixed after an earlier build silently mis-parsed `Note`-kind (text) depth labels
through a lossy numeric-cast path: every unparseable label collapsed to `0` with its
validity mask dropped, so `loc(int64_t{0})` matched whatever catalog happened to be first
regardless of its real text label, and every other integer threw `"label not found"` even
when a label search was fundamentally the wrong operation for a text-labeled archive. The
fix makes that mismatch fail loudly and immediately instead of guessing.

## Parameters

**label** (`std::string_view` / `std::int64_t` / `const pluss::Timeline&`)
    The depth label to search for. Its C++ type must match the depth labels' own payload
    kind; `loc(std::int64_t)` throws immediately if the depth labels aren't `Series`-kind
    rather than attempting to coerce them (see What It Does above).

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

Example 3. Calling `loc(std::int64_t)` on a `Note`-labeled (text) archive throws
immediately instead of silently mismatching.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})) });
auto c1 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({2.0})) });
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

try {
  archive.loc(std::int64_t(0));
} catch (const std::exception& e) {
  std::cout << "loc(0) threw: " << e.what();
}
```

### Output

```text
loc(0) threw: Archive::loc(int64): depth labels must be numeric Series
```

`archive`'s depth labels are `Note`-kind (`"train"`/`"test"`), so `loc(std::int64_t)` is
the wrong overload for this archive regardless of which integer is passed — use
[`loc(std::string_view)`](loc.md) instead, as in Example 1.

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
- `loc(std::int64_t)` throws `"Archive::loc(int64): depth labels must be numeric Series"`
  if the depth labels aren't `Series`-kind — check with
  [`label().values()`](label.md)'s `kind()` first if you're unsure.
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
