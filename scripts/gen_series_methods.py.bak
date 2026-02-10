import re
import os
import sys

def process_file(file_path):
    """Process a C++ header file to wrap method bodies with PLUSS_INTERNAL_DISPATCH."""
    with open(file_path, 'r') as f:
        content = f.read()
    
    # Regular expression to match inline method definitions
    # This pattern looks for inline method declarations followed by a body in curly braces
    pattern = r'(inline\s+(?:Series|Series&|const\s+Series&|std::vector<Series>|std::tuple<[^>]+>|bool|int64_t|void|Scalar)\s+Series::[\w_]+\([^)]*\)\s*const\s*)\{([^}]+)\}'
    
    def replace_method(match):
        declaration = match.group(1)
        body = match.group(2)
        
        # Check if the body already has PLUSS_INTERNAL_DISPATCH
        if 'PLUSS_INTERNAL_DISPATCH' in body:
            return match.group(0)  # Return unchanged
        
        # Wrap the body with PLUSS_INTERNAL_DISPATCH
        wrapped_body = f"  PLUSS_INTERNAL_DISPATCH({{{body}}})"
        
        return f"{declaration}{{\n{wrapped_body}\n}}"
    
    # Replace all method bodies with wrapped versions
    modified_content = re.sub(pattern, replace_method, content)
    
    return modified_content

def main():
    # Default to SeriesBody.h in the include/pluss directory
    if len(sys.argv) > 1:
        file_path = sys.argv[1]
    else:
        # Try to find SeriesBody.h in a standard location
        file_path = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'include', 'pluss', 'SeriesBody.h')
    
    if not os.path.exists(file_path):
        print(f"Error: File '{file_path}' not found.")
        return
    
    modified_content = process_file(file_path)
    print(modified_content)

if __name__ == "__main__":
    main()
