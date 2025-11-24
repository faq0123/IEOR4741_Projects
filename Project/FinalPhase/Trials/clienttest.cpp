#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <omp.h>
#include <chrono>

#ifdef __AVX2__
#include <immintrin.h>
#endif

using namespace std;

const int PORT = 12345;
const char* SERVER_IP = "127.0.0.1";
const int MODULO = 997;

// 性能计数器
struct PerfStats {
    long long io_time_ns = 0;
    long long parse_time_ns = 0;
    long long compute_time_ns = 0;
    long long send_time_ns = 0;
    int count = 0;
    
    void print() {
        if (count == 0) return;
        cerr << "\n=== Performance Stats (avg over " << count << " challenges) ===" << endl;
        cerr << "  I/O Read:    " << io_time_ns / count / 1e6 << " ms" << endl;
        cerr << "  Parse:       " << parse_time_ns / count / 1e6 << " ms" << endl;
        cerr << "  Compute:     " << compute_time_ns / count / 1e6 << " ms" << endl;
        cerr << "  Send:        " << send_time_ns / count / 1e6 << " ms" << endl;
        cerr << "  TOTAL:       " << (io_time_ns + parse_time_ns + compute_time_ns + send_time_ns) / count / 1e6 << " ms" << endl;
        
        // 瓶颈分析
        long long total = io_time_ns + parse_time_ns + compute_time_ns + send_time_ns;
        cerr << "\nBottleneck Analysis:" << endl;
        cerr << "  I/O:     " << (io_time_ns * 100 / total) << "%" << endl;
        cerr << "  Parse:   " << (parse_time_ns * 100 / total) << "%" << endl;
        cerr << "  Compute: " << (compute_time_ns * 100 / total) << "%" << endl;
        cerr << "  Send:    " << (send_time_ns * 100 / total) << "%" << endl;
        cerr << "========================================\n" << endl;
    }
};

class Timer {
public:
    void start() { t1 = chrono::high_resolution_clock::now(); }
    long long elapsed_ns() {
        auto t2 = chrono::high_resolution_clock::now();
        return chrono::duration_cast<chrono::nanoseconds>(t2 - t1).count();
    }
private:
    chrono::high_resolution_clock::time_point t1;
};

inline int fasterStoi(const char*& p) {
    int x = 0;
    while (*p == ' ' || *p == '\t') p++;
    while (*p >= '0' && *p <= '9') {
        x = x * 10 + (*p - '0');
        p++;
    }
    while (*p == ' ' || *p == '\t') p++;
    return x;
}

class SocketLineReader {
public:
    explicit SocketLineReader(int fd) : sockfd(fd) {
        buffer.reserve(10 * 1024 * 1024);
    }
    
    bool readLine(string& line) {
        while (true) {
            size_t pos = buffer.find('\n');
            if (pos != string::npos) {
                line = buffer.substr(0, pos);
                buffer.erase(0, pos + 1);
                return true;
            }
            char temp[65536];
            int n = recv(sockfd, temp, sizeof(temp), 0);
            if (n <= 0) return false;
            buffer.append(temp, n);
        }
    }
    
private:
    int sockfd;
    string buffer;
};

