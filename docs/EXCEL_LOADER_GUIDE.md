# Excel Loader Implementation for PlussMa

## Overview

The Excel loader provides functionality to read Microsoft Excel files (.xlsx format) into PlussMa DataFrames. This implementation follows pandas' approach and is designed to work seamlessly with the existing CSV loader architecture.

## Features

- ✅ Read .xlsx files (Excel 2007+ format)
- ✅ Automatic type inference (converts to appropriate column types)
- ✅ Sheet selection by name or default to first sheet
- ✅ Shared strings support for text data
- ✅ Compatible with RawTable/DataFrame architecture

## Dependencies

### Required: libzip

The Excel loader requires **libzip** for reading .xlsx files (which are ZIP archives containing XML).

#### Installation Instructions

**macOS:**
```bash
brew install libzip
```

**Ubuntu/Debian:**
```bash
sudo apt-get install libzip-dev
```

**Fedora/RHEL/CentOS:**
```bash
sudo dnf install libzip-devel
```

**Windows (vcpkg):**
```bash
vcpkg install libzip
```

## Usage

### Basic Usage

```cpp
#include <pluss/pluss.h>

// Load first sheet of Excel file
auto df = pluss::table::io::load_excel("data/myfile.xlsx");

// Load specific sheet by name
auto df2 = pluss::table::io::load_excel("data/myfile.xlsx", "Sheet2");
```

### Advanced Usage - Raw Data

```cpp
// Get raw string data before type conversion
auto raw = pluss::table::io::load_excel_raw("data/myfile.xlsx");

// Access column names
for (const auto& col : raw.columns) {
    std::cout << col << "\n";
}

// Access data
for (const auto& value : raw.data["ColumnName"]) {
    std::cout << value << "\n";
}

// Manually convert to DataFrame with custom settings
auto df = pluss::table::io::convert_raw_to_dataframe(raw);
```

## Implementation Details

### Architecture

The Excel loader follows the same pattern as the CSV loader:

1. **Excel File (.xlsx)** → ZIP extraction via libzip
2. **XML Parsing** → Extract sheet data and shared strings
3. **RawTable** → String-based intermediate representation
4. **DataFrame** → Type-inferred final structure via coordinator

### File Structure

- `include/pluss/datalog/table/io/excel.h` - Public API
- `src/datalog/table/io/excel.cpp` - Implementation
- Integrated into `loader.cpp` dispatcher

### Supported Excel Features

✅ **Supported:**
- .xlsx files (Excel 2007+)
- Multiple sheets (by name)
- Shared strings
- Numeric and text data
- Empty cells
- Headers in first row

⚠️ **Not Yet Supported:**
- .xls files (older Excel format) - requires libxls
- Formulas (evaluates to cached values)
- Merged cells (treats as individual cells)
- Charts and images
- Cell formatting/styles
- Multiple header rows
- Custom date/time formats (reads as strings)

## Pandas Compatibility

This implementation mirrors pandas' `read_excel()` functionality:

**Pandas:**
```python
import pandas as pd
df = pd.read_excel("file.xlsx", sheet_name="Sheet1")
```

**PlussMa:**
```cpp
auto df = pluss::table::io::load_excel("file.xlsx", "Sheet1");
```

## Comparison to CSV Loader

| Feature | CSV Loader | Excel Loader |
|---------|-----------|--------------|
| File Format | .csv | .xlsx |
| Dependencies | None (std library) | libzip |
| Parsing | Line-by-line | ZIP + XML |
| Sheet Support | N/A | ✅ Multiple sheets |
| Type Inference | ✅ | ✅ |
| Performance | Fast | Moderate |

## Troubleshooting

### Build Errors

**Error:** `zip.h: No such file or directory`
- **Solution:** Install libzip development package (see Dependencies section)

**Error:** `undefined reference to 'zip_open'`
- **Solution:** Ensure libzip is linked in CMakeLists.txt (should be automatic)

### Runtime Errors

**Error:** `Could not open Excel file as ZIP archive`
- **Cause:** File is not a valid .xlsx file or is corrupted
- **Solution:** Verify the file is .xlsx format (not .xls)

**Error:** `Could not read sheet: SheetName`
- **Cause:** Sheet name doesn't exist
- **Solution:** Check sheet names in Excel or use default (first sheet)

## Future Enhancements

Potential improvements for future versions:

1. **Formula Evaluation** - Compute formula values instead of cached results
2. **Multi-header Support** - Handle multiple header rows
3. **Date/Time Parsing** - Automatic Excel date format conversion
4. **Merged Cell Handling** - Proper support for merged cells
5. **Legacy Format Support** - Add .xls support via libxls
6. **Performance Optimization** - Stream-based parsing for large files
7. **Cell Metadata** - Preserve formatting/style information
8. **Write Support** - Add Excel writing capabilities

## Testing

Run the Excel loader test:

```bash
cd build
./excel_example ../data/your_file.xlsx
```

Or use the automated test suite:

```bash
ctest -R excel
```

## References

- [Pandas Excel I/O Documentation](https://pandas.pydata.org/docs/reference/api/pandas.read_excel.html)
- [XLSX File Format Specification](https://learn.microsoft.com/en-us/openspecs/office_standards/)
- [libzip Documentation](https://libzip.org/documentation/)
- [Office Open XML Format](https://en.wikipedia.org/wiki/Office_Open_XML)
