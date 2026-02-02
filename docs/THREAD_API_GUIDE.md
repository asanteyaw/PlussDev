# Thread API Guide

## Overview

`Thread` is the foundation of the thread-first design in PlussMa. It represents a 1D data structure (like a pandas Series) that can hold three types of values:
- **Series**: Numeric tensor data (built on PyTorch)
- **Note**: String/text data
- **Timeline**: Date/time data

**Design Philosophy**: Thread (1D) → DataFrame (2D) → Atlas (3D)

## Creating Threads

```cpp
#include <pluss/datalog/table/thread/thread.h>
using namespace pluss::table;

// Series Thread (numeric data)
auto prices = pluss::from_torch(torch::tensor({10.5, 12.3, 11.8}));
Thread price_thread(prices, "price");

// Note Thread (string data)
std::vector<Chord> names = {Chord("Alice"), Chord("Bob")};
Thread name_thread(pluss::make_note(names), "name");

// Timeline Thread (date/time data)
// Thread time_thread(timeline_data, "timestamp");
```

## Accessing Thread Content: The values() Method

The `values()` method returns a reference to the underlying variant that holds Series, Note, or Timeline.

### Method 1: Direct Type Checking (Recommended)

```cpp
Thread thread(pluss::from_torch(torch::tensor({1.0, 2.0, 3.0})), "data");

// Check type first
if (thread.is_series()) {
    const pluss::Series& series = thread.as_series();
    std::cout << "Mean: " << series.mean().item<double>() << "\n";
}

if (thread.is_note()) {
    const pluss::Note& note = thread.as_note();
    auto shape = note.shape();
}

if (thread.is_timeline()) {
    const pluss::Timeline& timeline = thread.as_timeline();
    // Use timeline methods
}
```

### Method 2: Using std::visit (Type-Safe Pattern Matching)

```cpp
const auto& content = thread.values();

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
const auto& content = thread.values();

if (std::holds_alternative<pluss::Series>(content)) {
    const auto& series = std::get<pluss::Series>(content);
    // Use series...
}
```

### Method 4: Mutable Access

```cpp
// Get mutable reference to modify content
auto& content = thread.values();

if (std::holds_alternative<pluss::Series>(content)) {
    auto& series = std::get<pluss::Series>(content);
    // Modify series in-place
}
```

## Thread Operations

### Statistical Operations (Series only)

Use Series methods directly:

```cpp
Thread thread(pluss::from_torch(torch::tensor({1.0, 2.0, 3.0, 4.0, 5.0})), "data");

if (thread.is_series()) {
    const auto& s = thread.as_series();
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
auto head = thread.head(10);        // First 10 elements
auto tail = thread.tail(5);         // Last 5 elements
auto slice = thread.iloc(2, 8);     // Elements from index 2 to 8

// Cumulative operations
auto cumsum = thread.cumsum();      // Cumulative sum
auto cumprod = thread.cumprod();    // Cumulative product
auto cummax = thread.cummax();      // Cumulative maximum
auto cummin = thread.cummin();      // Cumulative minimum

// Shift and difference
auto shifted = thread.shift(1);     // Shift by 1 position
auto diff = thread.diff(1);         // First difference

// Sorting and unique
auto sorted = thread.sort_values(true);  // Sort ascending
auto unique = thread.unique();           // Get unique values
```

### Missing Value Handling

```cpp
// Check for NA
auto is_na = thread.isna();         // Returns Series of bool
auto not_na = thread.notna();       // Returns Series of bool

// Handle NA
auto filled = thread.fillna(zero_thread);  // Fill NA with value
auto dropped = thread.dropna();            // Remove NA values
```

### Filtering

```cpp
// Boolean indexing
auto condition = pluss::from_torch(torch::tensor({true, false, true, false, true}));
auto filtered = thread.loc(condition);
```

## Series Methods Available

Since Thread wraps Series for numeric data, you have access to all Series methods:

```cpp
const auto& s = thread.as_series();

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
const auto& note = thread.as_note();

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
const auto& timeline = thread.as_timeline();

timeline.has_date();          // Check if has date component
timeline.has_time();          // Check if has time component
timeline.date_to_Series();    // Extract date as Series
timeline.time_to_Series();    // Extract time as Series
timeline.to_Series();         // Convert to numeric Series
```

## Best Practices

1. **Always check type before accessing**:
   ```cpp
   if (thread.is_series()) {
       const auto& s = thread.as_series();
       // Use s...
   }
   ```

2. **Use pluss::from_torch() for creating Series**:
   ```cpp
   auto series = pluss::from_torch(torch::tensor({1.0, 2.0, 3.0}));
   ```

3. **Prefer const references**:
   ```cpp
   const auto& series = thread.as_series();  // Avoid copying
   ```

4. **Handle exceptions**:
   ```cpp
   try {
       const auto& s = thread.as_series();
       // Use s...
   } catch (const std::bad_variant_access& e) {
       // Handle wrong type access
   }
   ```

5. **Use Thread operations when available**:
   ```cpp
   // Good: Use Thread method
   auto sorted = thread.sort_values(true);
   
   // Also valid: Use Series method directly
   if (thread.is_series()) {
       auto [sorted_vals, _] = thread.as_series().sort(0);
   }
   ```

## Integration with DataFrame

Thread is designed to be the foundation of DataFrame operations:

```cpp
// DataFrame operations delegate to Thread
df.head(10);           // Calls thread.head(10) for each column
df.cumsum();           // Calls thread.cumsum() for each column
df.fillna(value);      // Calls thread.fillna(value) for each column
df.sort_values(col);   // Uses thread.sort_values() on specified column
```

## Thread-First Design Philosophy

```
Thread (1D)
  ↓ Tile horizontally
DataFrame (2D)
  ↓ Stack vertically
Atlas (3D)
```

Each layer:
- **Thread**: 1D operations (statistical, cumulative, transformations)
- **DataFrame**: 2D operations (column-wise delegation, joins, pivots)
- **Atlas**: 3D operations (depth-wise aggregation, multi-dataset ops)

## Example: Complete Workflow

```cpp
#include <pluss/datalog/table/thread/thread.h>

using namespace pluss::table;

int main() {
    // Create thread
    auto data = pluss::from_torch(torch::tensor({100.0, 105.0, 103.0, 108.0, 110.0}));
    Thread prices(data, "stock_price");
    
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
    
    // Use Thread operations
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

## Summary

The `values()` method provides:
- **Direct access** to the underlying Series/Note/Timeline
- **Type-safe** pattern matching with std::visit
- **Mutable access** for in-place modifications
- **Clean API** for inspecting and extracting content

Thread is the foundation that makes DataFrame and Atlas possible through its clean 1D operations and flexible type system.
