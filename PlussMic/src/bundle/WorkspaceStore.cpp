#include "bundle/WorkspaceStore.h"

#include <algorithm>

#include "relational/Links.h"
#include "table/io/SnapshotCsv.h"

namespace pluss::bundle::store {

namespace {

using namespace pluss::relational;
using pluss::table::DataFrame;
using pluss::table::Slate;
using pluss::table::snapshot::bool_cell;
using pluss::table::snapshot::double_cell;
using pluss::table::snapshot::int_cell;
using pluss::table::snapshot::note_cell;

}  // namespace

DataFrame meta_frame(const std::vector<std::string>& tables,
                     const std::unordered_map<std::string, TableMeta>& meta_by_table) {
  std::vector<std::string> names, grains, pkeys, ckeys, times, labels, sources, provenances, notes;
  for (const auto& table : tables) {
    const auto it = meta_by_table.find(table);
    const TableMeta* meta = it != meta_by_table.end() ? &it->second : nullptr;
    names.push_back(table);
    grains.push_back(meta ? optional_text(meta->grain) : "");
    pkeys.push_back(meta ? join_compact(meta->primary_key, kInnerSep) : "");
    ckeys.push_back(meta ? encode_candidate_keys(meta->candidate_keys) : "");
    times.push_back(meta ? optional_text(meta->time_field) : "");
    labels.push_back(meta ? case_label_policy_string(meta->case_labels) : "unknown");
    sources.push_back(meta ? optional_text(meta->source) : "");
    provenances.push_back(meta ? optional_text(meta->provenance) : "");
    notes.push_back(meta ? optional_text(meta->note) : "");
  }
  return DataFrame({
      {"table", Slate(pluss::make_note(names))},
      {"grain", Slate(pluss::make_note(grains))},
      {"pkey", Slate(pluss::make_note(pkeys))},
      {"ckeys", Slate(pluss::make_note(ckeys))},
      {"time", Slate(pluss::make_note(times))},
      {"labels", Slate(pluss::make_note(labels))},
      {"source", Slate(pluss::make_note(sources))},
      {"provenance", Slate(pluss::make_note(provenances))},
      {"note", Slate(pluss::make_note(notes))},
  });
}

std::vector<std::pair<std::string, TableMeta>> parse_meta_frame(const DataFrame& frame) {
  std::vector<std::pair<std::string, TableMeta>> out;
  for (std::int64_t i = 0; i < frame.ncases(); ++i) {
    TableMeta meta;
    meta.grain = parse_optional_text(note_cell(frame, "grain", i));
    meta.primary_key = split_compact(note_cell(frame, "pkey", i), kInnerSep);
    meta.candidate_keys = decode_candidate_keys(note_cell(frame, "ckeys", i));
    meta.time_field = parse_optional_text(note_cell(frame, "time", i));
    meta.case_labels = parse_case_label_policy(note_cell(frame, "labels", i));
    meta.source = parse_optional_text(note_cell(frame, "source", i));
    meta.provenance = parse_optional_text(note_cell(frame, "provenance", i));
    meta.note = parse_optional_text(note_cell(frame, "note", i));
    if (meta.grain.has_value() || !meta.primary_key.empty() || !meta.candidate_keys.empty() ||
        meta.time_field.has_value() || meta.case_labels != CaseLabelPolicy::Unknown ||
        meta.source.has_value() || meta.provenance.has_value() || meta.note.has_value()) {
      out.emplace_back(note_cell(frame, "table", i), std::move(meta));
    }
  }
  return out;
}

DataFrame graph_frame(const std::vector<Relationship>& relationships) {
  std::vector<std::string> names, lefts, rights, keys, cards, statuses, notes;
  std::vector<bool> left_optional, right_optional;
  std::vector<double> confidences;
  for (const auto& rel : relationships) {
    names.push_back(rel.name);
    lefts.push_back(rel.left_table);
    rights.push_back(rel.right_table);
    keys.push_back(encode_relationship_keys(rel.keys));
    cards.push_back(relationship_cardinality_string(rel.cardinality));
    left_optional.push_back(rel.left_optional);
    right_optional.push_back(rel.right_optional);
    confidences.push_back(rel.confidence);
    statuses.push_back(validation_status_string(rel.status));
    notes.push_back(optional_text(rel.note));
  }
  return DataFrame({
      {"name", Slate(pluss::make_note(names))},
      {"left", Slate(pluss::make_note(lefts))},
      {"right", Slate(pluss::make_note(rights))},
      {"keys", Slate(pluss::make_note(keys))},
      {"cardinality", Slate(pluss::make_note(cards))},
      {"left_optional", Slate(pluss::make_series(left_optional))},
      {"right_optional", Slate(pluss::make_series(right_optional))},
      {"confidence", Slate(pluss::make_series(confidences))},
      {"status", Slate(pluss::make_note(statuses))},
      {"note", Slate(pluss::make_note(notes))},
  });
}

std::vector<Relationship> parse_graph_frame(const DataFrame& frame) {
  std::vector<Relationship> out;
  out.reserve(static_cast<std::size_t>(frame.ncases()));
  for (std::int64_t i = 0; i < frame.ncases(); ++i) {
    Relationship rel;
    rel.name = note_cell(frame, "name", i);
    rel.left_table = note_cell(frame, "left", i);
    rel.right_table = note_cell(frame, "right", i);
    rel.keys = decode_relationship_keys(note_cell(frame, "keys", i));
    rel.cardinality = parse_relationship_cardinality(note_cell(frame, "cardinality", i));
    rel.left_optional = bool_cell(frame, "left_optional", i);
    rel.right_optional = bool_cell(frame, "right_optional", i);
    rel.confidence = double_cell(frame, "confidence", i);
    rel.status = parse_relationship_status(note_cell(frame, "status", i));
    rel.note = parse_optional_text(note_cell(frame, "note", i));
    out.push_back(std::move(rel));
  }
  return out;
}

DataFrame views_frame(const std::vector<std::pair<std::string, JoinPlan>>& plans) {
  std::vector<std::string> view_names, roots, results, lefts, rights, hows, keys, suffixes, validates,
      relationships, step_results;
  std::vector<std::int64_t> step_indices;
  const auto blank = [&](const std::string& name, const JoinPlan& plan) {
    view_names.push_back(name);
    roots.push_back(plan.root_table);
    results.push_back(plan.result_name);
  };
  for (const auto& [name, plan] : plans) {
    if (plan.steps.empty()) {
      blank(name, plan);
      step_indices.push_back(-1);
      for (auto* column : {&lefts, &rights, &hows, &keys, &suffixes, &validates, &relationships, &step_results}) {
        column->emplace_back();
      }
      continue;
    }
    for (std::size_t i = 0; i < plan.steps.size(); ++i) {
      const auto& step = plan.steps[i];
      blank(name, plan);
      step_indices.push_back(static_cast<std::int64_t>(i));
      lefts.push_back(step.left_table);
      rights.push_back(step.right_table);
      hows.push_back(join_how_token(step.spec.how));
      keys.push_back(encode_relationship_keys(step.spec.keys));
      suffixes.push_back(step.spec.right_suffix);
      validates.push_back(step.spec.validate.has_value() ? relationship_cardinality_string(*step.spec.validate) : "");
      relationships.push_back(step.spec.relationship_name.value_or(""));
      step_results.push_back(step.spec.result_name.value_or(""));
    }
  }
  return DataFrame({
      {"view", Slate(pluss::make_note(view_names))},
      {"step", Slate(pluss::make_series(step_indices))},
      {"root", Slate(pluss::make_note(roots))},
      {"result", Slate(pluss::make_note(results))},
      {"left", Slate(pluss::make_note(lefts))},
      {"right", Slate(pluss::make_note(rights))},
      {"how", Slate(pluss::make_note(hows))},
      {"keys", Slate(pluss::make_note(keys))},
      {"right_suffix", Slate(pluss::make_note(suffixes))},
      {"validate", Slate(pluss::make_note(validates))},
      {"relationship", Slate(pluss::make_note(relationships))},
      {"step_result", Slate(pluss::make_note(step_results))},
  });
}

std::vector<std::pair<std::string, JoinPlan>> parse_views_frame(const DataFrame& frame) {
  std::unordered_map<std::string, JoinPlan> plans;
  std::vector<std::string> order;
  for (std::int64_t i = 0; i < frame.ncases(); ++i) {
    const auto name = note_cell(frame, "view", i);
    auto [it, inserted] = plans.emplace(name, JoinPlan{});
    if (inserted) {
      order.push_back(name);
      it->second.root_table = note_cell(frame, "root", i);
      it->second.result_name = note_cell(frame, "result", i);
    }
    if (int_cell(frame, "step", i) < 0) continue;
    JoinSpec spec;
    spec.how = parse_join_how(note_cell(frame, "how", i));
    spec.keys = decode_relationship_keys(note_cell(frame, "keys", i));
    spec.right_suffix = note_cell(frame, "right_suffix", i);
    spec.relationship_name = parse_optional_text(note_cell(frame, "relationship", i));
    const auto validate_text = note_cell(frame, "validate", i);
    if (!validate_text.empty()) spec.validate = parse_relationship_cardinality(validate_text);
    spec.result_name = parse_optional_text(note_cell(frame, "step_result", i));
    it->second.steps.push_back(JoinPlanStep{
        .left_table = note_cell(frame, "left", i),
        .right_table = note_cell(frame, "right", i),
        .spec = std::move(spec),
    });
  }
  std::vector<std::pair<std::string, JoinPlan>> out;
  out.reserve(order.size());
  for (const auto& name : order) out.emplace_back(name, std::move(plans.at(name)));
  return out;
}

} // namespace pluss::bundle::store
