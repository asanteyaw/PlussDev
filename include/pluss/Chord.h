#pragma once

#include <iostream>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>
#include <regex>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <ostream>

#include <internals/aux/utils/Memory.h>  // for intrusive_ptr and intrusive_ptr_target 
#include <internals/aux/utils/Macros.h>


namespace pluss::crypto {

inline std::vector<unsigned char> sha256(const unsigned char* data, size_t length) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (!context) throw std::runtime_error("Failed to create EVP_MD_CTX");

    if (EVP_DigestInit_ex(context, EVP_sha256(), nullptr) != 1 ||
        EVP_DigestUpdate(context, data, length) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("SHA256 init/update failed");
    }

    std::vector<unsigned char> hash(EVP_MD_size(EVP_sha256()));
    unsigned int hash_length = 0;

    if (EVP_DigestFinal_ex(context, hash.data(), &hash_length) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("SHA256 finalization failed");
    }

    EVP_MD_CTX_free(context);

    if (hash_length != hash.size()) {
        throw std::runtime_error("Hash length mismatch");
    }

    return hash;
}

inline std::string sha256_hex(const unsigned char* data, size_t length) {
    auto hash = sha256(data, length);
    std::ostringstream oss;
    for (auto byte : hash)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    return oss.str();
}

inline std::vector<unsigned char> md5(const unsigned char* data, size_t length) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (!context) throw std::runtime_error("Failed to create EVP_MD_CTX");

    if (EVP_DigestInit_ex(context, EVP_md5(), nullptr) != 1 ||
        EVP_DigestUpdate(context, data, length) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("MD5 init/update failed");
    }

    std::vector<unsigned char> hash(EVP_MD_size(EVP_md5()));
    unsigned int hash_length = 0;

    if (EVP_DigestFinal_ex(context, hash.data(), &hash_length) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("MD5 finalization failed");
    }

    EVP_MD_CTX_free(context);

    if (hash_length != hash.size()) {
        throw std::runtime_error("Hash length mismatch");
    }

    return hash;
}

inline std::string md5_hex(const unsigned char* data, size_t length) {
    auto hash = md5(data, length);
    std::ostringstream oss;
    for (auto byte : hash)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    return oss.str();
}

} // namespace pluss::crypto

namespace pluss {

// Backing class (now intrusive)
class ChordData : public intrusive_ptr_target {
public:
    std::string value;

    explicit ChordData(char c) : value(1, c) {}
    explicit ChordData(const std::string& s) : value(s) {}
    explicit ChordData(std::string&& s) : value(std::move(s)) {}
};

class ReversedChordView;

// Public wrapper
class Chord {
private:
    pluss::intrusive_ptr<ChordData> handle_;

public:
    Chord() : handle_(new ChordData(""), false) {}
    Chord(const std::string& s) : handle_(new ChordData(s), false) {}
    Chord(std::string&& s) : handle_(new ChordData(std::move(s)), false) {}
    Chord(const char* s) : handle_(new ChordData(std::string(s)), false) {}
    Chord(char c) : handle_(new ChordData(c), false) {}

    Chord(const Chord&) = default;
    Chord& operator=(const Chord&) = default;

    const std::string& str() const { return handle_->value; }
    std::string& str() { return handle_->value; }

    friend std::ostream& operator<<(std::ostream& os, const Chord& chord) {
        return os << chord.str();
    }

    // TODO: All string utilities and NLP features remain the same
    // You can call .str() internally

    // Custom helpers
    bool is_alpha() const;
    bool is_letter() const;
    char letter() const;
    char& letter();
    int Chord_score() const;

    // Char-specific helpers
    bool is_upper() const;
    bool is_lower() const;
    bool is_digit() const;
    bool is_space() const;
    bool is_alnum() const;
    bool is_blank() const;
    bool is_cntrl() const;
    bool is_graph() const;
    bool is_print() const;
    bool is_punct() const;
    bool is_xdigit() const;
    Chord to_upper() const;
    Chord to_lower() const;

