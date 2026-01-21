#!/usr/bin/env python3

import re
import os
from pathlib import Path

def parse_method_declarations(file_path):
    with open(file_path, 'r') as f:
        content = f.read()
    
    # Extract method declarations
    methods = []
    
    # Process the file line by line to extract method declarations
    lines = content.split('\n')
    for line in lines:
        line = line.strip()
        if not line or line.startswith('//'):
            continue
        
        # Check if it's a method declaration
        if 'Series' in line and ';' in line:
            try:
                # Determine if it's an inplace operation (ends with _)
                is_inplace = '_(' in line or ('Series& ' in line and line.split('Series& ')[1].split('(')[0].endswith('_'))
                
                # Determine if it returns a reference (Series&)
                return_ref = 'Series&' in line
                
                # Determine if it's a const method
                is_const = 'const;' in line
                
                # Extract method name
                if return_ref:
                    name = line.split('Series& ')[1].split('(')[0]
                else:
                    name = line.split('Series ')[1].split('(')[0]
                
                # Extract parameters
                params = line.split('(', 1)[1].split(');')[0]
                
                methods.append({
                    'name': name,
                    'params': params,
                    'is_inplace': is_inplace,
                    'is_const': is_const,
                    'return_ref': return_ref,
                    'full_decl': line
                })
            except Exception as e:
                print(f"Error parsing line: {line}")
                print(f"Error: {e}")
    
    return methods

def extract_param_names(params):
    """Extract parameter names from a parameter list string."""
    if not params.strip():
        return []
    
    param_names = []
    # Split by commas, but handle nested commas in template arguments
    param_list = []
    current_param = ""
    bracket_count = 0
    
    for char in params:
        if char == ',' and bracket_count == 0:
            param_list.append(current_param)
            current_param = ""
        else:
            current_param += char
            if char == '<':
                bracket_count += 1
            elif char == '>':
                bracket_count -= 1
    
    if current_param:
        param_list.append(current_param)
    
    for param in param_list:
        param = param.strip()
        if not param:
            continue
        
        # Remove default values
        param = re.sub(r'=\s*[^,)]+', '', param)
        
        # Get the last word, which should be the parameter name
        words = param.split()
        if words:
            param_name = words[-1]
            # Remove any reference or pointer symbols
            param_name = param_name.replace('&', '').replace('*', '')
            param_names.append(param_name)
    
    return param_names

def generate_implementation(method):
    name = method['name']
    params = method['params']
    is_inplace = method['is_inplace']
    is_const = method['is_const']
    return_ref = method['return_ref']
    
    # Extract parameter names
    param_names = extract_param_names(params)
    
    # Generate the implementation
    return_type = "Series&" if return_ref else "Series"
    const_str = " const" if is_const else ""
    
    implementation = f"inline {return_type} Series::{name}({params}){const_str} {{\n"
    implementation += f"  TORCH_CHECK(this->defined(), \"{name} called on undefined Series\");\n"
    
    # For simplicity, just use the at::Tensor method directly
    if is_inplace:
        if is_const:
            # If it's const, we need to use const_cast
            implementation += f"  at::Tensor(const_cast<Series&>(*this)).{name}({', '.join(param_names)});\n"
            implementation += "  return const_cast<Series&>(*this);\n"
        else:
            implementation += f"  at::Tensor(*this).{name}({', '.join(param_names)});\n"
            implementation += "  return *this;\n"
    else:
        if return_ref:
            if is_const:
                implementation += f"  at::Tensor(const_cast<Series&>(*this)).{name}({', '.join(param_names)});\n"
                implementation += "  return const_cast<Series&>(*this);\n"
            else:
                implementation += f"  at::Tensor(*this).{name}({', '.join(param_names)});\n"
                implementation += "  return *this;\n"
        else:
            implementation += f"  return Series(at::Tensor(*this).{name}({', '.join(param_names)}));\n"
    
    implementation += "}\n\n"
    return implementation

def generate_implementations_file(methods, output_path, is_inplace=False):
    implementations = ""
    
    for method in methods:
        if method['is_inplace'] == is_inplace:
            implementations += generate_implementation(method)
    
    with open(output_path, 'w') as f:
        f.write(implementations)

def main():
    # Get the project root directory
    script_dir = Path(os.path.dirname(os.path.abspath(__file__)))
    project_root = script_dir.parent
    
    # Input and output file paths
    decls_path = project_root / "include" / "pluss" / "SeriesMethodDecls.inl"
    outplace_path = project_root / "include" / "pluss" / "SeriesOutplaceOps.inl"
    inplace_path = project_root / "include" / "pluss" / "SeriesInplaceOps.inl"
    
    # Parse method declarations
    methods = parse_method_declarations(decls_path)
    
    # Generate implementation files
    generate_implementations_file([m for m in methods if not m['is_inplace']], outplace_path, is_inplace=False)
    generate_implementations_file([m for m in methods if m['is_inplace']], inplace_path, is_inplace=True)
    
    print(f"Generated {len([m for m in methods if not m['is_inplace']])} outplace operations in {outplace_path}")
    print(f"Generated {len([m for m in methods if m['is_inplace']])} inplace operations in {inplace_path}")

if __name__ == "__main__":
    main()
