<!-- pluss-hand-authored -->

# DataFrame::kindelize

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::kindelize() const -> pluss::kindle::Catalog
DataFrame::kindelize(const pluss::kindle::KindelizePolicy& policy) const -> pluss::kindle::Catalog
```

## What It Does

Converts a `DataFrame` into a `pluss::kindle::Catalog` — a keyed collection of
independent `Stream`s, one per feature, rather than a single aligned table. This is
the bridge into the Kindle module's stream-oriented API, useful when downstream code
wants to work with each feature as its own named, independently-sliceable stream
instead of as columns of one shared frame. [`tabelize()`](tabelize.md) is the reverse
direction: it takes a `Catalog` and rebuilds a `DataFrame` from it.

By default, every feature on the frame becomes one `Catalog` entry, keyed by its
existing feature name, and each resulting `Stream` carries the frame's case labels
along with it (so per-stream operations later can still refer back to which case each
value belonged to).

## Parameters

**policy** (`const pluss::kindle::KindelizePolicy&`, optional)

Controls how the conversion is done. Omitting it entirely is equivalent to passing a
default-constructed `KindelizePolicy`. Its fields:

- **`attach_labels`** (`bool`, default `true`): when `true`, each `Stream` carries the
  frame's case-label thread alongside its values. Set `false` to build plain,
  label-less streams instead.
- **`materialize_label`** (`bool`, default `false`): when `true`, the case-label thread
  is *also* added as its own additional `Catalog` entry (keyed by `label_key`), rather
  than only being attached to each feature's `Stream`.
- **`label_key`** (`std::string`, default `"_label"`): the key used for the
  materialized label entry when `materialize_label` is `true`. Throws
  `"Catalog::kindelize: label key collides with existing feature '<label_key>'"` if a
  feature already uses that exact name.

## Returns

`pluss::kindle::Catalog`
    A catalog with one entry per feature (keyed by feature name, in the frame's
    feature order), plus one extra `label_key`-keyed entry when
    `policy.materialize_label` is `true`.

## Examples

Example 1. Convert a frame into a catalog using the default policy.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"city",  Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
    {"sales", Thread(pluss::make_series({100.0, 150.0, 90.0}))},
}, Thread(pluss::make_note({"o1", "o2", "o3"})));
std::cout << orders;
```

### Output

```text
==========================
     | id    city    sales
--------------------------
o1   | 1     Accra   100  
o2   | 2     Tema    150  
o3   | 3     Kumasi  90   
--------------------------
[3 cs x 3 feat]
```

```cpp
auto catalog = orders.kindelize();
std::cout << catalog.nkeys() << "\n";
for (const auto& key : catalog.keys()) {
  std::cout << key << ": " << catalog.at(key).size() << " cases\n";
}
```

### Output

```text
3
id: 3 cases
city: 3 cases
sales: 3 cases
```

Each of the three features becomes its own catalog entry, keyed by its original
feature name, and each `Stream` carries `orders`'s three cases with it.

Example 2. Materialize the case-label thread as its own catalog entry.

```cpp
pluss::kindle::KindelizePolicy policy;
policy.materialize_label = true;

auto catalog_with_label = orders.kindelize(policy);
std::cout << catalog_with_label.nkeys() << "\n";
std::cout << catalog_with_label.has("_label") << "\n";
```

### Output

```text
4
true
```

With `materialize_label = true`, a fourth entry appears — keyed `"_label"` by default
— holding the case-label thread (`o1, o2, o3`) as a stream in its own right, in
addition to it already being attached to `id`, `city`, and `sales`'s individual
streams.

## Notes

- `kindelize()` requires a defined frame; calling it on a default-constructed
  `DataFrame` throws `"DataFrame::kindelize requires a defined DataFrame"`.
- The reverse conversion is [`DataFrame::tabelize(catalog, policy)`](tabelize.md) — a
  static method that rebuilds a `DataFrame` from a `Catalog`.
- `materialize_label`'s label key collides-with-an-existing-feature check only runs
  when `materialize_label` is `true`; leaving it at the default `false` never performs
  this check, since no extra entry is added.

## See Also

- [`DataFrame::tabelize`](tabelize.md): the reverse conversion, from `Catalog` back to
  `DataFrame`.
- [`DataFrame::features`](features.md): the plain, in-frame way to pull out a single
  feature as a `Thread`, without leaving the `DataFrame` representation at all.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="join.html">
    <span class="label">Previous</span>
    <span class="title">join</span>
  </a>
  <a class="next-link" href="kurtosis.html">
    <span class="label">Next</span>
    <span class="title">kurtosis</span>
  </a>
</div>