    // Fun new methods
    bool is_palindrome() const;
    bool is_anagram_of(const Chord& other) const;
    bool is_numeric() const;
    std::unordered_map<char, int> char_counts() const;
    char most_common_char() const;
    char least_common_char() const;
    ReversedChordView reversed() const;
    Chord reverse() const;
    Chord& reverse_();
    Chord remove_whitespace() const;
    Chord collapse_spaces() const;
    Chord without_punctuation() const;
    Chord only_letters() const;
    Chord title_case() const;
    Chord swap_case() const;
    std::string sha256() const;
    std::string md5() const;
    Chord escaped_html() const;
    Chord escaped_json() const;
    Chord url_encoded() const;
    bool matches(const std::string& regex_pattern) const;
    bool contains_regex(const std::string& pattern) const;
    Chord replace(const std::string& pattern, const std::string& replacement) const;
    std::vector<Chord> findall(const std::string& pattern) const;
    Chord extract(const std::string& pattern, int group = 0) const;
    std::vector<Chord> split_regex(const std::string& pattern) const;
    std::vector<Chord> split(char delimiter) const;
    std::vector<Chord> ngrams(int n) const;
    Chord slice(int start, int end) const;
    bool starts_with(const std::string& prefix) const;
    bool ends_with(const std::string& suffix) const;
    bool contains(const std::string& substring) const;
    Chord repeat(int times) const;
    std::string hash_sha256() const;         // requires OpenSSL
    int word_count() const;
    int char_count() const;
    std::vector<std::string> to_words() const;
    std::vector<char> to_charvec() const;
};

class ChordIterator {
    std::string::const_reverse_iterator it_;
public:
    explicit ChordIterator(std::string::const_reverse_iterator it) : it_(it) {}
    Chord operator*() const { return Chord(*it_); }
    ChordIterator& operator++() { ++it_; return *this; }
    bool operator!=(const ChordIterator& other) const { return it_ != other.it_; }
};

class ReversedChordView {
    Chord chord_;
public:
    explicit ReversedChordView(const Chord& n) : chord_(n) {}

    ChordIterator begin() const { return ChordIterator(chord_.str().rbegin()); }
    ChordIterator end() const { return ChordIterator(chord_.str().rend()); }
};

} // namespace pluss


namespace pluss{

    inline bool operator==(const Chord& lhs, const std::string& rhs) { return lhs.str() == rhs; }
    inline bool operator==(const std::string& lhs, const Chord& rhs) { return lhs == rhs.str(); }
    inline bool operator==(const Chord& lhs, const char* rhs) { return lhs.str() == std::string(rhs); }
    inline bool operator==(const char* lhs, const Chord& rhs) { return std::string(lhs) == rhs.str(); }
    inline bool operator!=(const Chord& lhs, const std::string& rhs) { return !(lhs == rhs); }
    inline bool operator!=(const std::string& lhs, const Chord& rhs) { return !(lhs == rhs); }
    inline bool operator!=(const Chord& lhs, const char* rhs) { return !(lhs == rhs); }
    inline bool operator!=(const char* lhs, const Chord& rhs) { return !(lhs == rhs); }
    inline Chord operator+(const Chord& a, const Chord& b) { return Chord(a.str() + b.str()); }
    inline bool Chord::is_alpha() const {return std::all_of(str().begin(), str().end(), ::isalpha);}
    inline bool Chord::is_letter() const { return str().size() == 1; }
    inline char Chord::letter() const { PLUSS_VALIDATE(is_letter(), "Chord does not hold a single character."); return str()[0]; }
    inline char& Chord::letter() { PLUSS_VALIDATE(is_letter(), "Chord does not hold a single character."); return str()[0]; }

    // Char-specific utilities (only for single-char Chords)
    inline bool Chord::is_upper() const { PLUSS_VALIDATE(is_letter(), "Chord must be a char."); return std::isupper(letter()); }
    inline bool Chord::is_lower() const { PLUSS_VALIDATE(is_letter(), "Chord must be a char."); return std::islower(letter()); }
    inline bool Chord::is_digit() const { PLUSS_VALIDATE(is_letter(), "Chord must be a char."); return std::isdigit(letter()); }
    inline bool Chord::is_space() const { PLUSS_VALIDATE(is_letter(), "Chord must be a char."); return std::isspace(letter()); }
    inline bool Chord::is_alnum() const { PLUSS_VALIDATE(is_letter(), "Chord must be a char."); return std::isalnum(letter()); }
    inline bool Chord::is_blank() const { PLUSS_VALIDATE(is_letter(), "Chord must be a char."); return std::isblank(letter()); }
    inline bool Chord::is_cntrl() const { PLUSS_VALIDATE(is_letter(), "Chord must be a char."); return std::iscntrl(letter()); }
    inline bool Chord::is_graph() const { PLUSS_VALIDATE(is_letter(), "Chord must be a char."); return std::isgraph(letter()); }
    inline bool Chord::is_print() const { PLUSS_VALIDATE(is_letter(), "Chord must be a char."); return std::isprint(letter()); }
    inline bool Chord::is_punct() const { PLUSS_VALIDATE(is_letter(), "Chord must be a char."); return std::ispunct(letter()); }
    inline bool Chord::is_xdigit() const { PLUSS_VALIDATE(is_letter(), "Chord must be a char."); return std::isxdigit(letter()); }
    // inline Chord Chord::to_upper() const { PLUSS_VALIDATE(is_letter(), "Chord must be a char."); return Chord(std::toupper(letter())); }
    // inline Chord Chord::to_lower() const { PLUSS_VALIDATE(is_letter(), "Chord must be a char."); return Chord(std::tolower(letter())); }
    inline Chord Chord::to_upper() const { std::string s = str(); std::transform(s.begin(), s.end(), s.begin(), ::toupper); return Chord(std::move(s)); }
    inline Chord Chord::to_lower() const { std::string s = str(); std::transform(s.begin(), s.end(), s.begin(), ::tolower); return Chord(std::move(s)); }

