#include "table/io/loader.h"
#include "table/io/csv.h"
#include "table/io/excel.h"
#include "table/io/parquet.h"
#include "table/io/rdbms.h"

namespace pluss::table::io {

std::shared_ptr<DataFrame> load(const std::string& format, const std::string& filename) {
    if (format == "csv") return load_csv(filename);
    if (format == "excel") return load_excel(filename);
    if (format == "parquet") return load_parquet(filename);
    if (format == "rdbms") return load_rdbms(filename);
    throw std::invalid_argument("Unsupported table format: " + format);
}

}