# Array files

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussQit](../index.md)

An array file holds one `Series` so that another program, or another tool, can read it. A `.npy` file holds a single array. A `.npz` file holds several named arrays in one archive. These are common exchange formats, so files written here open in other numerical software and files from there open here. For saving and loading PlussII's own values, including text and time, see the save and load guide.

## How it works

`save_npy` writes one `Series` to a `.npy` file and `load_npy` reads it back. `save_npz` writes a list of named arrays to a `.npz` file, one member for each name, and `load_npz` returns them as name and `Series` pairs in the order they were saved.

The shape, the element type and the values are kept. The dtypes that have a counterpart in the file formats are listed here; bfloat16 and the 8-bit floating-point types have none, so saving one is an error that says to convert it first.

| PlussII dtype | File type |
|---|---|
| `Bool` | boolean |
| `UInt8`, `UInt16`, `UInt32`, `UInt64` | unsigned integers of 8 to 64 bits |
| `Int8`, `Int16`, `Int32`, `Int64` | signed integers of 8 to 64 bits |
| `Float16`, `Float32`, `Float64` | floating point of 16, 32 and 64 bits |
| `Complex64`, `Complex128` | complex numbers made of two 32-bit or two 64-bit floats |

A `Series` that is not on the CPU is copied to the CPU first, and a view that is not contiguous is made contiguous, so what is written is always the values in row-major order.

Reading accepts what other tools write: either byte order, data stored in column-major order, and archives whose members are stored or compressed. Writing produces little-endian row-major data, with members stored uncompressed. Nothing here needs another library, and neither reading nor writing makes a copy of the array beyond the one it is read into.

## Signatures

```cpp
void pluss::io::save_npy(const pluss::Series& value, const std::filesystem::path& path)
pluss::Series pluss::io::load_npy(const std::filesystem::path& path)
void pluss::io::save_npz(const std::vector<std::pair<std::string, pluss::Series>>& arrays, const std::filesystem::path& path)
std::vector<std::pair<std::string, pluss::Series>> pluss::io::load_npz(const std::filesystem::path& path)
```

The header is `<pluss/array_file.h>`. An archive member `x` is stored as `x.npy`; the `.npy` is removed again when it is read.

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run.

## Example 1: One array and an archive

The program writes a 2 by 3 array to a `.npy` file and reads it back, then writes two arrays to one archive and lists what it holds.

```cpp
// Write an array to a file and read it back; several arrays in one archive.
#include <pluss/pluss.h>
#include <pluss/array_file.h>

#include <filesystem>
#include <iostream>

int main() {
  const auto dir = std::filesystem::temp_directory_path();
  const auto grid = pluss::make_series({1.5, -2.25, 3.0, 4.0, 5.5, 6.0}, {2, 3});

  pluss::io::save_npy(grid, dir / "grid.npy");
  const auto back = pluss::io::load_npy(dir / "grid.npy");
  std::cout << back << "\n";

  const auto weights = pluss::make_series({0.2, 0.3, 0.5});
  pluss::io::save_npz({{"grid", grid}, {"weights", weights}}, dir / "model.npz");
  for (const auto& [name, array] : pluss::io::load_npz(dir / "model.npz")) {
    std::cout << name << ": " << array.numel() << " values\n";
  }
}
```

Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1.5, -2.25, 3],
  [4, 5.5, 6]]
grid: 6 values
weights: 3 values
```

The array came back with its shape `[2,3]`, its dtype and every value. The archive returned its two members by name.

## Example 2: A dtype with no counterpart

A bfloat16 array has no form in these files.

```cpp
// What a dtype without a counterpart does.
#include <pluss/pluss.h>
#include <pluss/array_file.h>

#include <filesystem>
#include <iostream>

int main() {
  using pluss::codem::ScalarType;
  const auto path = std::filesystem::temp_directory_path() / "half.npy";

  const auto brain = pluss::make_series({1.0, 2.0}, {2}, pluss::Config{}.with_dtype(ScalarType::BFloat16));
  try {
    pluss::io::save_npy(brain, path);
  } catch (const pluss::Error& e) {
    std::cout << e.message() << "\n";
  }

  // Convert first, and it saves.
  pluss::io::save_npy(brain.to(pluss::codem::Platform::CPU, ScalarType::Float32), path);
  std::cout << "saved as " << pluss::io::load_npy(path).scalar_type() << "\n";
}
```

Output

```text
pluss::io::save_npy: this dtype has no array-file form; convert it first (astype to Float32 or Float64)
saved as float32
```

The error says what to do. After the conversion to `Float32` the array saved and read back as `float32`.

## Pitfalls

- **Integers and booleans keep their dtype.** A file of 64-bit integers reads as `Int64`, not as floating point. Convert after reading if you need another type.
- **A file that is not an array file is an error.** Reading a file without the format's header, one whose data is shorter than its shape, or an archive that fails its checksum stops with a message that names the file.
- **Archives larger than 4 GiB are not supported.** Write several smaller ones.
