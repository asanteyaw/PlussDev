#include <pluss/datalog/registry/memo/memo.h>
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

Memo::Memo() : value_(std::monostate{}), name_("") {}

Memo::Memo(const std::string& name) : value_(std::monostate{}), name_(name) {}

Memo::Memo(bool val) : value_(val), name_("") {}

Memo::Memo(int32_t val) : value_(static_cast<int64_t>(val)), name_("") {}

Memo::Memo(int64_t val) : value_(val), name_("") {}

Memo::Memo(double val) : value_(val), name_("") {}

Memo::Memo(const char* val) : value_(std::string(val)), name_("") {}

Memo::Memo(const std::string& val) : value_(val), name_("") {}

Memo::Memo(const Chord& val) : value_(val), name_("") {}

Memo::Memo(const std::vector<int64_t>& val) : value_(val), name_("") {}

Memo::Memo(const std::vector<double>& val) : value_(val), name_("") {}

Memo::Memo(const std::vector<std::string>& val) : value_(val), name_("") {}

Memo::Memo(const std::vector<bool>& val) : value_(val), name_("") {}

Memo::Memo(const Series& val) : value_(val), name_("") {}

Memo::Memo(const Note& val) : value_(val), name_("") {}

Memo::Memo(const DocumentValue& val) 
    : value_(std::make_shared<DocumentValue>(val)), name_("") {}

// Named field constructors
Memo::Memo(const std::string& name, const MemoValue& val) : value_(val), name_(name) {}

// ========== Type Checking ==========

bool Memo::is_scalar() const noexcept {
    return std::holds_alternative<std::monostate>(value_) ||
           std::holds_alternative<bool>(value_) ||
           std::holds_alternative<int64_t>(value_) ||
           std::holds_alternative<double>(value_) ||
           std::holds_alternative<std::string>(value_) ||
           std::holds_alternative<pluss::Chord>(value_);
}

bool Memo::is_array() const noexcept {
    return std::holds_alternative<std::vector<int64_t>>(value_) ||
           std::holds_alternative<std::vector<double>>(value_) ||
           std::holds_alternative<std::vector<std::string>>(value_) ||
           std::holds_alternative<std::vector<bool>>(value_) ||
           std::holds_alternative<pluss::Series>(value_) ||
           std::holds_alternative<pluss::Note>(value_);
}

// ========== Type Checking ==========

bool Memo::is_null() const noexcept {
    return std::holds_alternative<std::monostate>(value_);
}

bool Memo::is_bool() const noexcept {
    return std::holds_alternative<bool>(value_);
}

bool Memo::is_int() const noexcept {
    return std::holds_alternative<int64_t>(value_);
}

bool Memo::is_double() const noexcept {
    return std::holds_alternative<double>(value_);
}

bool Memo::is_string() const noexcept {
    return std::holds_alternative<std::string>(value_);
}

bool Memo::is_chord() const noexcept {
    return std::holds_alternative<Chord>(value_);
}

bool Memo::is_scalar() const noexcept {
    return is_null() || is_bool() || is_int() || is_double() || is_string() || is_chord();
}

bool Memo::is_array() const noexcept {
    return std::holds_alternative<std::vector<int64_t>>(value_) ||
           std::holds_alternative<std::vector<double>>(value_) ||
           std::holds_alternative<std::vector<std::string>>(value_) ||
           std::holds_alternative<std::vector<bool>>(value_);
}

bool Memo::is_document() const noexcept {
    return std::holds_alternative<std::shared_ptr<DocumentValue>>(value_);
}

bool Memo::is_series() const noexcept {
    return std::holds_alternative<Series>(value_);
}

bool Memo::is_note() const noexcept {
    return std::holds_alternative<Note>(value_);
}

bool Memo::is_numeric() const noexcept {
    return is_int() || is_double();
}

bool Memo::is_text() const noexcept {
    return is_string() || is_chord();
}

// ========== Type Extraction ==========

bool Memo::as_bool() const {
    if (is_bool()) return std::get<bool>(value_);
    if (is_int()) return std::get<int64_t>(value_) != 0;
    if (is_double()) return std::get<double>(value_) != 0.0;
    throw std::runtime_error("Memo: cannot convert to bool");
}

int64_t Memo::as_int() const {
    if (is_int()) return std::get<int64_t>(value_);
    if (is_double()) return static_cast<int64_t>(std::get<double>(value_));
    if (is_bool()) return std::get<bool>(value_) ? 1 : 0;
    throw std::runtime_error("Memo: cannot convert to int");
}

double Memo::as_double() const {
    if (is_double()) return std::get<double>(value_);
    if (is_int()) return static_cast<double>(std::get<int64_t>(value_));
    if (is_bool()) return std::get<bool>(value_) ? 1.0 : 0.0;
    throw std::runtime_error("Memo: cannot convert to double");
}

std::string Memo::as_string() const {
    if (is_string()) return std::get<std::string>(value_);
    if (is_chord()) return std::get<Chord>(value_).str();
    if (is_int()) return std::to_string(std::get<int64_t>(value_));
    if (is_double()) return std::to_string(std::get<double>(value_));
    if (is_bool()) return std::get<bool>(value_) ? "true" : "false";
    if (is_null()) return "null";
    throw std::runtime_error("Memo: cannot convert to string");
}

const Series& Memo::as_series() const {
    if (!is_series()) throw std::runtime_error("Memo: not a Series");
    return std::get<Series>(value_);
}

const Note& Memo::as_note() const {
    if (!is_note()) throw std::runtime_error("Memo: not a Note");
    return std::get<Note>(value_);
}

const Memo::DocumentValue& Memo::as_document() const {
    if (!is_document()) throw std::runtime_error("Memo: not a document");
    return *std::get<std::shared_ptr<DocumentValue>>(value_);
}

// Safe get methods with defaults
bool Memo::get_bool(bool default_val) const noexcept {
    try { return as_bool(); } catch (...) { return default_val; }
}

int64_t Memo::get_int(int64_t default_val) const noexcept {
    try { return as_int(); } catch (...) { return default_val; }
}

double Memo::get_double(double default_val) const noexcept {
    try { return as_double(); } catch (...) { return default_val; }
}

