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

#ifdef __AVX2__
#include <immintrin.h>
#endif

using namespace std;

const int PORT = 12345;
const char* SERVER_IP = "127.0.0.1";
const int MODULO = 997;

// Transform string to integer quickly
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

// Optimized trace computation
int compute_trace_mod(const vector<int>& A, const vector<int>& B, int N) {
    // Transpose B matrix
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
        // AVX2 vectorization
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

        // Loop unrolling
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
    // Set OpenMP thread count
    int num_threads = omp_get_max_threads();
    omp_set_num_threads(num_threads);
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cerr << "Socket creation failed" << endl;
        return 1;
    }
    
    // Set socket options
    int bufsize = 512 * 1024;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
    
    int flag = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    
    // Use traditional initialization method
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
    
    while (true) {
        if (!reader.readLine(line)) break;
        const char* p1 = line.c_str();
        int challenge_id = fasterStoi(p1);
        
        if (!reader.readLine(line)) break;
        const char* p2 = line.c_str();
        int N = fasterStoi(p2);
        
        A.resize(N * N);
        B.resize(N * N);
        
        if (!reader.readLine(line)) break;
        {
            const char* p = line.c_str();
            for (int i = 0; i < N * N; i++) {
                A[i] = fasterStoi(p);
            }
        }
        
        if (!reader.readLine(line)) break;
        {
            const char* p = line.c_str();
            for (int i = 0; i < N * N; i++) {
                B[i] = fasterStoi(p);
            }
        }
        
        int answer = compute_trace_mod(A, B, N);
        
        string ansStr = to_string(answer) + "\n";
        send(sockfd, ansStr.c_str(), ansStr.size(), 0);
    }
    
    close(sockfd);
    return 0;
}