CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall
TARGET = trading_sim
SRC = main.cpp market_snapshot.cpp order_manager.cpp

# default target
all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

# run: input sample_feed.txt, output to output.log
run: $(TARGET)
	./$(TARGET) sample_feed.txt > output_log.txt 2>&1
	@echo "Program finished. Logs saved to output_log.txt"

# clean
clean:
	rm -f $(TARGET)
