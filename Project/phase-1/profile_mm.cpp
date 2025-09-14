// profile_mm.cpp
// Focused profiling / comparison harness for matrix-matrix kernels (Part 2 Tasks 5 & 6).
// Provides timing (chrono) plus optional simple flop rate estimates and supports
// multiple kernels: naive, transposed-B, loop-reordered, and tiled.

#include "kernels.hpp"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <random>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#include <malloc.h>
#endif

namespace {

struct Timer { using clk=std::chrono::steady_clock; clk::time_point t0; void start(){ t0=clk::now(); } double stop_ms() const { auto t1=clk::now(); return std::chrono::duration<double,std::milli>(t1-t0).count(); } };

void* alloc_aligned(size_t bytes, size_t align=64) {
#ifdef _WIN32
    return _aligned_malloc(bytes, align);
#else
    void* p=nullptr; if (posix_memalign(&p, align, bytes)!=0) return nullptr; return p;
#endif
}
void free_aligned(void* p) {
#ifdef _WIN32
    _aligned_free(p);
#else
    free(p);
#endif
}

struct KernelEntry {
    const char* name;
    void (*fn_basic)(const double*, int, int, const double*, int, int, double*);
    void (*fn_tiled)(const double*, int, int, const double*, int, int, double*, int);
};

double checksum(const double* data, size_t n) {
    double s=0; for(size_t i=0;i<n;++i) s+=data[i]; return s; }

} // namespace

static void profile_mm(int n, int repeats, int block_size) {
    std::mt19937_64 rng(42);
    std::uniform_real_distribution<double> U(-1.0,1.0);

    size_t bytesA = size_t(n)*n*sizeof(double);
    size_t bytesB = size_t(n)*n*sizeof(double);
    size_t bytesBT= size_t(n)*n*sizeof(double);
    size_t bytesC = size_t(n)*n*sizeof(double);

    double* A  = static_cast<double*>(alloc_aligned(bytesA));
    double* B  = static_cast<double*>(alloc_aligned(bytesB));
    double* BT = static_cast<double*>(alloc_aligned(bytesBT));
    double* C  = static_cast<double*>(alloc_aligned(bytesC));
    if(!A||!B||!BT||!C){ std::cerr << "[Error] Allocation failed" << std::endl; return; }
    for(size_t i=0;i<size_t(n)*n;++i){ A[i]=U(rng); B[i]=U(rng); }
    // build BT (transpose of B)
    for(int i=0;i<n;++i) for(int j=0;j<n;++j) BT[j* n + i] = B[i * n + j];

    std::vector<KernelEntry> kernels;
    kernels.push_back({"mm_naive",           multiply_mm_naive, nullptr});
    kernels.push_back({"mm_transposed_b",    multiply_mm_transposed_b, nullptr});
    kernels.push_back({"mm_loop_reordered",  multiply_mm_loop_reordered, nullptr});
    kernels.push_back({"mm_tiled",           nullptr, multiply_mm_tiled});

    std::cout << "kernel,n,repeats,mean_ms,stdev_ms,GFLOP/s (approx)" << std::endl;
    const double flops_muladd = 2.0 * double(n) * n * n; // each C(i,j) has n mul+add

    for(const auto& ke : kernels) {
        std::vector<double> samples;
        samples.reserve(repeats);
        for(int r=0;r<repeats;++r){
            std::memset(C, 0, bytesC);
            Timer t; t.start();
            if(ke.fn_basic) {
                if(ke.fn_basic == multiply_mm_transposed_b) {
                    ke.fn_basic(A, n, n, BT, n, n, C); // expects B^T
                } else {
                    ke.fn_basic(A, n, n, B, n, n, C);
                }
            } else if(ke.fn_tiled) {
                ke.fn_tiled(A, n, n, B, n, n, C, block_size);
            }
            double ms = t.stop_ms();
            // simple sink
            volatile double cs = checksum(C, size_t(n)*n); (void)cs;
            samples.push_back(ms);
        }
        double mean=0, stdev=0; if(!samples.empty()){
            for(double v: samples) mean+=v; mean /= samples.size();
            for(double v: samples) stdev += (v-mean)*(v-mean); stdev = std::sqrt(stdev / samples.size());
        }
        double gflops = (flops_muladd / (mean/1000.0)) / 1e9;
        std::cout << ke.name << "," << n << "," << repeats << "," << mean << "," << stdev << "," << gflops << std::endl;
    }

    free_aligned(A); free_aligned(B); free_aligned(BT); free_aligned(C);
}

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif
    int n = 512;
    int repeats = 5;
    int block = 64;
    for(int i=1;i<argc;++i){ std::string arg=argv[i];
        if(arg=="-n" && i+1<argc) n=std::stoi(argv[++i]);
        else if(arg=="-r" && i+1<argc) repeats=std::stoi(argv[++i]);
        else if(arg=="-b" && i+1<argc) block=std::stoi(argv[++i]);
        else if(arg=="-h"||arg=="--help"){
            std::cout << "Usage: profile_mm [-n size] [-r repeats] [-b block]\n";
            return 0;
        }
    }
    profile_mm(n, repeats, block);
    return 0;
}
