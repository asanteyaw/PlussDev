// Search a pile of incident reports: clean the text, rank by keywords, find the ones that match a pattern and
// score how similar each report is to a query.
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::kindle::literals;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;

int main() {
  Catalog reports({"id"_k = Stream(pluss::make_series({1, 2, 3, 4, 5})),
                   "text"_k = Stream(pluss::make_note({"Heavy rain on the ring road, two lanes closed",
                                                         "Clear weather downtown, traffic normal",
                                                         "Storm warning issued, rain expected overnight",
                                                         "Road works on the ring road until Friday",
                                                         "Sunny and dry across the region"}))});

  // 1. Clean the text stream: lower case and punctuation removed. clean returns a catalog holding the cleaned text.
  auto cleaned = reports.clean("text");
  std::cout << "cleaned: " << cleaned.keys("text").as_note().host().vector()[0] << "\n";

  // 2. grep keeps the rows whose text matches a pattern.
  auto rainy = reports.grep("text", "rain");
  std::cout << "reports that mention rain: " << rainy.nrows() << ", ids " << pluss::series::print(rainy.keys("id").as_series()) << "\n";

  // 3. keywords: the most frequent words across the whole collection, as a small catalog of (keyword, count).
  auto top = reports.keywords("text", 3);
  std::cout << "most frequent words: " << pluss::note::print(top.keys("keyword").as_note()) << " with counts " << pluss::series::print(top.keys("count").as_series()) << "\n";

  // 4. Rank the reports against a query by cosine similarity of their word vectors.
  auto queries = pluss::make_note({"rain on the road", "rain on the road", "rain on the road", "rain on the road", "rain on the road"});
  auto scored = reports.set("query", Stream(queries)).similarity("text", "query");
  auto similarity = scored.keys("text__query_similarity").as_series();
  std::cout << "similarity to the query: " << pluss::series::print(similarity) << "\n";
  auto order = similarity.argsort(0, true);
  std::cout << "best matches first (report ids): " << pluss::series::print(reports.keys("id").as_series().index_select(0, order)) << "\n";
}
