#include <pluss/datalog/registry/memo/memo.h>
#include <pluss/SeriesBody.h>
#include <pluss/Note.h>
#include <pluss/Timeline.h>
#include <pluss/NoteOps.h>
#include <pluss/note_factory.h>
#include <pluss/series_factory.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <cctype>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <regex>

namespace pluss::registry {

// ========== Constructors ==========

Memo::Memo() : value_(std::monostate{}), field_name_("") {}

Memo::Memo(const Series& val) : value_(std::make_shared<Series>(val)), field_name_("") {}

Memo::Memo(const Note& val) : value_(std::make_shared<Note>(val)), field_name_("") {}

Memo::Memo(const Timeline& val) : value_(std::make_shared<Timeline>(val)), field_name_("") {}

Memo::Memo(const DocumentValue& val) 
    : value_(std::make_shared<DocumentValue>(val)), field_name_("") {}

Memo::Memo(const std::shared_ptr<DocumentValue>& val) 
    : value_(val), field_name_("") {}

// Static helper constructors
Memo Memo::from_scalar(double value) {
    return Memo(pluss::value({value}));
}

Memo Memo::from_scalar(int64_t value) {
    return Memo(pluss::value({static_cast<double>(value)}));
}

Memo Memo::from_scalar(bool value) {
    return Memo(pluss::value({value ? 1.0 : 0.0}));
}

Memo Memo::from_text(const std::string& value) {
    return Memo(pluss::make_note({pluss::Chord(value)}));
}

Memo Memo::from_text(const pluss::Chord& value) {
    return Memo(pluss::make_note({value}));
}

// ========== Type Checking ==========

bool Memo::is_null() const noexcept {
    return std::holds_alternative<std::monostate>(value_);
}

bool Memo::is_series() const noexcept {
    return std::holds_alternative<std::shared_ptr<Series>>(value_);
}

bool Memo::is_note() const noexcept {
    return std::holds_alternative<std::shared_ptr<Note>>(value_);
}

bool Memo::is_timeline() const noexcept {
    return std::holds_alternative<std::shared_ptr<Timeline>>(value_);
}

bool Memo::is_document() const noexcept {
    return std::holds_alternative<std::shared_ptr<DocumentValue>>(value_);
}

bool Memo::is_numeric() const noexcept {
    return is_series();
}

bool Memo::is_text() const noexcept {
    return is_note();
}

// ========== Type Extraction ==========

Series Memo::as_series() const {
    if (!is_series()) throw std::runtime_error("Memo: not a Series");
    return *std::get<std::shared_ptr<Series>>(value_);
}

Note Memo::as_note() const {
    if (!is_note()) throw std::runtime_error("Memo: not a Note");
    return *std::get<std::shared_ptr<Note>>(value_);
}

Timeline Memo::as_timeline() const {
    if (!is_timeline()) throw std::runtime_error("Memo: not a Timeline");
    return *std::get<std::shared_ptr<Timeline>>(value_);
}

DocumentValue Memo::as_document() const {
    if (!is_document()) throw std::runtime_error("Memo: not a document");
    return *std::get<std::shared_ptr<DocumentValue>>(value_);
}

// ========== Document Operations ==========

Memo Memo::operator[](const std::string& key) const {
    return get_field(key);
}

Memo Memo::get_field(const std::string& key) const {
    if (!is_document()) return Memo::null();
    const auto& doc = as_document();
    auto it = doc.find(key);
    if (it == doc.end()) return Memo::null();
    return it->second;
}

Memo Memo::get_nested(const std::string& path) const {
    // Support dot notation: "user.address.city"
    size_t pos = path.find('.');
    if (pos == std::string::npos) {
        return get_field(path);
    }
    
    std::string first = path.substr(0, pos);
    std::string rest = path.substr(pos + 1);
    Memo child = get_field(first);
    if (child.is_null()) return Memo::null();
    return child.get_nested(rest);
}

Memo Memo::set_field(const std::string& key, const Memo& value) const {
    DocumentValue doc;
    if (is_document()) {
        doc = as_document();
    }
    doc[key] = value;
    return Memo(doc);
}

Memo Memo::set_nested(const std::string& path, const Memo& value) const {
    size_t pos = path.find('.');
    if (pos == std::string::npos) {
        return set_field(path, value);
    }
    
    std::string first = path.substr(0, pos);
    std::string rest = path.substr(pos + 1);
    
    Memo child = get_field(first);
    if (!child.is_document()) {
        child = Memo::document({});
    }
    
    Memo updated_child = child.set_nested(rest, value);
    return set_field(first, updated_child);
}

Memo Memo::unset_field(const std::string& key) const {
    if (!is_document()) return *this;
    DocumentValue doc = as_document();
    doc.erase(key);
    return Memo(doc);
}

Memo Memo::rename_field(const std::string& old_key, const std::string& new_key) const {
    if (!is_document()) return *this;
    DocumentValue doc = as_document();
    auto it = doc.find(old_key);
    if (it == doc.end()) return *this;
    
    Memo value = it->second;
    doc.erase(old_key);
    doc[new_key] = value;
    return Memo(doc);
}

// ========== MongoDB Update Operators ==========

Memo Memo::$set(const std::string& field, const Memo& value) const {
    return set_field(field, value);
}

Memo Memo::$unset(const std::string& field) const {
    return unset_field(field);
}

Memo Memo::$rename(const std::string& old_field, const std::string& new_field) const {
    return rename_field(old_field, new_field);
}

Memo Memo::$inc(const std::string& field, double amount) const {
    if (!is_document()) return *this;
    Memo field_value = get_field(field);
    if (!field_value.is_numeric()) return *this;
    
    // Production: vectorized addition for Series
    auto series = field_value.as_series();
    auto amount_series = pluss::value({amount});
    auto result = series + amount_series;
    
    return set_field(field, Memo(result));
}

Memo Memo::$mul(const std::string& field, double factor) const {
    if (!is_document()) return *this;
    Memo field_value = get_field(field);
    if (!field_value.is_numeric()) return *this;
    
    // Production: vectorized multiplication for Series
    auto series = field_value.as_series();
    auto factor_series = pluss::value({factor});
    auto result = series * factor_series;
    
    return set_field(field, Memo(result));
}

Memo Memo::$min(const std::string& field, double value) const {
    if (!is_document()) return *this;
    Memo field_value = get_field(field);
    if (!field_value.is_numeric()) return *this;
    
    // Production: use Series.min() for comparison
    auto series = field_value.as_series();
    double current = series.min().item<double>();
    if (current <= value) return *this;
    
    return set_field(field, Memo::from_scalar(value));
}

Memo Memo::$max(const std::string& field, double value) const {
    if (!is_document()) return *this;
    Memo field_value = get_field(field);
    if (!field_value.is_numeric()) return *this;
    
    // Production: use Series.max() for comparison
    auto series = field_value.as_series();
    double current = series.max().item<double>();
    if (current >= value) return *this;
    
    return set_field(field, Memo::from_scalar(value));
}

Memo Memo::$push(const std::string& field, const Memo& value) const {
    if (!is_document()) return *this;
    Memo field_value = get_field(field);
    
    // Production: concatenate Series or Note using vectorized operations
    if (field_value.is_series() && value.is_numeric()) {
        auto existing = field_value.as_series();
        auto new_val = value.as_series();
        // Use torch::cat to concatenate tensors
        auto concatenated = pluss::from_torch(torch::cat({existing.data(), new_val.data()}, 0));
        return set_field(field, Memo(concatenated));
    }
    
    if (field_value.is_note() && value.is_text()) {
        auto existing = field_value.as_note();
        auto new_val = value.as_note();
        // Use Note's concat method
        auto concatenated = existing.concat(new_val);
        return set_field(field, Memo(concatenated));
    }
    
    // If field doesn't exist, set it to the value
    if (field_value.is_null()) {
        return set_field(field, value);
    }
    
    return *this;
}

Memo Memo::$pop(const std::string& field, int position) const {
    if (!is_document()) return *this;
    Memo field_value = get_field(field);
    
    if (field_value.is_series()) {
        auto series = field_value.as_series();
        auto sizes = series.sizes();
        if (sizes.size() == 0 || sizes[0] <= 1) return *this;  // Can't pop from empty or single element
        
        bool from_front = (position < 0);
        Series popped;
        
        if (from_front) {
            // Remove first: series[1:]
            popped = series.slice(0, 1, sizes[0]);
        } else {
            // Remove last: series[:-1]
            popped = series.slice(0, 0, sizes[0] - 1);
        }
        
        return set_field(field, Memo(popped));
    }
    
    // Note doesn't have efficient pop, would need to flatten, modify, and reconstruct
    return *this;
}

Memo Memo::$pull(const std::string& field, const Memo& value) const {
    if (!is_document()) return *this;
    Memo field_value = get_field(field);
    
    if (field_value.is_series() && value.is_numeric()) {
        auto series = field_value.as_series();
        auto val_series = value.as_series();
        double val = val_series[0].item<double>();
        
        // Production: vectorized masking - keep only elements not equal to val
        auto mask = series.ne(val);
        auto filtered = series.masked_select(mask);
        
        return set_field(field, Memo(filtered));
    }
    
    // Note filtering would require custom implementation with flatten/filter/reconstruct
    return *this;
}

// ========== MongoDB Query Operators ==========

bool Memo::$eq(const Memo& other) const {
    return *this == other;
}

bool Memo::$ne(const Memo& other) const {
    return !(*this == other);
}

bool Memo::$gt(const Memo& other) const {
    if (is_numeric() && other.is_numeric()) {
        auto s1 = as_series();
        auto s2 = other.as_series();
        // Production: use Series.gt() for vectorized comparison
        auto mask = s1.gt(s2);
        // For scalar comparison, check if any element is greater
        return mask.any().item<bool>();
    }
    if (is_text() && other.is_text()) {
        auto n1 = as_note().flatten();
        auto n2 = other.as_note().flatten();
        if (!n1.empty() && !n2.empty()) {
            return n1[0] > n2[0];
        }
    }
    return false;
}

bool Memo::$gte(const Memo& other) const {
    return $gt(other) || $eq(other);
}

bool Memo::$lt(const Memo& other) const {
    if (is_numeric() && other.is_numeric()) {
        auto s1 = as_series();
        auto s2 = other.as_series();
        // Production: use Series.lt() for vectorized comparison
        auto mask = s1.lt(s2);
        // For scalar comparison, check if any element is less
        return mask.any().item<bool>();
    }
    if (is_text() && other.is_text()) {
        auto n1 = as_note().flatten();
        auto n2 = other.as_note().flatten();
        if (!n1.empty() && !n2.empty()) {
            return n1[0] < n2[0];
        }
    }
    return false;
}

bool Memo::$lte(const Memo& other) const {
    return $lt(other) || $eq(other);
}

bool Memo::$in(const std::vector<Memo>& values) const {
    for (const auto& val : values) {
        if ($eq(val)) return true;
    }
    return false;
}

bool Memo::$nin(const std::vector<Memo>& values) const {
    return !$in(values);
}

bool Memo::$exists(const std::string& field) const {
    if (!is_document()) return false;
    return has_field(field);
}

bool Memo::$regex(const std::string& pattern) const {
    if (!is_text()) return false;
    // Production: extract strings from Note and apply regex
    auto flattened = as_note().flatten();
    if (flattened.empty()) return false;
    
    try {
        std::regex re(pattern);
        return std::regex_search(std::string(flattened[0]), re);
    } catch (...) {
        return false;
    }
}

// ========== Array Operations ==========

int64_t Memo::size() const {
    if (is_null()) return 0;
    if (is_series()) {
        auto sizes = as_series().sizes();
        return sizes.size() > 0 ? sizes[0] : 0;
    }
    if (is_note()) return as_note().shape()[0];
    if (is_timeline()) {
        // Timeline size check - using Series as fallback
        return 1;
    }
    if (is_document()) return static_cast<int64_t>(as_document().size());
    return 1;  // Scalar-like
}

Memo Memo::at(int64_t index) const {
    if (is_series()) {
        auto series = as_series();
        return Memo(series[index]);
    }
    // Note/Timeline indexing is more complex, skip for now  
    throw std::runtime_error("Memo: at() only supported for Series");
}

bool Memo::contains(const Memo& value) const {
    if (is_series() && value.is_numeric()) {
        auto series = as_series();
        auto val_series = value.as_series();
        // Production: vectorized membership test using eq() and any()
        auto mask = series.eq(val_series);
        return mask.any().item<bool>();
    }
    
    if (is_note() && value.is_text()) {
        auto note_vec = as_note().flatten();
        auto val_vec = value.as_note().flatten();
        if (!val_vec.empty()) {
            const auto& search_str = val_vec[0];
            return std::find(note_vec.begin(), note_vec.end(), search_str) != note_vec.end();
        }
    }
    
    return false;
}

// ========== Aggregations ==========

Memo Memo::sum() const {
    if (is_series()) {
        auto s = as_series();
        return Memo::from_scalar(s.sum().item<double>());
    }
    return Memo::from_scalar(0.0);
}

Memo Memo::mean() const {
    if (is_series()) {
        auto s = as_series();
        // Production: use Series.mean() directly
        return Memo::from_scalar(s.mean().item<double>());
    }
    return Memo::from_scalar(0.0);
}

Memo Memo::min() const {
    if (is_series()) {
        auto s = as_series();
        return Memo::from_scalar(s.min().item<double>());
    }
    return Memo::from_scalar(0.0);
}

Memo Memo::max() const {
    if (is_series()) {
        auto s = as_series();
        return Memo::from_scalar(s.max().item<double>());
    }
    return Memo::from_scalar(0.0);
}

Memo Memo::count() const {
    return Memo::from_scalar(static_cast<double>(size()));
}

// ========== String Operations ==========

Memo Memo::to_upper() const {
    if (!is_text()) throw std::runtime_error("to_upper requires text type");
    // Production: use Note.to_upper() method
    auto note = as_note();
    auto upper_note = note.to_upper();
    return Memo(upper_note);
}

Memo Memo::to_lower() const {
    if (!is_text()) throw std::runtime_error("to_lower requires text type");
    // Production: use Note.to_lower() method
    auto note = as_note();
    auto lower_note = note.to_lower();
    return Memo(lower_note);
}

Memo Memo::trim() const {
    if (!is_text()) throw std::runtime_error("trim requires text type");
    // Manual implementation since Note doesn't have trim()
    auto flattened = as_note().flatten();
    std::vector<Chord> trimmed_chords;
    
    for (const auto& str : flattened) {
        std::string s = str;
        // Trim leading whitespace
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        // Trim trailing whitespace
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
        trimmed_chords.push_back(Chord(s));
    }
    
    return Memo(pluss::make_note(trimmed_chords));
}

Memo Memo::concat(const Memo& other) const {
    if (!is_text() || !other.is_text()) throw std::runtime_error("concat requires text types");
    // Production: use Note.add() for concatenation
    auto note1 = as_note();
    auto note2 = other.as_note();
    auto concatenated = note1.add(note2);
    return Memo(concatenated);
}

Memo Memo::substr(int64_t start, int64_t length) const {
    if (!is_text()) throw std::runtime_error("substr requires text type");
    // Manual substring implementation
    auto flattened = as_note().flatten();
    std::vector<Chord> substr_chords;
    
    for (const auto& chord_str : flattened) {
        std::string str = chord_str;
        if (start < 0) start = 0;
        if (start >= static_cast<int64_t>(str.size())) {
            substr_chords.push_back(Chord(""));
            continue;
        }
        if (length < 0) length = str.size() - start;
        substr_chords.push_back(Chord(str.substr(start, length)));
    }
    
    return Memo(pluss::make_note(substr_chords));
}

bool Memo::starts_with(const std::string& prefix) const {
    if (!is_text()) return false;
    auto flattened = as_note().flatten();
    if (flattened.empty()) return false;
    std::string str = flattened[0];
    return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

bool Memo::ends_with(const std::string& suffix) const {
    if (!is_text()) return false;
    auto flattened = as_note().flatten();
    if (flattened.empty()) return false;
    std::string str = flattened[0];
    return str.size() >= suffix.size() && 
           str.substr(str.size() - suffix.size()) == suffix;
}

bool Memo::str_contains(const std::string& substr) const {
    if (!is_text()) return false;
    auto flattened = as_note().flatten();
    if (flattened.empty()) return false;
    return std::string(flattened[0]).find(substr) != std::string::npos;
}

// ========== Conversions ==========

Memo Memo::to_series() const {
    if (is_series()) return *this;
    if (is_numeric()) return *this;  // Already wrapped in Series
    throw std::runtime_error("Cannot convert to Series");
}

Memo Memo::to_note() const {
    if (is_note()) return *this;
    if (is_text()) return *this;  // Already wrapped in Note
    throw std::runtime_error("Cannot convert to Note");
}

Memo Memo::to_timeline() const {
    if (is_timeline()) return *this;
    throw std::runtime_error("Cannot convert to Timeline");
}

// ========== Serialization ==========

std::string Memo::to_json() const {
    std::ostringstream oss;
    
    if (is_null()) {
        oss << "null";
    } else if (is_series()) {
        // For single-element Series, output as scalar
        auto s = as_series();
        auto sizes = s.sizes();
        if (sizes.size() > 0 && sizes[0] == 1) {
            oss << std::setprecision(15) << s[0].item<double>();
        } else {
            // Array output
            oss << "[";
            int64_t n = sizes[0];
            for (int64_t i = 0; i < n && i < 100; ++i) {  // Limit to 100 elements
                if (i > 0) oss << ",";
                oss << s[i].item<double>();
            }
            if (n > 100) oss << ",...";
            oss << "]";
        }
    } else if (is_note()) {
        auto note = as_note();
        auto flattened = note.flatten();
        if (flattened.size() == 1) {
            oss << "\"" << flattened[0] << "\"";
        } else {
            oss << "[";
            for (size_t i = 0; i < flattened.size() && i < 100; ++i) {
                if (i > 0) oss << ",";
                oss << "\"" << flattened[i] << "\"";
            }
            if (flattened.size() > 100) oss << ",...";
            oss << "]";
        }
    } else if (is_document()) {
        oss << "{";
        const auto& doc = as_document();
        bool first = true;
        for (const auto& [key, val] : doc) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":" << val.to_json();
            first = false;
        }
        oss << "}";
    } else {
        oss << "null";
    }
    
    return oss.str();
}

std::string Memo::to_string() const {
    if (is_text()) {
        auto flattened = as_note().flatten();
        if (!flattened.empty()) return flattened[0];
    }
    return to_json();
}

void Memo::display() const {
    std::cout << to_json() << std::endl;
}

void Memo::info() const {
    std::cout << "Memo: " << (field_name_.empty() ? "<unnamed>" : field_name_) << "\n";
    std::cout << "Type: ";
    if (is_null()) std::cout << "null";
    else if (is_series()) std::cout << "Series[" << size() << "]";
    else if (is_note()) std::cout << "Note[" << size() << "]";
    else if (is_timeline()) std::cout << "Timeline";
    else if (is_document()) std::cout << "document{" << size() << " fields}";
    std::cout << "\n";
    
    if (!is_null() && size() < 100) {
        std::cout << "Value: " << to_json() << "\n";
    }
}

// ========== Static Factories ==========

Memo Memo::null() {
    return Memo();
}

Memo Memo::array(const std::vector<Memo>& elements) {
    // Production: convert to Series by extracting scalar values
    std::vector<double> vec;
    vec.reserve(elements.size());
    
    for (const auto& elem : elements) {
        if (elem.is_numeric()) {
            // Extract scalar from Series
            auto series = elem.as_series();
            vec.push_back(series[0].item<double>());
        } else {
            throw std::runtime_error("array() requires all numeric elements");
        }
    }
    
    return Memo(pluss::value(vec));
}

Memo Memo::document(const DocumentValue& doc) {
    return Memo(doc);
}

// ========== Operators ==========

bool Memo::operator==(const Memo& other) const {
    if (value_.index() != other.value_.index()) return false;
    
    if (is_null()) return true;
    
    if (is_series() && other.is_series()) {
        // Compare Series element-wise
        auto s1 = as_series();
        auto s2 = other.as_series();
        if (s1.sizes() != s2.sizes()) return false;
        auto eq_mask = s1.eq(s2);
        return eq_mask.all().item<bool>();
    }
    
    if (is_note() && other.is_note()) {
        auto n1 = as_note().flatten();
        auto n2 = other.as_note().flatten();
        return n1 == n2;
    }
    
    if (is_document()) {
        const auto& doc1 = as_document();
        const auto& doc2 = other.as_document();
        if (doc1.size() != doc2.size()) return false;
        for (const auto& [key, val] : doc1) {
            auto it = doc2.find(key);
            if (it == doc2.end() || it->second != val) return false;
        }
        return true;
    }
    
    return false;
}

bool Memo::operator!=(const Memo& other) const {
    return !(*this == other);
}

bool Memo::operator<(const Memo& other) const {
    return $lt(other);
}

bool Memo::operator<=(const Memo& other) const {
    return $lte(other);
}

bool Memo::operator>(const Memo& other) const {
    return $gt(other);
}

bool Memo::operator>=(const Memo& other) const {
    return $gte(other);
}

// ========== Additional Helper Methods ==========

bool Memo::has_field(const std::string& field) const {
    if (!is_document()) return false;
    const auto& doc = as_document();
    return doc.find(field) != doc.end();
}

} // namespace pluss::registry