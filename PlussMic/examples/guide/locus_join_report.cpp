// Join two catalogs through a declared relationship and read what the join did to the records: matched, orphaned and omitted, counted.
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::kindle::literals;
using pluss::bundle::Locus;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;
using pluss::relational::RelationshipCardinality;

int main() {
  Catalog shipments({"shipment"_k = Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})), "port"_k = Stream(pluss::make_note({"A", "B", "A", "C", "Z"})), "tonnes"_k = Stream(pluss::make_series({12.0, 7.5, 20.0, 3.0, 9.0}))});
  Catalog ports({"port"_k = Stream(pluss::make_note({"A", "B", "C", "D"})), "country"_k = Stream(pluss::make_note({"GH", "NG", "SN", "CI"}))});
  pluss::relational::TableMeta sm, pm;
  sm.primary_key = {"shipment"};
  pm.primary_key = {"port"};
  auto workspace = Locus({{"shipments", shipments}, {"ports", ports}})
                       .set_table_meta("shipments", sm)
                       .set_table_meta("ports", pm)
                       .link({.name = "shipment_port", .left_table = "shipments", .right_table = "ports", .keys = {{"port", "port"}}, .cardinality = RelationshipCardinality::ManyToOne});

  // 1. Join shipments to their port's country. Shipment 5 goes to port Z, which is not in the ports table.
  auto detailed = workspace.join_detailed("shipments", "ports");
  std::cout << detailed.catalog << "\n";

  // 2. The report says exactly what happened: nothing is lost uncounted.
  std::cout << "report: " << detailed.report.summary() << "\n";

  // 3. The orphans on each side, from the relationship audit: shipments to unknown ports, and ports no shipment uses.
  std::cout << workspace.relationship_audit("shipment_port").features({"left_orphans", "right_orphans", "valid"}).to_string() << "\n";
}
