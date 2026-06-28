# Pluss User Documentation

Pluss is a C++20 technical-computing ecosystem for arrays, tables, time
series, plotting, statistics, optimization, and machine learning.

The most common user-facing include is:

```cpp
#include <pluss/pluss.h>
```

The most common CMake target for downstream projects is:

```cmake
find_package(Pluss CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE Pluss::SDK)
```

## What to read first

If you are new to Pluss, read these pages in order:

1. [Installation and CMake](installation.md) explains how to build the SDK and
   consume it from another project.
2. [Quickstart](quickstart.md) builds a first program and introduces the main
   objects.
3. [User Guide](user_guide.md) gives the conceptual map of the ecosystem.
4. [Data Workflows](data_workflows.md) covers loading, inspecting, filtering,
   deriving, grouping, plotting, and saving table data.
5. [Numerical Workflows](numerical_workflows.md) covers `Series`, math,
   distributions, optimization, and differentiable calculations.

After that, use the topic guides and API pages below as reference material.

## Core mental model

Pluss is organized around a small set of composable data objects:

```text
Atlas -> DataFrame -> Thread -> Series / Note / Timeline / TimeIndex
```

- `Series` is the numeric and boolean tensor-style object.
- `Note` is the vectorized text object.
- `Timeline` and `TimeIndex` are time objects.
- `Thread` is a typed table column over one of those core objects.
- `DataFrame` is a 2D table of named `Thread` features over a case axis.
- `Atlas` is a stack of related `DataFrame` objects.

## Documentation map

```{toctree}
:maxdepth: 2

installation
quickstart
user_guide
data_workflows
numerical_workflows
lymph_user_guide
lymph_cookbook
lymph_combination_atlas
lymph_troubleshooting
nodej_user_guide
nodej_cookbook
plusspl_user_guide
plusspl_api_reference
flash_user_guide
flash_api_reference
plussma_optimization_user_guide
plussma_optimization_cookbook
plussma_distributions_user_guide
plussma_distributions_cookbook
plussma_finance_user_guide
plussma_finance_cookbook
plussma_finance_option_atlas
plussma_finance_troubleshooting
plussma_regression_user_guide
plussma_regression_api_reference
plussma_time_series_user_guide
plussma_hypothesis_user_guide
demo_catalog
plussmic_docs_hub
time_series_accessor_guide
series
series_query_user_guide
aux
aux_tutorial
tils_env
loader
io_save_load_guide
error_handling
autograd
autograd_api_reference
autodiff_architecture
optimization_diff_contract
dl_torch_maturity_blueprint
series_api_reference
series_all_methods_usage
note
content_moment_helpers
note_api_reference
note_all_methods_usage
timeline_timeindex
timeline_api_reference
timeindex_api_reference
timeline_all_methods_usage
timeindex_all_methods_usage
api_indexes
build_outputs
README
```
