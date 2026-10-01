<!-- pluss-hand-authored -->

# pluss::series::undefined

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::undefined()
```

## What It Builds

`undefined()` creates an explicit **absence sentinel**. It does not allocate
numeric storage, choose a dtype, or represent an empty shape. Its only purpose
is to let an API carry “no `Series` was supplied” while keeping that state
inside the `Series` type.

The same state is available through public default construction:

```cpp
pluss::Series pending{};
```

Neither spelling creates numeric data. Use `{}` as the concise form for local
placeholders and fields that will be assigned later. Use
`pluss::series::undefined()` when naming the absence is clearer, especially in
function arguments, conditional expressions, and returned sentinel values.
Defined values still come from public factories.

Do not substitute an empty series such as `pluss::series::empty({0}, ...)`.
That object is defined, has dtype and shape metadata, and participates in
ordinary operations. An undefined series has no payload at all.

## Parameters

None.

## Returns

`Series`
: An undefined handle for optional state. `defined()` returns `false` for the
  result.

## Where It Belongs

Use the sentinel at boundaries where absence is part of the contract:

- an optional bias, mask, initial state, or cached result
- a field that will be assigned by a later initialization phase
- a branch whose caller already checks `defined()` before numerical use

Prefer `std::optional<Series>` when the surrounding API is naturally expressed
as an optional C++ value. `pluss::series::undefined()` is most useful in tensor-shaped
interfaces where an undefined handle is already the established absence
protocol and where keeping one concrete parameter type simplifies dispatch.

The sentinel is not a delayed allocation. Numerical methods reject it; call a
value factory such as `make`, `zeros`, or `empty` before doing arithmetic.

## Examples

Example 1. The factory creates an explicit undefined state.

```cpp
const auto absent = pluss::series::undefined();
std::cout << std::boolalpha << "defined=" << absent.defined() << "\n";
```

### Output

```text
defined=false
```

Example 2. An optional parameter can carry either a normal factory result or
the named sentinel while keeping both branches the same type.

```cpp
const bool supply_bias = false;
const auto bias = supply_bias
    ? pluss::series::zeros({3}, pluss::ScalarType::Float64)
    : pluss::series::undefined();

std::cout << std::boolalpha << "bias_defined=" << bias.defined() << "\n";
```

### Output

```text
bias_defined=false
```

The same variable can later receive a factory-built `Series` through ordinary
copy or move assignment.

Example 3. Default construction is the compact spelling when several fields
begin in the undefined state.

```cpp
pluss::Series lb{}, ub{}, scaler{}, pen_val{};

std::cout << std::boolalpha
          << lb.defined() << " "
          << ub.defined() << " "
          << scaler.defined() << " "
          << pen_val.defined() << "\n";
```

### Output

```text
false false false false
```

Example 4. Undefined is not a numeric value. Arithmetic fails before a result
is produced.

```cpp
const auto absent = pluss::series::undefined();

try {
  (void)absent.add(1.0);
} catch (const std::exception& e) {
  const std::string message = e.what();
  std::cout << message.substr(0, message.find('\n')) << "\n";
}
```

### Output

```text
SeriesBase: undefined
```

## Failure Modes

- Arithmetic, indexing, shape queries that require storage, and device transfer
  reject an undefined receiver.
- `undefined()` does not accept a dtype, platform, or shape because those
  properties belong to defined storage.
- Do not print or serialize the sentinel as if it were data. Test `defined()`
  and encode absence explicitly at the surrounding protocol boundary.

## Performance And Ownership

Default construction and `pluss::series::undefined()` perform no data
allocation or element copy. The check performed by `defined()` is constant
time. Replacing either form by assignment follows normal intrusive-handle
ownership rules for the newly assigned `Series`.

## Summary

Use `Series{}` for a concise undefined placeholder or
`pluss::series::undefined()` to state absence explicitly. Use a value factory
for every numeric object, including zero-element series.

## See Also

- [`pluss::series::empty`](empty.md): allocate defined, uninitialized storage.
- [`pluss::series::zeros`](zeros.md): allocate a defined zero-filled value.
- [`pluss::series::make`](make.md): construct defined data from host values.
