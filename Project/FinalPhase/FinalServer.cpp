#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fstream>
#include <iomanip>
#include "json.hpp"

using namespace std;
using namespace std::chrono;
using json = nlohmann::json;

#define PORT 12345
#define BUFFER_SIZE 65536
#define RESULTS_FILE "/tmp/results.json"
#define MATRIX_SIZE 128
#define MODULO 997

struct ClientInfo {
    int socket;
    string name;
    thread clientThread;
};

vector<unique_ptr<ClientInfo>> clients;
mutex clientsMutex;

mutex logMutex;
atomic<int> challengeId{1};

struct ChallengeResult {
    string clientName;
    int answer;
    long long latency_ns;
    bool isCorrect;
    high_resolution_clock::time_point receiveTime;
};

struct ChallengeState {
    int id;
    vector<vector<int>> A;
    vector<vector<int>> B;
    int correctAnswer;
    high_resolution_clock::time_point startTime;
    vector<ChallengeResult> results;
};
mutex challengeMutex;
ChallengeState currentChallenge;

// 计算正确答案
int computeCorrectAnswer(const vector<vector<int>>& A, const vector<vector<int>>& B) {
    int N = A.size();
    long long trace = 0;
    
    for (int i = 0; i < N; i++) {
        for (int k = 0; k < N; k++) {
            trace += (long long)A[i][k] * (long long)B[k][i];
        }
    }
    
    int ans = trace % MODULO;
    return ans < 0 ? ans + MODULO : ans;
}

vector<vector<int>> generateMatrix(int N) {
    vector<vector<int>> mat(N, vector<int>(N));
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            mat[i][j] = rand() % MODULO;
    return mat;
}

void printLeaderboard() {
    lock_guard<mutex> lock(challengeMutex);
    
    if (currentChallenge.results.empty()) return;
    
    cout << "\n" << string(80, '=') << endl;
    cout << "🏆 CHALLENGE " << currentChallenge.id << " RESULTS" << endl;
    cout << string(80, '=') << endl;
    
    // 排序结果（按延迟）
    vector<ChallengeResult> sorted = currentChallenge.results;
    sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        if (a.isCorrect != b.isCorrect) return a.isCorrect > b.isCorrect;
        return a.latency_ns < b.latency_ns;
    });
    
    cout << left << setw(20) << "Client Name"
         << setw(12) << "Latency"
         << setw(10) << "Answer"
         << setw(10) << "Status" << endl;
    cout << string(80, '-') << endl;
    
    for (size_t i = 0; i < sorted.size(); i++) {
        const auto& r = sorted[i];
        
        // 计算延迟（毫秒和微秒）
        double latency_ms = r.latency_ns / 1e6;
        double latency_us = r.latency_ns / 1e3;
        
        string latency_str;
        if (latency_ms >= 1.0) {
            latency_str = to_string((int)latency_ms) + " ms";
        } else {
            latency_str = to_string((int)latency_us) + " μs";
        }
        
        // 奖牌
        string medal = "";
        if (r.isCorrect && i == 0) medal = "🥇 ";
        else if (r.isCorrect && i == 1) medal = "🥈 ";
        else if (r.isCorrect && i == 2) medal = "🥉 ";
        
        cout << medal << left << setw(20 - medal.size()) << r.clientName
             << setw(12) << latency_str
             << setw(10) << r.answer
             << (r.isCorrect ? "✅ CORRECT" : "❌ WRONG") << endl;
    }
    
    cout << string(80, '=') << endl;
    cout << "Correct Answer: " << currentChallenge.correctAnswer << endl;
    cout << string(80, '=') << "\n" << endl;
}

void logChallengeResult(int cid, const vector<ChallengeResult>& results) {
    json entry;
    entry["challenge_id"] = cid;

    string winner = "";
    long long bestLatency = LLONG_MAX;
    
    for (const auto& r : results) {
        if (r.isCorrect && r.latency_ns < bestLatency) {
            bestLatency = r.latency_ns;
            winner = r.clientName;
        }
    }
    
    entry["winner"] = winner;
    entry["winner_latency_ns"] = bestLatency;

    for (const auto& r : results) {
        entry["players"].push_back({
            {"name", r.clientName}, 
            {"latency_ns", r.latency_ns},
            {"latency_ms", r.latency_ns / 1e6},
            {"answer", r.answer},
            {"correct", r.isCorrect}
        });
    }

    json allResults;
    ifstream in(RESULTS_FILE);
    if (in) {
        try { in >> allResults; } catch (...) { allResults = json::array(); }
    }
    in.close();

    allResults.push_back(entry);

    ofstream out(RESULTS_FILE);
    out << allResults.dump(2);
}

