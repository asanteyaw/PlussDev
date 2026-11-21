<!-- pluss-hand-authored -->

# ShelfGroupBuilder::by

[Back to ShelfGroupBuilder API Reference](../../shelf_group_accessor_api_reference)

## Signatures

```cpp
ShelfGrouped ShelfGroupBuilder::by(const Shelf& key) const
ShelfGrouped ShelfGroupBuilder::by(std::initializer_list<Shelf> keys) const
ShelfGrouped ShelfGroupBuilder::by(pluss::msharp::RangeView::Of<Shelf> keys) const
```

## What It Does

Forms the groups. `Shelf::group()` only binds the value shelf and its `ShelfGroupOptions`; `by` supplies the key shelf
or shelves and returns a [`ShelfGrouped`](../shelf/group.md) on which the aggregate methods run. Rows with equal keys
belong to the same group. With several keys, a group is a distinct combination of key values.

## Parameters

**key** (`const Shelf&`, first overload)

One key shelf as long as the value shelf.

**keys** (`std::initializer_list<Shelf>` or `RangeView::Of<Shelf>`)

Several key shelves, each as long as the value shelf.

Options come from `ShelfGroupOptions`:

| Field | Type | Default | Effect |
|---|---|---|---|
| `dropnull` | `bool` | `true` | Rows whose key is `NULL` are left out. When `false`, they form a group of their own. |
| `sort` | `bool` | `false` | Groups come back in key order instead of first-occurrence order. |

## Returns

`ShelfGrouped`
    The grouped handle. It performs no aggregation until one of its methods is called.

## Errors

A key shelf whose length differs from the value shelf is refused with `ShelfGrouped: key length mismatch`.

## Examples

Example 1. One key shelf.

```cpp
using namespace pluss::table;

auto sales = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto region = Shelf(pluss::make_note({"east", "west", "east", "west", "east"}));

auto grouped = sales.group().by(region);
std::cout << grouped.count();
std::cout << grouped.sum();
```

### Output

```text
==================
     | key   count
------------------
0    | east  3    
1    | west  2    
------------------
[2 cs x 2 feat]=================
     | key   sum 
-----------------
0    | east  90  
1    | west  60  
-----------------
[2 cs x 2 feat]
```

Example 2. Several keys in an initializer list make a composite key.

```cpp
using namespace pluss::table;

auto sales = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto region = Shelf(pluss::make_note({"east", "west", "east", "west", "east"}));
auto tier = Shelf(pluss::make_note({"a", "a", "b", "b", "a"}));

std::cout << sales.group().by({region, tier}).sum();
```

### Output

```text
========================
     | key   key_2  sum 
------------------------
0    | east  a      60  
1    | west  a      20  
2    | east  b      30  
3    | west  b      40  
------------------------
[4 cs x 3 feat]
```

Example 3. `NULL` keys are dropped by default.

```cpp
using namespace pluss::table;

auto sales = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto key = Shelf(pluss::make_note({"x", "y", "x", "y", "x"}), pluss::make_series({true, true, false, true, true}));

std::cout << sales.group().by(key).count();
```

### Output

```text
==================
     | key   count
------------------
0    | x     2    
1    | y     2    
------------------
[2 cs x 2 feat]
```

Example 4. Keys must be as long as the values.

```cpp
using namespace pluss::table;

auto sales = Shelf(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}));
auto short_key = Shelf(pluss::make_note({"a", "b"}));

try {
  auto out = sales.group().by(short_key).count();
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\n";
}
```

### Output

```text
ShelfGrouped: key length mismatch
```

Example 5. Keep a `NULL` key as its own group with `dropnull = false`.

```cpp
using namespace pluss::table;

auto sales = Shelf(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}));
auto key = Shelf(pluss::make_note({"x", "y", "x", "y", "x"}), pluss::make_series({true, true, false, true, true}));

ShelfGroupOptions options;
options.dropnull = false;

std::cout << sales.group(options).by(key).count();
```

### Output

```text
==================
     | key   count
------------------
0    | x     2    
1    | y     2    
2    | NULL  1    
------------------
[3 cs x 2 feat]
```

## Notes

- The key columns of a result are named `key` for the first key, then `key_2`, `key_3` and so on.
- A `ShelfGrouped` can be used for several aggregates. Grouping work is done once and reused.

## See Also

- [`Shelf::group`](../shelf/group.md): enters the grouped workflow.
- [`ShelfGrouped::agg`](agg.md), [`transform`](transform.md), [`filter`](filter.md): what to do with the groups.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="agg.html">
    <span class="label">Previous</span>
    <span class="title">agg</span>
  </a>
  <a class="next-link" href="count.html">
    <span class="label">Next</span>
    <span class="title">count</span>
  </a>
</div>
