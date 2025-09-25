#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <fstream>
#include <iomanip>
#include <cstring>

struct Order{
    uint64_t id;
    int side;     // 0 short, 1 long
    int qty;
    int price;
    int payload[2];
};

static int order_book_table[64];
static int position_table[32];

class Processor{
public:
    virtual ~Processor() = default;
    virtual uint64_t process(Order& order) = 0;
};


class StrategyA_V : public Processor{
public:
    uint64_t process(Order& order) override {
        uint64_t result = 0;
        // 6 integer arithmetic operations
        result +=order.id * 15;
        result += order.qty * order.price;
        result -= order.price * 3; 
        result ^= order.side;
        result += order.payload[0] * 3;
        result -= order.payload[1] * 7;
        // Two fixed-size memory writes
        order_book_table[order.id % 64] += order.qty;
        position_table[order.id % 32] = order.price * (order.side - 1) * order.qty;
        // Conditional branch with counter update
        static int counter_a = 0;
        if(order.side == 1){
            counter_a++;
            result += counter_a;
        }
        return result;
    }
};
class StrategyB_V : public Processor {
public:
    uint64_t process(Order& order) override {
        // 6-10 integer arithmetic operations
        uint64_t result = 0;
        result = order.id + 42;
        result *= (order.qty + order.price);
        result ^= order.side;
        result += order.payload[0] << 2;
        result -= order.payload[1] >> 1;
        result = result * 31 + 17;
        // Two fixed-size memory writes
        order_book_table[order.id % 64] -= order.qty;
        position_table[order.id % 32] = order.price * 2;
        // Conditional branch with counter update
        static int counter_b = 0;
        if (order.side == 0) {
            counter_b++;
            result += counter_b * 2;
        }
        
        return result;
    }
};

class StrategyA_NV {
public: // Same with StrategyA_V
    uint64_t run(Order& order){
        uint64_t result = 0;
        // 6 integer arithmetic operations
        result +=order.id * 15;
        result += order.qty * order.price;
        result -= order.price * 3; 
        result ^= order.side;
        result += order.payload[0] * 3;
        result -= order.payload[1] * 7;
        // Two fixed-size memory writes
        order_book_table[order.id % 64] += order.qty;
        position_table[order.id % 32] = order.price * (order.side - 1) * order.qty;
        // Conditional branch with counter update
        static int counter_a = 0;
        if(order.side == 1){
            counter_a++;
            result += counter_a;
        }
        return result;
    }
};

class StrategyB_NV {
public: // Same with StrategyB_V
    uint64_t run(Order& order){
        // 6-10 integer arithmetic operations
        uint64_t result = 0;
        result = order.id + 42;
        result *= (order.qty + order.price);
        result ^= order.side;
        result += order.payload[0] << 2;
        result -= order.payload[1] >> 1;
        result = result * 31 + 17;
        // Two fixed-size memory writes
        order_book_table[order.id % 64] -= order.qty;
        position_table[order.id % 32] = order.price * 2;
        // Conditional branch with counter update
        static int counter_b = 0;
        if (order.side == 0) {
            counter_b++;
            result += counter_b * 2;
        }
        
        return result;
    }
};

enum class Pattern { HOMOGENEOUS, MIXED_RANDOM, BURSTY };
std::vector<int> generate_assignment_pattern(Pattern pattern, int num_orders, int seed = 12345){
    std::vector<int> assignment(num_orders,0);
    std::mt19937 rng(seed);
    switch (pattern){
        // All orders to Strategy A
        case Pattern::HOMOGENEOUS:
            std::fill(assignment.begin(), assignment.end(), 0); 
            break;
        // Randomly assign orders to Strategy A or B with equal probability
        case Pattern::MIXED_RANDOM:
            for (int i = 0; i < num_orders; i++){
                assignment[i] = rng() % 2;
            }
            break;
        // Assign orders in bursts: 64 to Strategy A, then 16 to Strategy B, repeat
        case Pattern::BURSTY:
            for (int i = 0; i < num_orders; i++) {
                int cycle_pos = i % 80; 
                assignment[i] = (cycle_pos < 64) ? 0 : 1;
            }
            break;
    }
    return assignment;
};

std::vector<Order> generate_orders(int num_orders, int seed = 54321){
    std::vector<Order> orders(num_orders);
    std::mt19937 rng(seed);
    for (int i = 0; i < num_orders; i++){
        orders[i] = {
            static_cast<uint64_t>(i + 1),
            static_cast<int>(rng() % 2),
            static_cast<int>(100 + (rng() % 500)),
            static_cast<int>(1000 + (rng() % 1000)),
            {static_cast<int>(rng() % 1000), static_cast<int>(rng() % 2000)}
        };
    }
    return orders;
}

