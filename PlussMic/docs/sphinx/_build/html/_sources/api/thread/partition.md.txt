<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7ef94413804f -->

# Thread::partition

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::partition(std::string_view separator,
                  std::string prefix = "part") const -> DataFrame
```

## What It Does

Splits every cell of a `Note` (string) thread at the *first* occurrence of a literal
`separator`, always producing exactly **three** output columns: the text before the
separator, the separator itself, and the text after it. Requires the thread to be
`Note`-kind. It's a narrower, simpler cousin of [`split()`](split.md) — always
literal (no regex), always stops at the first match, and always exactly three columns
regardless of the input.

```{important} A missing separator produces empty strings, not NULL
When `separator` doesn't occur in a cell at all, that row's middle and third columns
are set to the **empty string** `""`, not `NULL` — the row itself is still valid, it
simply had nothing to split. This is a meaningfully different behavior from
[`split()`](split.md), where a row that produced fewer pieces than the widest row gets
genuine `NULL`s in the missing columns. `partition()`'s validity for all three output
columns is copied directly from the *original row's own validity* — a valid input row
is valid in all three output columns even when the separator wasn't found; only a row
that was already invalid on input produces `NULL` in all three. See Example 1.
```

## Parameters

**separator** (`std::string_view`)
    The literal text to split on. Must not be empty — `partition("")` throws
    `"Thread::partition: separator cannot be empty"`.

**prefix** (`std::string`, default `"part"`)
    Base name for the three output columns: `<prefix>_0` (before), `<prefix>_1` (the
    separator, or `""` if not found), `<prefix>_2` (after, or `""` if not found).

## Returns

`DataFrame`
    A new frame with exactly three features: `<prefix>_0`, `<prefix>_1`, `<prefix>_2`.

## Examples

Example 1. The core behavior, including the not-found case's empty strings and the
genuinely-invalid row's `NULL`s, side by side.

```cpp
using namespace pluss::table;

auto entries = Thread(pluss::make_note({"key=value", "no-equals", "a=b=c", "x"}),
    pluss::make_series(std::vector<bool>{true, true, true, false}));
std::cout << entries;

auto out1 = entries.partition("=");
std::cout << out1;
```

### Output (before)

```text
0  key=value
1  no-equals
2  a=b=c
3  NULL

ptype: Note
```

### Output

```text
================================
     | part_0     part_1  part_2
--------------------------------
0    | key        =       value 
1    | no-equals                
2    | a          =       b=c   
3    | NULL       NULL    NULL  
--------------------------------
[4 cs x 3 feat]
```

Row 0 splits cleanly: `"key"`, `"="`, `"value"`. Row 1 has no `=` at all — its
`part_1` and `part_2` cells are **empty strings** (the blank space between the pipes
above, not the literal text `"NULL"`), because the row itself is valid. Row 2
(`"a=b=c"`) partitions only at the *first* `=`: `"a"`, `"="`, `"b=c"` — the second `=`
is left inside `part_2` untouched. Row 3 was invalid on input, so all three of its
output columns are genuinely `NULL`.

Example 2. A custom column prefix.

```cpp
auto out2 = entries.partition("=", "kv");
std::cout << out2;
```

### Output

```text
=============================
     | kv_0       kv_1  kv_2 
-----------------------------
0    | key        =     value
1    | no-equals             
2    | a          =     b=c  
3    | NULL       NULL  NULL 
-----------------------------
[4 cs x 3 feat]
```

Same values as Example 1, just under `kv_0`/`kv_1`/`kv_2` instead of the default
`part_0`/`part_1`/`part_2`.

Example 3. An empty separator throws immediately.

```cpp
try {
  auto bad = entries.partition("");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::partition: separator cannot be empty
```

Example 4. A non-`Note` thread throws.

```cpp
auto numbers = Thread(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.partition("=");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Thread::partition: expected Note payload
```

## Notes

- Requires the calling thread to be `Note`-kind.
- Always literal (no regex mode) and always stops at the *first* occurrence — for a
  regex-aware or multi-way split, use [`split()`](split.md).
- Always produces exactly three output columns, regardless of the input.
- A missing separator yields empty strings in the second and third columns, not
  `NULL` — see the important callout above. Only a row that was already invalid on
  input produces `NULL` in any of the three columns.
- An empty `separator` throws `"Thread::partition: separator cannot be empty"`.

## See Also

- [`Thread::split`](split.md): the more general, possibly-regex, unlimited-pieces
  sibling of this method — and one with genuine `NULL`-padding instead of empty
  strings for missing pieces.
- [`Thread::extract`](extract.md): regex capture-group extraction into columns.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pad.html">
    <span class="label">Previous</span>
    <span class="title">pad</span>
  </a>
  <a class="next-link" href="percentiles.html">
    <span class="label">Next</span>
    <span class="title">percentiles</span>
  </a>
</div>