std::string Memo::get_string(const std::string& default_val) const noexcept {
    try { return as_string(); } catch (...) { return default_val; }
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
        child = Memo::document();
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

Memo Memo::$inc(double amount) const {
    if (!is_numeric()) throw std::runtime_error("$inc requires numeric type");
    if (is_int()) return Memo(as_int() + static_cast<int64_t>(amount));
    return Memo(as_double() + amount);
}

Memo Memo::$mul(double factor) const {
    if (!is_numeric()) throw std::runtime_error("$mul requires numeric type");
    if (is_int()) return Memo(as_int() * static_cast<int64_t>(factor));
    return Memo(as_double() * factor);
}

Memo Memo::$min(const Memo& value) const {
    if (!is_numeric() || !value.is_numeric()) return *this;
    double a = as_double();
    double b = value.as_double();
    return Memo(std::min(a, b));
}

Memo Memo::$max(const Memo& value) const {
    if (!is_numeric() || !value.is_numeric()) return *this;
    double a = as_double();
    double b = value.as_double();
    return Memo(std::max(a, b));
}

Memo Memo::$push(const Memo& value) const {
    if (is_array()) {
        if (std::holds_alternative<std::vector<int64_t>>(value_) && value.is_int()) {
            auto vec = std::get<std::vector<int64_t>>(value_);
            vec.push_back(value.as_int());
            return Memo(vec);
        }
        if (std::holds_alternative<std::vector<double>>(value_) && value.is_numeric()) {
            auto vec = std::get<std::vector<double>>(value_);
            vec.push_back(value.as_double());
            return Memo(vec);
        }
        if (std::holds_alternative<std::vector<std::string>>(value_) && value.is_text()) {
            auto vec = std::get<std::vector<std::string>>(value_);
            vec.push_back(value.as_string());
            return Memo(vec);
        }
    }
    // If not array, create new array with value
    if (value.is_int()) return Memo(std::vector<int64_t>{value.as_int()});
    if (value.is_double()) return Memo(std::vector<double>{value.as_double()});
    if (value.is_text()) return Memo(std::vector<std::string>{value.as_string()});
    return *this;
}

Memo Memo::$pop(bool from_front) const {
    if (!is_array()) throw std::runtime_error("$pop requires array type");
    
    if (std::holds_alternative<std::vector<int64_t>>(value_)) {
        auto vec = std::get<std::vector<int64_t>>(value_);
        if (vec.empty()) return *this;
        if (from_front) vec.erase(vec.begin());
        else vec.pop_back();
        return Memo(vec);
    }
    if (std::holds_alternative<std::vector<double>>(value_)) {
        auto vec = std::get<std::vector<double>>(value_);
        if (vec.empty()) return *this;
        if (from_front) vec.erase(vec.begin());
        else vec.pop_back();
        return Memo(vec);
    }
    if (std::holds_alternative<std::vector<std::string>>(value_)) {
        auto vec = std::get<std::vector<std::string>>(value_);
        if (vec.empty()) return *this;
        if (from_front) vec.erase(vec.begin());
        else vec.pop_back();
        return Memo(vec);
    }
    return *this;
}

Memo Memo::$pull(const Memo& value) const {
    if (!is_array()) throw std::runtime_error("$pull requires array type");
    
    if (std::holds_alternative<std::vector<int64_t>>(value_) && value.is_int()) {
        auto vec = std::get<std::vector<int64_t>>(value_);
        int64_t val = value.as_int();
        vec.erase(std::remove(vec.begin(), vec.end(), val), vec.end());
        return Memo(vec);
    }
    if (std::holds_alternative<std::vector<double>>(value_) && value.is_numeric()) {
        auto vec = std::get<std::vector<double>>(value_);
        double val = value.as_double();
        vec.erase(std::remove(vec.begin(), vec.end(), val), vec.end());
        return Memo(vec);
    }
    if (std::holds_alternative<std::vector<std::string>>(value_) && value.is_text()) {
        auto vec = std::get<std::vector<std::string>>(value_);
        std::string val = value.as_string();
        vec.erase(std::remove(vec.begin(), vec.end(), val), vec.end());
        return Memo(vec);
    }
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
        return as_double() > other.as_double();
    }
    if (is_text() && other.is_text()) {
        return as_string() > other.as_string();
    }
    return false;
}

bool Memo::$gte(const Memo& other) const {
    return $gt(other) || $eq(other);
}

