# Stock Trading Engine

## Overview
This project implements a real-time stock trading engine that matches buy and sell orders. The engine is designed to work with 1,024 tickers and meets the challenge requirements by:
- Using lock-free data structures.
- Employing atomic operations and low-level thread management.

## Why C++?
C++ was chosen because it provides:
- **Lock-free Data Structures:** Essential for handling concurrent modifications efficiently.
- **Atomic Operations and Thread Management:** The use of `std::atomic` ensures safe concurrent updates without relying on locks, which is crucial for a real-time system.

## Files Included
- `StockTradingEngine.cpp`: The main C++ source file containing the code.
- `StockTradingEngine.exe`: A pre-built executable for Windows that simulates active stock transactions instantly.
