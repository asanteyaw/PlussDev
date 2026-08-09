<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 72d5260e3e99 -->

# Axel::labels

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
const std::optional<Slot>& Axel::labels() const
```

## What It Does

Returns the optional depth-axis label `Slot` — one label value per
registry frame — if this `Axel` was constructed with one.

## Parameters

`None`

## Returns

`const std::optional<Slot>&`
    An engaged `optional` holding the label `Slot` (size equal to
    `depth()`) if labels were provided at construction; `std::nullopt`
    otherwise.

## Examples

Example 1. An `Axel` built without a second constructor argument has no
labels; one built with a matching-size `Slot` does.

```cpp
auto r0 = pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(1)})}});
auto r1 = pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(2)})}});

auto unlabeled = pluss::bundle::Axel({r0, r1});
std::cout << "unlabeled: " << std::boolalpha << unlabeled.labels().has_value() << "\n";

auto labeled = pluss::bundle::Axel({r0, r1}, pluss::bundle::Slot({pluss::make_note("q1"), pluss::make_note("q2")}));
std::cout << "labeled: " << labeled.labels().has_value() << " first=" << std::get<pluss::Note>(labeled.labels()->at(0)).at(0);
```

### Output

```text
unlabeled: false
labeled: true first=q1
```

Example 2. A labels `Slot` whose size doesn't match `depth()` throws at
construction time, before `labels()` is ever called.

```cpp
auto r0 = pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(1)})}});
auto r1 = pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(2)})}});
try {
  auto bad = pluss::bundle::Axel({r0, r1}, pluss::bundle::Slot({pluss::make_note("only-one")}));
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Axel: labels size must match depth
```

## Notes

- The label `Slot`'s size must equal `depth()` — this is enforced once,
  at construction, not on every `labels()` call. An undefined label
  `Slot` passed as the second constructor argument also throws
  (`"Axel: labels slot undefined"`).
- Labels can hold any `Slot`-representable value per frame (`Note`,
  `Series`, `Timeline`, `TimeIndex`) — the two examples above use
  `Note` (text) labels, but nothing restricts labels to text.
- [`Axel::slice`](slice.md), [`head()`](head.md), [`tail()`](tail.md)
  all slice the label `Slot` alongside the registries, keeping labels
  aligned with their frames. [`concat_depth()`](concat_depth.md) is the
  one operation where labels can silently disappear — see its Notes.
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"`.

## See Also

- [`Axel::slice`](slice.md): one place labels are kept in sync
  automatically when frames are selected.
- [`Axel::concat_depth`](concat_depth.md): where an asymmetric label
  presence between two axels causes the result to lose labels entirely.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kindelize.html">
    <span class="label">Previous</span>
    <span class="title">kindelize</span>
  </a>
  <a class="next-link" href="map.html">
    <span class="label">Next</span>
    <span class="title">map</span>
  </a>
</div>
