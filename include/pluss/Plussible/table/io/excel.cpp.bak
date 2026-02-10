#include <pluss/pluss.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <regex>
#include <zip.h>
#include <iostream>
#include <cstring>

namespace pluss::table::io {

// Helper: Read file content from ZIP archive
static std::string read_zip_file(zip_t* archive, const std::string& filename) {
    struct zip_stat st;
    zip_stat_init(&st);
    if (zip_stat(archive, filename.c_str(), 0, &st) != 0) {
        return "";
    }

    zip_file_t* file = zip_fopen(archive, filename.c_str(), 0);
    if (!file) {
        return "";
    }

    std::string content(st.size, '\0');
    zip_int64_t bytes_read = zip_fread(file, &content[0], st.size);
    zip_fclose(file);

    if (bytes_read != static_cast<zip_int64_t>(st.size)) {
        throw std::runtime_error("Failed to read complete file from archive: " + filename);
    }

    return content;
}

// Simple XML tag extractor (lightweight alternative to full XML parser)
// Extracts content between <tagname>content</tagname>
static std::vector<std::string> extract_xml_content(const std::string& xml, const std::string& tag) {
    std::vector<std::string> results;
    std::string open_tag = "<" + tag;
    std::string close_tag = "</" + tag + ">";
    
    size_t pos = 0;
    while ((pos = xml.find(open_tag, pos)) != std::string::npos) {
        // Find the end of opening tag
        size_t tag_end = xml.find('>', pos);
        if (tag_end == std::string::npos) break;
        
        // Find closing tag
        size_t close_pos = xml.find(close_tag, tag_end);
        if (close_pos == std::string::npos) break;
        
        // Extract content between tags
        std::string content = xml.substr(tag_end + 1, close_pos - tag_end - 1);
        results.push_back(content);
        
        pos = close_pos + close_tag.length();
    }
    
    return results;
}

// Extract attribute value from XML tag
static std::string extract_attribute(const std::string& tag, const std::string& attr_name) {
    std::string pattern = attr_name + "=\"([^\"]*)\"";
    std::regex re(pattern);
    std::smatch match;
    if (std::regex_search(tag, match, re) && match.size() > 1) {
        return match[1].str();
    }
    return "";
}

// Get shared strings from xl/sharedStrings.xml
static std::vector<std::string> load_shared_strings(zip_t* archive) {
    std::vector<std::string> shared_strings;
    std::string xml = read_zip_file(archive, "xl/sharedStrings.xml");
    if (xml.empty()) {
        return shared_strings;
    }

    // Extract all <t> tags (text content)
    auto texts = extract_xml_content(xml, "t");
    return texts;
}

// Parse cell reference like "A1" to get column index
static int column_letter_to_index(const std::string& col_letters) {
    int index = 0;
    for (char c : col_letters) {
        if (c >= 'A' && c <= 'Z') {
            index = index * 26 + (c - 'A' + 1);
        }
    }
    return index - 1; // 0-indexed
}

// Parse cell reference "A1" to get column and row
static std::pair<int, int> parse_cell_reference(const std::string& ref) {
    size_t i = 0;
    std::string col_part;
    while (i < ref.length() && std::isalpha(ref[i])) {
        col_part += ref[i];
        i++;
    }
    std::string row_part = ref.substr(i);
    
    int col = column_letter_to_index(col_part);
    int row = row_part.empty() ? 0 : std::stoi(row_part) - 1; // 0-indexed
    
    return {row, col};
}

// Get the first sheet name from workbook.xml
static std::string get_first_sheet_name(zip_t* archive) {
    std::string xml = read_zip_file(archive, "xl/workbook.xml");
    if (xml.empty()) {
        throw std::runtime_error("Could not read workbook.xml");
    }

    // Find first <sheet> tag and extract name attribute
    size_t pos = xml.find("<sheet ");
    if (pos != std::string::npos) {
        size_t end_pos = xml.find('>', pos);
        if (end_pos != std::string::npos) {
            std::string tag = xml.substr(pos, end_pos - pos);
            return extract_attribute(tag, "name");
        }
    }
    
    return "Sheet1"; // default
}

// Get all sheet names from workbook
static std::vector<std::string> get_all_sheet_names(zip_t* archive) {
    std::vector<std::string> sheet_names;
    std::string xml = read_zip_file(archive, "xl/workbook.xml");
    if (xml.empty()) {
        return sheet_names;
    }

    // Find all <sheet> tags and extract name attributes
    size_t pos = 0;
    while ((pos = xml.find("<sheet ", pos)) != std::string::npos) {
        size_t end_pos = xml.find('>', pos);
        if (end_pos == std::string::npos) break;
        
        std::string tag = xml.substr(pos, end_pos - pos);
        std::string name = extract_attribute(tag, "name");
        if (!name.empty()) {
            sheet_names.push_back(name);
        }
        
        pos = end_pos;
    }
    
    return sheet_names;
}

// Get sheet ID (rId) from sheet name
static std::string get_sheet_rid(zip_t* archive, const std::string& sheet_name) {
    std::string xml = read_zip_file(archive, "xl/workbook.xml");
    if (xml.empty()) {
        throw std::runtime_error("Could not read workbook.xml");
    }

    // Find <sheet> tag with matching name
    size_t pos = 0;
    while ((pos = xml.find("<sheet ", pos)) != std::string::npos) {
        size_t end_pos = xml.find('>', pos);
        if (end_pos == std::string::npos) break;
        
        std::string tag = xml.substr(pos, end_pos - pos);
        std::string name = extract_attribute(tag, "name");
        
        if (name == sheet_name) {
            return extract_attribute(tag, "r:id");
        }
        
        pos = end_pos;
    }
    
    return "rId1"; // default to first sheet
}

// Map relationship ID to actual worksheet file path
static std::string map_rid_to_worksheet(zip_t* archive, const std::string& rid) {
    std::string xml = read_zip_file(archive, "xl/_rels/workbook.xml.rels");
    if (xml.empty()) {
        // Fallback: try common patterns
        if (rid == "rId1") return "xl/worksheets/sheet1.xml";
        if (rid == "rId2") return "xl/worksheets/sheet2.xml";
        if (rid == "rId3") return "xl/worksheets/sheet3.xml";
        return "xl/worksheets/sheet1.xml";
    }

    // Find <Relationship> tag with matching Id
    size_t pos = 0;
    while ((pos = xml.find("<Relationship ", pos)) != std::string::npos) {
        size_t end_pos = xml.find(">", pos);
        if (end_pos == std::string::npos) break;
        
        std::string tag = xml.substr(pos, end_pos - pos);
        std::string id = extract_attribute(tag, "Id");
        
        if (id == rid) {
            std::string target = extract_attribute(tag, "Target");
            if (!target.empty()) {
                // Target is relative, prepend xl/ if not present
                if (target.find("worksheets/") == 0) {
                    return "xl/" + target;
                }
                return target;
            }
        }
        
        pos = end_pos;
    }
    
    // Fallback: extract sheet number from rId and construct path
    if (rid.size() > 3 && rid.substr(0, 3) == "rId") {
        std::string num = rid.substr(3);
        return "xl/worksheets/sheet" + num + ".xml";
    }
    
    return "xl/worksheets/sheet1.xml";
}

// Parse sheet XML and extract data
static RawTable parse_sheet_xml(const std::string& xml, const std::vector<std::string>& shared_strings) {
    RawTable raw;
    
    // Extract all <row> elements
    std::vector<std::string> rows = extract_xml_content(xml, "row");
    
    if (rows.empty()) {
        return raw;
    }

    // Map to store grid: [row][col] = value
    std::map<int, std::map<int, std::string>> grid;
    int max_col = 0;
    int max_row = 0;

    // Parse each row
    for (const auto& row_xml : rows) {
        // Extract all <c> (cell) elements from this row
        size_t pos = 0;
        while ((pos = row_xml.find("<c ", pos)) != std::string::npos) {
            size_t end_pos = row_xml.find("</c>", pos);
            if (end_pos == std::string::npos) break;
            
            std::string cell = row_xml.substr(pos, end_pos - pos + 4);
            
            // Get cell reference (e.g., "A1")
            std::string ref = extract_attribute(cell, "r");
            if (ref.empty()) {
                pos = end_pos + 4;
                continue;
            }
            
            auto [row_idx, col_idx] = parse_cell_reference(ref);
            max_row = std::max(max_row, row_idx);
            max_col = std::max(max_col, col_idx);
            
            // Get cell type
            std::string cell_type = extract_attribute(cell, "t");
            
            // Extract value
            auto values = extract_xml_content(cell, "v");
            if (!values.empty()) {
                std::string value = values[0];
                
                // If type is 's', it's a shared string index
                if (cell_type == "s") {
                    int idx = std::stoi(value);
                    if (idx >= 0 && idx < static_cast<int>(shared_strings.size())) {
                        value = shared_strings[idx];
                    }
                }
                
                grid[row_idx][col_idx] = value;
            }
            
            pos = end_pos + 4;
        }
    }

    // Convert grid to RawTable format
    // First row is header
    if (max_row < 0) {
        return raw;
    }

    // Extract headers from first row
    for (int col = 0; col <= max_col; ++col) {
        std::string col_name = grid.count(0) && grid[0].count(col) 
            ? grid[0][col] 
            : "Column" + std::to_string(col + 1);
        raw.columns.push_back(col_name);
        raw.data[col_name] = std::vector<std::string>();
    }

    // Extract data rows (skip header row 0)
    for (int row = 1; row <= max_row; ++row) {
        for (int col = 0; col <= max_col; ++col) {
            std::string value = grid.count(row) && grid[row].count(col) 
                ? grid[row][col] 
                : "";
            raw.data[raw.columns[col]].push_back(value);
        }
    }

    return raw;
}

RawTable load_excel_raw(const std::string& filename, const std::string& sheet_name) {
    // Open the .xlsx file as a ZIP archive
    int err = 0;
    zip_t* archive = zip_open(filename.c_str(), ZIP_RDONLY, &err);
    if (!archive) {
        zip_error_t error;
        zip_error_init_with_code(&error, err);
        std::string error_msg = zip_error_strerror(&error);
        zip_error_fini(&error);
        throw std::runtime_error("Could not open Excel file as ZIP archive: " + filename + " (" + error_msg + ")");
    }

    try {
        // Load shared strings (used for text values)
        std::vector<std::string> shared_strings = load_shared_strings(archive);

        // Determine which sheet to read
        std::string target_sheet = sheet_name.empty() ? get_first_sheet_name(archive) : sheet_name;
        
        // Get the relationship ID for the sheet
        std::string rid = get_sheet_rid(archive, target_sheet);
        
        // Map the relationship ID to the actual worksheet file path
        std::string sheet_path = map_rid_to_worksheet(archive, rid);
        std::string sheet_xml = read_zip_file(archive, sheet_path);
        
        if (sheet_xml.empty()) {
            zip_close(archive);
            throw std::runtime_error("Could not read sheet: " + target_sheet + " (tried: " + sheet_path + ")");
        }

        // Parse the sheet XML
        RawTable result = parse_sheet_xml(sheet_xml, shared_strings);
        
        zip_close(archive);
        return result;
        
    } catch (...) {
        zip_close(archive);
        throw;
    }
}

std::shared_ptr<DataFrame> load_excel(const std::string& filename, const std::string& sheet_name) {
    RawTable raw = load_excel_raw(filename, sheet_name);
    return convert_raw_to_dataframe(raw);
}

std::vector<std::string> list_excel_sheets(const std::string& filename) {
    int err = 0;
    zip_t* archive = zip_open(filename.c_str(), ZIP_RDONLY, &err);
    if (!archive) {
        zip_error_t error;
        zip_error_init_with_code(&error, err);
        std::string error_msg = zip_error_strerror(&error);
        zip_error_fini(&error);
        throw std::runtime_error("Could not open Excel file: " + filename + " (" + error_msg + ")");
    }

    try {
        std::vector<std::string> sheets = get_all_sheet_names(archive);
        zip_close(archive);
        return sheets;
    } catch (...) {
        zip_close(archive);
        throw;
    }
}

std::unordered_map<std::string, std::shared_ptr<DataFrame>> load_excel_all_sheets(const std::string& filename) {
    std::unordered_map<std::string, std::shared_ptr<DataFrame>> result;
    
    // Get all sheet names
    std::vector<std::string> sheets = list_excel_sheets(filename);
    
    // Load each sheet
    for (const auto& sheet_name : sheets) {
        result[sheet_name] = load_excel(filename, sheet_name);
    }
    
    return result;
}

}  // namespace pluss::table::io
