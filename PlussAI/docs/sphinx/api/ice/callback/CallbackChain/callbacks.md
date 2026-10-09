<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c86b7b6bb2b2 -->
# `pluss::ai::ice::CallbackChain::callbacks`

[Back to `pluss::ai::ice::CallbackChain`](../CallbackChain.md)

## Signature

```cpp
[[nodiscard]] const std::vector<std::shared_ptr<pluss::ai::ice::Callback>>&
pluss::ai::ice::CallbackChain::callbacks() const noexcept;
```

## What It Does

Returns the ordered list this chain fans every hook out to, in the
exact order each was added via [`add`](add.md) or the bulk
constructor. `Coach::learn`'s own checkpoint logic reads this directly
to walk each held callback's `state_key`/`save_state`/`load_state` --
see [`CallbackChain`'s own page](../CallbackChain.md) for why that
happens against `callbacks()` rather than against the chain itself.

## Parameters

None.

## Returns

`const std::vector<std::shared_ptr<Callback>>&` -- a reference to the
chain's own list, never null entries (see
[`add`](add.md)'s null-skipping behavior).

## Errors

None.

## Practical Examples

Three examples: reading the list back after adding two callbacks,
calling the accessor through a `const` reference, and inspecting a held
[`EarlyStopCallback`](../EarlyStopCallback.md)'s own `state_key`.

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

  ice::CallbackChain chain;
  chain.add(std::make_shared<NoopCallback>());
  chain.add(std::make_shared<NoopCallback>());

  std::cout << "size = " << chain.callbacks().size() << "\n";
  for (const auto& callback : chain.callbacks()) {
    std::cout << "entry: non-null = " << (callback != nullptr) << ", state_key = \"" << callback->state_key() << "\"\n";
  }

  return 0;
}
```

Output

```text
size = 2
entry: non-null = 1, state_key = ""
entry: non-null = 1, state_key = ""
```

### Example 2

`callbacks()` is `const noexcept`, so it is reachable through a
`const CallbackChain&` -- useful for a function that only needs to
inspect a chain, not mutate it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <memory>

namespace {

class NoopCallback final : public pluss::ai::ice::Callback {};

void print_count(const pluss::ai::ice::CallbackChain& chain) {
  std::cout << "const accessor: callbacks().size() = " << chain.callbacks().size() << "\n";
}

} // namespace

int main() {
  namespace ice = pluss::ai::ice;

  ice::CallbackChain chain;
  chain.add(std::make_shared<NoopCallback>());

  print_count(chain);

  return 0;
}
```

Output

```text
const accessor: callbacks().size() = 1
```

### Example 3

Every held callback's own `state_key` is reachable this way --
`"early_stop"` for a built-in `EarlyStopCallback`, the same string
`Coach::learn`'s checkpoint save/restore logic looks for when it walks
`callbacks()` directly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <memory>

int main() {
  namespace ice = pluss::ai::ice;

  ice::ValidationPolicy validation{};
  ice::LoopRegularization regularization{};
  regularization.early_stopping = true;

  ice::CallbackChain chain;
  chain.add(std::make_shared<ice::EarlyStopCallback>(validation, regularization));

  for (const auto& callback : chain.callbacks()) {
    std::cout << "state_key = \"" << callback->state_key() << "\"\n";
  }

  return 0;
}
```

Output

```text
state_key = "early_stop"
```

## See Also

- [`add`](add.md) -- the only way entries get added to this list.
- [`Callback::state_key`](../Callback/state_key.md) -- what `Coach`
  reads from each entry for checkpointing.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add.html">
    <span class="label">Previous</span>
    <span class="title">add</span>
  </a>
</div>
<!-- apiref:nav:end -->
