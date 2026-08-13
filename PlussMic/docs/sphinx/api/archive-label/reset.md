<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5767141629f6 -->

# ArchiveLabelAccessor::reset

[Back to ArchiveLabelAccessor API Reference](../../archive_label_accessor_api_reference)

## Signature

```cpp
Archive ArchiveLabelAccessor::reset() const
```

## What It Does

Clears the archive's depth labels back to the unset state, returning a new archive.
After `reset()`, [`has()`](has.md) reports `false` again and
[`values()`](values.md)/[`align()`](align.md) both throw until labels are set again.

## Parameters

`None`

## Returns

`Archive`
    A new archive with no depth labels; the catalog stack itself (contents and order)
    is unchanged.

## Examples

Example 1. Set depth labels, then clear them back to unset.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({10.0})) });
auto c1 = pluss::kindle::Catalog({ "px"_k = pluss::kindle::Stream(pluss::make_series({20.0})) });
auto archive = pluss::kindle::Archive({c0, c1});

auto labeled = archive.label().set(pluss::kindle::Stream(pluss::make_note({"a", "b"})));
std::cout << "labeled has(): " << std::boolalpha << labeled.label().has() << "\n";

auto back = labeled.label().reset();
std::cout << "after reset has(): " << std::boolalpha << back.label().has() << "\n";
std::cout << "depth unchanged: " << back.depth();
```

### Output

```text
labeled has(): true
after reset has(): false
depth unchanged: 2
```

## Notes

- `reset()` clears the label state; the catalogs themselves and their order are
  untouched — only `depth()` (unaffected here) and the label state change.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.
- Returns a new archive — the original labeled archive is unaffected.

## See Also

- [`ArchiveLabelAccessor::set`](set.md): the inverse operation, attaching new depth
  labels.
- [`ArchiveLabelAccessor::has`](has.md): confirms the label was actually cleared.
- [`Archive::label`](../archive/label.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="set.html">
    <span class="label">Next</span>
    <span class="title">set</span>
  </a>
</div>
