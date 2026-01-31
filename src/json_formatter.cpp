// Minimal JSON formatter (pretty-printer) CLI
// Usage: jsonfmt [--indent N] [input-file]

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cctype>
#include <vector>
#include <stdexcept>

static void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " [--indent N] [input-file]\n";
}

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int indent = 2;
    std::string filename;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--indent" && i + 1 < argc) {
            try {
                indent = std::stoi(argv[++i]);
            } catch (const std::exception&) { indent = 2; }
            if (indent < 0) indent = 0;
        } else if (a == "-h" || a == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (a.rfind("--", 0) == 0) {
            std::cerr << "Unknown option: " << a << "\n";
            print_usage(argv[0]);
            return 1;
        } else {
            filename = a;
        }
    }

    // Read input
    std::string input;
    if (!filename.empty()) {
        std::ifstream in(filename);
        if (!in) {
            std::cerr << "Failed to open file: " << filename << "\n";
            return 2;
        }
        std::ostringstream ss;
        ss << in.rdbuf();
        input = ss.str();
    } else {
        std::ostringstream ss;
        ss << std::cin.rdbuf();
        input = ss.str();
    }

    std::string out;
    out.reserve(input.size() * 2 + 128);

    bool in_string = false;
    bool escape = false;
    int level = 0;

    auto add_indent = [&](int lev){
        for (int i = 0; i < lev * indent; ++i) out.push_back(' ');
    };

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

        if (in_string) {
            out.push_back(c);
            if (escape) {
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else if (c == '"') {
                in_string = false;
            }
            continue;
        }

        if (c == '"') {
            in_string = true;
            out.push_back(c);
            continue;
        }

        if (c == '{' || c == '[') {
            out.push_back(c);
            // look ahead for immediate closing to keep compact [] or {}
            size_t j = i + 1;
            while (j < input.size() && std::isspace((unsigned char)input[j])) ++j;
            if (j < input.size() && (input[j] == ']' || input[j] == '}')) {
                // empty container, don't add newline
                continue;
            }
            out.push_back('\n');
            ++level;
            add_indent(level);
            continue;
        }

        if (c == '}' || c == ']') {
            out.push_back('\n');
            --level;
            if (level < 0) level = 0;
            add_indent(level);
            out.push_back(c);
            continue;
        }

        if (c == ',') {
            out.push_back(c);
            out.push_back('\n');
            add_indent(level);
            continue;
        }

        if (c == ':') {
            out.push_back(c);
            out.push_back(' ');
            continue;
        }

        if (std::isspace((unsigned char)c)) {
            // collapse whitespace outside strings
            continue;
        }

        out.push_back(c);
    }

    // Final newline
    if (out.empty() || out.back() != '\n') out.push_back('\n');

    std::cout << out;
    return 0;
}