    inline int Chord::Chord_score() const {
        int score = 0;
        for (char c : str()) {
            if (std::isalpha(c))
                score += std::toupper(c) - 'A' + 1;
        }
        return score;
    }

    inline bool Chord::is_palindrome() const {
        std::string s = str();
        s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        std::string rev = s;
        std::reverse(rev.begin(), rev.end());
        return s == rev;
    }
    
    inline bool Chord::is_anagram_of(const Chord& other) const {
        std::string a = str(), b = other.str();
        std::sort(a.begin(), a.end());
        std::sort(b.begin(), b.end());
        return a == b;
    }
    
    inline bool Chord::is_numeric() const {
        const std::string& s = str();
        if (s.empty()) return false;
        size_t i = 0;
        if (s[0] == '-') i++;
        bool has_digit = false, has_dot = false;
        for (; i < s.size(); ++i) {
            if (std::isdigit(s[i])) has_digit = true;
            else if (s[i] == '.' && !has_dot) has_dot = true;
            else return false;
        }
        return has_digit;
    }
    
    inline std::unordered_map<char, int> Chord::char_counts() const {
        std::unordered_map<char, int> counts;
        for (char c : str()) ++counts[c];
        return counts;
    }
    
    inline char Chord::most_common_char() const {
        auto counts = char_counts();
        return std::max_element(counts.begin(), counts.end(),
            [](auto a, auto b) { return a.second < b.second; })->first;
    }
    
    inline char Chord::least_common_char() const {
        auto counts = char_counts();
        return std::min_element(counts.begin(), counts.end(),
            [](auto a, auto b) { return a.second < b.second; })->first;
    }
    
    inline ReversedChordView Chord::reversed() const {
        return ReversedChordView(*this);
    }
    
    inline Chord Chord::remove_whitespace() const {
        std::string out;
        std::copy_if(str().begin(), str().end(), std::back_inserter(out), [](char c) { return !std::isspace(c); });
        return Chord(out);
    }
    
    inline Chord Chord::collapse_spaces() const {
        std::istringstream iss(str());
        std::string token, out;
        while (iss >> token) {
            if (!out.empty()) out += ' ';
            out += token;
        }
        return Chord(out);
    }
    
    inline Chord Chord::without_punctuation() const {
        std::string out;
        std::copy_if(str().begin(), str().end(), std::back_inserter(out), [](char c) { return !std::ispunct(c); });
        return Chord(out);
    }
    
    inline Chord Chord::only_letters() const {
        std::string out;
        std::copy_if(str().begin(), str().end(), std::back_inserter(out), ::isalpha);
        return Chord(out);
    }
    
    inline Chord Chord::title_case() const {
        std::istringstream iss(str());
        std::string word, out;
        while (iss >> word) {
            if (!word.empty()) {
                word[0] = std::toupper(word[0]);
                std::transform(word.begin() + 1, word.end(), word.begin() + 1, ::tolower);
                if (!out.empty()) out += ' ';
                out += word;
            }
        }
        return Chord(out);
    }
    
    inline Chord Chord::swap_case() const {
        std::string out = str();
        for (char& c : out)
            c = std::islower(c) ? std::toupper(c) : std::isupper(c) ? std::tolower(c) : c;
        return Chord(out);
    }
    
    inline Chord Chord::escaped_html() const {
        std::string out;
        for (char c : str()) {
            switch (c) {
                case '&': out += "&amp;"; break;
                case '<': out += "&lt;"; break;
                case '>': out += "&gt;"; break;
                case '"': out += "&quot;"; break;
                case '\'': out += "&#39;"; break;
                default: out += c;
            }
        }
        return Chord(out);
    }
    
    inline Chord Chord::escaped_json() const {
        std::ostringstream oss;
        for (char c : str()) {
            switch (c) {
                case '\\': oss << "\\\\"; break;
                case '"': oss << "\\\""; break;
                case '\n': oss << "\\n"; break;
                case '\r': oss << "\\r"; break;
                case '\t': oss << "\\t"; break;
                default: oss << c;
            }
        }
        return Chord(oss.str());
    }
    
