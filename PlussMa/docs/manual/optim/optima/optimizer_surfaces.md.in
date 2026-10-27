# Optimizer Surfaces: Functional vs OOP

There are two ADAM surfaces in the Pluss stack by design.

## Functional Surface (`PlussMa`)

Namespace:

- `pluss::subsp::math::optim::adam_state_like(...)`
- `pluss::subsp::math::optim::adam_step_inplace(...)`
- `pluss::subsp::math::optim::adam_step(...)`

Intent:

- low-level `Series` update primitive
- explicit sensitivity/gradient updates
- useful for custom ML loops and research code

Demo:

- `PlussMa/examples/Numerical/optimization/adam_functional_demo.cpp`
- Includes both explicit-gradient and autodiff-generated-gradient usage.

## OOP Surface (`PlussAI`)

Namespace:

- `pluss::nn::optim::Adam`

Intent:

- module/parameter-list optimizer object
- DL-style training loop ergonomics
- parameter groups and richer optimizer policy

Demo:

- `PlussAI/examples/adam_oop_demo.cpp`
- Includes both explicit-gradient `step(grads)` and autodiff `step()` usage.

## Practical Rule

Use the namespace to communicate intent clearly:

- choose `pluss::subsp::math::optim::*` for functional/math-level optimization utilities
- choose `pluss::nn::optim::*` for neural-module training optimizers
