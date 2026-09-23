<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 47c85add7b3f -->

# DataFrame::ptypes

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::ptypes() const -> DTypes
```

## What It Does

Returns the coarse payload family of every feature in the frame — `Series`, `Note`,
`Timeline`, or `TimeIndex` — without the concrete scalar type underneath. Where
[`dtypes()`](dtypes.md) tells you `Series<float64>` versus `Series<int64>`,
`ptypes()` only tells you it's a `Series` at all. Reach for `ptypes()` when you're
routing logic by payload family (numeric work needs `Series`, text work needs `Note`,
and so on) and don't need the extra precision `dtypes()` provides.

Both `ptypes()` and [`dtypes()`](dtypes.md) hand back a `DataFrame::DTypes` object
rather than a raw map — an ordered feature-to-type view with lookup, iteration, a
bulk value accessor, and its own `operator<<`. Its full member surface is
[documented below](#the-dtypes-result-type); the two calls produce the same object
shape and differ only in what each type string says.

## Parameters

`None`

## Returns

`DTypes`
    An ordered `(feature name, payload family)` view — one entry per column, in the
    frame's feature order. See [The `DTypes` result type](#the-dtypes-result-type)
    for every member.

(the-dtypes-result-type)=

## The `DTypes` result type

`DataFrame::DTypes` is the small read-only object returned by both `ptypes()` and
`dtypes()`. It is an **ordered feature-to-type map**: one `(feature name, type
string)` pair per column, in the frame's current feature order. `ptypes()` fills
the type string with the payload family alone (`Series`, `Note`, ...); `dtypes()`
fills it with family *and* concrete type (`Series<float64>`, `Note<utf8>`, ...).
Everything here applies to the object from either call.

```cpp
using Entry = std::pair<std::string, std::string>;   // (feature name, type string)
```

| Member | Returns | Purpose |
| --- | --- | --- |
| `size()` / `empty()` | `std::int64_t` / `bool` | feature count described (always matches the frame), and whether it is zero |
| `contains(name)` | `bool` | whether `name` is one of the described columns — guard a lookup instead of catching `at()`'s throw |
| `at(name)` | `const std::string&` | the type string for one feature by name; throws `"DataFrame::DTypes::at: unknown feature '<name>'"` if absent |
| `operator[](name)` | `const std::string&` | identical to `at(name)` — same throw on an unknown name, no separate unchecked path |
| `entries()` | `const std::vector<Entry>&` | the whole `(name, type)` list by reference, in feature order — hold it, index it, hand it to an algorithm |
| `payloads()` | `std::vector<std::string>` | just the type strings, **value-owned**, in feature order — safe to call on a temporary |
| `begin()` / `end()` | `const_iterator` | range-`for` over `Entry` pairs; same order as `entries()` |
| `operator<<` | `std::ostream&` | a fixed `features  dtype` header, a `--------  --------` rule, then one unpadded `name  type` line per feature |

Notes on lifetime: `at()` / `operator[]` / `entries()` return references *into* the
`DTypes` object, so bind it to a name (`const auto dt = df.dtypes();`) before
taking a reference — a reference into a temporary dangles. `payloads()` is the
exception: it returns owned strings and is safe directly on `df.ptypes().payloads()`.

`DTypes` holds no reference to the frame. It is a snapshot — mutating the frame
afterwards does not change a `DTypes` you already have.

### The full surface at a glance

```cpp
#include <iostream>
#include <pluss/pluss.h>

