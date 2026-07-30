<!-- pluss-hand-authored -->

# ThreadNoteAccessor::split

[Back to ThreadNoteAccessor API Reference](../../thread_note_accessor_api_reference)

## Signatures

```cpp
ThreadNoteAccessor::split(std::string_view delimiter,
                          std::int64_t limit = -1,
                          std::string prefix = "part") const -> DataFrame

ThreadNoteAccessor::split(std::string_view pattern,
                          const TextOptions& options) const -> DataFrame
```

## What It Does

Splits every cell of the bound thread on a delimiter, expanding the result into
one output column per split position — direct forwards to
[`Thread::split()`](../thread/split.md). Requires the bound thread to be
`Note`-kind. Two overloads with different delimiter semantics:

- **`split(delimiter, limit, prefix)`**: always a literal substring split
  (never regex).
- **`split(pattern, options)`**: `pattern` is literal or regex depending on
  `options.regex`; `options.limit` caps the split count, `options.prefix` names
  the columns.

Both overloads produce ragged results — a row with fewer pieces than the widest
row is `NULL` in the extra columns.

```{warning} The two overloads compute the output column count differently for invalid rows
The single-delimiter overload determines the output column count from
**every** row's underlying text — including invalid rows. The
`(pattern, options)` overload only computes each row's split for rows that are
actually valid. See [`Thread::split()`](../thread/split.md) Example 4 for a
side-by-side demonstration.
```

## Parameters

**delimiter** / **pattern** (`std::string_view`)

The literal delimiter (first overload) or literal/regex pattern (second
overload, depending on `options.regex`) to split on. Must not be empty.

**limit** (`std::int64_t`, first overload, default `-1`)

Maximum number of splits to perform; `-1` means unlimited.

**prefix** (`std::string`, first overload, default `"part"`)

Base name for the output columns: `<prefix>_0`, `<prefix>_1`, ...

**options** (`TextOptions`, second overload)

`options.regex` (default `false`): treat `pattern` as regex. `options.limit`
(default `-1`): cap the number of splits. `options.prefix` (default `"part"`):
base name for output columns.

## Returns

`DataFrame`
    A new frame with one `<prefix>_0`, `<prefix>_1`, ... feature per split
    position. Rows with fewer pieces are `NULL` in the extra columns.

## Examples

Example 1. A plain literal split via the accessor, no limit.

```cpp
using namespace pluss::table;

auto paths = Thread(pluss::make_note({"a/b/c", "x/y", "solo"}));
auto out = paths.nt().split("/");
std::cout << out;
```

### Output

```text
=============================
     | part_0  part_1  part_2
-----------------------------
0    | a       b       c     
1    | x       y       NULL  
2    | solo    NULL    NULL  
-----------------------------
[3 cs x 3 feat]
```

Example 2. `limit` caps the number of splits — the remainder stays joined in the
last column.

```cpp
auto out2 = paths.nt().split("/", 1);
std::cout << out2;
```

### Output

```text
=====================
     | part_0  part_1
---------------------
0    | a       b/c   
1    | x       y     
2    | solo    NULL  
---------------------
[3 cs x 2 feat]
```

Example 3. The second overload: a regex pattern with a custom column prefix.

```cpp
auto rows = Thread(pluss::make_note({"a, b,c", "x , y", "single"}));
auto out3 = rows.nt().split(R"(\s*,\s*)", {.regex = true, .prefix = "field"});
std::cout << out3;
```

### Output

```text
================================
     | field_0  field_1  field_2
--------------------------------
0    | a        b        c      
1    | x        y        NULL   
2    | single   NULL     NULL   
--------------------------------
[3 cs x 3 feat]
```

Example 4. A non-`Note` thread throws.

```cpp
auto numbers = Thread(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.nt().split("/");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Thread::split: expected Note payload
```

## Notes

- `ThreadNoteAccessor::split()` is a one-line forward to
  [`Thread::split()`](../thread/split.md) — identical behavior and throws.
- An empty delimiter/pattern throws.
- `limit` caps the *number of splits*, not the *number of output columns*
  directly.
- The two overloads compute output width differently for invalid rows — see the
  warning above.
- `options.case_sensitive` has no effect on `split()`.

## See Also

- [`Thread::split`](../thread/split.md): the direct, non-accessor form this
  method forwards to.
- [`ThreadNoteAccessor::partition`](partition.md): a related but simpler split
  — always exactly 3 columns, first-occurrence-only.
- [`ThreadNoteAccessor::extract`](extract.md): regex capture-group extraction,
  another column-expanding method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rstrip.html">
    <span class="label">Previous</span>
    <span class="title">rstrip</span>
  </a>
  <a class="next-link" href="starts.html">
    <span class="label">Next</span>
    <span class="title">starts</span>
  </a>
</div>
