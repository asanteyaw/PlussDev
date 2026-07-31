# Config (Series creation options)

`pluss::Config` is the CPU-only configuration object used to control Series creation.

It is inspired by `torch::TensorOptions`, but intentionally smaller:

- CPU-only (no device selection yet)
- dtype is first-class
- reserved fields exist for forward compatibility (allocator, pinned memory, layouts)

## Example

```cpp
#include <pluss/pluss.h>

pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float16);

auto x = pluss::zeros({128, 64}, cfg);
```
