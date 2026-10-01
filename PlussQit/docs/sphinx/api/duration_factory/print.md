<!-- pluss-hand-authored -->
# pluss::duration::print
[Back to Duration Factory API Reference](../../duration_factory_api_reference)

## Call Forms
```cpp
std::string pluss::duration::print(const Duration& value);
std::ostream& pluss::duration::print(std::ostream& out,
                                     const Duration& value);
```

The string overload returns an owning diagnostic rendering. The stream
overload writes the identical representation and returns the stream for
chaining. Stream insertion delegates to the same renderer. Shape, `NaT`, and
all nine fractional-second digits are visible; printing never mutates data.

## Examples
```cpp
auto value = pluss::duration::nanos({0,-1});
std::cout << pluss::duration::print(value);
```
```text
Duration(shape=[2], dim=1, numel=2)
[ 0 days 00:00:00.000000000, -1 days 23:59:59.999999999 ]
```

```cpp
pluss::duration::print(std::cout, value) << '\n';
```

```cpp
std::ostringstream a, b;
pluss::duration::print(a, value);
b << value;
std::cout << std::boolalpha << (a.str() == b.str());
```
```text
true
```

