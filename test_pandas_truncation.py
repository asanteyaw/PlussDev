#!/usr/bin/env python3
"""Test pandas truncation behavior"""

import pandas as pd

# Test with exactly 25 rows (should show first 10, ..., last 10)
df = pd.DataFrame({'A': range(25), 'B': range(25, 50)})
print("=== 25 rows (should truncate) ===")
print(df)
print()

# Set display options to see truncation more clearly
pd.set_option('display.max_rows', 10)
print("=== Same 25 rows with max_rows=10 ===")
print(df)
print()

# Even smaller
pd.set_option('display.max_rows', 6)
print("=== Same 25 rows with max_rows=6 ===")
print(df)
