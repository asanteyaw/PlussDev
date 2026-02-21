# PlussMa
The next generation technical computing and AI mega library

## Quick Start

```cpp
#include <pluss/pluss.h>

using namespace pluss;

// Create a Series (1D numeric series)
auto data = Series({1.0, 2.0, 3.0, 4.0, 5.0});
std::cout << "Mean: " << data.mean().item<float>() << "\n";

// Create a DataFrame (2D table)
auto prices = Series({10.5, 12.3, 11.8, 13.1});
auto volumes = Series({100, 150, 120, 180});

Thread price_thread(prices, "price");
Thread volume_thread(volumes, "volume");

auto df = std::make_shared<DataFrame>(std::vector{price_thread, volume_thread});
std::cout << "DataFrame: " << df->shape().first << " rows × " 
          << df->shape().second << " columns\n";
```

## Performance-Optimized CSV Reading

For large datasets (millions of rows, GB+ files), use the optional parallel CSV reader:

```cpp
#include <pluss/pluss.h>

// Simple one-liner (auto-optimized)
auto df = ParallelCSVReader().read_mixed("large_dataset.csv");

// Custom configuration for maximum performance
ParallelCSVConfig config;
config.chunk_size = 500000;           // 500K rows per chunk
config.num_threads = 0;               // Auto-detect CPU cores
config.use_memory_mapping = true;     // Memory-map large files

auto reader = ParallelCSVReader(config);
auto df = reader.read_mixed("huge_file.csv");

// Expected performance: 3-5x faster than sequential parsing
// Handles multi-GB files efficiently via mmap
```

**When to use parallel reader:**
- Files > 100MB
- Datasets with 1M+ rows
- Need for faster iteration cycles

**Benefits:**
- 3-5x speedup on multi-core systems
- Memory-efficient (uses memory mapping)
- Progress callbacks for monitoring
- Drop-in replacement (returns standard DataFrame)
- All downstream operations use standard `pluss.h` API

See [`examples/parallel_csv_example.cpp`](examples/parallel_csv_example.cpp) for comprehensive examples.

## Architecture

PlussMa follows a thread-first design philosophy:

- **Series**: 1D numeric series (PyTorch-backed)
- **Note**: 1D text/string data
- **Timeline**: 1D date/time data
- **Thread**: Unified 1D container (holds Series, Note, or Timeline)
- **DataFrame**: 2D table (collection of Threads)
- **Atlas**: 3D data structure (collection of DataFrames)

**Design Philosophy**: Thread (1D) → DataFrame (2D) → Atlas (3D)

## Documentation

- [Thread API Guide](docs/THREAD_API_GUIDE.md) - Comprehensive guide to working with Threads
- [DataFrame + Atlas User Guide](docs/DATAFRAME_ATLAS_USER_GUIDE.md) - End-user guide for table APIs, verbs, and IO
- [Examples](examples/) - Working code examples
- [Tests](test/) - Extensive test suite with real-world datasets

## Building

```bash
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH=/path/to/libtorch ..
make
```

**Enable experimental C++20 modules:**

```bash
cmake -DCMAKE_PREFIX_PATH=/path/to/libtorch -DPLUSS_ENABLE_MODULES=ON ..
make
```

## Requirements

- C++17 or later
- LibTorch (PyTorch C++ API)
- CMake 3.15+

## License

See [LICENSE](LICENSE) and [NOTICE.md](NOTICE.md)
