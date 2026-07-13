# Thread::chars

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::chars(std::int64_t start, std::optional<std::int64_t> stop = std::nullopt, std::int64_t step = 1) -> Thread
```

Slice characters out of each note-backed text entry.

This is the per-entry string-slicing surface for text threads.

## Parameters

**start**

Inclusive starting character position.

**stop**

Optional exclusive ending position.

Leave this empty to slice to the end of each string.

**step**

Character stride.

- `1`: keep every character in the requested window.
- `2`: keep every second character, and so on.

## Returns

`Thread`
    Note-backed thread containing the extracted character slices.

## Examples

Example 1. Take the leading code letter from each text value.

```cpp
using namespace pluss::table;

auto codes = Thread(pluss::make_note({"A-001", "B-002", "C-003"}));

auto out = codes.chars(0, 1);
```

### Output

```text
0  A
1  B
2  C
dtype: Note
```

Example 2. Take every second character from the beginning to the end.

```cpp
using namespace pluss::table;

auto codes = Thread(pluss::make_note({"A-001", "B-002", "C-003"}));

auto out = codes.chars(0, std::nullopt, 2);
```

### Output

```text
0  A01
1  B02
2  C03
dtype: Note
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cast.html">
    <span class="label">Previous</span>
    <span class="title">cast</span>
  </a>
  <a class="next-link" href="clear_cases.html">
    <span class="label">Next</span>
    <span class="title">clear_cases</span>
  </a>
</div>