//Benchmark virtual implementation
uint64_t benchmark_virtual(const std::vector<Order>& orders, 
                          const std::vector<int>& assignments,
                          std::chrono::nanoseconds& elapsed) {

    StrategyA_V strategy_a;
    StrategyB_V strategy_b;
    std::vector<Processor*> processors = {&strategy_a, &strategy_b};

    volatile uint64_t checksum = 0;

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < orders.size(); i++) {
        Order order = orders[i]; // Copy to avoid const issues
        int strategy_idx = assignments[i];
        uint64_t result = processors[strategy_idx]->process(order);
        checksum += result;
    }

    auto end = std::chrono::high_resolution_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    return checksum;
}

// Benchmark non-virtual implementation
uint64_t benchmark_non_virtual(const std::vector<Order>& orders,
                              const std::vector<int>& assignments,
                              std::chrono::nanoseconds& elapsed) {
    
    StrategyA_NV strategy_a;
    StrategyB_NV strategy_b;
    
    volatile uint64_t checksum = 0;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < orders.size(); i++) {
        Order order = orders[i]; // Copy to avoid const issues
        uint64_t result;
        
        if (assignments[i] == 0) {
            result = strategy_a.run(order);
        } else {
            result = strategy_b.run(order);
        }
        
        checksum += result;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    return checksum;
}

void warmup(int warmup_orders = 1000000) {
    auto orders = generate_orders(warmup_orders);
    auto assignments = generate_assignment_pattern(Pattern::MIXED_RANDOM, warmup_orders);
    
    std::chrono::nanoseconds dummy_elapsed;
    benchmark_virtual(orders, assignments, dummy_elapsed);
    benchmark_non_virtual(orders, assignments, dummy_elapsed);
}

const char* pattern_to_string(Pattern p) {
    switch (p) {
        case Pattern::HOMOGENEOUS: return "homogeneous";
        case Pattern::MIXED_RANDOM: return "mixed_random";
        case Pattern::BURSTY: return "bursty";
        default: return "unknown";
    }
}

int main() {
    const int NUM_ORDERS = 500000; // Chosen for ~0.5-2 second runs
    const int NUM_REPEATS = 10;

    std::ofstream csv_file("results.csv");
    std::cout << "# HFT Order Processing Performance Benchmark" << std::endl;
    std::cout << "# Orders per run: " << NUM_ORDERS << std::endl;
    std::cout << "# Repeats per configuration: " << NUM_REPEATS << std::endl;
    std::cout << "# " << std::endl;
    
    // Warmup
    std::cout << "# Performing warmup..." << std::endl;
    warmup();
    std::cout << "# Warmup completed!" << std::endl;
    
    // CSV
    std::cout << "pattern,impl,repeat,orders,elapsed_ns,ops_per_sec,checksum" << std::endl;
    csv_file << "pattern,impl,repeat,orders,elapsed_ns,ops_per_sec,checksum" << std::endl;
    
    std::vector<Pattern> patterns = {Pattern::HOMOGENEOUS, Pattern::MIXED_RANDOM, Pattern::BURSTY};
    
    for (Pattern pattern : patterns) {
        auto orders = generate_orders(NUM_ORDERS);
        auto assignments = generate_assignment_pattern(pattern, NUM_ORDERS);
        // Virtual implementation
        for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
            // Reset tables
            memset(order_book_table, 0, sizeof(order_book_table));
            memset(position_table, 0, sizeof(position_table));
            
            std::chrono::nanoseconds elapsed;
            uint64_t checksum = benchmark_virtual(orders, assignments, elapsed);
            
            double ops_per_sec = static_cast<double>(NUM_ORDERS) * 1e9 / elapsed.count();
            
            std::string output = std::string(pattern_to_string(pattern)) + ",virtual," + std::to_string(repeat) + "," +
                               std::to_string(NUM_ORDERS) + "," + std::to_string(elapsed.count()) + "," +
                               std::to_string(static_cast<long long>(ops_per_sec)) + "," + 
                               std::to_string(checksum);
            
            std::cout << output << std::endl;
            csv_file << output << std::endl;
        }
        
        // Non-virtual implementation
        for (int repeat = 0; repeat < NUM_REPEATS; repeat++) {
            // Reset tables for consistent state
            memset(order_book_table, 0, sizeof(order_book_table));
            memset(position_table, 0, sizeof(position_table));
            
            std::chrono::nanoseconds elapsed;
            uint64_t checksum = benchmark_non_virtual(orders, assignments, elapsed);
            
            double ops_per_sec = static_cast<double>(NUM_ORDERS) * 1e9 / elapsed.count();
            
            std::string output = std::string(pattern_to_string(pattern)) + ",non_virtual," + std::to_string(repeat) + "," +
                               std::to_string(NUM_ORDERS) + "," + std::to_string(elapsed.count()) + "," +
                               std::to_string(static_cast<long long>(ops_per_sec)) + "," + 
                               std::to_string(checksum);
            
            std::cout << output << std::endl;
            csv_file << output << std::endl;
        }
    }
    
    csv_file.close();
    std::cout << "# Results saved to results.csv" << std::endl;
    
    return 0;
}