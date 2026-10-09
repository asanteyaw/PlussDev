<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 23cbea6cb377 -->
# `pluss::ai::ice::Coach::add_callback`

[Back to `pluss::ai::ice::Coach`](../Coach.md)

## Signature

```cpp
pluss::ai::ice::Coach& pluss::ai::ice::Coach::add_callback(std::shared_ptr<pluss::ai::ice::Callback> callback);
```

## What It Does

Appends `callback` to this `Coach`'s own list -- the primary way to
observe or steer a run from the inside, since
[`Model::learn`](../../model/Model/learn.md)'s own convenience
overloads always run through a fresh `Coach` that can install configured
built-ins but cannot receive caller-supplied callbacks (see
[`callback`'s own family page](../../callback.md)). A `nullptr`
argument is silently skipped, same as
[`CallbackChain::add`](../../callback/CallbackChain/add.md).

## Parameters

`std::shared_ptr<Callback> callback`
: The callback to append. Shared ownership -- keep your own
  `shared_ptr` to the same instance to read whatever state it
  accumulates after the run.

## Returns

`Coach&` -- `*this`, so calls chain.

## Errors

None -- `nullptr` is accepted and simply ignored.

## Practical Examples

Three examples: the returned reference confirming it really is
`*this`, chaining several calls in one expression, and a null argument
being silently skipped.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <memory>

namespace {

class NoopCallback final : public pluss::ai::ice::Callback {};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;

  ice::Coach coach;
  ice::Coach& ref = coach.add_callback(std::make_shared<NoopCallback>());
  std::cout << "add_callback returns *this: " << (&ref == &coach) << "\n";

  return 0;
}
```

Output

```text
add_callback returns *this: 1
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <memory>

namespace {

class NoopCallback final : public pluss::ai::ice::Callback {};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;

  // add_callback returns Coach& specifically so calls chain.
  ice::Coach coach;
  coach.add_callback(std::make_shared<NoopCallback>())
      .add_callback(std::make_shared<NoopCallback>())
      .add_callback(std::make_shared<NoopCallback>());

  std::cout << "callbacks().size() = " << coach.callbacks().size() << "\n";

  return 0;
}
```

Output

```text
callbacks().size() = 3
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Coach coach;
  coach.add_callback(nullptr);
  std::cout << "callbacks().size() after add_callback(nullptr) = " << coach.callbacks().size() << "\n";

  return 0;
}
```

Output

```text
callbacks().size() after add_callback(nullptr) = 0
```

## See Also

- [`callbacks`](callbacks.md) -- read the list this appends to.
- [`CallbackChain::add`](../../callback/CallbackChain/add.md) -- the
  same null-skipping shape, one level down.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="assess.html">
    <span class="label">Next</span>
    <span class="title">assess</span>
  </a>
</div>
<!-- apiref:nav:end -->