int main() {
  using namespace pluss::table;
  using pluss::make_note;
  using pluss::make_series;

  const auto df = DataFrame({
      {"user_id", make_series({101.0, 102.0, 103.0})},
      {"score",   make_series({1.5, 2.0, 3.5})},
      {"name",    make_note({"Ada", "Bola", "Chen"})},
  });

  const auto pt = df.ptypes();

  std::cout << pt << "\n\n";                                  // operator<<
  std::cout << "size=" << pt.size() << " empty=" << std::boolalpha << pt.empty() << "\n";
  std::cout << "contains(name)=" << pt.contains("name")
            << " contains(missing)=" << pt.contains("missing") << "\n";
  std::cout << "at(name)=" << pt.at("name")
            << "  operator[](score)=" << pt["score"] << "\n";

  std::cout << "entries():\n";
  for (const auto& [feature, type] : pt.entries()) {
    std::cout << "  " << feature << " -> " << type << "\n";
  }

  std::cout << "payloads():";
  for (const auto& p : pt.payloads()) std::cout << " " << p;
  std::cout << "\n";

  std::cout << "range-for:\n";
  for (const auto& [feature, type] : pt) {
    std::cout << "  " << feature << " : " << type << "\n";
  }

  // dtypes() -> same object, richer strings
  std::cout << "dtypes payloads():";
  for (const auto& p : df.dtypes().payloads()) std::cout << " [" << p << "]";
  std::cout << "\n";
  return 0;
}
```

Output

```text
features  dtype
--------  --------
user_id  Series
score  Series
name  Note

size=3 empty=false
contains(name)=true contains(missing)=false
at(name)=Note  operator[](score)=Series
entries():
  user_id -> Series
  score -> Series
  name -> Note
payloads(): Series Series Note
range-for:
  user_id : Series
  score : Series
  name : Note
dtypes payloads(): [Series<float64>] [Series<float64>] [Note<utf8>]
```

## Examples

Example 1. Inspect payload families without the concrete scalar type.

```cpp
using namespace pluss::table;

auto users = DataFrame({
    {"user_id",     pluss::make_series({101.0, 102.0, 103.0})},
    {"signup_note", pluss::make_note({"referral", "ad", "referral"})},
});

auto out = users.ptypes();
std::cout << out;
```

### Output

```text
features  dtype
--------  --------
user_id  Series
signup_note  Note
```

`ptypes()` reports `Series` for `user_id` regardless of whether it's stored as
`float64` or `int64` underneath — for that level of detail, use
[`dtypes()`](dtypes.md) instead. As with `dtypes()`, the row alignment above is not a
typo: `DTypes::operator<<` prints a fixed header and separator, then each `name  ptype`
pair with a plain two-space gap and no column-width padding.

Example 2. Look up one feature's payload family directly.

```cpp
auto out = users.ptypes();
if (out.contains("signup_note")) {
  std::cout << out["signup_note"] << "\n";  // "Note"
}
```

Example 3. Route work by payload family — iterate the pairs, keep the names you
want.

```cpp
const auto pt = users.ptypes();

std::vector<std::string> numeric;
std::vector<std::string> text;
for (const auto& [name, family] : pt) {
  if (family == "Series") numeric.push_back(name);
  else if (family == "Note") text.push_back(name);
}

std::cout << "numeric:";
for (const auto& n : numeric) std::cout << " " << n;
std::cout << "\ntext:";
for (const auto& n : text) std::cout << " " << n;
std::cout << "\n";
```

### Output

```text
numeric: user_id
text: signup_note
```

## Notes

- `ptypes()` reports the *coarse* payload family only (`Series`, `Note`, `Timeline`,
  `TimeIndex`). Use [`dtypes()`](dtypes.md) when you need the concrete scalar type
  (`Series<float64>` vs `Series<int64>`) as well.
- Iteration order, `entries()`, and `payloads()` all follow the frame's current
  feature order — the same order [`features()`](features.md) returns.
- `payloads()` returns owned strings, so `const auto p = df.ptypes().payloads();`
  is safe; a reference from `at()` / `operator[]` / `entries()` is not — bind the
  `DTypes` object to a name first.

## See Also

- [`DataFrame::dtypes`](dtypes.md): the same `DTypes` object, with concrete type
  strings (`Series<float64>`).
- [`DataFrame::features`](features.md): the feature-name order `DTypes` follows.
- [`DataFrame::feataxis`](feataxis.md): select or route features by payload family
  in one call.
- [`DataFrame::cast`](cast.md), [`DataFrame::astype`](astype.md): change a feature's
  dtype (and, for `Timeline`/`TimeIndex`, its payload family) after inspecting it here.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="prod.html">
    <span class="label">Previous</span>
    <span class="title">prod</span>
  </a>
  <a class="next-link" href="put.html">
    <span class="label">Next</span>
    <span class="title">put</span>
  </a>
</div>
