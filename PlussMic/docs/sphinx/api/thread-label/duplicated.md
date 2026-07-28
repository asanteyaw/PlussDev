<!-- pluss-hand-authored -->

# ThreadLabelAccessor::duplicated

[Back to ThreadLabelAccessor API Reference](../../thread_label_accessor_api_reference)

## Signature

```cpp
ThreadLabelAccessor::duplicated(bool keep_first = true, bool keep_last = false) const -> Thread
```

## What It Does

Marks each row `true` if its case-label value has already appeared earlier
(or, with `keep_last = true`, will appear later) among the rows sharing that
value — `false` for the first (or, with `keep_last`, the last) occurrence of
every distinct value, and `false` for any value that never repeats at all.

## Parameters

**keep_first** (`bool`, default `true`)

If `true`, the first occurrence of each repeated value is `false` (not marked
duplicate); every later occurrence is `true`.

**keep_last** (`bool`, default `false`)

If `true`, the *last* occurrence is spared instead; every earlier one is
`true`. Cannot be combined with `keep_first = true`.

## Returns

`Thread`
    A `Bool` thread, one entry per row, `display_name` `"duplicated"`,
    displayed against the same case labels as `*this`.

## Examples

Example 1. Mark duplicates on the case labels, with one repeated value.

```cpp
using namespace pluss::table;

auto tagged = Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}))
                  .label().set(Thread(pluss::make_note({"east", "west", "east", "north"})));

auto out = tagged.label().duplicated();
std::cout << out;
```

### Output

```text
east   false
west   false
east   true
north  false

ptype: Series
```

`"east"` appears at rows `0` and `2`; with the default `keep_first = true`,
row `0` is spared (`false`) and row `2` is marked (`true`).

Example 2. Spare the *last* occurrence instead of the first.

```cpp
auto out2 = tagged.label().duplicated(false, true);
std::cout << out2;
```

### Output

```text
east   true
west   false
east   false
north  false

ptype: Series
```

Row `2` (`"east"`'s last occurrence) is spared and row `0` is marked instead
— the opposite of Example 1's default.

Example 3. Pass `keep_first = true` together with `keep_last = true` to see
the call throw.

```cpp
try {
  auto bad = tagged.label().duplicated(true, true);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Thread::duplicated: keep_first and keep_last cannot both be true
```

## Notes

- `keep_first` and `keep_last` cannot both be `true`.
- Unlike [`repeated()`](repeated.md), a value that occurs more than once
  still has exactly one `false` row (its spared occurrence) here.
- The result carries the same case labels as `*this`, unlike
  [`distinct()`](distinct.md)/[`freq()`](freq.md), whose results print with a
  plain default index.

## See Also

- [`ThreadLabelAccessor::repeated`](repeated.md): marks **every** occurrence
  of a repeated value `true`, with no spared occurrence.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="distinct.html">
    <span class="label">Previous</span>
    <span class="title">distinct</span>
  </a>
  <a class="next-link" href="fillnull.html">
    <span class="label">Next</span>
    <span class="title">fillnull</span>
  </a>
</div>
