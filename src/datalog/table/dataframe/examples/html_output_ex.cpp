#include <pluss/pluss.h>
#include <iostream>
#include <fstream>

using namespace pluss::table;

int main() {
    // Load messy data
    auto df = DataFrame::load("csv", "../data/messy_data.csv");
    
    // Generate HTML output
    std::cout << "Generating HTML output..." << std::endl;
    std::string html = df->to_html();
    
    // Save to file
    std::ofstream html_file("messy_data.html");
    html_file << "<!DOCTYPE html>\n";
    html_file << "<html>\n<head>\n";
    html_file << "<style>\n";
    html_file << "table.dataframe { border-collapse: collapse; margin: 25px 0; font-size: 0.9em; font-family: sans-serif; }\n";
    html_file << "table.dataframe thead tr { background-color: #009879; color: #ffffff; text-align: left; }\n";
    html_file << "table.dataframe th, table.dataframe td { padding: 12px 15px; }\n";
    html_file << "table.dataframe tbody tr { border-bottom: 1px solid #dddddd; }\n";
    html_file << "table.dataframe tbody tr:nth-of-type(even) { background-color: #f3f3f3; }\n";
    html_file << "</style>\n";
    html_file << "</head>\n<body>\n";
    html_file << "<h1>Messy Data DataFrame</h1>\n";
    html_file << html;
    html_file << "\n</body>\n</html>";
    html_file.close();
    
    std::cout << "HTML saved to: messy_data.html" << std::endl;
    
    // Generate LaTeX output
    std::cout << "\nGenerating LaTeX output..." << std::endl;
    std::string latex = df->to_latex();
    
    std::ofstream latex_file("messy_data.tex");
    latex_file << "\\documentclass{article}\n";
    latex_file << "\\usepackage{booktabs}\n";
    latex_file << "\\begin{document}\n\n";
    latex_file << "\\section*{Messy Data DataFrame}\n\n";
    latex_file << latex;
    latex_file << "\n\\end{document}";
    latex_file.close();
    
    std::cout << "LaTeX saved to: messy_data.tex" << std::endl;
    
    // Also demonstrate with describe data
    std::cout << "\n=== Terminal Output (for comparison) ===" << std::endl;
    df->head(5, 0);  // Explicit format parameter to use display version
    
    return 0;
}
