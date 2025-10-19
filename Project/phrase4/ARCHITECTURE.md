# HFT System Architecture Diagram

## System Flow Diagram

```
┌──────────────────────────────────────────────────────────────────────────┐
│                     HIGH-FREQUENCY TRADING SYSTEM                        │
│                           Architecture Overview                           │
└──────────────────────────────────────────────────────────────────────────┘

┌─────────────────────┐
│  MarketDataFeed     │  (Simulates market ticks)
│  - generateTick()   │
│  - alignas(64)      │  ◄─── Cache-aligned for performance
└──────────┬──────────┘
           │
           │ Market Data (bid/ask/timestamp)
           ▼
┌─────────────────────┐
│   OrderManager      │  (OMS - Order Management System)
│  - createOrder()    │
│  - updateState()    │  ◄─── Uses shared_ptr for order info
│  - cancelOrder()    │
└──────────┬──────────┘
           │
           │ Order (unique_ptr)
           ▼
┌─────────────────────────────────────────────────────────────────┐
│                        OrderBook                                │
│  ┌──────────────────┐         ┌──────────────────┐            │
│  │  Buy Orders      │         │  Sell Orders     │            │
│  │  (multimap)      │         │  (multimap)      │            │
│  │  Price DESC      │         │  Price ASC       │            │
│  └──────────────────┘         └──────────────────┘            │
│                                                                 │
│  Memory Pool: Pre-allocated blocks for reduced allocation      │
│               overhead (custom allocator)                       │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             │ Orders
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                     MatchingEngine                              │
│  - matchOrder()        ◄─── Core matching logic                │
│  - matchAll()                                                   │
│                                                                 │
│  Algorithm:                                                     │
│    1. Check price compatibility (bid >= ask)                   │
│    2. Match at aggressive order price                          │
│    3. Handle partial fills                                     │
│    4. Generate Trade objects                                   │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             │ Trade objects
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                      TradeLogger                                │
│  - logTrade()          ◄─── RAII: automatic file management    │
│  - logTrades()                                                  │
│  - flush()                                                      │
│                                                                 │
│  Output: CSV file with timestamp, IDs, symbol, price, qty      │
└─────────────────────────────────────────────────────────────────┘
           │
           │ writes to
           ▼
    [ trades.log ]


┌──────────────────────────────────────────────────────────────────────────┐
│                        LATENCY MEASUREMENT                               │
└──────────────────────────────────────────────────────────────────────────┘

    Timer (high_resolution_clock)
      ↓
    start()  ─────┐
                  │
    Market tick   │
       ↓          │  Tick-to-Trade
    Create order  │  Latency
       ↓          │  (nanoseconds)
    Match order   │
       ↓          │
    stop() ───────┘
       ↓
    Store latency
       ↓
    Analyze (min, max, mean, P50, P95, P99, P99.9)


┌──────────────────────────────────────────────────────────────────────────┐
│                      DATA STRUCTURES & TYPES                             │
└──────────────────────────────────────────────────────────────────────────┘

Template Parameters:
  PriceType    = double    (can be float, int, etc.)
  OrderIdType  = int       (must be integral type)

Order<PriceType, OrderIdType>
  ├── id: OrderIdType
  ├── symbol: string
  ├── price: PriceType
  ├── quantity: int
  ├── is_buy: bool
  └── timestamp: high_resolution_clock::time_point

Trade<PriceType, OrderIdType>
  ├── buy_order_id: OrderIdType
  ├── sell_order_id: OrderIdType
  ├── symbol: string
  ├── price: PriceType
  ├── quantity: int
  └── timestamp: high_resolution_clock::time_point

OrderInfo<PriceType, OrderIdType>
  ├── id: OrderIdType
  ├── symbol: string
  ├── price: PriceType
  ├── original_quantity: int
  ├── remaining_quantity: int
  ├── is_buy: bool
  ├── state: OrderState (NEW, PARTIAL_FILLED, FILLED, CANCELLED)
  ├── created_at: time_point
  └── updated_at: time_point


┌──────────────────────────────────────────────────────────────────────────┐
│                     ADVANCED C++ FEATURES USED                           │
└──────────────────────────────────────────────────────────────────────────┘

✓ Templates                 → Generic, type-safe components
✓ Smart Pointers            → unique_ptr, shared_ptr (RAII)
✓ Memory Pool Allocator     → Custom allocation for performance
✓ Cache-Line Alignment      → alignas(64) for CPU cache
✓ Static Assertions         → Compile-time type checking
✓ Move Semantics            → Efficient resource transfer
✓ RAII                      → Automatic resource management
✓ High-Resolution Timing    → std::chrono nanosecond precision
✓ STL Containers            → multimap, vector, unordered_map
✓ Lambda Functions          → For custom comparators


┌──────────────────────────────────────────────────────────────────────────┐
│                        PERFORMANCE OPTIMIZATIONS                         │
└──────────────────────────────────────────────────────────────────────────┘

1. Cache-Line Alignment
   - MarketData aligned to 64 bytes
   - Reduces cache misses
   - Better CPU cache utilization

2. Memory Pool Allocation
   - Pre-allocated memory blocks
   - Reduces malloc/free overhead
   - Better cache locality

3. Smart Pointer Overhead
   - Minimal with modern compilers
   - Reference counting optimized
   - Move semantics avoid copies

4. Container Choice
   - multimap: O(log n) insert/lookup
   - vector: O(1) amortized append with reserve()
   - unordered_map: O(1) average lookup

5. Batch Processing
   - TradeLogger batches writes
   - Reduces I/O overhead
   - Configurable batch size


┌──────────────────────────────────────────────────────────────────────────┐
│                          BENCHMARKING METHODOLOGY                        │
└──────────────────────────────────────────────────────────────────────────┘

Test Scenarios:
  1. Basic Latency Test      → 10K orders, clean book
  2. High-Load Test          → 10K orders, pre-populated book
  3. Burst Test              → 100 bursts × 100 orders
  4. Consistency Test        → 100, 1K, 10K, 50K orders
  5. Comparative Test        → Varying book depths

Metrics Collected:
  - Min/Max latency
  - Mean & Standard Deviation
  - Percentiles: P50, P90, P95, P99, P99.9
  - Throughput (ticks/second)

Statistical Analysis:
  - Sorted latencies for percentile calculation
  - Variance and standard deviation
  - Distribution analysis
```

