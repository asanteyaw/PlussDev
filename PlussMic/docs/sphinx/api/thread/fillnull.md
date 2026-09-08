<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 659443fa590b -->

# Thread::fillnull

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::fillnull(double value) const -> Thread
Thread::fillnull(std::string_view value) const -> Thread
Thread::fillnull(const pluss::Timeline& value) const -> Thread
Thread::fillnull(const pluss::TimeIndex& value) const -> Thread
```

## What It Does

Replaces every `NULL` cell with `value`, leaving already-valid cells untouched. The
overload selects which payload kind is required: `double` requires a `Series`-kind
thread, `std::string_view` requires `Note`-kind, and the `Timeline`/`TimeIndex`
overloads both accept either a `Timeline`- or `TimeIndex`-kind calling thread
symmetrically. This is the same operation [`replace(null_t, to)`](replace.md)
delegates to internally when `from` is `pluss::series::null`.

## Parameters

**value**
    The replacement for `NULL` cells. Its C++ type selects the overload — `double`
    for `Series`, `std::string_view`/`const char*` for `Note`, `pluss::Timeline` or
    `pluss::TimeIndex` for `Timeline`/`TimeIndex`-kind threads (either scalar type
    works against either temporal thread kind). The `Timeline`/`TimeIndex` overloads
    require a scalar value (`numel() == 1` / `size() == 1`).

## Returns

`Thread`
    A new thread, same length and kind as `*this`, with every `NULL` cell replaced
    by `value`.

## Examples

Example 1. Fill `NULL` numeric cells with a sentinel value.

```cpp
using namespace pluss::table;

auto readings = Thread(pluss::make_series({10.0, 20.0, 30.0}),
    pluss::make_series(std::vector<bool>{true, false, true}));
auto out = readings.fillnull(0.0);
std::cout << out;
```

### Output

```text
0  10
1  0
2  30

ptype: Series
```

Example 2. Fill `NULL` text cells with a placeholder string.

```cpp
auto cities = Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}),
    pluss::make_series(std::vector<bool>{true, false, true}));
auto out2 = cities.fillnull("Unknown");
std::cout << out2;
```

### Output

```text
0  Accra
1  Unknown
2  Kumasi

ptype: Note
```

Example 3. Fill `NULL` timestamps with a scalar `Timeline` value.

```cpp
auto events = Thread(pluss::make_timeline(
    {"2024-01-01T00:00:00Z", "2024-02-01T00:00:00Z", "2024-03-01T00:00:00Z"}),
    pluss::make_series(std::vector<bool>{true, false, true}));
auto out3 = events.fillnull(pluss::make_timeline({"2000-01-01T00:00:00Z"}));
std::cout << out3;
```

### Output

```text
0  2024-01-01
1  2000-01-01
2  2024-03-01

ptype: Timeline
```

Example 4. The `TimeIndex` overload works against a `Timeline`-kind thread too — the
two temporal overloads are interchangeable regardless of which temporal kind is
calling.

```cpp
auto out3b = events.fillnull(pluss::make_timeindex({"2000-01-01T00:00:00Z"}));
std::cout << out3b;
```

### Output

```text
0  2024-01-01
1  2000-01-01
2  2024-03-01

ptype: Timeline
```

Identical result to Example 3 — `events` is `Timeline`-kind, and the `TimeIndex`
overload fills it just as readily as the `Timeline` overload did.

Example 5. Value-kind mismatches throw.

```cpp
try {
  auto bad = readings.fillnull("not a number");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::fillnull(string) is only valid for Note payload
```

## Notes

- Overload selection is by the C++ type of `value`, which also determines the
  required payload kind.
- `fillnull(double)` requires `Series`; message: `"Thread::fillnull(double) is only
  valid for Series payload"`.
- `fillnull(string_view)` requires `Note`; message: `"Thread::fillnull(string) is
  only valid for Note payload"`.
- `fillnull(Timeline)` and `fillnull(TimeIndex)` both accept either a `Timeline`- or
  `TimeIndex`-kind calling thread; messages: `"Thread::fillnull(Timeline) is only
  valid for Timeline/TimeIndex payload"` and `"Thread::fillnull(TimeIndex) is only
  valid for Timeline/TimeIndex payload"`, respectively.
- The `Timeline`/`TimeIndex` overloads require a scalar (single-element) replacement
  value, or throw `"...replacement value must be scalar"`.
- A mutating sibling, [`fillnull_()`](fillnull_.md), applies the same fill in place.

## See Also

- [`Thread::fillnull_`](fillnull_.md): the mutating (in-place) form of this method.
- [`Thread::replace`](replace.md): `replace(pluss::series::null, to)` is equivalent to
  `fillnull(to)`.
- [`Thread::isnull`](isnull.md): the validity check `fillnull()` acts on.
- [`Thread::ffill`](ffill.md) / [`Thread::bfill`](bfill.md): fill `NULL` cells from
  neighboring values instead of a fixed replacement.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ffill.html">
    <span class="label">Previous</span>
    <span class="title">ffill</span>
  </a>
  <a class="next-link" href="fillnull_.html">
    <span class="label">Next</span>
    <span class="title">fillnull_</span>
  </a>
</div>
