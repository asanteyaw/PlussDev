# Slate API Guide

## Overview

`Slate` is the foundation of the slate-first design in PlussMa. It represents a 1D data structure (like a pandas Series) that can hold three types of values:
- **Series**: Numeric series data (built on PyTorch)
- **Note**: String/text data
- **Timeline**: Date/time data

**Design Philosophy**: Slate (1D) → DataFrame (2D) → Atlas (3D)

## Creating Slates

```cpp
#include <pluss/datalog/table/slate/slate.h>
using namespace pluss::table;

// Series Slate (numeric data)
auto prices = pluss::series::from_torch(torch::series({10.5, 12.3, 11.8}));
Slate price_slate(prices, "price");

// Note Slate (string data)
std::vector<Chord> names = {Chord("Alice"), Chord("Bob")};
Slate name_slate(pluss::make_note(names), "name");

// Timeline Slate (date/time data)
// Slate time_slate(timeline_data, "timestamp");
```

## Accessing Slate Content: The values() Method

The `values()` method returns a reference to the underlying variant that holds Series, Note, or Timeline.

### Method 1: Direct Type Checking (Recommended)

```cpp
Slate slate(pluss::series::from_torch(torch::series({1.0, 2.0, 3.0})), "data");

// Check type first
if (slate.is_series()) {
    const pluss::Series& series = slate.as_series();
    std::cout << "Mean: " << series.mean().item<double>() << "\n";
}

if (slate.is_note()) {
    const pluss::Note& note = slate.as_note();
    auto shape = note.shape();
}

if (slate.is_timeline()) {
    const pluss::Timeline& timeline = slate.as_timeline();
    // Use timeline methods
}
```

### Method 2: Using std::visit (Type-Safe Pattern Matching)

```cpp
const auto& content = slate.values();

std::visit([](auto&& value) {
    using T = std::decay_t<decltype(value)>;
    if constexpr (std::is_same_v<T, pluss::Series>) {
        std::cout << "Series with " << value.numel() << " elements\n";
    } else if constexpr (std::is_same_v<T, pluss::Note>) {
        std::cout << "Note with shape: " << value.shape()[0] << "\n";
    } else if constexpr (std::is_same_v<T, pluss::Timeline>) {
        std::cout << "Timeline\n";
    }
}, content);
```

### Method 3: Using std::holds_alternative

```cpp
const auto& content = slate.values();

if (std::holds_alternative<pluss::Series>(content)) {
    const auto& series = std::get<pluss::Series>(content);
    // Use series...
}
```

### Method 4: Mutable Access

```cpp
// Get mutable reference to modify content
auto& content = slate.values();

if (std::holds_alternative<pluss::Series>(content)) {
    auto& series = std::get<pluss::Series>(content);
    // Modify series in-place
}
```

## Slate Operations

### Statistical Operations (Series only)

Use Series methods directly:

```cpp
Slate slate(pluss::series::from_torch(torch::series({1.0, 2.0, 3.0, 4.0, 5.0})), "data");

if (slate.is_series()) {
    const auto& s = slate.as_series();
    double mean = s.mean().item<double>();
    double sum = s.sum().item<double>();
    double std = s.std().item<double>();
    double min = s.min().item<double>();
    double max = s.max().item<double>();
}
```

### Transformations

```cpp
// Slicing
auto head = slate.head(10);        // First 10 elements
auto tail = slate.tail(5);         // Last 5 elements
auto slice = slate.iloc(2, 8);     // Elements from index 2 to 8

// Cumulative operations
auto cumsum = slate.cumsum();      // Cumulative sum
auto cumprod = slate.cumprod();    // Cumulative product
auto cummax = slate.cummax();      // Cumulative maximum
auto cummin = slate.cummin();      // Cumulative minimum

// Shift and difference
auto shifted = slate.shift(1);     // Shift by 1 position
auto diff = slate.diff(1);         // First difference

// Sorting and unique
auto sorted = slate.sort_values(true);  // Sort ascending
auto unique = slate.unique();           // Get unique values
```

### Missing Value Handling

```cpp
// Check for NA
auto is_na = slate.isna();         // Returns Series of bool
auto not_na = slate.notna();       // Returns Series of bool

// Handle NA
auto filled = slate.fillna(zero_slate);  // Fill NA with value
auto dropped = slate.dropna();            // Remove NA values
```

