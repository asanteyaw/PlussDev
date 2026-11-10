<!-- generated from contract.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 82c9f87c24d4 -->

# `math::Function::contract`

[Back to `math::Function`](../Function.md)

Sums the product of two tensor functions over chosen pairs of output axes.

## Signature

```cpp
math::Function math::Function::contract(const math::Function &rhs, pluss::RangeView<std::int64_t> lhs_axes, pluss::RangeView<std::int64_t> rhs_axes) const;
math::Function math::Function::contract(const math::Function &rhs, std::initializer_list<std::int64_t> lhs_axes, std::initializer_list<std::int64_t> rhs_axes) const;
```

## What It Does

`contract` is the general tensor product: `lhs_axes[k]` of the receiver is summed against `rhs_axes[k]` of the argument, for every `k`. The axes that are not contracted survive: the receiver's remaining axes first, in order, then the argument's. Axis numbers count *output* axes from zero; batch axes are not involved.

[`matmul`](matmul.md) is the special case that pairs the receiver's last axis with the argument's first. Reach for `contract` when the pairing is something else: `AᵀA` pairs the first axes of two matrices, the Frobenius inner product of two matrices pairs both axes, and a tensor-times-vector product can contract any axis of the tensor. The lists must have the same length and every paired axis must have the same length.

The two overloads differ only in how the axis lists are given: a brace list for fixed axes, a `RangeView` for lists built at run time.

## Parameters

`const math::Function &rhs`
: The second tensor. Both operands must share a variable trace, or one must be a constant such as `math::function(matrix)`, which is how a fixed matrix enters a formula.

`std::initializer_list<std::int64_t> lhs_axes`
: Output axes of the receiver to contract, by number from zero.

`std::initializer_list<std::int64_t> rhs_axes`
: Output axes of `rhs` to contract, paired with `lhs_axes` position by position. The `RangeView` overload takes both as ranges.

## Returns

A new `Function` whose output axes are the receiver's uncontracted axes followed by the argument's.

## Errors

- The two axis lists must have equal length, and each pair of contracted axes equal length.
- An axis number outside the output rank is refused.

## Practical Examples

### Example 1

`AᵀA` by pairing axis 0 of `A` with axis 0 of `A`. The survivors are the two column axes, so the result is `2 × 2`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto A = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}).reshape({3, 2});  // 3 x 2

  const math::Function gram = math::function({"x"}, [&](math::Function x) { return math::function(A).contract(math::function(A), {0}, {0}) * x; });
  out.writeln("shape:", gram.shape());
  out.writeln("A^T A at x = 1:", gram(1.0).reshape({4}).items<double>());
}
```

Output

```text
shape: [2, 2]
A^T A at x = 1: [35.0000, 44.0000, 44.0000, 56.0000]
```

`AᵀA = [[1+9+25, 2+12+30], [2+12+30, 4+16+36]] = [[35, 44], [44, 56]]`.

### Example 2

Pairing every axis gives a scalar, the Frobenius inner product `Σ Aᵢⱼ Bᵢⱼ`. Here `B` is `A` itself, so the result is the squared Frobenius norm `1 + 4 + 9 + 16 + 25 + 36 = 91`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto A = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}).reshape({3, 2});  // 3 x 2

  const math::Function norm2 = math::function({"x"}, [&](math::Function x) { return math::function(A).contract(math::function(A), {0, 1}, {0, 1}) * x; });
  out.writeln("rank:", norm2.rank(), "  value:", norm2(1.0).item<double>());
}
```

Output

```text
rank: 0   value: 91.0000
```

### Example 3

The run-time form. `matmul` is `contract` over the receiver's last axis and the argument's first; building the axis lists in vectors gives the same function.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto A = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}).reshape({3, 2});  // 3 x 2

  const auto B = pluss::make_series({1.0, 0.0, 2.0, 1.0, 0.0, 3.0, 1.0, 1.0}).reshape({2, 4});
  const std::vector<std::int64_t> left = {1}, right = {0};
  const math::Function by_contract = math::function({"x"}, [&](math::Function x) {
    return math::function(A).contract(math::function(B), pluss::RangeView<std::int64_t>(left), pluss::RangeView<std::int64_t>(right)) * x;
  });
  const math::Function by_matmul = math::function({"x"}, [&](math::Function x) { return math::function(A).matmul(math::function(B)) * x; });
  out.writeln("contract:", by_contract(1.0).reshape({12}).items<double>());
  out.writeln("matmul:  ", by_matmul(1.0).reshape({12}).items<double>());
}
```

Output

```text
contract: [1.0000, 6.0000, 4.0000, 3.0000, 3.0000, 12.0000, 10.0000, 7.0000, 5.0000, 18.0000, 16.0000, 11.0000]
matmul:   [1.0000, 6.0000, 4.0000, 3.0000, 3.0000, 12.0000, 10.0000, 7.0000, 5.0000, 18.0000, 16.0000, 11.0000]
```

## See Also

- [`matmul`](matmul.md): the common case
- [`transpose`](transpose.md): reorder axes first if needed

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="conj.html">
    <span class="label">Previous</span>
    <span class="title">conj</span>
  </a>
  <a class="next-link" href="cos.html">
    <span class="label">Next</span>
    <span class="title">cos</span>
  </a>
</div>
<!-- apiref:nav:end -->
