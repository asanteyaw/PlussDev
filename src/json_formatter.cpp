// Minimal JSON formatter (pretty-printer) CLI
// Usage: jsonfmt [--indent N] [input-file]

#include <bits/stdc++.h>
using namespace std;

static void print_usage(const char* prog) {
    cerr << "Usage: " << prog << " [--indent N] [input-file]\n";
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int indent = 2;
    string filename;

    for (int i = 1; i < argc; ++i) {
        string a = argv[i];
        if (a == "--indent" && i + 1 < argc) {
            indent = stoi(argv[++i]);
            if (indent < 0) indent = 0;
        } else if (a == "-h" || a == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (a.rfind("--", 0) == 0) {
            cerr << "Unknown option: " << a << "\n";
            print_usage(argv[0]);
            return 1;
        } else {
            filename = a;
        }
    }

    // Read input
    string input;
    if (!filename.empty()) {
        ifstream in(filename);
        if (!in) {
            cerr << "Failed to open file: " << filename << "\n";
            return 2;
        }
        std::ostringstream ss;
        ss << in.rdbuf();
        input = ss.str();
    } else {
        std::ostringstream ss;
        ss << cin.rdbuf();
        input = ss.str();
    }

    string out;
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
            while (j < input.size() && isspace((unsigned char)input[j])) ++j;
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

        if (isspace((unsigned char)c)) {
            // collapse whitespace outside strings
            continue;
        }

        out.push_back(c);
    }

    // Final newline
    if (out.empty() || out.back() != '\n') out.push_back('\n');

    cout << out;
    return 0;
}
