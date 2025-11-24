#include <iostream>
#include <vector>
#include <string>  
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <omp.h>

using namespace std;

const int PORT = 12345;
const char* SERVER_IP = "127.0.0.1";
const int MODULO = 997;

// Fast atoi
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
    explicit SocketLineReader(int fd) : sockfd(fd) {}
    bool readLine(string& line) {
        while (true) {
            size_t pos = buffer.find('\n');
            if (pos != string::npos) {
                line = buffer.substr(0, pos);
                buffer.erase(0, pos + 1);
                return true;
            }
            char temp[4096];
            int n = recv(sockfd, temp, sizeof(temp), 0);
            if (n <= 0) return false;
            buffer.append(temp, n);
        }
    }
private:
    int sockfd;
    string buffer;
};

// Parallel trace
int compute_trace_mod(const vector<int>& A, const vector<int>& B, int N) {
    long long trace = 0;

    #pragma omp parallel for reduction(+:trace) schedule(static)
    for (int i = 0; i < N; i++) {
        for (int k = 0; k < N; k++) {
            trace += (long long)A[i * N + k] * (long long)B[k * N + i];
        }
    }

    int ans = trace % MODULO;
    return ans < 0 ? ans + MODULO : ans;
}

int main(int argc, char* argv[]) {

    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return 1;

    // Connect
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);

    if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        return 1;
    }

    // Send group name
    string groupName = "MyHFTGroup\n";
    if (argc > 1) groupName = string(argv[1]) + "\n";
    send(sockfd, groupName.c_str(), groupName.size(), 0);

    SocketLineReader reader(sockfd);

    while (true) {
        string line;

        // Read challenge id
        if (!reader.readLine(line)) break;
        const char* p1 = line.c_str();
        int challenge_id = fasterStoi(p1);

        // Read N
        if (!reader.readLine(line)) break;
        const char* p2 = line.c_str();
        int N = fasterStoi(p2);

        // Read A
        if (!reader.readLine(line)) break;
        vector<int> A(N * N);
        {
            const char* p = line.c_str();
            for (int i = 0; i < N * N; i++) A[i] = fasterStoi(p);
        }

        // Read B
        if (!reader.readLine(line)) break;
        vector<int> B(N * N);
        {
            const char* p = line.c_str();
            for (int i = 0; i < N * N; i++) B[i] = fasterStoi(p);
        }

        // Compute
        int answer = compute_trace_mod(A, B, N);

        // Send result
        string ansStr = to_string(answer) + "\n";
        send(sockfd, ansStr.c_str(), ansStr.size(), 0);
    }

    close(sockfd);
    return 0;
}
