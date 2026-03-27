# Pluss Foundations

This document defines the baseline product narrative for `Plusses`: what it is, who it is for, what it solves, and how we decide what belongs in the repo.

## Positioning

`Plusses` is a C++20-first technical computing stack that unifies numeric tensors, text tensors, time indexes, tabular analytics, optimization, scientific computing, plotting, and ML workflows behind one coherent API family.

It is positioned as:

- a production-capable C++ stack,
- a collection of high performance spaces for any technical computing task
- a single-toolchain alternative to stitching many disconnected libraries,
- an SDK-style package that downstream projects can consume.

Vision framing:

- one integrated C++ stack that plays the role of multiple major technical-computing libraries in one space.

## Nutshell
Namespace: `pluss`
Product: `Plusses` (satirically can be referred to as `the Plusses`)
Spaces: PlussQit, PlussMa, PlussMic, etc

## Problem We Are Solving

Modern C++ data/ML/scientific workflows are fragmented:

- numeric compute, table ops, optimization, and model code often live in separate stacks,
- data movement and type conversion between stacks is expensive and fragile,
- API styles differ by subsystem, increasing integration cost and maintenance burden,
- platform behavior (CPU/GPU) is inconsistent across modules.

`Plusses` addresses this by making end-to-end workflows possible within one runtime and one API vocabulary.

## Stack Mapping

Conceptual mapping of Plusses spaces:

- The `PlussQit` space: This is the core space, with numerical computing and text processing engine supporting the other spaces.
- The `PlussMic` space: Efficient data analysis and manipulation functionalities and utilities.
- The `PlussPl` space: Immersive plotting engine for visual workflows and pictorial representations of data.
- The `PlussMa` space: High performance scientific computing spanning across different scientific and engineering domains, with numerical methods and optimization engines
- The `PlussAI` space: Artificial intelligence with machine learning, deep learning, robotics and other AI functionalities
- The `PlussTilsView` space: Graphical user interface for outputting results and visualizing workflows in real time.

This mapping is directional and maturity-dependent, with growing implementations and incoporation. More spaces may be added with time.

## Space Topology

Top-level organization model:

- `space -> subspaces -> libraries -> packages -> modules`

Examples:

- `PlussAI` (space) -> `NodeJ` / `NodeK` (subspaces)
- `PlussMa` (space) -> math/stat/finance/natsci subspaces
- `PlussQit` (space) -> `CodeM` / `CodeN` / `CodeH` libraries

## Primary Users

`Plusses` targets users who need one or more of:

- C++-native quantitative/scientific modeling with strong performance control,
- Data engineering + modeling without jumping across multiple languages,
- Reproducible platform-aware compute (CPU and GPU) in one codebase,
- an SDK that can be shipped and linked in third-party projects.

## Core Value Proposition

Users should be able to:

- Write minimal code that still covers full pipelines,
- Keep data in shared core abstractions (`Series`, `Note`, `Timeline`, `DataFrame`, `Atlas`),
- Move from preprocessing to modeling to diagnostics without runtime impedance mismatch,
- Keep compute platform behavior explicit and consistent.

## Scope By Space
`PlussQit` is the core and fundamental space fuelling the `Plusses`. All other spaces are built on top of it.

Current stack structure by space:

- `PlussQit` space: core primitives and runtime (`Series`, `Note`, `Timeline/Timeline`). `Series` is the core tensor engine for numerical computing. `Note` is the core tensor-like engine for word and text processing. `Timeline` and `TimeIndex` provide utilities for date-time operations and time series data processing.
- `PlussMic` space: Offers three data processing models. `Table` provides `DataFrame` for 2D structured data processing and `Atlas` for 3D structured data processing. `Kindle` provides a more slightly more general 2D(`Catalog`) and 3D(`Archive`) for data processing. Unlike `Table`, it can handle data of unequal column length. `Bundle` is the most general data processing model on offer. Unlike the other two, it can handle data with columns of heterogeneous types, with `Registry` for 2D and `Aisle` for 3D data processing. I/O workflows are also offered in the space.
- `PlussPl` space: Responsible for all plotting utilities and functionalities. Offers the platform for `Flash`, a higher level engine built on top of `PlussPl` functionalities to provide a convenient way of plotting, with complex plots made easier.
- `PlussMa` space: Provides several subspaces for a wide range of scientific computing tasks, quantiative modelling and numerical optimization work. The `Math` subspace provides libraries for tasks in linear algebra, optimization, finance, etc. The `Stats` subspace provides various libraries for statistical tasks like hypothesis testing, time series and inference. `NatSci` and `Eng` subspaces provide the platform natural sciences and engineering work.
- `PlussAI` space: Provides Machine and Deep learning surfaces (functional and module-style APIs), robotics and other artificial intelligence tools for AI work. 
- `PlussTilsView` space: Responsible for rendering/window/session infrastructure

## Product Goals

1. Correctness first: numerically sound behavior and explicit failure over silent degradation.
2. Platform consistency: operations should follow input platform semantics unless explicitly documented otherwise.
3. API coherence: shared naming and behavior across modules.
4. End-to-end usability: from preprocessing to modeling to reporting in one stack.
5. Third-party adoption: stable SDK packaging and consumption flow.

## Design Principles

These principles should guide implementation and docs:

- concise but descriptive naming with Pluss-native semantics,
- enum/config-driven surfaces over stringly configuration where possible,
- minimal boilerplate in user-facing demos,
- modular architecture with clear subsystem boundaries,
- avoid API additions that duplicate existing behavior without clear user gain.

## Success Criteria

We treat the stack as successful when users can:

- build and consume the SDK in external projects with `find_package(Pluss)`,
- complete typical workflows with few explicit conversions or glue code,
- rely on consistent behavior across CPU and accelerated backends,
- read docs and quickly understand where functionality lives and why.

## Documentation Roadmap (Foundational Order)

Recommended order for formal documentation build-out:

1. Foundations and positioning (this document)
2. API surface contracts per space (`Qit`, `Mic`, `Ma`, `AI`, `Pl`)
3. Runtime/platform semantics and backend behavior guarantees
4. Packaging/SDK consumption and versioning policy
5. Domain playbooks (finance, optimization, time series, deep learning, visualization)