    inline Chord Chord::url_encoded() const {
        std::ostringstream oss;
        for (char c : str()) {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') oss << c;
            else oss << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)c;
        }
        return Chord(oss.str());
    }
    
    inline bool Chord::matches(const std::string& pattern) const {
        return std::regex_match(str(), std::regex(pattern));
    }

    inline bool Chord::contains_regex(const std::string& pattern) const {
        return std::regex_search(str(), std::regex(pattern));
    }
    
    inline Chord Chord::replace(const std::string& pattern, const std::string& replacement) const {
        return Chord(std::regex_replace(str(), std::regex(pattern), replacement));
    }
    
    inline std::vector<Chord> Chord::findall(const std::string& pattern) const {
        std::vector<Chord> results;
        std::regex re(pattern);
        auto begin = std::sregex_iterator(str().begin(), str().end(), re);
        auto end = std::sregex_iterator();
        for (auto it = begin; it != end; ++it) {
            results.emplace_back(it->str());
        }
        return results;
    }
    
    inline Chord Chord::extract(const std::string& pattern, int group) const {
        try {
            // Make a local copy of the string to ensure it stays in scope
            std::string s = str();
            
            // Use a simpler regex approach
            std::regex re(pattern);
            std::smatch match;
            
            // Search the string
            if (std::regex_search(s, match, re)) {
                // Check if the group index is valid
                if (group >= 0 && group < static_cast<int>(match.size())) {
                    // Create a new string from the match to avoid any reference issues
                    std::string result = match[group].str();
                    return Chord(result);
                }
            }
            
            // Return empty string if no match or invalid group
            return Chord("");
        } catch (const std::exception& e) {
            // Handle any regex exceptions
            return Chord("");
        }
    }
    
    inline std::vector<Chord> Chord::split_regex(const std::string& pattern) const {
        try {
            std::regex re(pattern);
            std::string s = str();
            std::sregex_token_iterator begin(s.begin(), s.end(), re, -1);
            std::sregex_token_iterator end;
            std::vector<Chord> tokens;
            for (auto it = begin; it != end; ++it) {
                tokens.emplace_back(it->str());
            }
            return tokens;
        } catch (const std::exception& e) {
            // Handle any regex exceptions
            return {Chord(str())};
        }
    }
    
    inline std::vector<Chord> Chord::split(char delimiter) const {
        std::istringstream iss(str());
        std::string token;
        std::vector<Chord> parts;
        while (std::getline(iss, token, delimiter)) parts.emplace_back(token);
        return parts;
    }
    
    inline std::vector<Chord> Chord::ngrams(int n) const {
        std::vector<Chord> grams;
        const std::string& s = str();
        if (n <= 0 || s.size() < (size_t)n) return grams;
        for (size_t i = 0; i <= s.size() - n; ++i)
            grams.emplace_back(s.substr(i, n));
        return grams;
    }
    
    inline Chord Chord::slice(int start, int end) const {
        int len = static_cast<int>(str().size());
        if (start < 0) start += len;
        if (end < 0) end += len;
        start = std::max(0, std::min(start, len));
        end = std::max(0, std::min(end, len));
        if (start >= end) return Chord("");
        return Chord(str().substr(start, end - start));
    }

    inline bool Chord::starts_with(const std::string& prefix) const {
        return str().rfind(prefix, 0) == 0;
    }
    inline bool Chord::ends_with(const std::string& suffix) const {
        return str().size() >= suffix.size() &&
               str().compare(str().size() - suffix.size(), suffix.size(), suffix) == 0;
    }
    inline bool Chord::contains(const std::string& s) const {
        return str().find(s) != std::string::npos;
    }
    
    inline Chord Chord::reverse() const {
        std::string rev = str();
        std::reverse(rev.begin(), rev.end());
        return Chord(std::move(rev));
    }
    
    inline Chord& Chord::reverse_() {
        std::reverse(str().begin(), str().end());
        return *this;
    }
    
    inline Chord Chord::repeat(int times) const {
        if (times <= 0) return Chord("");
        std::string result;
        result.reserve(str().size() * times);
        for (int i = 0; i < times; ++i) result += str();
        return Chord(result);
    }
    
    inline std::string Chord::sha256() const {
        const auto& data = reinterpret_cast<const unsigned char*>(str().data());
        return pluss::crypto::sha256_hex(data, str().size());
    }
    
    inline std::string Chord::md5() const {
        const auto& data = reinterpret_cast<const unsigned char*>(str().data());
        return pluss::crypto::md5_hex(data, str().size());
    }
    
    inline int Chord::word_count() const {
        std::istringstream iss(str());
        return std::distance(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>());
    }
    
    inline int Chord::char_count() const {
        return static_cast<int>(str().size());
    }
    
    inline std::vector<std::string> Chord::to_words() const {
        std::istringstream iss(str());
        std::vector<std::string> words{std::istream_iterator<std::string>(iss), {}};
        return words;
    }
    
    inline std::vector<char> Chord::to_charvec() const {
        return std::vector<char>(str().begin(), str().end());
    }
    
} // namespace pluss
