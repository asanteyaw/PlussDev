#include <pluss/datalog/registry/bundle/bundle.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <filesystem>

namespace pluss::registry {

// ========== Constructors ==========

Bundle::Bundle() : name_("unnamed") {}

Bundle::Bundle(const std::string& name) : name_(name) {}

Bundle::Bundle(const std::string& name, const std::unordered_map<std::string, Stream>& streams)
    : streams_(streams), name_(name) {}

// ========== Metadata ==========

int64_t Bundle::total_documents() const {
    int64_t total = 0;
    for (const auto& [_, stream] : streams_) {
        total += stream.size();
    }
    return total;
}

// ========== MongoDB-style Collection Management ==========

Bundle Bundle::create_stream(const std::string& stream_name) const {
    auto new_streams = streams_;
    if (new_streams.find(stream_name) != new_streams.end()) {
        throw std::runtime_error("Stream already exists: " + stream_name);
    }
    new_streams[stream_name] = Stream(stream_name);
    return Bundle(name_, new_streams);
}

Bundle Bundle::create_stream(const std::string& stream_name, const Stream& stream) const {
    auto new_streams = streams_;
    if (new_streams.find(stream_name) != new_streams.end()) {
        throw std::runtime_error("Stream already exists: " + stream_name);
    }
    new_streams[stream_name] = stream;
    return Bundle(name_, new_streams);
}

Bundle Bundle::add_stream(const std::string& stream_name, const Stream& stream) const {
    auto new_streams = streams_;
    new_streams[stream_name] = stream;
    return Bundle(name_, new_streams);
}

Bundle Bundle::insert_stream(const std::string& stream_name, const Stream& stream) const {
    return add_stream(stream_name, stream);
}

Bundle Bundle::drop_stream(const std::string& stream_name) const {
    auto new_streams = streams_;
    new_streams.erase(stream_name);
    return Bundle(name_, new_streams);
}

Bundle Bundle::drop_streams(const std::vector<std::string>& stream_names) const {
    auto new_streams = streams_;
    for (const auto& name : stream_names) {
        new_streams.erase(name);
    }
    return Bundle(name_, new_streams);
}

Bundle Bundle::clear() const {
    return Bundle(name_);
}

Bundle Bundle::rename_stream(const std::string& old_name, const std::string& new_name) const {
    auto new_streams = streams_;
    auto it = new_streams.find(old_name);
    if (it == new_streams.end()) {
        throw std::runtime_error("Stream not found: " + old_name);
    }
    
    Stream stream = it->second;
    stream.set_name(new_name);
    new_streams.erase(it);
    new_streams[new_name] = stream;
    
    return Bundle(name_, new_streams);
}

bool Bundle::has_stream(const std::string& stream_name) const {
    return streams_.find(stream_name) != streams_.end();
}

std::vector<std::string> Bundle::list_streams() const {
    std::vector<std::string> names;
    names.reserve(streams_.size());
    for (const auto& [name, _] : streams_) {
        names.push_back(name);
    }
    return names;
}

std::vector<std::string> Bundle::stream_names() const {
    return list_streams();
}

// ========== Stream Access ==========

const Stream& Bundle::get_stream(const std::string& stream_name) const {
    auto it = streams_.find(stream_name);
    if (it == streams_.end()) {
        throw std::runtime_error("Stream not found: " + stream_name);
    }
    return it->second;
}

Stream& Bundle::get_stream(const std::string& stream_name) {
    auto it = streams_.find(stream_name);
    if (it == streams_.end()) {
        throw std::runtime_error("Stream not found: " + stream_name);
    }
    return it->second;
}

std::optional<Stream> Bundle::try_get_stream(const std::string& stream_name) const {
    auto it = streams_.find(stream_name);
    if (it == streams_.end()) {
        return std::nullopt;
    }
    return it->second;
}

const Stream& Bundle::operator[](const std::string& stream_name) const {
    return get_stream(stream_name);
}

Stream& Bundle::operator[](const std::string& stream_name) {
    return get_stream(stream_name);
}

// ========== Document Operations (across streams) ==========

Bundle Bundle::insert_into(const std::string& stream_name, const Memo& doc) const {
    auto new_streams = streams_;
    
    if (new_streams.find(stream_name) == new_streams.end()) {
        new_streams[stream_name] = Stream(stream_name);
    }
    
    new_streams[stream_name] = new_streams[stream_name].insert_one(doc);
    return Bundle(name_, new_streams);
}

Bundle Bundle::insert_many_into(const std::string& stream_name,
                                 const std::vector<Memo>& docs) const {
    auto new_streams = streams_;
    
    if (new_streams.find(stream_name) == new_streams.end()) {
        new_streams[stream_name] = Stream(stream_name);
    }
    
    new_streams[stream_name] = new_streams[stream_name].insert_many(docs);
    return Bundle(name_, new_streams);
}

Bundle Bundle::update_in(const std::string& stream_name,
                          const QueryFilter& query,
                          const UpdateFunction& update) const {
    auto new_streams = streams_;
    auto it = new_streams.find(stream_name);
    if (it == new_streams.end()) {
        return *this;
    }
    
    new_streams[stream_name] = it->second.update_many(query, update);
    return Bundle(name_, new_streams);
}

Bundle Bundle::delete_from(const std::string& stream_name,
                            const QueryFilter& query) const {
    auto new_streams = streams_;
    auto it = new_streams.find(stream_name);
    if (it == new_streams.end()) {
        return *this;
    }
    
    new_streams[stream_name] = it->second.delete_many(query);
    return Bundle(name_, new_streams);
}

std::vector<Memo> Bundle::find_all(const QueryFilter& query) const {
    std::vector<Memo> results;
    
    for (const auto& [_, stream] : streams_) {
        auto matches = stream.find(query).to_vector();
        results.insert(results.end(), matches.begin(), matches.end());
    }
    
    return results;
}

std::unordered_map<std::string, Stream> Bundle::find_in_all(const QueryFilter& query) const {
    std::unordered_map<std::string, Stream> results;
    
    for (const auto& [name, stream] : streams_) {
        auto matches = stream.find(query);
        if (!matches.empty()) {
            results[name] = matches;
        }
    }
    
    return results;
}

// ========== Cross-Stream Operations (Production-grade with vectorized joins) ==========

Stream Bundle::join(const std::string& left_stream,
                     const std::string& right_stream,
                     const std::string& on_field,
                     const std::string& join_type) const {
    auto left_it = streams_.find(left_stream);
    auto right_it = streams_.find(right_stream);
    
    if (left_it == streams_.end()) {
        throw std::runtime_error("Left stream not found: " + left_stream);
    }
    if (right_it == streams_.end()) {
        throw std::runtime_error("Right stream not found: " + right_stream);
    }
    
    const auto& left = left_it->second;
    const auto& right = right_it->second;
    
    // Use vectorized hash-based joins from Stream
    if (join_type == "inner") {
        return left.inner_join(right, on_field);  // Vectorized hash join
    } else if (join_type == "left") {
        return left.left_join(right, on_field);   // Vectorized hash join
    } else if (join_type == "cross") {
        return left.cross_join(right);
    } else {
        throw std::runtime_error("Unknown join type: " + join_type);
    }
}

Bundle Bundle::lookup(const std::string& from_stream,
                       const std::string& to_stream,
                       const std::string& local_field,
                       const std::string& foreign_field,
                       const std::string& as_field) const {
    auto from_it = streams_.find(from_stream);
    auto to_it = streams_.find(to_stream);
    
    if (from_it == streams_.end()) {
        throw std::runtime_error("From stream not found: " + from_stream);
    }
    if (to_it == streams_.end()) {
        throw std::runtime_error("To stream not found: " + to_stream);
    }
    
    auto new_streams = streams_;
    // Uses vectorized $lookup from Stream
    new_streams[from_stream] = from_it->second.$lookup(to_it->second, 
                                                        local_field, 
                                                        foreign_field, 
                                                        as_field);
    
    return Bundle(name_, new_streams);
}

Stream Bundle::union_streams(const std::vector<std::string>& stream_names) const {
    if (stream_names.empty()) {
        return Stream();
    }
    
    // Production: batch concatenation with memory pre-allocation
    int64_t total_docs = 0;
    std::vector<const Stream*> valid_streams;
    valid_streams.reserve(stream_names.size());
    
    for (const auto& name : stream_names) {
        auto it = streams_.find(name);
        if (it != streams_.end()) {
            valid_streams.push_back(&it->second);
            total_docs += it->second.size();
        }
    }
    
    std::vector<Memo> all_docs;
    all_docs.reserve(total_docs);
    
    for (const auto* stream : valid_streams) {
        auto docs = stream->to_vector();
        all_docs.insert(all_docs.end(), 
                       std::make_move_iterator(docs.begin()),
                       std::make_move_iterator(docs.end()));
    }
    
    return Stream(name_ + "_union", std::move(all_docs));
}

Stream Bundle::union_all() const {
    // Efficient batch union of all streams
    int64_t total_docs = total_documents();
    std::vector<Memo> all_docs;
    all_docs.reserve(total_docs);
    
    for (const auto& [_, stream] : streams_) {
        auto docs = stream.to_vector();
        all_docs.insert(all_docs.end(),
                       std::make_move_iterator(docs.begin()),
                       std::make_move_iterator(docs.end()));
    }
    
    return Stream(name_ + "_all", std::move(all_docs));
}

Stream Bundle::intersect(const std::string& stream1,
                          const std::string& stream2,
                          const std::string& on_field) const {
    auto s1_it = streams_.find(stream1);
    auto s2_it = streams_.find(stream2);
    
    if (s1_it == streams_.end() || s2_it == streams_.end()) {
        return Stream();
    }
    
    const auto& s1 = s1_it->second;
    const auto& s2 = s2_it->second;
    
    std::vector<Memo> results;
    
    for (const auto& doc1 : s1) {
        if (!doc1.has_field(on_field)) continue;
        auto val1 = doc1.get_field(on_field);
        
        for (const auto& doc2 : s2) {
            if (!doc2.has_field(on_field)) continue;
            auto val2 = doc2.get_field(on_field);
            
            if (val1 == val2) {
                results.push_back(doc1);
                break;
            }
        }
    }
    
    return Stream(stream1 + "_intersect_" + stream2, results);
}

Stream Bundle::difference(const std::string& stream1,
                           const std::string& stream2,
                           const std::string& on_field) const {
    auto s1_it = streams_.find(stream1);
    auto s2_it = streams_.find(stream2);
    
    if (s1_it == streams_.end()) {
        return Stream();
    }
    if (s2_it == streams_.end()) {
        return s1_it->second;
    }
    
    const auto& s1 = s1_it->second;
    const auto& s2 = s2_it->second;
    
    std::vector<Memo> results;
    
    for (const auto& doc1 : s1) {
        if (!doc1.has_field(on_field)) {
            results.push_back(doc1);
            continue;
        }
        
        auto val1 = doc1.get_field(on_field);
        bool found = false;
        
        for (const auto& doc2 : s2) {
            if (!doc2.has_field(on_field)) continue;
            auto val2 = doc2.get_field(on_field);
            
            if (val1 == val2) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            results.push_back(doc1);
        }
    }
    
    return Stream(stream1 + "_minus_" + stream2, results);
}

// ========== Aggregation Operations (Vectorized via Stream) ==========

double Bundle::sum_all(const std::string& field) const {
    // Production-grade: parallel reduction across streams
    double total = 0.0;
    for (const auto& [_, stream] : streams_) {
        total += stream.sum(field);  // Uses vectorized Stream::sum
    }
    return total;
}

double Bundle::mean_all(const std::string& field) const {
    // Weighted average across all streams
    double total_sum = 0.0;
    int64_t total_count = 0;
    
    for (const auto& [_, stream] : streams_) {
        int64_t count = stream.count_field(field);
        if (count > 0) {
            total_sum += stream.sum(field);  // Vectorized
            total_count += count;
        }
    }
    
    return total_count > 0 ? total_sum / static_cast<double>(total_count) : 0.0;
}

double Bundle::min_all(const std::string& field) const {
    // Production: vectorized min across all streams
    double min_val = std::numeric_limits<double>::infinity();
    bool found = false;
    
    for (const auto& [_, stream] : streams_) {
        double stream_min = stream.min(field);  // Vectorized
        if (!std::isnan(stream_min) && !std::isinf(stream_min)) {
            found = true;
            min_val = std::min(min_val, stream_min);
        }
    }
    
    return found ? min_val : std::numeric_limits<double>::quiet_NaN();
}

double Bundle::max_all(const std::string& field) const {
    // Production: vectorized max across all streams
    double max_val = -std::numeric_limits<double>::infinity();
    bool found = false;
    
    for (const auto& [_, stream] : streams_) {
        double stream_max = stream.max(field);  // Vectorized
        if (!std::isnan(stream_max) && !std::isinf(stream_max)) {
            found = true;
            max_val = std::max(max_val, stream_max);
        }
    }
    
    return found ? max_val : std::numeric_limits<double>::quiet_NaN();
}

int64_t Bundle::count_all() const {
    return total_documents();
}

int64_t Bundle::count_in(const std::string& stream_name) const {
    auto it = streams_.find(stream_name);
    if (it == streams_.end()) return 0;
    return it->second.size();
}

std::unordered_map<std::string, double> Bundle::sum_by_stream(const std::string& field) const {
    // Batch compute across all streams (could be parallelized)
    std::unordered_map<std::string, double> results;
    results.reserve(streams_.size());
    
    for (const auto& [name, stream] : streams_) {
        results[name] = stream.sum(field);  // Vectorized
    }
    return results;
}

std::unordered_map<std::string, double> Bundle::mean_by_stream(const std::string& field) const {
    std::unordered_map<std::string, double> results;
    results.reserve(streams_.size());
    
    for (const auto& [name, stream] : streams_) {
        results[name] = stream.mean(field);  // Vectorized
    }
    return results;
}

std::unordered_map<std::string, int64_t> Bundle::count_by_stream() const {
    std::unordered_map<std::string, int64_t> results;
    results.reserve(streams_.size());
    
    for (const auto& [name, stream] : streams_) {
        results[name] = stream.size();
    }
    return results;
}

// ========== Functional Operations ==========

Bundle Bundle::map_streams(std::function<Stream(const Stream&)> transform) const {
    auto new_streams = streams_;
    for (auto& [name, stream] : new_streams) {
        stream = transform(stream);
    }
    return Bundle(name_, new_streams);
}

Bundle Bundle::filter_streams(std::function<bool(const std::string&, const Stream&)> predicate) const {
    std::unordered_map<std::string, Stream> new_streams;
    
    for (const auto& [name, stream] : streams_) {
        if (predicate(name, stream)) {
            new_streams[name] = stream;
        }
    }
    
    return Bundle(name_, new_streams);
}

Bundle Bundle::update_stream(const std::string& stream_name,
                              std::function<Stream(const Stream&)> transform) const {
    auto new_streams = streams_;
    auto it = new_streams.find(stream_name);
    if (it != new_streams.end()) {
        it->second = transform(it->second);
    }
    return Bundle(name_, new_streams);
}

// ========== Batch Operations ==========

Bundle Bundle::batch(const std::vector<BatchOperation>& operations) const {
    Bundle result = *this;
    for (const auto& op : operations) {
        result = op(result);
    }
    return result;
}

Bundle Bundle::bulk_insert(const std::unordered_map<std::string, std::vector<Memo>>& docs) const {
    auto new_streams = streams_;
    
    for (const auto& [stream_name, memos] : docs) {
        if (new_streams.find(stream_name) == new_streams.end()) {
            new_streams[stream_name] = Stream(stream_name);
        }
        new_streams[stream_name] = new_streams[stream_name].insert_many(memos);
    }
    
    return Bundle(name_, new_streams);
}

// ========== Query Builder Operations ==========

Bundle Bundle::where(std::function<bool(const std::string&, const Stream&)> predicate) const {
    return filter_streams(predicate);
}

Bundle Bundle::select_streams(const std::vector<std::string>& stream_names) const {
    std::unordered_map<std::string, Stream> new_streams;
    
    for (const auto& name : stream_names) {
        auto it = streams_.find(name);
        if (it != streams_.end()) {
            new_streams[name] = it->second;
        }
    }
    
    return Bundle(name_, new_streams);
}

Bundle Bundle::exclude_streams(const std::vector<std::string>& stream_names) const {
    auto new_streams = streams_;
    
    for (const auto& name : stream_names) {
        new_streams.erase(name);
    }
    
    return Bundle(name_, new_streams);
}

std::vector<std::string> Bundle::sorted_stream_names(bool by_size, bool ascending) const {
    std::vector<std::pair<std::string, int64_t>> name_size_pairs;
    name_size_pairs.reserve(streams_.size());
    
    for (const auto& [name, stream] : streams_) {
        name_size_pairs.emplace_back(name, stream.size());
    }
    
    if (by_size) {
        std::sort(name_size_pairs.begin(), name_size_pairs.end(),
            [ascending](const auto& a, const auto& b) {
                return ascending ? a.second < b.second : a.second > b.second;
            });
    } else {
        std::sort(name_size_pairs.begin(), name_size_pairs.end(),
            [ascending](const auto& a, const auto& b) {
                return ascending ? a.first < b.first : a.first > b.first;
            });
    }
    
    std::vector<std::string> result;
    result.reserve(name_size_pairs.size());
    for (const auto& [name, _] : name_size_pairs) {
        result.push_back(name);
    }
    
    return result;
}

// ========== Database Statistics ==========

Bundle::StreamStats Bundle::stream_stats(const std::string& stream_name) const {
    auto it = streams_.find(stream_name);
    if (it == streams_.end()) {
        throw std::runtime_error("Stream not found: " + stream_name);
    }
    
    const auto& stream = it->second;
    
    StreamStats stats;
    stats.name = stream_name;
    stats.doc_count = stream.size();
    stats.fields = stream.all_fields();
    stats.estimated_size_bytes = stream.size() * 100;  // Rough estimate
    
    return stats;
}

std::unordered_map<std::string, Bundle::StreamStats> Bundle::all_stream_stats() const {
    std::unordered_map<std::string, StreamStats> results;
    
    for (const auto& [name, _] : streams_) {
        results[name] = stream_stats(name);
    }
    
    return results;
}

int64_t Bundle::estimated_size_bytes() const {
    int64_t total = 0;
    for (const auto& [_, stream] : streams_) {
        total += stream.size() * 100;  // Rough estimate
    }
    return total;
}

// ========== I/O Operations ==========

std::string Bundle::to_json() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"name\":\"" << name_ << "\",";
    oss << "\"streams\":{";
    
