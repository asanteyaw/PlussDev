import os
import re

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
TARGET_EXTENSIONS = ['.h', '.hpp', '.cpp', '.cc', '.cxx']

pattern = re.compile(r'#include\s*<c10/(.*?)>')
replacement = r'#include <MSharp/\1>'

for dirpath, _, filenames in os.walk(ROOT):
    for filename in filenames:
        if any(filename.endswith(ext) for ext in TARGET_EXTENSIONS):
            filepath = os.path.join(dirpath, filename)
            try:
                with open(filepath, 'r', encoding='utf-8') as f:
                    content = f.read()
                new_content = pattern.sub(replacement, content)
                if new_content != content:
                    with open(filepath, 'w', encoding='utf-8') as f:
                        f.write(new_content)
                    print(f"Updated: {filepath}")
            except Exception as e:
                print(f"Error processing {filepath}: {e}")
print("Replacement complete.")