int compute_trace_mod(const vector<int>& A, const vector<int>& B, int N) {
    vector<int> BT(N * N);
    
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            BT[i * N + j] = B[j * N + i];
        }
    }
    
    long long trace = 0;
    
    #pragma omp parallel for reduction(+:trace) schedule(static)
    for (int i = 0; i < N; i++) {
        long long row_sum = 0;
        int k = 0;
        
        #ifdef __AVX2__
        __m256i sum_vec = _mm256_setzero_si256();
        for (; k + 7 < N; k += 8) {
            __m256i a_vec = _mm256_loadu_si256((const __m256i*)&A[i * N + k]);
            __m256i b_vec = _mm256_loadu_si256((const __m256i*)&BT[i * N + k]);
            __m256i prod = _mm256_mullo_epi32(a_vec, b_vec);
            sum_vec = _mm256_add_epi32(sum_vec, prod);
        }
        
        int tmp[8];
        _mm256_storeu_si256((__m256i*)tmp, sum_vec);
        for (int j = 0; j < 8; j++) {
            row_sum += tmp[j];
        }
        #endif
        
        for (; k + 3 < N; k += 4) {
            row_sum += (long long)A[i * N + k] * BT[i * N + k];
            row_sum += (long long)A[i * N + k + 1] * BT[i * N + k + 1];
            row_sum += (long long)A[i * N + k + 2] * BT[i * N + k + 2];
            row_sum += (long long)A[i * N + k + 3] * BT[i * N + k + 3];
        }
        
        for (; k < N; k++) {
            row_sum += (long long)A[i * N + k] * BT[i * N + k];
        }
        
        trace += row_sum;
    }
    
    int ans = trace % MODULO;
    return ans < 0 ? ans + MODULO : ans;
}

int main(int argc, char* argv[]) {
    // 检测AVX2
    #ifdef __AVX2__
    cerr << "✓ AVX2 ENABLED" << endl;
    #else
    cerr << "✗ AVX2 DISABLED (compile with -mavx2)" << endl;
    #endif
    
    // 显示CPU核心数
    int max_threads = omp_get_max_threads();
    cerr << "CPU Cores available: " << max_threads << endl;
    
    // 可选：限制线程数以避免和server竞争
    if (argc > 2) {
        int threads = atoi(argv[2]);
        omp_set_num_threads(threads);
        cerr << "Limiting to " << threads << " threads" << endl;
    } else {
        omp_set_num_threads(max_threads);
    }
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cerr << "Socket creation failed" << endl;
        return 1;
    }
    
    int bufsize = 512 * 1024;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
    
    int flag = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);
    
    if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Connection failed" << endl;
        close(sockfd);
        return 1;
    }
    
    string groupName = "MyHFTGroup\n";
    if (argc > 1) groupName = string(argv[1]) + "\n";
    send(sockfd, groupName.c_str(), groupName.size(), 0);
    
    SocketLineReader reader(sockfd);
    
    vector<int> A, B;
    A.reserve(1024 * 1024);
    B.reserve(1024 * 1024);
    
    string line;
    line.reserve(10 * 1024 * 1024);
    
    Timer timer;
    PerfStats stats;
    
    while (true) {
        // I/O: 读取challenge信息
        timer.start();
        if (!reader.readLine(line)) break;
        const char* p1 = line.c_str();
        int challenge_id = fasterStoi(p1);
        
        if (!reader.readLine(line)) break;
        const char* p2 = line.c_str();
        int N = fasterStoi(p2);
        
        A.resize(N * N);
        B.resize(N * N);
        
        if (!reader.readLine(line)) break;
        string line_a = line;
        if (!reader.readLine(line)) break;
        string line_b = line;
        stats.io_time_ns += timer.elapsed_ns();
        
        // Parse: 解析矩阵
        timer.start();
        {
            const char* p = line_a.c_str();
            for (int i = 0; i < N * N; i++) A[i] = fasterStoi(p);
        }
        {
            const char* p = line_b.c_str();
            for (int i = 0; i < N * N; i++) B[i] = fasterStoi(p);
        }
        stats.parse_time_ns += timer.elapsed_ns();
        
        // Compute: 计算trace
        timer.start();
        int answer = compute_trace_mod(A, B, N);
        stats.compute_time_ns += timer.elapsed_ns();
        
        // Send: 发送结果
        timer.start();
        string ansStr = to_string(answer) + "\n";
        send(sockfd, ansStr.c_str(), ansStr.size(), 0);
        stats.send_time_ns += timer.elapsed_ns();
        
        stats.count++;
        
        // 每10次打印一次
        if (stats.count % 10 == 0) {
            stats.print();
            stats = PerfStats(); // 重置
        }
    }
    
    if (stats.count > 0) stats.print();
    
    close(sockfd);
    return 0;
}