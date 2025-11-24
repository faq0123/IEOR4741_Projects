# Makefile for HFT System

CXX = g++
CXXFLAGS = -std=c++17 -pthread -O3 -Wall
TARGET = hft_system
SOURCE = hft_system.cpp

.PHONY: all clean run benchmark visualize help

all: $(TARGET)

$(TARGET): $(SOURCE)
	@echo "Compiling HFT system..."
	$(CXX) $(CXXFLAGS) $(SOURCE) -o $(TARGET)
	@echo "✓ Build complete!"

run: $(TARGET)
	@echo "Running HFT system..."
	./$(TARGET)

benchmark: $(TARGET)
	@echo "Running comprehensive benchmarks..."
	@chmod +x benchmark.sh
	./benchmark.sh

visualize:
	@echo "Generating performance visualizations..."
	@chmod +x visualize.py
	python3 visualize.py

full: clean all benchmark visualize
	@echo ""
	@echo "=========================================="
	@echo "Full benchmark and analysis complete!"
	@echo "Check benchmark_results/ for outputs"
	@echo "=========================================="

clean:
	@echo "Cleaning up..."
	rm -f $(TARGET)
	rm -f orders.csv performance.csv
	rm -rf benchmark_results
	@echo "✓ Clean complete!"

help:
	@echo "HFT System - Available targets:"
	@echo "  make          - Compile the system"
	@echo "  make run      - Run single simulation"
	@echo "  make benchmark- Run full benchmark suite"
	@echo "  make visualize- Generate performance charts"
	@echo "  make full     - Clean + build + benchmark + visualize"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make help     - Show this help"