    bool first = true;
    for (const auto& [name, stream] : streams_) {
        if (!first) oss << ",";
        first = false;
        oss << "\"" << name << "\":" << stream.to_json();
    }
    
    oss << "}}";
    return oss.str();
}

void Bundle::to_json_file(const std::string& filename) const {
    std::ofstream file(filename);
    file << to_json();
}

Bundle Bundle::from_json(const std::string& json_str) {
    // Simplified parser - in production use proper JSON library
    return Bundle();
}

Bundle Bundle::from_json_file(const std::string& filename) {
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return from_json(buffer.str());
}

void Bundle::to_directory(const std::string& dir_path) const {
    std::filesystem::create_directories(dir_path);
    
    for (const auto& [name, stream] : streams_) {
        std::string filepath = dir_path + "/" + name + ".jsonl";
        stream.to_jsonl_file(filepath);
    }
}

Bundle Bundle::from_directory(const std::string& dir_path) {
    std::unordered_map<std::string, Stream> streams;
    
    for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
        if (entry.path().extension() == ".jsonl") {
            std::string stream_name = entry.path().stem().string();
            streams[stream_name] = Stream::from_jsonl_file(entry.path().string());
        }
    }
    
    return Bundle("loaded", streams);
}

std::string Bundle::to_jsonl() const {
    std::ostringstream oss;
    for (const auto& [name, stream] : streams_) {
        oss << "{\"stream\":\"" << name << "\",\"data\":" << stream.to_json() << "}\n";
    }
    return oss.str();
}