### Filtering

```cpp
// Boolean indexing
auto condition = pluss::series::from_torch(torch::series({true, false, true, false, true}));
auto filtered = slate.loc(condition);
```

## Series Methods Available

Since Slate wraps Series for numeric data, you have access to all Series methods:

```cpp
const auto& s = slate.as_series();

// Element-wise operations
s.abs();           // Absolute value
s.sqrt();          // Square root
s.log();           // Natural logarithm
s.exp();           // Exponential

// Reduction operations
s.sum();           // Sum all elements
s.mean();          // Mean
s.std();           // Standard deviation
s.var();           // Variance
s.min();           // Minimum
s.max();           // Maximum
s.median();        // Median

// Shape operations
s.numel();         // Number of elements
s.dim();           // Number of dimensions
s.sizes();         // Shape as array

// Indexing
s[0];              // Access element
s.narrow(0, 2, 5); // Narrow dimension 0 from index 2, length 5
s.slice(0, 2, 8);  // Slice dimension 0 from 2 to 8

// Comparisons
s.eq(5.0);         // Equal to
s.ne(5.0);         // Not equal to
s.gt(5.0);         // Greater than
s.lt(5.0);         // Less than
s.ge(5.0);         // Greater or equal
s.le(5.0);         // Less or equal

// Sorting (returns tuple)
auto [values, indices] = s.sort(0);        // Sort with indices
auto [cummax_vals, _] = s.cummax(0);       // Cumulative max
auto [cummin_vals, _] = s.cummin(0);       // Cumulative min
```

## Note Methods Available

For string/text data:

```cpp
const auto& note = slate.as_note();

note.to_lower();              // Convert to lowercase
note.to_upper();              // Convert to uppercase  
note.is_alpha();              // Check if alphabetic
note.eq(other_note);          // Element-wise equality
note.neq(other_note);         // Element-wise inequality
note.flatten();               // Flatten to vector
note.replace(pattern, repl);  // Replace pattern
note.slice(indices, dim);     // Slice by indices
note.T();                     // Transpose
```

## Timeline Methods Available

For date/time data:

```cpp
const auto& timeline = slate.as_timeline();

timeline.has_date();          // Check if has date component
timeline.has_time();          // Check if has time component
timeline.date_to_Series();    // Extract date as Series
timeline.time_to_Series();    // Extract time as Series
timeline.to_Series();         // Convert to numeric Series
```

## Best Practices

1. **Always check type before accessing**:
   ```cpp
   if (slate.is_series()) {
       const auto& s = slate.as_series();
       // Use s...
   }
   ```

2. **Use pluss::series::from_torch() for creating Series**:
   ```cpp
   auto series = pluss::series::from_torch(torch::series({1.0, 2.0, 3.0}));
   ```

3. **Prefer const references**:
   ```cpp
   const auto& series = slate.as_series();  // Avoid copying
   ```

4. **Handle exceptions**:
   ```cpp
   try {
       const auto& s = slate.as_series();
       // Use s...
   } catch (const std::bad_variant_access& e) {
       // Handle wrong type access
   }
   ```

5. **Use Slate operations when available**:
   ```cpp
   // Good: Use Slate method
   auto sorted = slate.sort_values(true);
   
   // Also valid: Use Series method directly
   if (slate.is_series()) {
       auto [sorted_vals, _] = slate.as_series().sort(0);
   }
   ```

## Integration with DataFrame

Slate is designed to be the foundation of DataFrame operations:

```cpp
// DataFrame operations delegate to Slate
df.head(10);           // Calls slate.head(10) for each column
df.cumsum();           // Calls slate.cumsum() for each column
df.fillna(value);      // Calls slate.fillna(value) for each column
df.sort_values(col);   // Uses slate.sort_values() on specified column
```

## Slate-First Design Philosophy

```
Slate (1D)
  ↓ Tile horizontally
DataFrame (2D)
  ↓ Stack vertically
Atlas (3D)
```

Each layer:
- **Slate**: 1D operations (statistical, cumulative, transformations)
- **DataFrame**: 2D operations (column-wise delegation, joins, pivots)
- **Atlas**: 3D operations (depth-wise aggregation, multi-dataset ops)

