<!-- pluss-hand-authored -->

# Nexus::show

[Back to Nexus API Reference](../../nexus_api_reference)

## Signatures

```cpp
void Nexus::show(const display::DisplayOptions& options = {}) const

void Nexus::show(std::ostream& os, const display::DisplayOptions& options = {}) const
```

## What It Does

Prints [`to_string()`](to_string.md)'s summary directly to a stream — `std::cout` for the
one-argument overload, or an explicit `std::ostream&` for the two-argument overload.

## Parameters

**os** (`std::ostream&`, two-argument overload)
    The stream to print to.

**options** (`const display::DisplayOptions&`, default `{}`)
    Accepted but currently unused, same as [`to_string()`](to_string.md)'s own `options`.

## Returns

`void`

## Examples

Example 1. Printing to `std::cout`.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben", "Cy"}))},
});

auto biz = Nexus({{"customers", customers}});

biz.show(); // writes directly to std::cout
```

### Output

```text
Nexus
-----
tables: 1
customers -> [3 cs x 2 feat]
```

Example 2. Printing to an explicit stream — here, a `std::ostringstream`, to capture the
output as a string rather than writing to `std::cout` directly.

```cpp
std::ostringstream oss;
biz.show(oss);
std::cout << (oss.str() == biz.to_string());
```

### Output

```text
1
```

## Notes

- Both overloads are thin wrappers over [`to_string()`](to_string.md) — see that page for
  exactly what the printed text contains.
- `options` is currently a no-op.

## See Also

- [`Nexus::to_string`](to_string.md): get the same text back as a `std::string` instead of
  printing it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_table_meta.html">
    <span class="label">Previous</span>
    <span class="title">set_table_meta</span>
  </a>
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
