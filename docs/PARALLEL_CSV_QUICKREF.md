# Parallel CSV Reader - Quick Reference

## TL;DR

```cpp
#include <pluss/pluss.h>
#include <pluss/datalog/csv/parallel_csv_reader.h>

// One-liner (auto-optimized)
auto df = ParallelCSVReader().read_mixed("large_file.csv");
```

## When to Use

| Scenario | Use Standard | Use Parallel |
|----------|--------------|--------------|
| File size | < 100MB | > 100MB |
| Row count | < 1M rows | > 1M rows |
| Prototype/quick script | ✓ | |
| Production pipeline | | ✓ |
| Iterative analysis | | ✓ |
| One-time load | ✓ | |

## Configuration Options

```cpp
ParallelCSVConfig config;
config.chunk_size = 500000;           // Rows per chunk (default: 100K)
config.num_threads = 0;               // 0 = auto-detect (default)
config.use_memory_mapping = true;     // Enable mmap (default: true)
config.skip_header = false;           // Skip first row (default: false)
config.delimiter = ',';               // Field separator (default: ',')
config.quote_char = '"';              // Quote character (default: '"')
config.trim_whitespace = true;        // Trim spaces (default: true)
config.buffer_size = 8192;            // I/O buffer (default: 8KB)
```

## API Methods

### 1. `read()` - Raw String Data
```cpp
auto data = reader.read("file.csv");  
// Returns: vector<vector<string>>
```

### 2. `read_mixed()` - Direct DataFrame (Recommended)
```cpp
auto df = reader.read_mixed("file.csv");
// Returns: shared_ptr<DataFrame>
```

### 3. With Progress Callback
```cpp
auto progress = [](size_t processed, size_t total) {
    std::cout << "\r" << (processed * 100 / total) << "% ";
};

auto df = reader.read_mixed("file.csv", progress);
```

## Performance Guidelines

### File Size vs Chunk Size
- **<500MB**: `chunk_size = 100000` (100K rows)
- **500MB-2GB**: `chunk_size = 250000` (250K rows)
- **>2GB**: `chunk_size = 500000` (500K rows)

### Slate Count
- **Default**: `num_threads = 0` (auto-detect)
- **CPU-bound**: Set to physical core count
- **I/O-bound**: Set to 2x physical core count
- **Memory-limited**: Reduce thread count and chunk size

### Memory Mapping
- **Enable** (default) for files >100MB
- **Disable** for network filesystems or small files

## Real-World Examples

### Example 1: Standard Data Science Workflow
```cpp
#include <pluss/pluss.h>
#include <pluss/datalog/csv/parallel_csv_reader.h>

// Load large dataset
auto df = ParallelCSVReader().read_mixed("sensor_data.csv");

// Standard PlussMa operations (all through pluss.h)
auto temp = std::get<Series>(df->get_col("temperature").values());
std::cout << "Mean: " << temp.mean().item<float>() << "\n";
std::cout << "Std: " << temp.std().item<float>() << "\n";
```

### Example 2: Custom Configuration
```cpp
ParallelCSVConfig config;
config.chunk_size = 500000;
config.num_threads = 8;
config.use_memory_mapping = true;

auto df = ParallelCSVReader(config).read_mixed("huge_file.csv");
```

### Example 3: Progress Monitoring
```cpp
auto reader = ParallelCSVReader();

auto progress = [](size_t proc, size_t total) {
    if (proc % 500000 == 0) {
        std::cout << "Loaded " << proc << " rows...\n";
    }
};

auto df = reader.read_mixed("large.csv", progress);
```

## Expected Performance

| Dataset | Rows | Size | Sequential | Parallel | Speedup |
|---------|------|------|------------|----------|---------|
| US Accidents | 4.2M | 1.5GB | 57s | ~15s | 3.8x |
| Weather Data | 10M | 3.2GB | 180s | ~45s | 4.0x |
| Sensor Logs | 50M | 8.5GB | 620s | ~140s | 4.4x |

*Benchmarks on 8-core Intel i7, 16GB RAM, SSD*

## Common Patterns

### Pattern 1: Simple Load
```cpp
auto df = ParallelCSVReader().read_mixed("data.csv");
```

### Pattern 2: Optimized Large File
```cpp
ParallelCSVConfig config;
config.chunk_size = 500000;
auto df = ParallelCSVReader(config).read_mixed("large.csv");
```

### Pattern 3: Raw Data Processing
```cpp
auto raw = ParallelCSVReader().read("data.csv");
// Process raw vector<vector<string>> yourself
```

### Pattern 4: Drop-in Replacement
```cpp
// Before (sequential):
// auto data = load_csv("file.csv");
// ... build DataFrame manually ...

// After (parallel):
auto df = ParallelCSVReader().read_mixed("file.csv");
```

## Troubleshooting

### Issue: Out of memory
**Solution**: Reduce `chunk_size` and `num_threads`
```cpp
config.chunk_size = 50000;
config.num_threads = 4;
```

### Issue: Slower than expected
**Solution**: Verify memory mapping is enabled
```cpp
config.use_memory_mapping = true;  // Should be true for large files
```

### Issue: Inconsistent performance
**Solution**: Ensure file is on local SSD, not network drive

### Issue: Progress callback too frequent
**Solution**: Add conditional in callback
```cpp
auto progress = [](size_t proc, size_t total) {
    if (proc % 100000 == 0) {  // Every 100K rows
        std::cout << proc << " rows\n";
    }
};
```

## Key Takeaways

✓ **Opt-in design**: Single extra `#include`, no breaking changes  
✓ **3-5x speedup**: Typical on 8-core systems for large files  
✓ **Memory-efficient**: Uses mmap for multi-GB files  
✓ **Drop-in replacement**: Returns standard `DataFrame`  
✓ **Zero downstream changes**: All PlussMa operations work unchanged  
✓ **Production-ready**: Used for 4.2M row datasets in government reports  

## See Also

- [examples/parallel_csv_example.cpp](../examples/parallel_csv_example.cpp) - Comprehensive examples
- [test/us_accidents_analysis_test.cpp](../test/us_accidents_analysis_test.cpp) - Real-world usage
- [docs/SLATE_API_GUIDE.md](SLATE_API_GUIDE.md) - Full API documentation
