#!/bin/bash

echo "========== HFT System Benchmark =========="
echo "Running multiple configurations..."
echo ""

# Compile
echo "Compiling..."
g++ -std=c++17 -pthread -O3 hft_system.cpp -o hft_system
if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi
echo "Compilation successful!"
echo ""

# Create benchmark results directory
mkdir -p benchmark_results

# Test configurations
configs=(
    "1000:3"
    "10000:3"
    "50000:3"
    "10000:1"
    "10000:5"
)

for config in "${configs[@]}"; do
    IFS=':' read -r rate threads <<< "$config"
    
    echo "=========================================="
    echo "Testing: Rate=$rate/s, Threads=$threads"
    echo "=========================================="
    
    # Create config file
    cat > config.txt <<CONF
market_data_rate=$rate
strategy_threshold=2.0
simulation_seconds=10
verbose_logging=false
num_strategy_threads=$threads
CONF
    
    # Run simulation
    ./hft_system
    
    # Save results
    mv orders.csv "benchmark_results/orders_${rate}_${threads}.csv"
    mv performance.csv "benchmark_results/performance_${rate}_${threads}.csv"
    
    echo ""
    sleep 1
done

echo "=========================================="
echo "Benchmark complete!"
echo "Results saved in benchmark_results/"
echo "=========================================="