## Class Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                         Class Hierarchy                         │
└─────────────────────────────────────────────────────────────────┘

┌──────────────────────┐
│  MarketDataFeed      │
├──────────────────────┤
│ - rng                │
│ - price_dist         │
│ - size_dist          │
├──────────────────────┤
│ + generateTick()     │
│ + generateTicks()    │
└──────────────────────┘

┌────────────────────────────────┐
│  Order<Price, OrderId>         │
├────────────────────────────────┤
│ + id: OrderId                  │
│ + symbol: string               │
│ + price: Price                 │
│ + quantity: int                │
│ + is_buy: bool                 │
│ + timestamp: time_point        │
└────────────────────────────────┘
            ▲
            │ uses
            │
┌─────────────────────────────────┐
│  OrderBook<Price, OrderId>      │
├─────────────────────────────────┤
│ - buy_orders: multimap          │
│ - sell_orders: multimap         │
│ - symbol: string                │
│ - memory_pool: MemoryPool<T>    │
├─────────────────────────────────┤
│ + addBuyOrder()                 │
│ + addSellOrder()                │
│ + popBestBuy()                  │
│ + popBestSell()                 │
│ + getBestBid()                  │
│ + getBestAsk()                  │
│ + canMatch()                    │
└─────────────────────────────────┘
            ▲
            │ uses
            │
