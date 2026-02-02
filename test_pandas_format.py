#!/usr/bin/env python3
"""Test pandas formatting with actual data files to match df_ex"""

import pandas as pd

# Load the actual data files
print("=== Loading messy_data.csv ===")
messy = pd.read_csv('data/messy_data.csv')
print(f"Shape: {messy.shape}")
print()

print("=== messy.head(10) ===")
print(messy.head(10))
print()

print("=== messy.tail(10) ===")
print(messy.tail(10))
print()

print("=== Full messy DataFrame (default display) ===")
print(messy)
print()

print("=== messy.info() ===")
print(messy.info())
print()

print("=== messy.describe() ===")
print(messy.describe())
print()

print("\n" + "="*80 + "\n")

print("=== Loading bank-full.csv ===")
bank = pd.read_csv('data/bank-full.csv', sep=';')
print(f"Shape: {bank.shape}")
print()

print("=== bank.head(10) ===")
print(bank.head(10))
print()

print("=== bank.tail(10) ===")
print(bank.tail(10))
print()

print("=== Full bank DataFrame (default display) ===")
print(bank)
print()

print("=== bank.describe() ===")
print(bank.describe())
print()
