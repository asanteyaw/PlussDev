# Part X: Atlas

`Atlas` represents a stack of related `DataFrame` objects.

## Construct an Atlas

```cpp
auto frame0 = df.slice(0, 100);
auto frame1 = df.slice(100, 200);

pluss::table::Atlas atlas({frame0, frame1});
```

With depth labels:

```cpp
pluss::table::Thread labels = pluss::make_note({"train", "valid"});
pluss::table::Atlas atlas({train, valid}, labels);
```

### Verified example: split a frame into an Atlas

```cpp
auto sales = sales_frame();
auto south = sales.query("region == 'south'");
auto other = sales.query("region != 'south'");
Thread labels = pluss::make_note({"south", "other"});

pluss::table::Atlas atlas({south, other}, labels);
std::cout << "depth=" << atlas.depth()
          << ", ncases=" << atlas.ncases()
          << ", nfeatures=" << atlas.nfeatures() << '\n';
std::cout << atlas.loc("south").to_string() << '\n';

auto catalog = sales.kindelize();
auto roundtrip = catalog.tabelize();
std::cout << "roundtrip equals original="
          << (roundtrip.equals(sales) ? "true" : "false") << '\n';
```

Output:

```text
depth=2, ncases=2, nfeatures=4
==================================
     | city   region  units  price
----------------------------------
0    | Accra  south   10     3
1    | Tema   south   7      4
----------------------------------
[2 cs x 4 feat]
roundtrip equals original=false
```

The Atlas has two frames: one for the south region and one for all other
regions. `atlas.loc("south")` uses the depth labels and returns the first frame.
`ncases()` is `2` because both frames in this example have two rows.

The final line demonstrates an important practical detail: converting through
`Catalog` and back can produce a table that displays similarly but is not
strictly `equals(...)` to the original frame in this build. Equality checks are
stricter than user-facing display equivalence, so use them when exact internal
round-trip identity matters.

## Inspect

```cpp
atlas.defined();
atlas.empty();
atlas.depth();
atlas.ncases();
atlas.nfeatures();
atlas.column_names();
atlas.axis0_labels();
atlas.show();
```

All frames in an Atlas should have compatible schemas. Keep feature names and
case semantics aligned unless you are intentionally modeling heterogeneous
frames.

## Access frames

```cpp
auto first = atlas.frame(0);
auto also_first = atlas.dloc(0);
auto by_label = atlas.loc("train");
```

`loc` supports string, integer, and timeline labels when the depth labels
support those lookup modes.

## Slice and sort depth

```cpp
auto first_five = atlas.head(5);
auto last_five = atlas.tail(5);
auto subset = atlas.slice_frames(10, 20);
auto sorted = atlas.sort_depth(true, true);
```

Relabel or align depth:

```cpp
auto relabeled = atlas.relabel_depth(new_labels);
auto aligned = atlas.align_depth(target_labels);
```

## Map frames

Apply a DataFrame transform to every frame:

```cpp
auto cleaned = atlas.map_frames([](const pluss::table::DataFrame& frame) {
  return frame.fillnull(0.0);
});
```

Aggregate across frames:

```cpp
auto summary = atlas.aggregate_over_frames(
    [](const std::vector<pluss::table::DataFrame>& frames) {
      return frames.front().copy();
    });
```

Use `aggregate_over_frames` when the aggregation requires seeing all frames at
once.

## Squeeze and unsqueeze

Create an Atlas from a DataFrame:

```cpp
auto atlas = df.unsqueeze();
```

Return to a DataFrame when depth is reducible:

```cpp
auto frame = atlas.squeeze();
```

## Convert to Archive

```cpp
auto archive = atlas.kindelize();
auto roundtrip = archive.tabelize();
```

This is the Atlas equivalent of `DataFrame <-> Catalog` conversion.

## Atlas plotting

```cpp
atlas.plot();
```

Atlas plots are useful for:

- depth overlays;
- scenario comparisons;
- frame facets;
- depth surfaces;
- rolling depth analysis.

See [Atlas API Reference](../atlas_api_reference.md) and the Atlas plot examples
under `PlussMic/examples/table/atlas_plot_demo.cpp`.
