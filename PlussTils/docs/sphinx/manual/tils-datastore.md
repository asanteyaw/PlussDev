# Datastores: ready-made data

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussTils Core Manual](core.md)

A first program, a guide or a bug report needs some data to work on. PlussII comes with 26 small datasets, one or more for every shape of data the PlussMic containers hold: a single column, a table, a catalog whose streams are all the same length, a catalog whose streams differ in length, and the same two for registries. Each is made in memory from numbers written into the library or from a fixed formula, so it is the same on every machine and needs no file, no network and no licence.

```cpp
#include <pluss/tils/kit/datastore.h>   // or <pluss/tils.h>, which includes it
```

The datasets need PlussMic, so `pluss/pluss.h` (which includes it) is all a program needs besides this header.

## How it works

`pluss::loader::datastores()` lists every dataset with its name, a description, its type, its shape and its size. `pluss::loader::datastore(name)` returns one.

A dataset has one fixed type: a table is a `DataFrame` and a ragged catalog is a `Catalog`, so the function returns a `Datastore` that holds the dataset. Take it out in the type you expect, in either of two ways:

- ask for it with `frame()`, `shelf()`, `stream()`, `catalog()`, `slot()` or `registry()`;
- assign it to a variable of that type, which converts it.

Asking for a type the dataset is not is an error that says what it holds.

The **shape** column says how the data is arranged: `1D` is one column, `2D` has several columns of the same length, and `ragged 2D` has columns of different lengths, so some positions exist in one column and not in another.

| Name | Type | Shape | What it holds |
|---|---|---|---|
| `sales` | `DataFrame` | 2D | Units, revenue and returns for four cities over six months |
| `accounts` | `DataFrame` | 2D | Seven accounts with a missing balance, NaN, `inf` and `-inf`: all four kinds of cell |
| `sensors` | `DataFrame` | 2D | Two days of hourly readings: a `Timeline` column and a six-hour gap in the humidity |
| `anscombe` | `DataFrame` | 2D | Anscombe's quartet: four pairs of columns with the same statistics and four different shapes |
| `messages` | `DataFrame` | 2D | Twelve short order and delivery-problem messages with a label |
| `temperature` | `Shelf` | 1D | Forty-eight hourly temperatures in degrees Celsius |
| `cities` | `Shelf` | 1D | Eight cities of Ghana, as text |
| `gaps` | `Shelf` | 1D | Seven ratios with a missing value, NaN, `inf` and `-inf` |
| `hours` | `Shelf` | 1D | The 24 hours of one day, as times |
| `ratings` | `Shelf` | 1D | Thirty survey answers from 1 to 5 |
| `pulse` | `Stream` | 1D | Twelve heart-rate readings, one a minute, labelled by minute |
| `words` | `Stream` | 1D | Six code words, as text |
| `outliers` | `Stream` | 1D | Eight readings with a missing value, a NaN and an outlier |
| `weather` | `Catalog` | 2D | A day of hourly temperature, humidity, pressure and wind |
| `grades` | `Catalog` | 2D | Marks of six students in three subjects |
| `prices` | `Catalog` | 2D | Closing prices of three shares over ten days, one price missing |
| `rainfall` | `Catalog` | ragged 2D | Monthly rainfall at four stations with 12, 12, 9 and 7 months on record, one month missing |
| `sessions` | `Catalog` | ragged 2D | Seconds spent on each page by five visitors who opened 5, 3, 8, 1 and 6 pages |
| `reviews` | `Catalog` | ragged 2D | Review comments for three products, four, two and three of them |
| `scores` | `Slot` | 1D | Six scores, one value in each cell |
| `spectra` | `Slot` | 1D | Four spectra of eight points, one curve in each cell |
| `mixed` | `Slot` | 1D | One cell of every kind: a number, text, a time, a duration and an empty cell |
| `patients` | `Registry` | 2D | Five patients with a name, a heart rate, an admission time and a waiting time |
| `orders` | `Registry` | 2D | Four orders with a customer, a list of line prices and a note |
| `trials` | `Registry` | ragged 2D | Three runs of an experiment with 3, 5 and 2 trials |
| `diary` | `Registry` | ragged 2D | A diary with 4 moods, 2 notes and 3 dates |

Except for the facts in `anscombe` (a published table), the numbers are invented. A name that is not on the list is an error that shows the names that exist and suggests the closest.

## Signatures

| Call | Returns |
|---|---|
| `pluss::loader::datastores()` | a `std::vector<pluss::loader::DatastoreInfo>`, one per dataset |
| `pluss::loader::datastore(std::string_view name)` | a `pluss::loader::Datastore` |
| `Datastore::name()` | the dataset's name |
| `Datastore::type()` | `"Shelf"`, `"DataFrame"`, `"Stream"`, `"Catalog"`, `"Slot"` or `"Registry"` |
| `Datastore::frame()`, `shelf()`, `stream()`, `catalog()`, `slot()`, `registry()` | the dataset in that type; an error if it is another |
| `Datastore::as<T>()` | the same, with the type as a template argument |

A `DatastoreInfo` has `name`, a description in `about`, `type`, `shape`, `cases` (the length of the longest column) and `features` (the number of columns).

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run.

## Example 1: What there is

The program lists every dataset with its type, shape and size.

```cpp
// The datasets that come with PlussII.
#include <pluss/pluss.h>
#include <pluss/tils/kit/datastore.h>

#include <iostream>

int main() {
  for (const auto& item : pluss::loader::datastores()) {
    std::cout << item.name << ": " << item.type << ", " << item.shape << ", " << item.cases << " x " << item.features << "\n";
  }
}
```