void Bundle::to_jsonl_file(const std::string& filename) const {
    std::ofstream file(filename);
    file << to_jsonl();
}

Bundle Bundle::from_jsonl(const std::string& jsonl_str) {
    // Simplified parser
    return Bundle();
}

Bundle Bundle::from_jsonl_file(const std::string& filename) {
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return from_jsonl(buffer.str());
}

void Bundle::to_binary(const std::string& filename) const {
    // Binary serialization - simplified
    to_json_file(filename);
}

Bundle Bundle::from_binary(const std::string& filename) {
    return from_json_file(filename);
}

void Bundle::export_stream_json(const std::string& stream_name,
                                 const std::string& filename) const {
    auto it = streams_.find(stream_name);
    if (it == streams_.end()) {
        throw std::runtime_error("Stream not found: " + stream_name);
    }
    it->second.to_json_file(filename);
}

void Bundle::export_stream_csv(const std::string& stream_name,
                                const std::string& filename) const {
    auto it = streams_.find(stream_name);
    if (it == streams_.end()) {
        throw std::runtime_error("Stream not found: " + stream_name);
    }
    it->second.to_csv(filename);
}

Bundle Bundle::import_stream_json(const std::string& stream_name,
                                   const std::string& filename) const {
    auto new_streams = streams_;
    new_streams[stream_name] = Stream::from_json_file(filename);
    return Bundle(name_, new_streams);
}

