<!-- pluss-hand-authored -->
# NoteNlpAccessor::normalize_numbers

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::normalize_numbers() const
```

## Replace Each Numeric Run With A Canonical Zero

`normalize_numbers()` replaces every run beginning with an ASCII digit by
the single character `0`. It preserves the surrounding sentence while making
surface forms such as `7`, `1,234.50`, and `2024-01-02` compare as the same
numeric category. Use it when downstream logic needs ordinary text containing
a compact canonical value. Use `mask_numbers()` instead when an explicit
`[NUM]` marker is important to readers or token classifiers.

A run follows this exact byte grammar:

| Input byte | Effect inside a run |
|---|---|
| `0` through `9` | Always consumed. |
| `, . - / : _` followed immediately by a digit | Separator and following numeric portion remain in the same run. |
| Any other byte | Ends the run and is preserved. |

Therefore a date-shaped or time-shaped string is normalized as one run, even
if it is not a valid calendar date or clock time. This method recognizes
syntax, not numeric meaning. A leading sign is outside the run, so `+7` becomes
`+0`; digits embedded in an identifier are still found, so `x8y` becomes
`x0y`; and `1..2` contains two runs because the first dot is not followed by a
digit.

Only ASCII digits participate. The method does not parse values, round them,
preserve magnitude, or distinguish dates, currency, versions, and identifiers.
Apply a narrower semantic operation first when those distinctions matter.

The implementation scans each cell once and performs no regex compilation or
backtracking. The result is a fresh Text Note with identical shape and cell
order. It is idempotent because every replacement is itself one canonical
numeric run. Empty and undefined behavior follows the other text transforms;
non-Text dtypes are rejected.

## Example 1: Several Numeric Surface Forms

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"42 1,234.50 2024-01-02 +7 x8y"})
        .nlp().normalize_numbers()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["0 0 0 +0 x0y"]
```

## Example 2: Separator Boundaries

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"1/2 3:45 6_000 1..2 and no digits"})
        .nlp().normalize_numbers()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["0 0 0 0..0 and no digits"]
```

## Example 3: Preserve Higher-Rank Structure

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{" can't ", "12", "“yes”", "plain"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(
    matrix.nlp().normalize_numbers()) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  [" can't ", "0"], 
  ["“yes”", "plain"]
]
```

## Example 4: Idempotence

```cpp
const auto once = pluss::note::make({"Build 1847 costs $12.50"})
                      .nlp().normalize_numbers();
std::cout << "numbers idempotent=" << std::boolalpha
          << (once.nlp().normalize_numbers() == once).all().bool_item()
          << '\n';
```

### Output

```text
numbers idempotent=true
```

## See Also

- [`mask_numbers`](mask_numbers.md): use `[NUM]` rather than `0` while retaining the same run grammar.
- [`mask_dates`](mask_dates.md): recognize date-shaped spans with range checks.
- [`mask_pii`](mask_pii.md): apply semantic masking in precedence order.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="named_entities.html"><span class="label">Previous</span><span class="title">named_entities</span></a>
  <a class="next-link" href="normalize_punct.html"><span class="label">Next</span><span class="title">normalize_punct</span></a>
</div>
