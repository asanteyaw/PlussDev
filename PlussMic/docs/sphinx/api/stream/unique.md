<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 405866cfe223 -->

# Stream::unique

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::unique() const noexcept -> bool
```

## What It Does

Reports whether **this `Stream` handle is the sole owner** of its underlying payload — a
reference-count check on the internal `intrusive_ptr`, not a check on the stream's *values*.

```{important} unique() is about ownership, not distinct values
Despite the name, `unique()` has nothing to do with whether the stream's values are
distinct — a stream `[1, 2, 2, 3]` with duplicate values still reports `unique(): true` if
no other `Stream` handle shares its payload. Conversely, a stream of genuinely distinct
values reports `unique(): false` the moment a copy of it exists anywhere. For a
values-are-distinct check, use [`ndistinct()`](ndistinct.md) or
[`distinct()`](distinct.md) instead.
```

`Stream`'s copy constructor is a cheap handle copy (shared ownership, like
`std::shared_ptr`), not a deep data copy — `unique()` tells you whether such a sharing copy
currently exists. It goes back to `true` once every other sharing handle goes out of scope.

## Parameters

`None`

## Returns

`bool`
    `true` if no other `Stream` handle currently shares this stream's payload; `false` if
    at least one copy exists elsewhere.

## Examples

Example 1. A freshly constructed stream is its own sole owner — even with duplicate values.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 2.0, 3.0}));

std::cout << stream.unique();
```

### Output

```text
1
```

Example 2. Copying the stream drops `unique()` to `false` on *both* handles, for as long as
both are alive.

```cpp
auto s1 = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}));
std::cout << "s1.unique() before copy: " << s1.unique() << "\n";

auto s2 = s1;
std::cout << "s1.unique() with s2 alive: " << s1.unique() << "\n";
std::cout << "s2.unique(): " << s2.unique() << "\n";
```

### Output

```text
s1.unique() before copy: 1
s1.unique() with s2 alive: 0
s2.unique(): 0
```

Example 3. Once the sharing copy goes out of scope, `unique()` returns to `true`.

```cpp
auto s1 = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}));
{
  auto s2 = s1;
  std::cout << "while s2 alive: " << s1.unique() << "\n";
}
std::cout << "after s2 destroyed: " << s1.unique();
```

### Output

```text
while s2 alive: 0
after s2 destroyed: 1
```

## Notes

- Ownership-sharing check, not a value-distinctness check — see the important callout
  above. This is the single most important thing to know before relying on this method's
  name.
- Moving a stream (`std::move`) does **not** create a second owner — the moved-from handle
  is left empty, so the moved-to handle still reports `unique(): true`.
- Methods that derive a new stream (e.g. [`head()`](head.md), [`abs()`](abs.md)) allocate a
  fresh payload, so the result is its own sole owner, independent of the receiver's
  `unique()` status.
- `noexcept`; safe to call at any time.

## See Also

- [`Stream::ndistinct`](ndistinct.md) / [`Stream::distinct`](distinct.md): the
  values-are-distinct operations this method's name might suggest, but does not perform.
- [`Stream::defined`](defined.md): a different ownership-adjacent check — whether the
  handle has a payload at all, regardless of sharing.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_slate.html">
    <span class="label">Previous</span>
    <span class="title">to_slate</span>
  </a>
  <a class="next-link" href="var.html">
    <span class="label">Next</span>
    <span class="title">var</span>
  </a>
</div>
