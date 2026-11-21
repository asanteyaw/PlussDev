<!-- pluss-hand-authored -->

# ShelfGrouped::filter

[Back to ShelfGrouped API Reference](../../shelf_group_accessor_api_reference)

## Signatures

```cpp
Shelf ShelfGrouped::filter(const pluss::Series& keep) const
Shelf ShelfGrouped::filter(const Shelf& keep) const
```

## What It Does

Keeps or drops whole groups. `keep` has one boolean per group, in group order. The result is the value shelf with only
the rows of kept groups, in their original order.

## Parameters

**keep** (`const pluss::Series&` or `const Shelf&`)

One flag per group, in the order the aggregate methods list the groups: first occurrence of the key, or key order with
`ShelfGroupOptions::sort = true`.

## Returns

`Shelf`
    The rows of the groups whose flag is true.

## Errors

A `keep` whose length differs from the number of groups is refused with
`ShelfGrouped::filter: keep mask length must equal number of groups`.

## Examples

Example 1. Keep the rows of the first group only.

```cpp
using namespace pluss::table;

auto sales = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto region = Shelf(pluss::make_note({"east", "west", "east", "west", "east"}));
auto keep = pluss::make_series({1.0, 0.0}).gt(0.5);   // one flag per group, in group order

std::cout << sales.group().by(region).filter(keep);
```

### Output

```text
0  10
1  30
2  50
ptype: Series
```

Example 2. Pass the flags as a `Shelf`.

```cpp
using namespace pluss::table;

auto sales = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto region = Shelf(pluss::make_note({"east", "west", "east", "west", "east"}));
auto keep = Shelf(pluss::make_series({0.0, 1.0}).gt(0.5));

std::cout << sales.group().by(region).filter(keep);
```

### Output

```text
0  20
1  40
ptype: Series
```

Example 3. One flag per group is required.

```cpp
using namespace pluss::table;

auto sales = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto region = Shelf(pluss::make_note({"east", "west", "east", "west", "east"}));
auto keep = pluss::make_series({1.0, 0.0, 1.0}).gt(0.5);

try {
  auto out = sales.group().by(region).filter(keep);
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\n";
}
```

### Output

```text
ShelfGrouped::filter: keep mask length must equal number of groups
```

## Notes

- Compute the flags from an aggregate, for example the groups whose [`sum`](sum.md) exceeds a limit, and pass them in.

## See Also

- [`ShelfGroupBuilder::by`](by.md): forms the groups.
- [`ShelfGrouped::transform`](transform.md): per-row access to group aggregates.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count.html">
    <span class="label">Previous</span>
    <span class="title">count</span>
  </a>
  <a class="next-link" href="max.html">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>
