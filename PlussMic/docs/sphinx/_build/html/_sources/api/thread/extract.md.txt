<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6867479ea607 -->

# Thread::extract

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::extract(std::string_view pattern,
                pluss::RangeView<std::string_view> names = {}) const -> DataFrame
```

## What It Does

Extracts one or more regex capture groups from a `Note` (string) thread into a new
`DataFrame` — one feature per capture group. Requires the thread to be `Note`-kind,
and `pattern` must contain at least one capture group (`(...)`), or the call throws.

## Parameters

**pattern** (`std::string_view`)

A regular expression containing at least one capture group. Only the parenthesized
groups become output features — the rest of the pattern is used for matching only.

**names** (`pluss::RangeView<std::string_view>`, optional)

Feature names for the capture groups, one per group. Leave it empty to get
auto-generated names `match_0`, `match_1`, and so on (in capture-group order); if
supplied, its length must exactly match the number of capture groups.

## Returns

`DataFrame`
    A new frame with one `Note`-kind feature per capture group. Rows where the
    pattern doesn't match stay present, with `NULL` in every capture-group column.

## Examples

Example 1. Extract two capture groups with explicit output feature names.

```cpp
using namespace pluss::table;

auto parts = Thread(pluss::make_note({"A-100", "B-205", "C", "D-400"}));
auto out = parts.extract("([A-Z])-([0-9]+)", {"code", "id"});
std::cout << out;
```

### Output

```text
==================
      | code  id  
------------------
0     | A     100 
1     | B     205 
2     | NULL  NULL
3     | D     400 
------------------
[4 cs x 2 feat]
```

Row `2` (`"C"`) doesn't match the pattern at all (no `-` followed by digits), so
both capture-group columns are `NULL` for that row — the row itself still appears
in the output rather than being dropped.

Example 2. Without `names`, capture groups get auto-generated column names
`match_0`, `match_1`, ... in capture-group order.

```cpp
auto out2 = parts.extract("([A-Z])-([0-9]+)");
std::cout << out2;
```

### Output

```text
========================
      | match_0  match_1
------------------------
0     | A        100    
1     | B        205    
2     | NULL     NULL   
3     | D        400    
------------------------
[4 cs x 2 feat]
```

Example 3. A pattern with no capture group throws.

```cpp
try {
  auto bad = parts.extract("[A-Z]-[0-9]+");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Thread::extract: pattern needs a capture group
```

## Notes

- Auto-generated names are `match_0`, `match_1`, ... — not `group_0`/`group_1`.
- `names`'s length must exactly equal the number of capture groups, or the call
  throws.
- A non-matching row stays in the output with `NULL` in every capture-group column,
  rather than being dropped — unlike a `filter()`-based approach.
- A non-`Note` calling thread throws `"Thread::extract: expected Note payload"`.

## See Also

- [`Thread::matches`](matches.md) / [`Thread::fullmatch`](fullmatch.md): boolean
  regex checks, when you only need a mask rather than the captured text.
- [`Thread::split`](split.md): delimiter-based decomposition instead of regex
  capture groups.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="exp.html">
    <span class="label">Previous</span>
    <span class="title">exp</span>
  </a>
  <a class="next-link" href="ffill.html">
    <span class="label">Next</span>
    <span class="title">ffill</span>
  </a>
</div>
