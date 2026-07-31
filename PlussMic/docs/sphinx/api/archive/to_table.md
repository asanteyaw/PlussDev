<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5f7f172c8a25 -->

# Archive::to_table

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::to_table(const TabelizePolicy& policy = {}, bool render = false) const -> pluss::table::Atlas
```

## What It Does

A thin wrapper over [`tabelize()`](tabelize.md): builds the same `pluss::table::Atlas`, and
when `render = true`, additionally prints it to `std::cout` before returning it — the same
plain-text, non-blocking print [`Catalog::to_table(policy, true)`](../catalog/to_table.md)
performs. An earlier build instead routed `render = true` through the no-argument
`Atlas::show()` overload, which does terminal/backend detection and could hang
indefinitely when output wasn't an interactive terminal (e.g. redirected to a file, or run
non-interactively); that path has been replaced with a direct `std::cout` print, so
`render = true` is now safe to use in any context, interactive or not.

## Parameters

**policy** (`const TabelizePolicy&`, optional)
    The same alignment policy [`tabelize()`](tabelize.md) accepts.

**render** (`bool`, default `false`)
    When `true`, additionally prints the resulting `Atlas` to `std::cout` before returning
    it — safe in any context, interactive or not.

## Returns

`pluss::table::Atlas`
    The tabelized, depth-stacked `Atlas`.

## Examples

Example 1. Convert to an `Atlas` without rendering (the safe default), then print one
depth-slice explicitly.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
});
auto archive = pluss::kindle::Archive({c0});

auto atlas = archive.to_table();
std::cout << atlas.dloc(0);
```

### Output

```text
===========
     | id  
-----------
0    | 1   
1    | 2   
-----------
[2 cs x 1 feat]
```

Example 2. Pass `render = true` to print the `Atlas` to `std::cout` immediately, in
addition to getting it back as the return value.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
});
auto archive = pluss::kindle::Archive({c0});

auto atlas = archive.to_table({}, /*render=*/true);
```

### Output

```text
Atlas(depth=1, rows=2, cols=1)
frame[0]
===========
     | id  
-----------
0    | 1   
1    | 2   
-----------
[2 cs x 1 feat]
```

The print happens as a side effect of the call itself — `atlas` is also returned normally,
identical to what Example 1's `render = false` call would have produced.

## Notes

- `render = false` (the default) is equivalent to plain [`tabelize()`](tabelize.md); prefer
  that method directly if you never intend to render.
- `render = true` prints via `std::cout` unconditionally — it does not attempt terminal
  detection or an alternate (e.g. HTML/window) backend the way `Atlas::show()`'s
  no-argument overload does elsewhere.

## See Also

- [`Archive::tabelize`](tabelize.md): the safe, render-free equivalent this method wraps.
- [`Catalog::to_table`](../catalog/to_table.md): the single-catalog analog, using the same
  direct-`std::cout` print for its `render = true` path.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string.html">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
  <a class="next-link" href="unique.html">
    <span class="label">Next</span>
    <span class="title">unique</span>
  </a>
</div>
