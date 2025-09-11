#ifndef DLP_MONITOR_H
#define DLP_MONITOR_H

#include <string>
#include <vector>
#include <regex>
#include <unordered_set>
#include <functional>
#include <atomic>
#include <memory>



struct DLPPolicy {
    std::string name;
    std::vector<std::string> file_extensions;
    std::vector<std::regex> content_patterns;
    std::vector<std::string> restricted_paths;
    bool block_transfer;
};

struct DLPEvent {
    std::string timestamp;
    std::string type;  // "file_access", "data_transfer", "clipboard"
    std::string file_path;
    std::string destination;
    std::string user;
    std::string policy_violated;
    bool blocked;
};

class DLPMonitor {
public:
    DLPMonitor();
    ~DLPMonitor();

    void addPolicy(const DLPPolicy& policy);
    void removePolicy(const std::string& policy_name);
    void startMonitoring();
    void stopMonitoring();
    void setCallback(std::function<void(const DLPEvent&)> callback);

private:
    void monitorFileSystem();
    void monitorClipboard();
    void monitorNetworkTransfers();
    void fallbackNetworkMonitoring();
    void monitorNetworkConnections();
    void monitorSuspiciousProcesses();
    void monitorFileTransfers();
    void checkPortAgainstPolicies(int port);
    void checkDestinationAgainstPolicies(const std::string& destination);
    std::string hexToIp(const std::string& hex_addr);
    static void handleNetworkEvent(void* cb_cookie, void* data, int data_size);
    void checkNetworkTransfer(void* event_data);
    bool checkFileAgainstPolicies(const std::string& file_path);
    bool checkContentAgainstPolicies(const std::string& content);

    std::vector<DLPPolicy> policies_;
    std::unordered_set<std::string> monitored_paths_;
    std::atomic<bool> running_;
    std::function<void(const DLPEvent&)> callback_;
};

#endif // DLP_MONITOR_H
