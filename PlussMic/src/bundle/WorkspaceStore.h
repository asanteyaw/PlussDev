#pragma once

// How a workspace's relational metadata (table metadata, links, saved plans) is
// written to and read from small CSV files. Shared by Nexus and Locus: whichever
// wrote a store, the schema/graph/views files read back identically.

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "relational/Codec.h"
#include "relational/Types.h"
#include "table/DataFrame.h"

namespace pluss::bundle::store {

// One row per table: table, grain, pkey, ckeys, time, labels, source, provenance, note.
pluss::table::DataFrame meta_frame(const std::vector<std::string>& tables,
                                   const std::unordered_map<std::string, relational::TableMeta>& meta);
// Only tables with some metadata are returned.
std::vector<std::pair<std::string, relational::TableMeta>> parse_meta_frame(const pluss::table::DataFrame& frame);

// One row per link.
pluss::table::DataFrame graph_frame(const std::vector<relational::Relationship>& relationships);
std::vector<relational::Relationship> parse_graph_frame(const pluss::table::DataFrame& frame);

// One row per plan step (a plan with no steps is one row with step = -1). Plans
// are written in the order given; pass them sorted by name for stable files.
pluss::table::DataFrame views_frame(const std::vector<std::pair<std::string, relational::JoinPlan>>& plans);
std::vector<std::pair<std::string, relational::JoinPlan>> parse_views_frame(const pluss::table::DataFrame& frame);

} // namespace pluss::bundle::store