void handleClient(ClientInfo* client) {
    char buffer[BUFFER_SIZE];

    // Receive group name
    memset(buffer, 0, BUFFER_SIZE);
    int bytesReceived = recv(client->socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytesReceived <= 0) {
        cerr << "❌ Failed to receive client name." << endl;
        close(client->socket);
        return;
    }

    // 去除换行符
    string name = string(buffer);
    name.erase(remove(name.begin(), name.end(), '\n'), name.end());
    name.erase(remove(name.begin(), name.end(), '\r'), name.end());
    client->name = name;
    
    cout << "👤 Registered client: [" << client->name << "]" << endl;

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        bytesReceived = recv(client->socket, buffer, BUFFER_SIZE - 1, 0);
        
        // 记录接收时间（尽可能精确）
        auto receiveTime = high_resolution_clock::now();
        
        if (bytesReceived <= 0) {
            cerr << "❌ Client " << client->name << " disconnected." << endl;
            break;
        }

        int receivedAnswer = atoi(buffer);

        lock_guard<mutex> lock(challengeMutex);
        
        // 计算延迟（纳秒级）
        long long latency_ns = duration_cast<nanoseconds>(
            receiveTime - currentChallenge.startTime
        ).count();
        
        bool isCorrect = (receivedAnswer == currentChallenge.correctAnswer);
        
        currentChallenge.results.push_back({
            client->name,
            receivedAnswer,
            latency_ns,
            isCorrect,
            receiveTime
        });

        // 实时显示
        double latency_ms = latency_ns / 1e6;
        cout << (isCorrect ? "✅" : "❌") << " " 
             << setw(20) << left << client->name 
             << " | Answer: " << setw(5) << receivedAnswer
             << " | Latency: " << fixed << setprecision(2) << latency_ms << " ms"
             << " (Challenge " << currentChallenge.id << ")" << endl;
    }

    close(client->socket);
}

void broadcastChallengeLoop() {
    while (true) {
        int cid = challengeId++;
        
        cout << "\n📢 Generating Challenge " << cid << "..." << endl;
        
        vector<vector<int>> A = generateMatrix(MATRIX_SIZE);
        vector<vector<int>> B = generateMatrix(MATRIX_SIZE);
        
        // 计算正确答案
        int correctAns = computeCorrectAnswer(A, B);
        
        // 记录开始时间
        auto startTime = high_resolution_clock::now();
        
        {
            lock_guard<mutex> lock(challengeMutex);
            currentChallenge = {cid, A, B, correctAns, startTime, {}};
        }

        // 序列化challenge
        stringstream ss;
        ss << cid << "\n";
        ss << MATRIX_SIZE << "\n";
        for (const auto& row : A)
            for (int val : row)
                ss << val << " ";
        ss << "\n";
        for (const auto& row : B)
            for (int val : row)
                ss << val << " ";
        ss << "\n";

        string payload = ss.str();
        
        cout << "📡 Broadcasting Challenge " << cid 
             << " (Size: " << payload.size() << " bytes)" << endl;

        // 广播给所有客户端
        {
            lock_guard<mutex> lock(clientsMutex);
            for (auto& client : clients) {
                send(client->socket, payload.c_str(), payload.size(), 0);
            }
            cout << "   Sent to " << clients.size() << " client(s)" << endl;
        }

        // 等待10秒收集结果
        this_thread::sleep_for(seconds(10));

        // 打印排行榜
        printLeaderboard();

        // 保存结果
        vector<ChallengeResult> snapshot;
        {
            lock_guard<mutex> lock(challengeMutex);
            snapshot = currentChallenge.results;
        }

        {
            lock_guard<mutex> lock(logMutex);
            logChallengeResult(cid, snapshot);
        }
    }
}

void startServer() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 优化Server的TCP设置
    int bufsize = 2 * 1024 * 1024;  // 2MB
    setsockopt(serverSocket, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    setsockopt(serverSocket, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (::bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 10) < 0) {
        perror("Listen failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    cout << "🚀 HFT Competition Server" << endl;
    cout << "   Listening on 127.0.0.1:" << PORT << endl;
    cout << "   Matrix Size: " << MATRIX_SIZE << "x" << MATRIX_SIZE << endl;
    cout << "   Challenge Interval: 10 seconds" << endl;
    cout << string(80, '=') << "\n" << endl;

    thread broadcaster(broadcastChallengeLoop);
    broadcaster.detach();

    while (true) {
        sockaddr_in clientAddr;
        memset(&clientAddr, 0, sizeof(clientAddr));
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            perror("Accept failed");
            continue;
        }
        
        // 优化客户端连接
        int flag = 1;
        setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

        cout << "📡 New connection from: " << inet_ntoa(clientAddr.sin_addr) << endl;

        auto client = make_unique<ClientInfo>();
        client->socket = clientSocket;
        ClientInfo* clientPtr = client.get();

        {
            lock_guard<mutex> lock(clientsMutex);
            clients.push_back(std::move(client));
        }

        thread t(handleClient, clientPtr);
        t.detach();
    }

    close(serverSocket);
}

int main() {
    srand(time(nullptr));
    startServer();
    return 0;
}