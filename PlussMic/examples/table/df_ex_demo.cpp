#include <pluss/pluss.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::filesystem::path data_file(const std::string& name) {
  return std::filesystem::path(PLUSSMIC_SOURCE_DIR) / "data" / name;
}

} // namespace

int main() {
   auto df = pluss::table::DataFrame::load_csv(
      data_file("messy_data.csv").string(),
      /*header=*/"infer", // use "none" if csv has no header row -> feat_0, feat_1, ...
      /*case=*/"Agency Name",
      /*engine=*/"auto", // auto (default), serial, parallel
      /*ncases=*/20);

   df.head(10);
   df.tail();
   df.info();

   pluss::Series code = df.features("Agency Code").value();
   std::cout << "Agency Code feature numel: " << code.numel() << "\n";
   pluss::Timeline updated_date = df.features("Updated Date").value();
   std::cout << "Updated Date feature numel: " << updated_date.numel() << "\n";
   auto all_feat_values = df.values();
   std::cout << "values() returns " << all_feat_values.size() << " feature threads\n";
   std::cout << "cell[0, Agency Code]: " << df.cases(0, "Agency Code") << "\n";
   df.cases(0).head();
   df.cases({1, 5}).head();

   // positional overload for quick loads
   auto raw_df = pluss::table::DataFrame::load_csv(
      data_file("bank-full.csv").string(),
      /*header=*/"none",
      /*ncases=*/5);
   raw_df.head(3);

   df.save_csv(data_file("messy_data_copy.csv").string(), /*add_cases=*/false);


   //==== Excel demo (requires libzip) ====
   auto xls_df = pluss::table::DataFrame::load_excel(
      data_file("test_grades.xlsx").string(),
      /*sheet name=*/"Data", // or use overload with sheet index: /*sheet index=*/0
      /*header=*/true);
   xls_df.head();

   // multi-sheet workbook load via same load_excel family
   auto loaded = pluss::table::DataFrame::load_excel(
      "../../data/report.xlsx",
      /*sheet names=*/{"Products", "Prices"},
      /*header=*/true);

   std::cout << "loaded workbook :\n" << loaded << "\n";

   for (const auto& [sheet_name, sheet_df] : loaded.workbook()) {
     std::cout << "\nSheet: " << sheet_name << "\n";
     sheet_df.head();
   }

   std::cout << "loaded workbook :\n" << loaded << "\n";
   loaded.sheets("Prices").head();

   // grab one sheet to use
   let products_df = loaded.sheets("Products");
   products_df.head();

   // list all sheet names
   auto sheet_names = loaded.sheet_names();
   std::cout << "\nAll sheet names in workbook:\n" << sheet_names << "\n";

   // read a sheet by index
   auto first_sheet = loaded.sheet_by_index(0);
   first_sheet.head();

   // save a dataframe to Excel
   products_df.save_excel("products_copy.xlsx", {{"sheet name", "Products"}});

   // load entire workbook with all sheets (note: may consume more memory if many/large sheets)
   auto loaded_full = pluss::table::DataFrame::load_excel("../../data/report.xlsx", nullptr, true);
   std::cout << "loaded full workbook :\n" << loaded_full << "\n";

  return 0;
}
