# Phase-3 Trading System

## Overview of the System Architecture
This project implements a simplified event-driven trading system. It has three main components:

- **MarketSnapshot** (`market_snapshot.h/.cpp`):  
  Maintains the current state of the order book, tracking best bid/ask prices and quantities. It updates whenever a new BID/ASK event arrives and prints changes in the top of book.

- **OrderManager** (`order_manager.h/.cpp`):  
  Manages active orders using RAII (`std::unique_ptr`). It handles order placement, cancellation, and fills, and automatically removes filled or cancelled orders.

- **Main + Strategy** (`main.cpp`):  
  Orchestrates the system by reading events from a feed file (`sample_feed.txt`) using the provided `feed_parser.h`.  
  A simple spread-based strategy is implemented:  
  - Place a **BUY** order when spread ≤ 0.02  
  - Place a **SELL** order when spread ≥ 0.10  
  Duplicate orders at the same price are avoided.

Overall, the system simulates a market data feed, order book updates, and strategy-driven trading actions.

---

## Memory Safety
Memory is managed safely using RAII and smart pointers:
- **Order objects** are stored as `std::unique_ptr<MyOrder>` inside `std::map`.  
  When an order is cancelled or fully filled, it is removed from the map, and its memory is automatically freed.  
- **Market price levels** are also stored as `std::unique_ptr<PriceLevel>`, ensuring automatic cleanup.  
- No raw `new`/`delete` is exposed; ownership is always unique and scoped, preventing leaks and dangling pointers.

This design ensures that memory is reclaimed deterministically when objects go out of scope or are erased from containers.

---

## Compilation and Running
A `Makefile` is provided.

To compile:
```bash
make
```

To run with the `sample_feed.txt` and save output logs:
```bash
make run
```

To clean:
```bash
make clean
```

## Verifying Correctness and Memory Safety
- **Correctness**
  - Run with `sample_feed.txt` or your own feed file.
  - Observe that the system prints top-of-book updates when best bid/ask changes.- Verify that strategy places BUY orders when spread ≤ 0.02 and SELL orders when spread ≥ 0.10.
  - Confirm that duplicate orders at the same price are not placed.

- **Memory Safety**
  - The system uses `std::unique_ptr`, so no manual memory management is required.
  - Tools like valgrind can be used to verify, the output of following should show no memory leaks.
```bash
  valgrind ./trading_sim sample_feed.txt
```

---

## Acknowledgements
Assignment spec from IEOR 4741. Completed by the team listed.