┌─────────────────────────────────┐
│  MatchingEngine<Price, OrderId> │
├─────────────────────────────────┤
│ - order_book: OrderBook&        │
│ - trades: vector<Trade>         │
├─────────────────────────────────┤
│ + matchOrder()                  │
│ + matchAll()                    │
│ + getTrades()                   │
└─────────────────────────────────┘
            │
            │ produces
            ▼
┌────────────────────────────────┐
│  Trade<Price, OrderId>         │
├────────────────────────────────┤
│ + buy_order_id: OrderId        │
│ + sell_order_id: OrderId       │
│ + symbol: string               │
│ + price: Price                 │
│ + quantity: int                │
│ + timestamp: time_point        │
└────────────────────────────────┘
            ▲
            │ logs
            │
┌─────────────────────────────────┐
│  TradeLogger<Price, OrderId>    │
├─────────────────────────────────┤
│ - trades: vector<Trade>         │
│ - log_file: unique_ptr<ofstream>│
│ - batch_mode: bool              │
├─────────────────────────────────┤
│ + logTrade()                    │
│ + logTrades()                   │
│ + flush()                       │
│ + ~TradeLogger()  (RAII)        │
└─────────────────────────────────┘

┌─────────────────────────────────┐
│  OrderManager<Price, OrderId>   │
├─────────────────────────────────┤
│ - orders: unordered_map         │
│ - next_order_id: OrderId        │
├─────────────────────────────────┤
│ + createOrder()                 │
│ + updateOrderState()            │
│ + updateRemainingQuantity()     │
│ + cancelOrder()                 │
│ + getOrderInfo()                │
└─────────────────────────────────┘
            │
            │ manages
            ▼
┌────────────────────────────────┐
│  OrderInfo<Price, OrderId>     │
├────────────────────────────────┤
│ + id: OrderId                  │
│ + symbol: string               │
│ + price: Price                 │
│ + original_quantity: int       │
│ + remaining_quantity: int      │
│ + is_buy: bool                 │
│ + state: OrderState            │
│ + created_at: time_point       │
│ + updated_at: time_point       │
└────────────────────────────────┘

┌──────────────────┐
│  Timer           │
├──────────────────┤
│ - m_start        │
├──────────────────┤
│ + start()        │
│ + stop()         │
│ + elapsed()      │
└──────────────────┘
```

## Sequence Diagram: Order Matching Flow

```
Client          OrderManager    OrderBook       MatchingEngine   TradeLogger
  │                  │              │                  │              │
  │ generateTick     │              │                  │              │
  │─────────────────>│              │                  │              │
  │                  │              │                  │              │
  │ createOrder      │              │                  │              │
  │─────────────────>│              │                  │              │
  │                  │              │                  │              │
  │                  │ unique_ptr   │                  │              │
  │<─────────────────│              │                  │              │
  │                  │              │                  │              │
  │ matchOrder (move)│              │                  │              │
  │──────────────────┼──────────────┼─────────────────>│              │
  │                  │              │                  │              │
  │                  │              │ popBestBuy/Sell  │              │
  │                  │              │<─────────────────│              │
  │                  │              │                  │              │
  │                  │              │ best order       │              │
  │                  │              │─────────────────>│              │
  │                  │              │                  │              │
  │                  │              │                  │ [match logic]│
  │                  │              │                  │              │
  │                  │              │ add partial fill │              │
  │                  │              │<─────────────────│              │
  │                  │              │                  │              │
  │                  │              │                  │ Trade objects│
  │                  │              │                  │─────────────>│
  │                  │              │                  │              │
  │                  │              │                  │              │ logTrades()
  │                  │              │                  │              │────────┐
  │                  │              │                  │              │        │
  │                  │              │                  │              │<───────┘
  │                  │              │                  │              │
  │ vector<Trade>    │              │                  │              │
  │<─────────────────┼──────────────┼──────────────────│              │
  │                  │              │                  │              │
```
