<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3d267dd67fa6 -->

# CatalogNoteAccessor::strip

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signatures

```cpp
Catalog CatalogNoteAccessor::strip() const

Catalog CatalogNoteAccessor::strip(std::string_view chars) const
```

## What It Does

Trims characters from **both** edges of every bound key's text, returning a new
catalog with those keys replaced. The no-argument overload trims whitespace; the
`chars`-argument overload trims any of the given characters, in any order, from
either end.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Every method on this accessor -- not just strip() -- routes through
Stream::nt() for each bound key. If any bound key isn't Note-kind (Series,
Timeline, TimeIndex), the call throws "Thread::strip is only valid for Note payload"
(the exact message names whichever method actually ran). Since Catalog::nt() binds
*every* key of the catalog by default, this is the most common way to trip the throw
-- project down to just the text keys first with Catalog::keys(names) or
Catalog::keyaxis().note() before calling .nt().
```

## Parameters

**chars** (`std::string_view`, one-argument overload only)
    The set of characters to strip from both edges. Every leading or trailing
    character that appears anywhere in `chars` is removed, stopping at the first
    character on each side that doesn't.

## Returns

`Catalog`
    A new catalog with every bound key's text trimmed on both edges.

## Examples

Example 1. No-argument overload trims leading and trailing whitespace.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "city"_k = pluss::kindle::Stream(pluss::make_note({"  Accra  ", "  Tema  "})),
});
auto out = ct.nt().strip();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [city]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
city                     | Note         | Note             | 2       | [Accra, Tema]                                    
========================================================================================================================
```

Example 2. `chars`-argument overload trims a specific character set from both edges.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "code"_k = pluss::kindle::Stream(pluss::make_note({"xxAccraxx"})),
});
auto out = ct.nt().strip("x");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [code]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
code                     | Note         | Note             | 1       | [Accra]                                          
========================================================================================================================
```

Example 3 (edge case). Calling any method on this accessor with a non-`Note`-kind key
bound throws — see the important callout above.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
});
try {
  auto out = ct.nt().strip();
} catch (const pluss::Error& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Thread::strip is only valid for Note payload
```

## Notes

- See the important callout above — this precondition applies to every method on
  `CatalogNoteAccessor`, not only `strip()`.
- The `chars`-argument overload treats `chars` as a *set* of characters, not a literal
  prefix/suffix — order within `chars` doesn't matter.
- For a one-sided trim, see [`lstrip()`](lstrip.md) and [`rstrip()`](rstrip.md).

## See Also

- [`CatalogNoteAccessor::lstrip`](lstrip.md) / [`rstrip()`](rstrip.md): the one-sided
  variants.
- [`CatalogNoteAccessor::pad`](pad.md): the inverse operation, adding characters
  instead of removing them.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.
- [`RegistryNoteAccessor::strip`](../registry-note/strip.md): the scribble-model
  equivalent, which can be scoped to a key subset via `fields(keys).nt()` first.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="starts.html">
    <span class="label">Previous</span>
    <span class="title">starts</span>
  </a>
  <a class="next-link" href="titlecase.html">
    <span class="label">Next</span>
    <span class="title">titlecase</span>
  </a>
</div>
