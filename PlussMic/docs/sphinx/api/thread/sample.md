# Thread::sample

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::sample(Float frac) -> template <class Float> requires std::floating_point<Float> Thread

Thread::sample(Float frac, std::uint64_t random_state, bool replace = false) -> template <class Float> requires std::floating_point<Float> Thread

Thread::sample(std::int64_t n) -> Thread

Thread::sample(std::int64_t n, std::uint64_t random_state, bool replace = false) -> Thread
```

## What It Does

Returns a smaller preview cut from the current object so you can inspect rows or depth items without carrying the full payload forward.

## Parameters

**frac**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**random_state**
    Optional seed used to make sampling or shuffling reproducible.

**replace**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

**n**
    Number of rows, entries, or depth items to keep in the preview.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.sample(10.0);
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.sample(10.0, {}, true);
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 3. Apply `sample` to the current thread using explicit `n` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.sample(2);
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 4. Apply `sample` to the current thread using explicit `n, random_state, replace` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.sample(2, {}, true);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rxor.html">
    <span class="label">Previous</span>
    <span class="title">rxor</span>
  </a>
  <a class="next-link" href="scan.html">
    <span class="label">Next</span>
    <span class="title">scan</span>
  </a>
</div>