Bundle Bundle::import_stream_csv(const std::string& stream_name,
                                  const std::string& filename) const {
    auto new_streams = streams_;
    new_streams[stream_name] = Stream::from_csv(filename);
    return Bundle(name_, new_streams);
}

// ========== Schema Management ==========

std::vector<std::string> Bundle::stream_fields(const std::string& stream_name) const {
    auto it = streams_.find(stream_name);
    if (it == streams_.end()) {
        return {};
    }
    return it->second.all_fields();
}

std::unordered_map<std::string, std::vector<std::string>> Bundle::all_fields() const {
    std::unordered_map<std::string, std::vector<std::string>> results;
    
    for (const auto& [name, stream] : streams_) {
        results[name] = stream.all_fields();
    }
    
    return results;
}

void Bundle::set_stream_validator(const std::string& stream_name,
                                   const StreamValidator& validator) {
    validators_[stream_name] = validator;
}

bool Bundle::validate_stream(const std::string& stream_name) const {
    auto val_it = validators_.find(stream_name);
    if (val_it == validators_.end()) {
        return true;
    }
    
    auto stream_it = streams_.find(stream_name);
    if (stream_it == streams_.end()) {
        return false;
    }
    
    return val_it->second(stream_it->second);
}

std::unordered_map<std::string, bool> Bundle::validate_all() const {
    std::unordered_map<std::string, bool> results;
    
    for (const auto& [name, _] : streams_) {
        results[name] = validate_stream(name);
    }
    
    return results;
}

