# Demo Script Usage Guide

## Quick Start for Video Recording

I created an automated demo script (`demo_script.sh`) that will walk through all the important aspects of your HFT system.

### How to Use During Video Recording

1. **Start your screen recording software** (QuickTime, OBS, etc.)

2. **Open terminal and navigate to project:**
   ```bash
   cd /Users/wdecddr/Desktop/cpp_programming/phrase4
   ```

3. **Run the demo script:**
   ```bash
   ./demo_script.sh
   ```

4. **Follow the prompts:**
   - The script will pause at each section and wait for you to press Enter
   - Read the messages displayed on screen
   - Press Enter when ready to continue to the next section

### What the Script Demonstrates

The script automatically shows:

1. **Project Structure** (Section 1)
   - Directory organization
   - File layout

2. **Key Code Features** (Section 2)
   - Template-based Order structure
   - Custom MemoryPool implementation
   - Cache-aligned MarketData

3. **Build Process** (Section 3)
   - Clean build from scratch
   - CMake configuration
   - Compilation with optimizations

4. **Main Simulation** (Section 4)
   - Runs 3 test scenarios
   - Shows order matching in action
   - Displays trade statistics

5. **Performance Benchmarks** (Section 5)
   - Comprehensive latency tests
   - Throughput measurements
   - Statistical analysis (P50, P95, P99)

6. **Code Verification** (Section 6)
   - Searches for advanced C++ features in code
   - Shows templates, smart pointers, move semantics, etc.

7. **Trade Logs** (Section 7)
   - Displays sample trade output
   - Shows file logging functionality

8. **Summary** (Section 8)
   - Performance achievements
   - Features implemented
   - Final statistics

### Tips for Recording

- **Before starting:**
  - Close unnecessary applications
  - Set terminal to full screen or large window
  - Use a readable font size (14-16pt recommended)
  - Test your microphone if doing voice-over

- **During recording:**
  - Speak clearly when explaining each section
  - Don't rush - the script pauses for you
  - Point out key numbers in the performance output
  - Mention the advanced C++ features as they appear

- **If you make a mistake:**
  - Just run the script again
  - It cleans up and rebuilds everything automatically

### Manual Mode (If Needed)

If you prefer more control, you can run commands manually:

```bash
# 1. Show structure
tree -L 2 -I 'build|bin' .

# 2. Show key files
cat include/Order.hpp
cat include/OrderBook.hpp

# 3. Build
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8
cd ..

# 4. Run simulation
./bin/hft_app

# 5. Run benchmarks
./bin/test_latency

# 6. Check logs
head trades.log
```

### Expected Duration

The full automated demo takes approximately **3-5 minutes** to complete.

### Troubleshooting

If the script fails:
- Make sure you're in the `phrase4` directory
- Ensure the script is executable: `chmod +x demo_script.sh`
- Check that CMake and a C++ compiler are installed
- Run `make clean` before rerunning if needed

### After Recording

Stop your recording and you'll have a complete demonstration covering:
- Code quality and organization
- Build process
- Functionality
- Performance metrics
- Advanced C++ implementation

Good luck with your video recording!
