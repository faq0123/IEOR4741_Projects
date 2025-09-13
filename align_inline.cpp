#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <cstring>

void multiply_row(const double* A, int rows, int cols, const double* x, double* y) {
    if (!A || !x || !y || rows <= 0 || cols <= 0) return;
    for (int i = 0; i < rows; i++) {
        double sum = 0.0;
        for (int j = 0; j < cols; j++) sum += A[i * cols + j] * x[j];
        y[i] = sum;
    }
}

void multiply_col(const double* A, int rows, int cols, const double* x, double* y) {
    if (!A || !x || !y || rows <= 0 || cols <= 0) return;
    for (int i = 0; i < rows; i++) y[i] = 0.0;
    for (int j = 0; j < cols; j++) {
        const double xj = x[j];
        for (int i = 0; i < rows; i++) y[i] += A[j * rows + i] * xj;
    }
}

void multiply_naive(const double* A, int rA, int cA,
                    const double* B, int rB, int cB, double* C) {
    if (!A || !B || !C || rA <= 0 || cA <= 0 || rB <= 0 || cB <= 0 || cA != rB) return;
    std::memset(C, 0, sizeof(double) * rA * cB);
    for (int i = 0; i < rA; i++) {
        for (int k = 0; k < cA; k++) {
            double a = A[i * cA + k];
            for (int j = 0; j < cB; j++) C[i * cB + j] += a * B[k * cB + j];
        }
    }
}

void multiply_transposed(const double* A, int rA, int cA,
                         const double* BT, int rB, int cB, double* C) {
    if (!A || !BT || !C || rA <= 0 || cA <= 0 || rB <= 0 || cB <= 0 || rB != cA) return;
    for (int i = 0; i < rA; i++) {
        for (int j = 0; j < cB; j++) {
            double sum = 0.0;
            for (int k = 0; k < cA; k++) sum += A[i * cA + k] * BT[j * rB + k];
            C[i * cB + j] = sum;
        }
    }
}

void transpose(const double* B, int rB, int cB, double* BT) {
    for (int i = 0; i < rB; i++)
        for (int j = 0; j < cB; j++) BT[j * rB + i] = B[i * cB + j];
}



#ifndef Rdim
#define Rdim 1024
#endif
#ifndef Kdim
#define Kdim 1024
#endif
#ifndef Cdim
#define Cdim 1024
#endif

static void* alloc_aligned(std::size_t bytes, std::size_t align = 64) {
#if defined(_MSC_VER)
    return _aligned_malloc(bytes, align);
#elif (_POSIX_C_SOURCE >= 200112L) || defined(__APPLE__)
    void* p=nullptr; if (posix_memalign(&p, align, bytes)!=0) return nullptr; return p;
#else
    return std::aligned_alloc(align, ((bytes+align-1)/align)*align);
#endif
}
static void free_aligned(void* p) {
#if defined(_MSC_VER)
    _aligned_free(p);
#else
    std::free(p);
#endif
}

static void fill_rand(double* a, std::size_t n, unsigned seed) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> U(-1.0,1.0);
    for (std::size_t i=0;i<n;++i) a[i]=U(rng);
}

template<typename F>
static double bench_ms(F&& fn, int runs=3) {
    using clk=std::chrono::high_resolution_clock;
    double best=1e300;
    for(int r=0;r<runs;++r){ auto t0=clk::now(); fn(); auto t1=clk::now();
        double ms=std::chrono::duration<double,std::milli>(t1-t0).count();
        if(ms<best) best=ms;
    }
    return best;
}

#if defined(_MSC_VER)
  #define NOINLINE __declspec(noinline)
  #define ALWAYS_INLINE __forceinline
#else
  #define NOINLINE __attribute__((noinline))
  #define ALWAYS_INLINE __attribute__((always_inline)) inline
#endif

ALWAYS_INLINE static double dot_inline(const double* a, const double* b, int n){
    double s=0.0; for(int i=0;i<n;++i) s += a[i]*b[i]; return s;
}
NOINLINE static double dot_noinline(const double* a, const double* b, int n){
    double s=0.0; for(int i=0;i<n;++i) s += a[i]*b[i]; return s;
}