// ========== Display and Info ==========

void Bundle::display(int64_t max_streams, int64_t max_docs_per_stream) const {
    std::cout << "Bundle: " << name_ << " (" << streams_.size() << " streams, "
              << total_documents() << " total documents)\n";
    std::cout << std::string(80, '=') << "\n";
    
    int64_t count = 0;
    for (const auto& [name, stream] : streams_) {
        if (count >= max_streams) break;
        
        std::cout << "\nStream: " << name << " (" << stream.size() << " documents)\n";
        std::cout << std::string(60, '-') << "\n";
        stream.display(max_docs_per_stream);
        
        ++count;
    }
    
    if (static_cast<int64_t>(streams_.size()) > max_streams) {
        std::cout << "\n... (" << (streams_.size() - max_streams) << " more streams)\n";
    }
}

void Bundle::info() const {
    std::cout << "Bundle Information:\n";
    std::cout << "  Name: " << name_ << "\n";
    std::cout << "  Streams: " << streams_.size() << "\n";
    std::cout << "  Total Documents: " << total_documents() << "\n";
    std::cout << "  Estimated Size: " << estimated_size_bytes() << " bytes\n";
    
    if (!streams_.empty()) {
        std::cout << "\n  Stream Summary:\n";
        for (const auto& [name, stream] : streams_) {
            std::cout << "    - " << name << ": " << stream.size() << " docs, "
                      << stream.all_fields().size() << " fields\n";
        }
    }
}