## Example: Complete Workflow

```cpp
#include <pluss/datalog/table/slate/slate.h>

using namespace pluss::table;

int main() {
    // Create slate
    auto data = pluss::series::from_torch(torch::series({100.0, 105.0, 103.0, 108.0, 110.0}));
    Slate prices(data, "stock_price");
    
    // Check content type
    std::cout << "Type: " << (prices.is_series() ? "Series" : "Other") << "\n";
    
    // Access using values()
    const auto& content = prices.values();
    std::visit([](auto&& val) {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, pluss::Series>) {
            std::cout << "Contains Series with " << val.numel() << " elements\n";
        }
    }, content);
    
    // Use Slate operations
    auto diff = prices.diff(1);
    auto cumsum = prices.cumsum();
    auto sorted = prices.sort_values(true);
    
    // Access Series directly for more operations
    if (prices.is_series()) {
        const auto& s = prices.as_series();
        std::cout << "Mean: " << s.mean().item<double>() << "\n";
        std::cout << "Std: " << s.std().item<double>() << "\n";
    }
    
    return 0;
}
```

## Performance: Loading Large Datasets

### Standard CSV Parsing (Simple Use Cases)

For small to medium datasets (<100MB), use the standard approach:

```cpp
#include <pluss/pluss.h>

// Your standard CSV parsing code
std::vector<std::vector<std::string>> data = load_csv("data.csv");

// Build DataFrame from Slates
std::vector<Slate> threads;
threads.emplace_back(Series(column_tensor), "column_name");
auto df = std::make_shared<DataFrame>(threads);
```

**When to use:** Files <100MB, quick prototyping, simple pipelines.

### Parallel CSV Reader (Large Datasets)

For large datasets (1M+ rows, 100MB+ files), opt-in to the parallel reader:

```cpp
#include <pluss/pluss.h>
#include <pluss/datalog/csv/parallel_csv_reader.h>  // Opt-in for performance

// Simple usage (auto-optimized)
auto df = ParallelCSVReader().read_mixed("large_dataset.csv");

// Advanced: Custom configuration
ParallelCSVConfig config;
config.chunk_size = 500000;           // Rows per chunk
config.num_threads = 0;               // 0 = auto-detect cores
config.use_memory_mapping = true;     // Memory-map for GB+ files
config.delimiter = ',';
config.trim_whitespace = true;

ParallelCSVReader reader(config);

// With progress callback
auto progress = [](size_t processed, size_t total) {
    std::cout << "\rProgress: " << (processed * 100.0 / total) << "%";
};

auto df = reader.read_mixed("huge_file.csv", progress);
```

**Performance Benefits:**
- **3-5x faster** than sequential parsing on 8-core systems
- **Memory-efficient**: Uses memory mapping for multi-GB files
- **Scalable**: Automatically uses available CPU cores
- **Compatible**: Returns standard `DataFrame` - all downstream operations work unchanged

**When to use:** Files >100MB, datasets with 1M+ rows, iterative analysis workflows.

**Example Results (US Accidents dataset: 1.5GB, 4.2M rows, 49 columns):**
- Sequential parsing: ~57 seconds
- Parallel parsing (8 threads, mmap): **~12-15 seconds** (estimated)

See [`examples/parallel_csv_example.cpp`](../examples/parallel_csv_example.cpp) for comprehensive usage examples.

### Design Trade-offs

| Approach | Use Case | Speed | Memory | Complexity |
|----------|----------|-------|--------|------------|
| Standard | <100MB files | Good | Low | Simple |
| Parallel | >100MB, 1M+ rows | Excellent | Optimized | One extra `#include` |

**Recommendation**: Start with standard parsing. Add parallel reader when you notice CSV loading is a bottleneck.

## Summary

The `values()` method provides:
- **Direct access** to the underlying Series/Note/Timeline
- **Type-safe** pattern matching with std::visit
- **Mutable access** for in-place modifications
- **Clean API** for inspecting and extracting content

Slate is the foundation that makes DataFrame and Atlas possible through its clean 1D operations and flexible type system.

For large-scale data processing, the optional parallel CSV reader provides significant performance gains while maintaining API compatibility.
