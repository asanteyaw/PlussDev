<!-- pluss-hand-authored -->
# Note::end

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::const_iterator Note::end() const
```

## Mark The End Of Flat Iteration

`end()` returns the sentinel one position beyond the final Text or Bytes cell.
It exists to terminate iterator loops and define the half-open range
`[begin(), end())`. The sentinel is comparable and subtractable only through
forward-iterator algorithms such as `std::distance`; it must never be
dereferenced.

Like `begin()`, `end()` holds an intrusive reference to the backing arena.
Repeated calls are constant-time and allocate nothing. The range length is
`numel()`, irrespective of rank, because iteration follows the Note's flat
row-major logical order.

For a defined empty Note and for an undefined Note, `begin()` and `end()` are
equal. This makes range-based loops naturally perform zero iterations. The
undefined case is intentionally limited to passive range detection:
`copy_flat_to()` and `for_each_flat()` reject undefined Notes because those
calls request an active traversal operation.

Bool, Int64, and Float64 Notes reject `end()` just as they reject `begin()`.
Use the corresponding native span accessor for those dtypes.

## Example 1: Explicit Half-Open Loop

```cpp
const auto words = pluss::note::make({"north", "east", "south", "west"});
for (auto it = words.begin(); it != words.end(); ++it)
  std::cout << *it << (std::next(it) == words.end() ? '\n' : ' ');
```

### Output

```text
north east south west
```

## Example 2: Measure A Range

```cpp
const auto scalar = pluss::note::make("solo");
const auto empty = pluss::note::make(std::vector<std::string>{});
std::cout << "scalar distance=" << std::distance(scalar.begin(), scalar.end())
          << " value=" << *scalar.begin() << '\n'
          << "empty distance=" << std::distance(empty.begin(), empty.end())
          << " equal=" << std::boolalpha << (empty.begin() == empty.end()) << '\n';
```

### Output

```text
scalar distance=1 value=solo
empty distance=0 equal=true
```

## Example 3: Undefined Means No Range

```cpp
const auto value = pluss::Note::undefined();
std::cout << std::boolalpha
          << "equal=" << (value.begin() == value.end()) << '\n';
```

### Output

```text
equal=true
```

An undefined handle is not the same object state as a defined empty Note; both
simply have an empty iterator range. Use `defined()` when that distinction
matters.

## Example 4: Never Dereference The Sentinel

```cpp
const auto words = pluss::note::make({"north", "east"});
auto it = words.begin();
while (it != words.end()) {
  std::cout << *it << '\n';
  ++it;
}
```

### Output

```text
north
east
```

The comparison must occur before dereference. `*words.end()` is a contract
violation and throws rather than yielding an empty cell.

## See Also

- [`begin`](begin.md): obtain the first iterator and the full lifetime contract.
- [`empty`](empty.md): test zero cardinality without constructing iterators.
- [`numel`](numel.md): obtain the exact flat range length directly.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html"><span class="label">Previous</span><span class="title">empty</span></a>
  <a class="next-link" href="ends_with.html"><span class="label">Next</span><span class="title">ends_with</span></a>
</div>