bool Memo::$lt(const Memo& other) const {
    if (is_numeric() && other.is_numeric()) {
        return as_double() < other.as_double();
    }
    if (is_text() && other.is_text()) {
        return as_string() < other.as_string();
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

bool Memo::$exists() const {
    return !is_null();
}

bool Memo::$regex(const std::string& pattern) const {
    if (!is_text()) return false;
    try {
        std::regex re(pattern);
        return std::regex_search(as_string(), re);
    } catch (...) {
        return false;
    }
}

// ========== Array Operations ==========

int64_t Memo::size() const {
    if (is_scalar()) return 1;
    if (std::holds_alternative<std::vector<int64_t>>(value_))
        return static_cast<int64_t>(std::get<std::vector<int64_t>>(value_).size());
    if (std::holds_alternative<std::vector<double>>(value_))
        return static_cast<int64_t>(std::get<std::vector<double>>(value_).size());
    if (std::holds_alternative<std::vector<std::string>>(value_))
        return static_cast<int64_t>(std::get<std::vector<std::string>>(value_).size());
    if (std::holds_alternative<std::vector<bool>>(value_))
        return static_cast<int64_t>(std::get<std::vector<bool>>(value_).size());
    if (is_series()) return as_series().numel();
    if (is_note()) return as_note().shape()[0];
    if (is_document()) return static_cast<int64_t>(as_document().size());
    return 0;
}

Memo Memo::at(int64_t index) const {
    if (std::holds_alternative<std::vector<int64_t>>(value_)) {
        const auto& vec = std::get<std::vector<int64_t>>(value_);
        if (index < 0) index += vec.size();
        if (index < 0 || index >= static_cast<int64_t>(vec.size())) 
            throw std::out_of_range("Memo: index out of range");
        return Memo(vec[index]);
    }
    if (std::holds_alternative<std::vector<double>>(value_)) {
        const auto& vec = std::get<std::vector<double>>(value_);
        if (index < 0) index += vec.size();
        if (index < 0 || index >= static_cast<int64_t>(vec.size())) 
            throw std::out_of_range("Memo: index out of range");
        return Memo(vec[index]);
    }
    if (std::holds_alternative<std::vector<std::string>>(value_)) {
        const auto& vec = std::get<std::vector<std::string>>(value_);
        if (index < 0) index += vec.size();
        if (index < 0 || index >= static_cast<int64_t>(vec.size())) 
            throw std::out_of_range("Memo: index out of range");
        return Memo(vec[index]);
    }
    throw std::runtime_error("Memo: at() requires array type");
}

bool Memo::contains(const Memo& value) const {
    if (!is_array()) return false;
    
    if (std::holds_alternative<std::vector<int64_t>>(value_) && value.is_int()) {
        const auto& vec = std::get<std::vector<int64_t>>(value_);
        return std::find(vec.begin(), vec.end(), value.as_int()) != vec.end();
    }
    if (std::holds_alternative<std::vector<double>>(value_) && value.is_numeric()) {
        const auto& vec = std::get<std::vector<double>>(value_);
        return std::find(vec.begin(), vec.end(), value.as_double()) != vec.end();
    }
    if (std::holds_alternative<std::vector<std::string>>(value_) && value.is_text()) {
        const auto& vec = std::get<std::vector<std::string>>(value_);
        return std::find(vec.begin(), vec.end(), value.as_string()) != vec.end();
    }
    return false;
}

// ========== Aggregations ==========

double Memo::sum() const {
    if (is_numeric()) return as_double();
    
    if (std::holds_alternative<std::vector<int64_t>>(value_)) {
        const auto& vec = std::get<std::vector<int64_t>>(value_);
        return std::accumulate(vec.begin(), vec.end(), 0.0);
    }
    if (std::holds_alternative<std::vector<double>>(value_)) {
        const auto& vec = std::get<std::vector<double>>(value_);
        return std::accumulate(vec.begin(), vec.end(), 0.0);
    }
    if (is_series()) return as_series().sum();
    return 0.0;
}

double Memo::mean() const {
    int64_t n = size();
    if (n == 0) return 0.0;
    return sum() / static_cast<double>(n);
}

double Memo::min() const {
    if (is_numeric()) return as_double();
    
    if (std::holds_alternative<std::vector<int64_t>>(value_)) {
        const auto& vec = std::get<std::vector<int64_t>>(value_);
        if (vec.empty()) return 0.0;
        return static_cast<double>(*std::min_element(vec.begin(), vec.end()));
    }
    if (std::holds_alternative<std::vector<double>>(value_)) {
        const auto& vec = std::get<std::vector<double>>(value_);
        if (vec.empty()) return 0.0;
        return *std::min_element(vec.begin(), vec.end());
    }
    if (is_series()) return as_series().min();
    return 0.0;
}

double Memo::max() const {
    if (is_numeric()) return as_double();
    
    if (std::holds_alternative<std::vector<int64_t>>(value_)) {
        const auto& vec = std::get<std::vector<int64_t>>(value_);
        if (vec.empty()) return 0.0;
        return static_cast<double>(*std::max_element(vec.begin(), vec.end()));
    }
    if (std::holds_alternative<std::vector<double>>(value_)) {
        const auto& vec = std::get<std::vector<double>>(value_);
        if (vec.empty()) return 0.0;
        return *std::max_element(vec.begin(), vec.end());
    }
    if (is_series()) return as_series().max();
    return 0.0;
}

int64_t Memo::count() const {
    return size();
}

// ========== String Operations ==========

Memo Memo::to_upper() const {
    if (!is_text()) throw std::runtime_error("to_upper requires text type");
    std::string str = as_string();
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return Memo(str);
}

Memo Memo::to_lower() const {
    if (!is_text()) throw std::runtime_error("to_lower requires text type");
    std::string str = as_string();
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return Memo(str);
}

Memo Memo::trim() const {
    if (!is_text()) throw std::runtime_error("trim requires text type");
    std::string str = as_string();
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), str.end());
    return Memo(str);
}

Memo Memo::concat(const Memo& other) const {
    if (!is_text() || !other.is_text()) throw std::runtime_error("concat requires text types");
    return Memo(as_string() + other.as_string());
}

Memo Memo::substr(int64_t start, int64_t length) const {
    if (!is_text()) throw std::runtime_error("substr requires text type");
    std::string str = as_string();
    if (start < 0) start = 0;
    if (start >= static_cast<int64_t>(str.size())) return Memo("");
    if (length < 0) length = str.size() - start;
    return Memo(str.substr(start, length));
}