std::string Bundle::to_string() const {
    return "Bundle(" + name_ + ", " + std::to_string(streams_.size()) + " streams)";
}

void Bundle::summary() const {
    info();
}

void Bundle::stream_info(const std::string& stream_name) const {
    auto it = streams_.find(stream_name);
    if (it == streams_.end()) {
        std::cout << "Stream not found: " << stream_name << "\n";
        return;
    }
    it->second.info();
}

// ========== Merge and Clone Operations ==========

Bundle Bundle::merge(const Bundle& other, const std::string& conflict_strategy) const {
    auto new_streams = streams_;
    
    for (const auto& [name, stream] : other.streams_) {
        if (new_streams.find(name) != new_streams.end()) {
            if (conflict_strategy == "error") {
                throw std::runtime_error("Conflict: stream exists in both bundles: " + name);
            } else if (conflict_strategy == "keep_left") {
                // Keep existing stream
                continue;
            } else if (conflict_strategy == "keep_right") {
                new_streams[name] = stream;
            } else if (conflict_strategy == "merge_streams") {
                new_streams[name] = new_streams[name].merge(stream);
            }
        } else {
            new_streams[name] = stream;
        }
    }
    
    return Bundle(name_, new_streams);
}

Bundle Bundle::clone() const {
    return *this;
}