Output

```text
sales: DataFrame, 2D, 24 x 5
accounts: DataFrame, 2D, 7 x 3
sensors: DataFrame, 2D, 48 x 3
anscombe: DataFrame, 2D, 11 x 6
messages: DataFrame, 2D, 12 x 3
temperature: Shelf, 1D, 48 x 1
cities: Shelf, 1D, 8 x 1
gaps: Shelf, 1D, 7 x 1
hours: Shelf, 1D, 24 x 1
ratings: Shelf, 1D, 30 x 1
pulse: Stream, 1D, 12 x 1
words: Stream, 1D, 6 x 1
outliers: Stream, 1D, 8 x 1
weather: Catalog, 2D, 24 x 4
grades: Catalog, 2D, 6 x 3
prices: Catalog, 2D, 10 x 3
rainfall: Catalog, ragged 2D, 12 x 4
sessions: Catalog, ragged 2D, 8 x 5
reviews: Catalog, ragged 2D, 4 x 3
scores: Slot, 1D, 6 x 1
spectra: Slot, 1D, 4 x 1
mixed: Slot, 1D, 5 x 1
patients: Registry, 2D, 5 x 4
orders: Registry, 2D, 4 x 3
trials: Registry, ragged 2D, 5 x 3
diary: Registry, ragged 2D, 4 x 3
```

## Example 2: A table, and the wrong type

A `DataFrame` variable takes the table directly. Asking the same dataset for a `Catalog` is an error that names what it holds.

```cpp
// A table, assigned to a variable of its type, and a wrong guess about the type.
#include <pluss/pluss.h>
#include <pluss/tils/kit/datastore.h>

#include <iostream>

int main() {
  pluss::table::DataFrame sales = pluss::loader::datastore("sales");
  std::cout << sales.head(3) << "\n";
  try {
    (void)pluss::loader::datastore("sales").catalog();
  } catch (const pluss::Error& e) {
    std::cout << e.message() << "\n";
  }
}
```

Output

```text
===============================================
     | city    month    units  revenue  returns
-----------------------------------------------
0    | Accra   2024-01  116    27.84    3      
1    | Tema    2024-01  90     19.8     2      
2    | Kumasi  2024-01  144    33.84    4      
-----------------------------------------------
[3 cs x 5 feat]
pluss::loader::datastore("sales") holds a DataFrame, which is not the type that was asked for
```

## Example 3: A catalog whose streams differ in length

The rainfall stations began recording in different months, so Accra and Kumasi have 12 records, Tamale 9 and Wa 7. Wa also has one month with a missing reading. The `cs` column of the printout is each stream's length.

```cpp
// A catalog whose streams differ in length.
#include <pluss/pluss.h>
#include <pluss/tils/kit/datastore.h>

#include <iostream>

int main() {
  const auto rain = pluss::loader::datastore("rainfall").catalog();
  std::cout << rain << "\n";
  std::cout << "records at Tamale: " << rain.get("Tamale")->size() << ", at Wa: " << rain.get("Wa")->size() << "\n";
}
```

Output

```text
========================================================================================================================
kindle::Catalog  keys=4
keys: [Accra, Kumasi, Tamale, Wa]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
Accra                    | Series       | Series[float64]  | 12      | [18, 34, 62, ..., 16]                            
Kumasi                   | Series       | Series[float64]  | 12      | [24, 49, 98, ..., 25]                            
Tamale                   | Series       | Series[float64]  | 9       | [2, 8, 39, ..., 252]                             
Wa                       | Series       | Series[float64]  | 7       | [1, 5, 31, ..., 169]                             
========================================================================================================================

records at Tamale: 9, at Wa: 7
```

## Example 4: Registries, and a misspelt name

The `trials` registry is ragged: its fields hold from 2 to 5 values. The `patients` registry is not, so every field holds 5. A misspelt name is an error that suggests the closest.

```cpp
// A registry whose fields differ in length, and one whose fields are all the same length.
#include <pluss/pluss.h>
#include <pluss/tils/kit/datastore.h>

#include <iostream>

int main() {
  const auto trials = pluss::loader::datastore("trials").registry();
  std::cout << "trials: " << trials.nfields() << " fields, " << trials.minstate() << " to " << trials.maxstate() << " values each\n";
  const auto patients = pluss::loader::datastore("patients").registry();
  std::cout << "patients: " << patients.nfields() << " fields, " << patients.nstate() << " values each\n";
  try {
    (void)pluss::loader::datastore("rainfal");
  } catch (const pluss::Error& e) {
    std::cout << e.message() << "\n";
  }
}
```

Output

```text
trials: 3 fields, 2 to 5 values each
patients: 4 fields, 5 values each
pluss::loader::datastore: no dataset called 'rainfal' Did you mean 'rainfall'?
```

## Example 5: The single-column types

One dataset of each of the three single-column types: a stream, a column and a slot.

```cpp
// One dataset of each single-column type.
#include <pluss/pluss.h>
#include <pluss/tils/kit/datastore.h>

#include <iostream>

int main() {
  std::cout << pluss::loader::datastore("pulse").stream().size() << " readings in a stream\n";
  std::cout << pluss::loader::datastore("cities").shelf().size() << " cities in a column\n";
  std::cout << pluss::loader::datastore("mixed").slot().size() << " cells in a slot\n";
}
```

Output

```text
12 readings in a stream
8 cities in a column
5 cells in a slot
```