static void mm_bt_inline(const double* A,int rA,int cA,const double* BT,int rB,int cB,double* Cmat){
    for(int i=0;i<rA;++i){
        const double* Ai = A + std::size_t(i)*cA; double* Ci = Cmat + std::size_t(i)*cB;
        for(int j=0;j<cB;++j){ const double* BTj = BT + std::size_t(j)*rB; Ci[j] = dot_inline(Ai, BTj, cA); }
    }
}
static void mm_bt_noinline(const double* A,int rA,int cA,const double* BT,int rB,int cB,double* Cmat){
    for(int i=0;i<rA;++i){
        const double* Ai = A + std::size_t(i)*cA; double* Ci = Cmat + std::size_t(i)*cB;
        for(int j=0;j<cB;++j){ const double* BTj = BT + std::size_t(j)*rB; Ci[j] = dot_noinline(Ai, BTj, cA); }
    }
}

int main(int argc, char** argv){
    bool aligned = true;
    if (argc>1 && std::string(argv[1])=="--unaligned") aligned = false;

    const std::size_t bytesA = std::size_t(Rdim)*Kdim*sizeof(double);
    const std::size_t bytesB = std::size_t(Kdim)*Cdim*sizeof(double);
    const std::size_t bytesBT= std::size_t(Cdim)*Kdim*sizeof(double);
    const std::size_t bytesC = std::size_t(Rdim)*Cdim*sizeof(double);
    const std::size_t bytesx = std::size_t(Kdim)*sizeof(double);
    const std::size_t bytesy = std::size_t(Rdim)*sizeof(double);

    auto alloc = [&](std::size_t nbytes){ return aligned? alloc_aligned(nbytes,64) : std::malloc(nbytes); };
    auto dealloc = [&](void* p){ aligned? free_aligned(p) : std::free(p); };

    double *A=(double*)alloc(bytesA), *B=(double*)alloc(bytesB), *BT=(double*)alloc(bytesBT);
    double *C1=(double*)alloc(bytesC), *C2=(double*)alloc(bytesC), *x=(double*)alloc(bytesx), *y=(double*)alloc(bytesy);
    if(!A||!B||!BT||!C1||!C2||!x||!y){ return 1; }

    fill_rand(A, std::size_t(Rdim)*Kdim, 1);
    fill_rand(B, std::size_t(Kdim)*Cdim, 2);
    fill_rand(x, std::size_t(Kdim),     3);
    transpose(B, Kdim, Cdim, BT);

    multiply_row(A, Rdim, Kdim, x, y);
    multiply_naive(A, Rdim, Kdim, B, Kdim, Cdim, C1);
    multiply_transposed(A, Rdim, Kdim, BT, Kdim, Cdim, C2);

    std::cout<<(aligned? "[aligned 64B]":"[unaligned]")<<"  R="<<Rdim<<" K="<<Kdim<<" C="<<Cdim<<"\n";

    double t_mv   = bench_ms([&]{ multiply_row(A,Rdim,Kdim,x,y); }, 3);
    double t_mm0  = bench_ms([&]{ multiply_naive(A,Rdim,Kdim,B,Kdim,Cdim,C1); }, 3);
    double t_bt   = bench_ms([&]{ multiply_transposed(A,Rdim,Kdim,BT,Kdim,Cdim,C1); }, 3);
    double t_inl  = bench_ms([&]{ mm_bt_inline(A,Rdim,Kdim,BT,Kdim,Cdim,C1); }, 3);
    double t_noin = bench_ms([&]{ mm_bt_noinline(A,Rdim,Kdim,BT,Kdim,Cdim,C2); }, 3);

    std::cout<<"MV row-major: "<<t_mv<<" ms\n";
    std::cout<<"MM naive: "<<t_mm0<<" ms\n";
    std::cout<<"MM with BT: "<<t_bt<<" ms\n";
    std::cout<<"MM with BT (inline): "<<t_inl<<" ms\n";
    std::cout<<"MM with BT (noinline): "<<t_noin<<" ms\n";

    dealloc(A); dealloc(B); dealloc(BT); dealloc(C1); dealloc(C2); dealloc(x); dealloc(y);
    return 0;
}