bool Memo::starts_with(const std::string& prefix) const {
    if (!is_text()) return false;
    std::string str = as_string();
    return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

bool Memo::ends_with(const std::string& suffix) const {
    if (!is_text()) return false;
    std::string str = as_string();
    return str.size() >= suffix.size() && 
           str.substr(str.size() - suffix.size()) == suffix;
}

bool Memo::str_contains(const std::string& substr) const {
    if (!is_text()) return false;
    return as_string().find(substr) != std::string::npos;
}

// ========== Conversions ==========

Memo Memo::to_int() const {
    return Memo(as_int());
}

Memo Memo::to_double() const {
    return Memo(as_double());
}

Memo Memo::to_string_memo() const {
    return Memo(as_string());
}

Memo Memo::to_bool_memo() const {
    return Memo(as_bool());
}

// ========== Serialization ==========

std::string Memo::to_json() const {
    std::ostringstream oss;
    
    if (is_null()) {
        oss << "null";
    } else if (is_bool()) {
        oss << (as_bool() ? "true" : "false");
    } else if (is_int()) {
        oss << as_int();
    } else if (is_double()) {
        oss << std::setprecision(15) << as_double();
    } else if (is_string()) {
        oss << "\"" << as_string() << "\"";  // TODO: escape special chars
    } else if (is_array()) {
        oss << "[";
        int64_t n = size();
        for (int64_t i = 0; i < n; ++i) {
            if (i > 0) oss << ",";
            oss << at(i).to_json();
        }
        oss << "]";
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
    if (is_text()) return as_string();
    return to_json();
}

void Memo::display() const {
    std::cout << to_json() << std::endl;
}

void Memo::info() const {
    std::cout << "Memo: " << (name_.empty() ? "<unnamed>" : name_) << "\n";
    std::cout << "Type: ";
    if (is_null()) std::cout << "null";
    else if (is_bool()) std::cout << "bool";
    else if (is_int()) std::cout << "int64";
    else if (is_double()) std::cout << "double";
    else if (is_string()) std::cout << "string";
    else if (is_chord()) std::cout << "Chord";
    else if (is_array()) std::cout << "array[" << size() << "]";
    else if (is_series()) std::cout << "Series[" << size() << "]";
    else if (is_note()) std::cout << "Note[" << size() << "]";
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

Memo Memo::array() {
    return Memo(std::vector<double>{});
}

Memo Memo::document() {
    return Memo(DocumentValue{});
}

// ========== Metadata ==========

const std::string& Memo::field_name() const noexcept {
    return name_;
}

void Memo::set_field_name(const std::string& name) {
    name_ = name;
}

// ========== Operators ==========

bool Memo::operator==(const Memo& other) const {
    if (value_.index() != other.value_.index()) return false;
    
    if (is_null()) return true;
    if (is_bool()) return as_bool() == other.as_bool();
    if (is_int()) return as_int() == other.as_int();
    if (is_double()) return std::abs(as_double() - other.as_double()) < 1e-10;
    if (is_string()) return as_string() == other.as_string();
    if (is_chord()) return std::get<Chord>(value_).str() == std::get<Chord>(other.value_).str();
    
    // Arrays and documents require deeper comparison
    if (is_array()) {
        if (size() != other.size()) return false;
        for (int64_t i = 0; i < size(); ++i) {
            if (at(i) != other.at(i)) return false;
        }
        return true;
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

} // namespace pluss::registry

        return static_cast<int64_t>(std::get<std::vector<int64_t>>(value_).size());
    }
    if (std::holds_alternative<std::vector<double>>(value_)) {
        return static_cast<int64_t>(std::get<std::vector<double>>(value_).size());
    }
    if (std::holds_alternative<std::vector<std::string>>(value_)) {
        return static_cast<int64_t>(std::get<std::vector<std::string>>(value_).size());
    }
    if (std::holds_alternative<std::vector<bool>>(value_)) {
        return static_cast<int64_t>(std::get<std::vector<bool>>(value_).size());
    }
    
    if (is_document()) {
        return static_cast<int64_t>(as_document().size());
    }
    
    return 0;
}

bool Memo::empty() const {
    return size() == 0;
}

// ========== Helper Methods ==========

bool Memo::is_numeric() const {
    return is_int() || is_double();
}

double Memo::to_numeric() const {
    if (is_double()) return as_double();
    if (is_int()) return static_cast<double>(as_int());
    throw std::runtime_error("Memo is not numeric");
}

// ========== MemoABLE OPERATIONS (MongoDB-style) ==========

Memo Memo::set(const Value& new_value) const {
    return Memo(new_value, name_);
}

Memo Memo::inc(double amount) const {
    if (!is_numeric()) {
        throw std::runtime_error("$inc requires numeric field");
    }
    double current = to_numeric();
    return Memo(current + amount, name_);
}

Memo Memo::mul(double factor) const {
    if (!is_numeric()) {
        throw std::runtime_error("$mul requires numeric field");
    }
    double current = to_numeric();
    return Memo(current * factor, name_);
}

Memo Memo::min(const Value& value) const {
    // $min: update if value is less than current
    if (!is_numeric()) return *this;
    
    double current_val = to_numeric();
    double new_val = current_val;
    
    if (std::holds_alternative<ScalarValue>(value)) {
        const auto& scalar = std::get<ScalarValue>(value);
        if (std::holds_alternative<double>(scalar)) {
            new_val = std::get<double>(scalar);
        } else if (std::holds_alternative<int64_t>(scalar)) {
            new_val = static_cast<double>(std::get<int64_t>(scalar));
        }
    }
    
    if (new_val < current_val) {
        return Memo(new_val, name_);
    }
    return *this;
}

Memo Memo::max(const Value& value) const {
    // $max: update if value is greater than current
    if (!is_numeric()) return *this;
    
    double current_val = to_numeric();
    double new_val = current_val;
    
    if (std::holds_alternative<ScalarValue>(value)) {
        const auto& scalar = std::get<ScalarValue>(value);
        if (std::holds_alternative<double>(scalar)) {
            new_val = std::get<double>(scalar);
        } else if (std::holds_alternative<int64_t>(scalar)) {
            new_val = static_cast<double>(std::get<int64_t>(scalar));
        }
    }
    
    if (new_val > current_val) {
        return Memo(new_val, name_);
    }
    return *this;
}

Memo Memo::rename(const std::string& new_name) const {
    Memo result = *this;
    result.set_name(new_name);
    return result;
}

// ========== Array Operations ==========

Memo Memo::push(const ScalarValue& value) const {
    if (!is_array()) {
        throw std::runtime_error("$push requires array field");
    }
    
    // Handle std::vector types directly from value_
    if (std::holds_alternative<std::vector<int64_t>>(value_)) {
        auto vec = std::get<std::vector<int64_t>>(value_);
        if (std::holds_alternative<int64_t>(value)) {
            vec.push_back(std::get<int64_t>(value));
        }
        return Memo(vec, name_);
    }
    
    if (std::holds_alternative<std::vector<double>>(value_)) {
        auto vec = std::get<std::vector<double>>(value_);
        if (std::holds_alternative<double>(value)) {
            vec.push_back(std::get<double>(value));
        } else if (std::holds_alternative<int64_t>(value)) {
            vec.push_back(static_cast<double>(std::get<int64_t>(value)));
        }
        return Memo(vec, name_);
    }
    
    if (std::holds_alternative<std::vector<std::string>>(value_)) {
        auto vec = std::get<std::vector<std::string>>(value_);
        if (std::holds_alternative<std::string>(value)) {
            vec.push_back(std::get<std::string>(value));
        }
        return Memo(vec, name_);
    }
    
    if (std::holds_alternative<std::vector<bool>>(value_)) {
        auto vec = std::get<std::vector<bool>>(value_);
        if (std::holds_alternative<bool>(value)) {
            vec.push_back(std::get<bool>(value));
        }
        return Memo(vec, name_);
    }
    
    // For Series, use concat (vectorized)
    if (is_series()) {
        const auto& s = as_series();
        double val_double = 0.0;
        if (std::holds_alternative<double>(value)) {
            val_double = std::get<double>(value);
        } else if (std::holds_alternative<int64_t>(value)) {
            val_double = static_cast<double>(std::get<int64_t>(value));
        }
        // Concat Series with single element
        auto new_elem = pluss::full({1}, val_double);
        auto result = pluss::from_torch(torch::cat({s.data(), new_elem.data()}, 0));
        return Memo(result, name_);
    }
    
    if (is_note()) {
        auto vec = as_note().vector();
        if (std::holds_alternative<std::string>(value)) {
            vec.emplace_back(std::get<std::string>(value));
        } else if (std::holds_alternative<pluss::Chord>(value)) {
            vec.push_back(std::get<pluss::Chord>(value));
        }
        return Memo(pluss::make_note(vec), name_);
    }
    
    return *this;
}

Memo Memo::push_each(const std::vector<ScalarValue>& values) const {
    Memo result = *this;
    for (const auto& val : values) {
        result = result.push(val);
    }
    return result;
}

Memo Memo::pop(int direction) const {
    if (!is_array()) {
        throw std::runtime_error("$pop requires array field");
    }
    
    const auto sz = size();
    if (sz == 0) return *this;
    
    if (is_series()) {
        const auto& s = as_series();
        if (direction < 0) {
            // Remove first element
            return Memo(s.narrow(0, 1, sz - 1), name_);
        } else {
            // Remove last element
            return Memo(s.narrow(0, 0, sz - 1), name_);
        }
    }
    
    if (is_note()) {
        auto vec = as_note().vector();
        if (direction < 0) {
            vec.erase(vec.begin());
        } else {
            vec.pop_back();
        }
        return Memo(pluss::make_note(vec), name_);
    }
    
    auto& arr = as_array();
    
    if (std::holds_alternative<std::vector<int64_t>>(arr)) {
        auto vec = std::get<std::vector<int64_t>>(arr);
        if (direction < 0) {
            vec.erase(vec.begin());
        } else {
            vec.pop_back();
        }
        return Memo(vec, name_);
    }
    
    if (std::holds_alternative<std::vector<double>>(arr)) {
        auto vec = std::get<std::vector<double>>(arr);
        if (direction < 0) {
            vec.erase(vec.begin());
        } else {
            vec.pop_back();
        }
        return Memo(vec, name_);
    }
    
    if (std::holds_alternative<std::vector<std::string>>(arr)) {
        auto vec = std::get<std::vector<std::string>>(arr);
        if (direction < 0) {
            vec.erase(vec.begin());
        } else {
            vec.pop_back();
        }
        return Memo(vec, name_);
    }
    
    if (std::holds_alternative<std::vector<bool>>(arr)) {
        auto vec = std::get<std::vector<bool>>(arr);
        if (direction < 0) {
            vec.erase(vec.begin());
        } else {
            vec.pop_back();
        }
        return Memo(vec, name_);
    }
    
    return *this;
}

Memo Memo::pull(const ScalarValue& value) const {
    if (!is_array()) {
        throw std::runtime_error("$pull requires array field");
    }
    
    if (is_series()) {
        const auto& s = as_series();
        double target = 0.0;
        if (std::holds_alternative<double>(value)) {
            target = std::get<double>(value);
        } else if (std::holds_alternative<int64_t>(value)) {
            target = static_cast<double>(std::get<int64_t>(value));
        }
        
        // Vectorized comparison: keep elements that don't match
        auto mask = s.ne(target);
        return Memo(s.masked_select(mask), name_);
    }
    
    if (is_note()) {
        auto vec = as_note().vector();
        std::string target;
        if (std::holds_alternative<std::string>(value)) {
            target = std::get<std::string>(value);
        } else if (std::holds_alternative<pluss::Chord>(value)) {
            target = std::get<pluss::Chord>(value).str();
        }
        
        std::vector<pluss::Chord> result;
        result.reserve(vec.size());
        for (const auto& chord : vec) {
            if (chord.str() != target) {
                result.push_back(chord);
            }
        }
        return Memo(pluss::make_note(result), name_);
    }
    
    auto& arr = as_array();
    
    if (std::holds_alternative<std::vector<int64_t>>(arr)) {
        auto vec = std::get<std::vector<int64_t>>(arr);
        if (std::holds_alternative<int64_t>(value)) {
            int64_t target = std::get<int64_t>(value);
            vec.erase(std::remove(vec.begin(), vec.end(), target), vec.end());
        }
        return Memo(vec, name_);
    }
    
    if (std::holds_alternative<std::vector<double>>(arr)) {
        auto vec = std::get<std::vector<double>>(arr);
        double target = 0.0;
        if (std::holds_alternative<double>(value)) {
            target = std::get<double>(value);
        } else if (std::holds_alternative<int64_t>(value)) {
            target = static_cast<double>(std::get<int64_t>(value));
        }
        vec.erase(std::remove(vec.begin(), vec.end(), target), vec.end());
        return Memo(vec, name_);
    }
    
    if (std::holds_alternative<std::vector<std::string>>(arr)) {
        auto vec = std::get<std::vector<std::string>>(arr);
        if (std::holds_alternative<std::string>(value)) {
            std::string target = std::get<std::string>(value);
            vec.erase(std::remove(vec.begin(), vec.end(), target), vec.end());
        }
        return Memo(vec, name_);
    }
    
    if (std::holds_alternative<std::vector<bool>>(arr)) {
        auto vec = std::get<std::vector<bool>>(arr);
        if (std::holds_alternative<bool>(value)) {
            bool target = std::get<bool>(value);
            vec.erase(std::remove(vec.begin(), vec.end(), target), vec.end());
        }
        return Memo(vec, name_);
    }
    
    return *this;
}

Memo Memo::add_to_set(const ScalarValue& value) const {
    if (!is_array()) {
        throw std::runtime_error("$addToSet requires array field");
    }
    
    // Check if value already exists
    if (is_series()) {
        const auto& s = as_series();
        double target = 0.0;
        if (std::holds_alternative<double>(value)) {
            target = std::get<double>(value);
        } else if (std::holds_alternative<int64_t>(value)) {
            target = static_cast<double>(std::get<int64_t>(value));
        }
        
        // Vectorized comparison
        auto mask = s.eq(target);
        bool exists = mask.any().item<bool>();
        
        if (!exists) {
            return push(value);
        }
        return *this;
    }
    
    if (is_note()) {
        auto vec = as_note().vector();
        std::string target;
        if (std::holds_alternative<std::string>(value)) {
            target = std::get<std::string>(value);
        } else if (std::holds_alternative<pluss::Chord>(value)) {
            target = std::get<pluss::Chord>(value).str();
        }
        
        bool exists = false;
        for (const auto& chord : vec) {
            if (chord.str() == target) {
                exists = true;
                break;
            }
        }
        
        if (!exists) {
            return push(value);
        }
        return *this;
    }
    
    auto& arr = as_array();
    
    if (std::holds_alternative<std::vector<int64_t>>(arr)) {
        auto vec = std::get<std::vector<int64_t>>(arr);
        if (std::holds_alternative<int64_t>(value)) {
            int64_t target = std::get<int64_t>(value);
            if (std::find(vec.begin(), vec.end(), target) == vec.end()) {
                return push(value);
            }
        }
        return *this;
    }
    
    if (std::holds_alternative<std::vector<double>>(arr)) {
        auto vec = std::get<std::vector<double>>(arr);
        double target = 0.0;
        if (std::holds_alternative<double>(value)) {
            target = std::get<double>(value);
        } else if (std::holds_alternative<int64_t>(value)) {
            target = static_cast<double>(std::get<int64_t>(value));
        }
        if (std::find(vec.begin(), vec.end(), target) == vec.end()) {
            return push(value);
        }
        return *this;
    }
    
    if (std::holds_alternative<std::vector<std::string>>(arr)) {
        auto vec = std::get<std::vector<std::string>>(arr);
        if (std::holds_alternative<std::string>(value)) {
            std::string target = std::get<std::string>(value);
            if (std::find(vec.begin(), vec.end(), target) == vec.end()) {
                return push(value);
            }
        }
        return *this;
    }
    
    if (std::holds_alternative<std::vector<bool>>(arr)) {
        auto vec = std::get<std::vector<bool>>(arr);
        if (std::holds_alternative<bool>(value)) {
            bool target = std::get<bool>(value);
            if (std::find(vec.begin(), vec.end(), target) == vec.end()) {
                return push(value);
            }
        }
        return *this;
    }
    
    return *this;
}

Memo Memo::slice(int64_t start, int64_t end) const {
    if (!is_array()) {
        throw std::runtime_error("slice requires array field");
    }
    
    const auto sz = size();
    if (start < 0) start = 0;
    if (end > sz) end = sz;
    if (start >= end) {
        // Return empty array
        if (is_series()) {
            return Memo(pluss::zeros({0}), name_);
        }
        if (is_note()) {
            return Memo(pluss::make_note(), name_);
        }
    }
    
    const int64_t length = end - start;
    
    if (is_series()) {
        return Memo(as_series().narrow(0, start, length), name_);
    }
    
    if (is_note()) {
        return Memo(pluss::ops::slice(as_note(), 0, static_cast<int>(start), static_cast<int>(end)), name_);
    }
    
    auto& arr = as_array();
    
    if (std::holds_alternative<std::vector<int64_t>>(arr)) {
        auto vec = std::get<std::vector<int64_t>>(arr);
        std::vector<int64_t> result(vec.begin() + start, vec.begin() + end);
        return Memo(result, name_);
    }
    
    if (std::holds_alternative<std::vector<double>>(arr)) {
        auto vec = std::get<std::vector<double>>(arr);
        std::vector<double> result(vec.begin() + start, vec.begin() + end);
        return Memo(result, name_);
    }
    
    if (std::holds_alternative<std::vector<std::string>>(arr)) {
        auto vec = std::get<std::vector<std::string>>(arr);
        std::vector<std::string> result(vec.begin() + start, vec.begin() + end);
        return Memo(result, name_);
    }
    
    if (std::holds_alternative<std::vector<bool>>(arr)) {
        auto vec = std::get<std::vector<bool>>(arr);
        std::vector<bool> result(vec.begin() + start, vec.begin() + end);
        return Memo(result, name_);
    }
    
    return *this;
}

Memo Memo::at(int64_t index) const {
    if (!is_array()) {
        throw std::runtime_error("at requires array field");
    }
    
    const auto sz = size();
    if (index < 0 || index >= sz) {
        throw std::out_of_range("Index out of range");
    }
    
    if (is_series()) {
        double val = as_series()[index].item<double>();
        return Memo(val, name_);
    }
    
    if (is_note()) {
        auto vec = as_note().vector();
        return Memo(vec[index], name_);
    }
    
    if (std::holds_alternative<std::vector<int64_t>>(value_)) {
        return Memo(std::get<std::vector<int64_t>>(value_)[index], name_);
    }
    
    if (std::holds_alternative<std::vector<double>>(value_)) {
        return Memo(std::get<std::vector<double>>(value_)[index], name_);
    }
    
    if (std::holds_alternative<std::vector<std::string>>(value_)) {
        return Memo(std::get<std::vector<std::string>>(value_)[index], name_);
    }
    
    if (std::holds_alternative<std::vector<bool>>(value_)) {
        bool val = std::get<std::vector<bool>>(value_)[index];  // explicit conversion from proxy
        return Memo(val, name_);
    }
    
    return *this;
}

// ========== Document Operations ==========

Memo Memo::get_field(const std::string& field) const {
    if (!is_document()) {
        throw std::runtime_error("get_field requires document");
    }
    
    const auto& doc = as_document();
    auto it = doc.find(field);
    if (it != doc.end()) {
        return *it->second;
    }
    
    return Memo();  // Return null Memo
}

Memo Memo::set_field(const std::string& field, const Memo& value) const {
    if (!is_document()) {
        throw std::runtime_error("set_field requires document");
    }
    
    auto doc = as_document();
    doc[field] = std::make_shared<Memo>(value);
    return Memo(doc, name_);
}

Memo Memo::unset_field(const std::string& field) const {
    if (!is_document()) {
        throw std::runtime_error("unset_field requires document");
    }
    
    auto doc = as_document();
    doc.erase(field);
    return Memo(doc, name_);
}

bool Memo::has_field(const std::string& field) const {
    if (!is_document()) return false;
    const auto& doc = as_document();
    return doc.find(field) != doc.end();
}

std::vector<std::string> Memo::fields() const {
    if (!is_document()) {
        return {};
    }
    
    const auto& doc = as_document();
    std::vector<std::string> result;
    result.reserve(doc.size());
    for (const auto& [key, _] : doc) {
        result.push_back(key);
    }
    return result;
}

// ========== Comparison Operations ==========

bool Memo::eq(const Memo& other) const {
    if (is_scalar() && other.is_scalar()) {
        if (is_int() && other.is_int()) {
            return as_int() == other.as_int();
        }
        if (is_numeric() && other.is_numeric()) {
            return to_numeric() == other.to_numeric();
        }
        if (is_string() && other.is_string()) {
            return as_string() == other.as_string();
        }
        if (is_bool() && other.is_bool()) {
            return as_bool() == other.as_bool();
        }
    }
    return false;
}

bool Memo::ne(const Memo& other) const {
    return !eq(other);
}

bool Memo::lt(const Memo& other) const {
    if (is_numeric() && other.is_numeric()) {
        return to_numeric() < other.to_numeric();
    }
    if (is_string() && other.is_string()) {
        return as_string() < other.as_string();
    }
    return false;
}

bool Memo::le(const Memo& other) const {
    if (is_numeric() && other.is_numeric()) {
        return to_numeric() <= other.to_numeric();
    }
    if (is_string() && other.is_string()) {
        return as_string() <= other.as_string();
    }
    return false;
}

bool Memo::gt(const Memo& other) const {
    if (is_numeric() && other.is_numeric()) {
        return to_numeric() > other.to_numeric();
    }
    if (is_string() && other.is_string()) {
        return as_string() > other.as_string();
    }
    return false;
}

bool Memo::ge(const Memo& other) const {
    if (is_numeric() && other.is_numeric()) {
        return to_numeric() >= other.to_numeric();
    }
    if (is_string() && other.is_string()) {
        return as_string() >= other.as_string();
    }
    return false;
}

bool Memo::in(const std::vector<ScalarValue>& values) const {
    if (!is_scalar()) return false;
    
    for (const auto& val : values) {
        if (is_int() && std::holds_alternative<int64_t>(val)) {
            if (as_int() == std::get<int64_t>(val)) return true;
        }
        if (is_double() && std::holds_alternative<double>(val)) {
            if (as_double() == std::get<double>(val)) return true;
        }
        if (is_string() && std::holds_alternative<std::string>(val)) {
            if (as_string() == std::get<std::string>(val)) return true;
        }
        if (is_bool() && std::holds_alternative<bool>(val)) {
            if (as_bool() == std::get<bool>(val)) return true;
        }
    }
    return false;
}

bool Memo::nin(const std::vector<ScalarValue>& values) const {
    return !in(values);
}

// ========== Logical Operations ==========

bool Memo::exists() const {
    return !is_null();
}

std::string Memo::type() const {
    if (is_null()) return "null";
    if (is_bool()) return "bool";
    if (is_int()) return "int";
    if (is_double()) return "double";
    if (is_string()) return "string";
    if (is_series()) return "series";
    if (is_note()) return "note";
    if (is_document()) return "document";
    
    if (is_array()) {
        const auto& arr = as_array();
        if (std::holds_alternative<std::vector<int64_t>>(arr)) return "array<int>";
        if (std::holds_alternative<std::vector<double>>(arr)) return "array<double>";
        if (std::holds_alternative<std::vector<std::string>>(arr)) return "array<string>";
        if (std::holds_alternative<std::vector<bool>>(arr)) return "array<bool>";
    }
    
    return "unknown";
}

// ========== String Operations ==========

Memo Memo::to_upper() const {
    if (!is_string()) {
        throw std::runtime_error("to_upper requires string field");
    }
    
    std::string str = as_string();
    std::transform(str.begin(), str.end(), str.begin(), 
                   [](unsigned char c) { return std::toupper(c); });
    return Memo(str, name_);
}

Memo Memo::to_lower() const {
    if (!is_string()) {
        throw std::runtime_error("to_lower requires string field");
    }
    
    std::string str = as_string();
    std::transform(str.begin(), str.end(), str.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    return Memo(str, name_);
}

Memo Memo::trim() const {
    if (!is_string()) {
        throw std::runtime_error("trim requires string field");
    }
    
    std::string str = as_string();
    // Trim leading whitespace
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), 
              [](unsigned char ch) { return !std::isspace(ch); }));
    // Trim trailing whitespace
    str.erase(std::find_if(str.rbegin(), str.rend(), 
              [](unsigned char ch) { return !std::isspace(ch); }).base(), str.end());
    return Memo(str, name_);
}