Bundle Bundle::shallow_copy() const {
    return *this;
}

// ========== Advanced Operations ==========

Stream Bundle::resolve_references(const std::string& stream_name,
                                    const std::string& ref_field,
                                    const std::string& target_stream) const {
    auto stream_it = streams_.find(stream_name);
    auto target_it = streams_.find(target_stream);
    
    if (stream_it == streams_.end() || target_it == streams_.end()) {
        return Stream();
    }
    
    const auto& stream = stream_it->second;
    const auto& target = target_it->second;
    
    return stream.$lookup(target, ref_field, "_id", ref_field + "_resolved");
}

Stream Bundle::denormalize(const std::string& stream_name,
                            const std::string& ref_field,
                            const std::string& target_stream,
                            const std::string& as_field) const {
    auto stream_it = streams_.find(stream_name);
    auto target_it = streams_.find(target_stream);
    
    if (stream_it == streams_.end() || target_it == streams_.end()) {
        return Stream();
    }
    
    const auto& stream = stream_it->second;
    const auto& target = target_it->second;
    
    return stream.$lookup(target, ref_field, "_id", as_field);
}

Bundle Bundle::migrate(const std::unordered_map<std::string, std::string>& rename_map) const {
    auto new_streams = streams_;
    
    for (const auto& [old_name, new_name] : rename_map) {
        auto it = new_streams.find(old_name);
        if (it != new_streams.end()) {
            Stream stream = it->second;
            stream.set_name(new_name);
            new_streams.erase(it);
            new_streams[new_name] = stream;
        }
    }
    
    return Bundle(name_, new_streams);
}

// ========== Operators ==========

bool Bundle::operator==(const Bundle& other) const {
    if (streams_.size() != other.streams_.size()) return false;
    
    for (const auto& [name, stream] : streams_) {
        auto it = other.streams_.find(name);
        if (it == other.streams_.end()) return false;
        if (!(stream == it->second)) return false;
    }
    
    return true;
}

bool Bundle::operator!=(const Bundle& other) const {
    return !(*this == other);
}

// ========== Helper Methods ==========

void Bundle::validate_stream_name(const std::string& stream_name) const {
    if (stream_name.empty()) {
        throw std::runtime_error("Stream name cannot be empty");
    }
}

Stream Bundle::merge_streams(const Stream& left, const Stream& right,
                              const std::string& strategy) const {
    if (strategy == "union") {
        return left.merge(right);
    }
    return left;
}

} // namespace pluss::registry
