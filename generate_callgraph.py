#!/usr/bin/env python3
"""
Simple call graph generator for Arduino/PlatformIO projects
Analyzes C++ source files and generates a basic call graph
"""

import os
import re
import sys
from collections import defaultdict

def extract_functions(file_path):
    """Extract function definitions and calls from a C++ file"""
    functions = []
    calls = defaultdict(list)
    
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Find function definitions
        func_pattern = r'^\s*(?:void|bool|int|float|char\*?|String)\s+(\w+)\s*\([^)]*\)\s*\{'
        for match in re.finditer(func_pattern, content, re.MULTILINE):
            functions.append(match.group(1))
            
        # Find function calls within each function
        lines = content.split('\n')
        current_function = None
        brace_count = 0
        
        for line in lines:
            line = line.strip()
            
            # Check if we're entering a function
            func_match = re.match(func_pattern, line)
            if func_match:
                current_function = func_match.group(1)
                brace_count = line.count('{') - line.count('}')
                continue
                
            if current_function:
                brace_count += line.count('{') - line.count('}')
                
                # Look for function calls
                call_pattern = r'(\w+)\s*\('
                for match in re.finditer(call_pattern, line):
                    called_func = match.group(1)
                    if called_func not in ['if', 'while', 'for', 'switch', 'Serial', 'printf', 'delay']:
                        calls[current_function].append(called_func)
                
                # Exit function when braces balance
                if brace_count <= 0:
                    current_function = None
                    
    except Exception as e:
        print(f"Error processing {file_path}: {e}")
        
    return functions, dict(calls)

def generate_mermaid_graph(all_functions, all_calls):
    """Generate a Mermaid flowchart from the call graph"""
    
    print("```mermaid")
    print("graph TD")
    
    node_id = {}
    counter = 0
    
    # Assign unique IDs to functions
    for func in all_functions:
        node_id[func] = f"F{counter}"
        counter += 1
        
    # Create nodes
    for func in all_functions:
        print(f"    {node_id[func]}[{func}]")
        
    # Create edges
    for caller, callees in all_calls.items():
        if caller in node_id:
            for callee in callees:
                if callee in node_id:
                    print(f"    {node_id[caller]} --> {node_id[callee]}")
                    
    print("```")

def main():
    """Main function to analyze the weather station project"""
    
    src_dir = "src"
    if not os.path.exists(src_dir):
        print("Error: src directory not found!")
        return
        
    all_functions = set()
    all_calls = defaultdict(list)
    
    # Analyze all C++ files
    cpp_files = []
    for root, dirs, files in os.walk(src_dir):
        for file in files:
            if file.endswith(('.cpp', '.h')):
                cpp_files.append(os.path.join(root, file))
                
    print("# Weather Station Call Graph Analysis")
    print(f"\nAnalyzing {len(cpp_files)} files...")
    
    for file_path in cpp_files:
        print(f"Processing: {file_path}")
        functions, calls = extract_functions(file_path)
        
        all_functions.update(functions)
        for caller, callees in calls.items():
            all_calls[caller].extend(callees)
            
    print(f"\nFound {len(all_functions)} functions")
    print(f"Found {sum(len(callees) for callees in all_calls.values())} function calls")
    
    print("\n## Function Call Graph")
    generate_mermaid_graph(all_functions, all_calls)
    
    print("\n## Function List by File")
    for file_path in cpp_files:
        functions, calls = extract_functions(file_path)
        if functions:
            print(f"\n### {file_path}")
            for func in sorted(functions):
                callees = calls.get(func, [])
                if callees:
                    print(f"- **{func}()** calls: {', '.join(sorted(set(callees)))}")
                else:
                    print(f"- **{func}()**")

if __name__ == "__main__":
    main()