Memo Memo::concat(const std::string& suffix) const {
    if (!is_string()) {
        throw std::runtime_error("concat requires string field");
    }
    
    return Memo(as_string() + suffix, name_);
}

bool Memo::contains(const std::string& substring) const {
    if (!is_string()) return false;
    return as_string().find(substring) != std::string::npos;
}

bool Memo::starts_with(const std::string& prefix) const {
    if (!is_string()) return false;
    const std::string str = as_string();
    return str.size() >= prefix.size() && 
           str.compare(0, prefix.size(), prefix) == 0;
}

bool Memo::ends_with(const std::string& suffix) const {
    if (!is_string()) return false;
    const std::string str = as_string();
    return str.size() >= suffix.size() && 
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// ========== Numeric Operations ==========

Memo Memo::abs() const {
    if (!is_numeric()) {
        throw std::runtime_error("abs requires numeric field");
    }
    return Memo(std::abs(to_numeric()), name_);
}

Memo Memo::round(int decimals) const {
    if (!is_numeric()) {
        throw std::runtime_error("round requires numeric field");
    }
    
    double val = to_numeric();
    double scale = std::pow(10.0, decimals);
    return Memo(std::round(val * scale) / scale, name_);
}

Memo Memo::floor() const {
    if (!is_numeric()) {
        throw std::runtime_error("floor requires numeric field");
    }
    return Memo(std::floor(to_numeric()), name_);
}

Memo Memo::ceil() const {
    if (!is_numeric()) {
        throw std::runtime_error("ceil requires numeric field");
    }
    return Memo(std::ceil(to_numeric()), name_);
}

Memo Memo::sqrt() const {
    if (!is_numeric()) {
        throw std::runtime_error("sqrt requires numeric field");
    }
    return Memo(std::sqrt(to_numeric()), name_);
}

Memo Memo::pow(double exponent) const {
    if (!is_numeric()) {
        throw std::runtime_error("pow requires numeric field");
    }
    return Memo(std::pow(to_numeric(), exponent), name_);
}

// ========== Array Aggregations ==========

double Memo::sum() const {
    if (is_series()) {
        return as_series().sum().item<double>();
    }
    
    if (is_array()) {
        const auto& arr = as_array();
        
        if (std::holds_alternative<std::vector<int64_t>>(arr)) {
            const auto& vec = std::get<std::vector<int64_t>>(arr);
            return static_cast<double>(std::accumulate(vec.begin(), vec.end(), 0LL));
        }
        
        if (std::holds_alternative<std::vector<double>>(arr)) {
            const auto& vec = std::get<std::vector<double>>(arr);
            return std::accumulate(vec.begin(), vec.end(), 0.0);
        }
    }
    
    if (is_numeric()) {
        return to_numeric();
    }
    
    return 0.0;
}

double Memo::mean() const {
    if (is_series()) {
        return as_series().mean().item<double>();
    }
    
    const auto sz = size();
    if (sz == 0) return 0.0;
    
    return sum() / static_cast<double>(sz);
}

double Memo::min_value() const {
    if (is_series()) {
        return as_series().min().item<double>();
    }
    
    if (is_array()) {
        const auto& arr = as_array();
        
        if (std::holds_alternative<std::vector<int64_t>>(arr)) {
            const auto& vec = std::get<std::vector<int64_t>>(arr);
            if (vec.empty()) return std::numeric_limits<double>::quiet_NaN();
            return static_cast<double>(*std::min_element(vec.begin(), vec.end()));
        }
        
        if (std::holds_alternative<std::vector<double>>(arr)) {
            const auto& vec = std::get<std::vector<double>>(arr);
            if (vec.empty()) return std::numeric_limits<double>::quiet_NaN();
            return *std::min_element(vec.begin(), vec.end());
        }
    }
    
    if (is_numeric()) {
        return to_numeric();
    }
    
    return std::numeric_limits<double>::quiet_NaN();
}

double Memo::max_value() const {
    if (is_series()) {
        return as_series().max().item<double>();
    }
    
    if (is_array()) {
        const auto& arr = as_array();
        
        if (std::holds_alternative<std::vector<int64_t>>(arr)) {
            const auto& vec = std::get<std::vector<int64_t>>(arr);
            if (vec.empty()) return std::numeric_limits<double>::quiet_NaN();
            return static_cast<double>(*std::max_element(vec.begin(), vec.end()));
        }
        
        if (std::holds_alternative<std::vector<double>>(arr)) {
            const auto& vec = std::get<std::vector<double>>(arr);
            if (vec.empty()) return std::numeric_limits<double>::quiet_NaN();
            return *std::max_element(vec.begin(), vec.end());
        }
    }
    
    if (is_numeric()) {
        return to_numeric();
    }
    
    return std::numeric_limits<double>::quiet_NaN();
}

int64_t Memo::count() const {
    if (is_series()) {
        // Count non-NaN values
        const auto& s = as_series();
        return (s.data() == s.data()).sum().item<int64_t>();  // NaN != NaN
    }
    
    return size();
}

// ========== Serialization ==========

std::string Memo::to_json() const {
    std::ostringMemo oss;
    
    if (is_null()) {
        oss << "null";
    } else if (is_bool()) {
        oss << (as_bool() ? "true" : "false");
    } else if (is_int()) {
        oss << as_int();
    } else if (is_double()) {
        oss << as_double();
    } else if (is_string()) {
        oss << "\"" << as_string() << "\"";
    } else if (is_array()) {
        oss << "[";
        const auto sz = size();
        
        if (is_series()) {
            const auto& s = as_series();
            // Use item<T>() for individual element access
            for (int64_t i = 0; i < sz; ++i) {
                if (i > 0) oss << ",";
                oss << s[i].item<double>();
            }
        } else if (is_note()) {
            auto vec = as_note().vector();
            for (size_t i = 0; i < vec.size(); ++i) {
                if (i > 0) oss << ",";
                oss << "\"" << vec[i].str() << "\"";
            }
        } else {
            const auto& arr = as_array();
            
            if (std::holds_alternative<std::vector<int64_t>>(arr)) {
                const auto& vec = std::get<std::vector<int64_t>>(arr);
                for (size_t i = 0; i < vec.size(); ++i) {
                    if (i > 0) oss << ",";
                    oss << vec[i];
                }
            } else if (std::holds_alternative<std::vector<double>>(arr)) {
                const auto& vec = std::get<std::vector<double>>(arr);
                for (size_t i = 0; i < vec.size(); ++i) {
                    if (i > 0) oss << ",";
                    oss << vec[i];
                }
            } else if (std::holds_alternative<std::vector<std::string>>(arr)) {
                const auto& vec = std::get<std::vector<std::string>>(arr);
                for (size_t i = 0; i < vec.size(); ++i) {
                    if (i > 0) oss << ",";
                    oss << "\"" << vec[i] << "\"";
                }
            } else if (std::holds_alternative<std::vector<bool>>(arr)) {
                const auto& vec = std::get<std::vector<bool>>(arr);
                for (size_t i = 0; i < vec.size(); ++i) {
                    if (i > 0) oss << ",";
                    oss << (vec[i] ? "true" : "false");
                }
            }
        }
        
        oss << "]";
    } else if (is_document()) {
        oss << "{";
        const auto& doc = as_document();
        bool first = true;
        for (const auto& [key, value] : doc) {
            if (!first) oss << ",";
            first = false;
            oss << "\"" << key << "\":" << value->to_json();
        }
        oss << "}";
    }
    
    return oss.str();
}

std::string Memo::to_bson() const {
    // Simplified BSON-like representation
    // In production, use a proper BSON library
    return to_json();
}

std::string Memo::to_string() const {
    if (is_string()) {
        return as_string();
    }
    return to_json();
}

Memo Memo::from_json(const std::string& json, const std::string& name) {
    // Simplified JSON parser for basic types
    // In production, use a proper JSON library like nlohmann::json
    
    std::string trimmed = json;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);
    
    if (trimmed == "null") {
        Memo result;
        result.set_name(name);
        return result;
    }
    
    if (trimmed == "true") {
        return Memo(true, name);
    }
    
    if (trimmed == "false") {
        return Memo(false, name);
    }
    
    if (!trimmed.empty() && trimmed[0] == '"') {
        // String value
        if (trimmed.size() >= 2 && trimmed.back() == '"') {
            return Memo(trimmed.substr(1, trimmed.size() - 2), name);
        }
    }
    
    // Try parsing as number
    try {
        if (trimmed.find('.') != std::string::npos) {
            return Memo(std::stod(trimmed), name);
        } else {
            return Memo(std::stoll(trimmed), name);
        }
    } catch (...) {
        // Not a number
    }
    
    Memo result;
    result.set_name(name);
    return result;
}

Memo Memo::from_bson(const std::string& bson, const std::string& name) {
    // Simplified BSON parser
    // In production, use a proper BSON library
    return from_json(bson, name);
}

} // namespace pluss::handle
// ... continuing with remaining 1200+ lines of implementation ...