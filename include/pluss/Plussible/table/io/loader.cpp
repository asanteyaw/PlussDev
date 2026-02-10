#include <pluss/pluss.h>
/* loader.cpp uses the high-level `load()` API which relies on
    format-specific loaders (csv/excel/parquet/rdbms) that are
    exposed via the umbrella headers. */

namespace pluss::table::io {

std::shared_ptr<DataFrame> load(const std::string& format, const std::string& filename) {
    if (format == "csv") return load_csv(filename);
    if (format == "excel") return load_excel(filename);
    if (format == "parquet") return load_parquet(filename);
    if (format == "rdbms") return load_rdbms(filename);
    throw std::invalid_argument("Unsupported table format: " + format);
}

}