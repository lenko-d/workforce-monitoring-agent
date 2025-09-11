#include "dlp_monitor.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <arpa/inet.h>
#include <algorithm>


// Conditional includes for BCC/eBPF (if available)
#ifdef USE_BCC
#include <bcc/BPF.h>
#endif

namespace fs = std::filesystem;

DLPMonitor::DLPMonitor() : running_(false) {}

DLPMonitor::~DLPMonitor() {
    stopMonitoring();
}

void DLPMonitor::addPolicy(const DLPPolicy& policy) {
    policies_.push_back(policy);
    // Update monitored paths
    for (const auto& path : policy.restricted_paths) {
        monitored_paths_.insert(path);
    }
}

void DLPMonitor::removePolicy(const std::string& policy_name) {
    policies_.erase(
        std::remove_if(policies_.begin(), policies_.end(),
            [&policy_name](const DLPPolicy& p) { return p.name == policy_name; }),
        policies_.end()
    );
}

void DLPMonitor::startMonitoring() {
    if (running_) return;
    running_ = true;

    // Start monitoring threads
    std::thread(&DLPMonitor::monitorFileSystem, this).detach();
    std::thread(&DLPMonitor::monitorClipboard, this).detach();
    std::thread(&DLPMonitor::monitorNetworkTransfers, this).detach();
}

void DLPMonitor::stopMonitoring() {
    running_ = false;
}

void DLPMonitor::setCallback(std::function<void(const DLPEvent&)> callback) {
    callback_ = callback;
}

void DLPMonitor::monitorFileSystem() {
    int inotify_fd = inotify_init();
    if (inotify_fd < 0) {
        std::cerr << "Failed to initialize inotify" << std::endl;
        return;
    }

    // Map watch descriptors to paths for proper path reconstruction
    std::unordered_map<int, std::string> wd_to_path;

    // Add watches for monitored paths
    for (const auto& path : monitored_paths_) {
        int wd = inotify_add_watch(inotify_fd, path.c_str(),
            IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_CLOSE_WRITE);
        if (wd >= 0) {
            wd_to_path[wd] = path;
            std::cout << "Monitoring path: " << path << " (wd: " << wd << ")" << std::endl;
        } else {
            std::cerr << "Failed to watch path: " << path << " (errno: " << errno << ")" << std::endl;
        }
    }

    if (wd_to_path.empty()) {
        std::cerr << "No paths were successfully monitored" << std::endl;
        close(inotify_fd);
        return;
    }

    const size_t BUF_LEN = 4096;
    char buffer[BUF_LEN];

    std::cout << "File system monitoring started" << std::endl;

    while (running_) {
        ssize_t len = read(inotify_fd, buffer, BUF_LEN);
        if (len < 0) {
            if (errno != EAGAIN) {
                std::cerr << "Error reading inotify events: " << strerror(errno) << std::endl;
            }
            continue;
        }

        ssize_t i = 0;
        while (i < len) {
            struct inotify_event* event = (struct inotify_event*)&buffer[i];

            // Find the path for this watch descriptor
            auto it = wd_to_path.find(event->wd);
            if (it == wd_to_path.end()) {
                i += sizeof(struct inotify_event) + event->len;
                continue;
            }

            std::string watch_path = it->second;
            std::string full_file_path;

            if (event->len > 0) {
                // Construct full path
                if (watch_path.back() == '/') {
                    full_file_path = watch_path + event->name;
                } else {
                    full_file_path = watch_path + "/" + event->name;
                }

                std::cout << "File event: " << full_file_path << " (mask: " << event->mask << ")" << std::endl;

                // Check if this file violates any policies
                if (checkFileAgainstPolicies(full_file_path)) {
                    if (callback_) {
                        auto now = std::chrono::system_clock::now();
                        auto time_t = std::chrono::system_clock::to_time_t(now);
                        std::stringstream ss;
                        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");

                        DLPEvent dlp_event{
                            ss.str(),
                            "file_access",
                            full_file_path,
                            "",
                            "current_user",
                            "File access policy violation",
                            true  // Set blocked to true for policy violations
                        };
                        std::cout << "DLP violation detected: " << full_file_path << std::endl;
                        callback_(dlp_event);
                    }
                }
            }

            i += sizeof(struct inotify_event) + event->len;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Clean up watches
    for (const auto& pair : wd_to_path) {
        inotify_rm_watch(inotify_fd, pair.first);
    }
    close(inotify_fd);
    std::cout << "File system monitoring stopped" << std::endl;
}

void DLPMonitor::monitorClipboard() {
    // Clipboard monitoring functionality has been removed
    // This is a placeholder for future clipboard monitoring implementations
    while (running_) {
        // Basic clipboard monitoring placeholder
        // Could be implemented using different approaches:
        // - Command-line tools like xclip
        // - Wayland protocols (if applicable)
        // - Other clipboard monitoring libraries

        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}



void DLPMonitor::monitorNetworkTransfers() {
    // eBPF program source for network monitoring
    const char* bpf_program = R"(
#include <uapi/linux/ptrace.h>
#include <net/sock.h>
#include <bcc/proto.h>

struct transfer_event_t {
    u32 pid;
    u32 uid;
    u32 saddr;
    u32 daddr;
    u16 sport;
    u16 dport;
    u32 size;
    char comm[16];
};

BPF_PERF_OUTPUT(transfer_events);

int trace_tcp_sendmsg(struct pt_regs *ctx, struct sock *sk, struct msghdr *msg, size_t size) {
    if (size < 100) return 0; // Only monitor significant transfers

    struct transfer_event_t event = {};
    event.pid = bpf_get_current_pid_tgid() >> 32;
    event.uid = bpf_get_current_uid_gid();
    bpf_get_current_comm(&event.comm, sizeof(event.comm));

    // Get socket addresses
    u32 saddr = 0, daddr = 0;
    u16 sport = 0, dport = 0;

    bpf_probe_read(&saddr, sizeof(saddr), &sk->__sk_common.skc_rcv_saddr);
    bpf_probe_read(&daddr, sizeof(daddr), &sk->__sk_common.skc_daddr);
    bpf_probe_read(&sport, sizeof(sport), &sk->__sk_common.skc_num);
    bpf_probe_read(&dport, sizeof(dport), &sk->__sk_common.skc_dport);

    event.saddr = saddr;
    event.daddr = daddr;
    event.sport = sport;
    event.dport = ntohs(dport);
    event.size = size;

    transfer_events.perf_submit(ctx, &event, sizeof(event));
    return 0;
}

int trace_udp_sendmsg(struct pt_regs *ctx, struct sock *sk, struct msghdr *msg, size_t size) {
    if (size < 100) return 0; // Only monitor significant transfers

    struct transfer_event_t event = {};
    event.pid = bpf_get_current_pid_tgid() >> 32;
    event.uid = bpf_get_current_uid_gid();
    bpf_get_current_comm(&event.comm, sizeof(event.comm));

    // Get socket addresses for UDP
    u32 saddr = 0, daddr = 0;
    u16 sport = 0, dport = 0;

    bpf_probe_read(&saddr, sizeof(saddr), &sk->__sk_common.skc_rcv_saddr);
    bpf_probe_read(&daddr, sizeof(daddr), &sk->__sk_common.skc_daddr);
    bpf_probe_read(&sport, sizeof(sport), &sk->__sk_common.skc_num);
    bpf_probe_read(&dport, sizeof(dport), &sk->__sk_common.skc_dport);

    event.saddr = saddr;
    event.daddr = daddr;
    event.sport = sport;
    event.dport = ntohs(dport);
    event.size = size;

    transfer_events.perf_submit(ctx, &event, sizeof(event));
    return 0;
}
)";

#ifdef USE_BCC
    try {
        // Initialize BCC for eBPF
        std::unique_ptr<bcc::BPF> bpf(new bcc::BPF());
        auto init_res = bpf->init(bpf_program);
        if (init_res.code() != 0) {
            std::cerr << "Failed to initialize eBPF program: " << init_res.msg() << std::endl;
            fallbackNetworkMonitoring();
            return;
        }

        // Attach kprobes
        auto tcp_attach_res = bpf->attach_kprobe("tcp_sendmsg", "trace_tcp_sendmsg");
        if (tcp_attach_res.code() != 0) {
            std::cerr << "Failed to attach TCP kprobe: " << tcp_attach_res.msg() << std::endl;
        }

        auto udp_attach_res = bpf->attach_kprobe("udp_sendmsg", "trace_udp_sendmsg");
        if (udp_attach_res.code() != 0) {
            std::cerr << "Failed to attach UDP kprobe: " << udp_attach_res.msg() << std::endl;
        }

        // Open perf buffer
        auto perf_buffer = bpf->open_perf_buffer("transfer_events", handleNetworkEvent, nullptr, this);

        if (!perf_buffer) {
            std::cerr << "Failed to open perf buffer" << std::endl;
            fallbackNetworkMonitoring();
            return;
        }

        std::cout << "eBPF network monitoring started" << std::endl;

        while (running_) {
            // Poll for events
            bpf->poll_perf_buffer("transfer_events", 100); // 100ms timeout
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "eBPF network monitoring stopped" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "eBPF monitoring error: " << e.what() << std::endl;
        // Fallback to basic network monitoring
        fallbackNetworkMonitoring();
    }
#else
    // BCC not available, use fallback
    std::cout << "BCC not available, using fallback network monitoring" << std::endl;
    fallbackNetworkMonitoring();
#endif
}

void DLPMonitor::handleNetworkEvent(void* cb_cookie, void* data, int data_size) {
    DLPMonitor* monitor = static_cast<DLPMonitor*>(cb_cookie);
    if (!monitor) return;

    struct transfer_event_t {
        uint32_t pid;
        uint32_t uid;
        uint32_t saddr;
        uint32_t daddr;
        uint16_t sport;
        uint16_t dport;
        uint32_t size;
        char comm[16];
    };

    if (data_size < sizeof(transfer_event_t)) return;

    transfer_event_t* event = static_cast<transfer_event_t*>(data);

    // Check if this transfer violates DLP policies
    monitor->checkNetworkTransfer(event);
}

void DLPMonitor::checkNetworkTransfer(void* event_data) {
    struct transfer_event_t {
        uint32_t pid;
        uint32_t uid;
        uint32_t saddr;
        uint32_t daddr;
        uint16_t sport;
        uint16_t dport;
        uint32_t size;
        char comm[16];
    };

    transfer_event_t* event = static_cast<transfer_event_t*>(event_data);

    // Convert IP addresses to strings
    char src_ip[INET_ADDRSTRLEN];
    char dst_ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &event->saddr, src_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &event->daddr, dst_ip, INET_ADDRSTRLEN);

    // Check against restricted destinations
    std::string destination = std::string(dst_ip) + ":" + std::to_string(event->dport);

    for (const auto& policy : policies_) {
        bool violation = false;
        std::string violation_reason;

        // Check file size thresholds (large transfers might indicate data exfiltration)
        if (event->size > 1024 * 1024) { // 1MB threshold
            violation = true;
            violation_reason = "Large network transfer detected";
        }

        // Check destination against restricted IPs/ports
        for (const auto& restricted_dest : policy.restricted_paths) {
            if (destination.find(restricted_dest) != std::string::npos) {
                violation = true;
                violation_reason = "Transfer to restricted destination";
                break;
            }
        }

        // Check for suspicious protocols/ports
        std::vector<int> suspicious_ports = {21, 22, 25, 110, 143, 993, 995}; // FTP, SSH, SMTP, POP3, IMAP
        if (std::find(suspicious_ports.begin(), suspicious_ports.end(), event->dport) != suspicious_ports.end()) {
            violation = true;
            violation_reason = "Transfer using potentially insecure protocol";
        }

        if (violation && callback_) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");

            DLPEvent dlp_event{
                ss.str(),
                "network_transfer",
                std::string(event->comm),
                destination,
                "current_user",  // In real implementation, get actual username
                violation_reason,
                policy.block_transfer
            };
            callback_(dlp_event);
        }
    }
}

void DLPMonitor::fallbackNetworkMonitoring() {
    std::cout << "Starting fallback network monitoring..." << std::endl;

    while (running_) {
        // Use system tools to monitor network connections
        monitorNetworkConnections();

        // Check for suspicious processes with network activity
        monitorSuspiciousProcesses();

        // Monitor for large file transfers via common protocols
        monitorFileTransfers();

        std::this_thread::sleep_for(std::chrono::seconds(5)); // Check every 5 seconds
    }

    std::cout << "Fallback network monitoring stopped" << std::endl;
}

void DLPMonitor::monitorNetworkConnections() {
    // Use 'ss' command to monitor network connections
    FILE* pipe = popen("ss -tuln 2>/dev/null | grep -E ':(21|22|25|110|143|993|995|80|443)' || true", "r");
    if (!pipe) return;

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line(buffer);

        // Parse connection information
        // Example: tcp   LISTEN 0      128                      0.0.0.0:22                     0.0.0.0:*
        if (line.find("tcp") != std::string::npos || line.find("udp") != std::string::npos) {
            // Extract port information
            size_t colon_pos = line.find_last_of(":");
            if (colon_pos != std::string::npos) {
                std::string port_str = line.substr(colon_pos + 1);
                size_t space_pos = port_str.find(" ");
                if (space_pos != std::string::npos) {
                    port_str = port_str.substr(0, space_pos);
                }

                try {
                    int port = std::stoi(port_str);
                    checkPortAgainstPolicies(port);
                } catch (const std::exception&) {
                    // Invalid port number, skip
                }
            }
        }
    }

    pclose(pipe);
}

void DLPMonitor::monitorSuspiciousProcesses() {
    // Monitor processes that might be involved in data exfiltration
    std::vector<std::string> suspicious_commands = {
        "scp", "rsync", "ftp", "sftp", "wget", "curl", "nc", "netcat", "ssh"
    };

    for (const auto& cmd : suspicious_commands) {
        std::string command = "pgrep -x " + cmd + " 2>/dev/null || true";
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) continue;

        char buffer[256];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            // Found suspicious process
            std::string pid_str(buffer);
            pid_str.erase(pid_str.find_last_not_of(" \n\r\t") + 1);

            // Get process information
            std::string ps_command = "ps -p " + pid_str + " -o pid,ppid,cmd 2>/dev/null || true";
            FILE* ps_pipe = popen(ps_command.c_str(), "r");
            if (ps_pipe) {
                char ps_buffer[512];
                if (fgets(ps_buffer, sizeof(ps_buffer), ps_pipe) != nullptr) {
                    std::string process_info(ps_buffer);

                    // Check if this violates any policies
                    for (const auto& policy : policies_) {
                        if (policy.block_transfer && callback_) {
                            auto now = std::chrono::system_clock::now();
                            auto time_t = std::chrono::system_clock::to_time_t(now);
                            std::stringstream ss;
                            ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");

                            DLPEvent dlp_event{
                                ss.str(),
                                "suspicious_process",
                                cmd,
                                "network",
                                "current_user",
                                "Suspicious network process detected: " + cmd,
                                false  // Don't block, just alert
                            };
                            callback_(dlp_event);
                        }
                    }
                }
                pclose(ps_pipe);
            }
        }
        pclose(pipe);
    }
}

void DLPMonitor::monitorFileTransfers() {
    // Monitor for large files that might be transferred
    // Check /proc/net/tcp for established connections with large data transfers

    std::ifstream tcp_file("/proc/net/tcp");
    if (!tcp_file.is_open()) return;

    std::string line;
    std::getline(tcp_file, line); // Skip header

    while (std::getline(tcp_file, line)) {
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> fields;

        while (std::getline(iss, token, ' ')) {
            if (!token.empty()) {
                fields.push_back(token);
            }
        }

        if (fields.size() >= 8) {
            // Parse local and remote addresses
            std::string local_addr = fields[1];
            std::string remote_addr = fields[2];
            std::string state = fields[3];

            // Only check established connections (state 01)
            if (state == "01") {
                // Convert hex addresses to readable format
                std::string readable_remote = hexToIp(remote_addr);
                checkDestinationAgainstPolicies(readable_remote);
            }
        }
    }
}

void DLPMonitor::checkPortAgainstPolicies(int port) {
    std::vector<int> suspicious_ports = {21, 22, 25, 110, 143, 993, 995}; // FTP, SSH, SMTP, POP3, IMAP

    if (std::find(suspicious_ports.begin(), suspicious_ports.end(), port) != suspicious_ports.end()) {
        for (const auto& policy : policies_) {
            if (policy.block_transfer && callback_) {
                auto now = std::chrono::system_clock::now();
                auto time_t = std::chrono::system_clock::to_time_t(now);
                std::stringstream ss;
                ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");

                DLPEvent dlp_event{
                    ss.str(),
                    "suspicious_port",
                    "Network connection",
                    "localhost:" + std::to_string(port),
                    "current_user",
                    "Connection to suspicious port: " + std::to_string(port),
                    false  // Alert only, don't block
                };
                callback_(dlp_event);
            }
        }
    }
}

void DLPMonitor::checkDestinationAgainstPolicies(const std::string& destination) {
    for (const auto& policy : policies_) {
        for (const auto& restricted_dest : policy.restricted_paths) {
            if (destination.find(restricted_dest) != std::string::npos) {
                if (callback_) {
                    auto now = std::chrono::system_clock::now();
                    auto time_t = std::chrono::system_clock::to_time_t(now);
                    std::stringstream ss;
                    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");

                    DLPEvent dlp_event{
                        ss.str(),
                        "restricted_destination",
                        "Network transfer",
                        destination,
                        "current_user",
                        "Transfer to restricted destination: " + destination,
                        policy.block_transfer
                    };
                    callback_(dlp_event);
                }
                break;
            }
        }
    }
}

std::string DLPMonitor::hexToIp(const std::string& hex_addr) {
    // Convert hex IP:port format to readable format
    // Example: 0100007F:0016 -> 127.0.0.1:22

    size_t colon_pos = hex_addr.find(':');
    if (colon_pos == std::string::npos) return hex_addr;

    std::string ip_hex = hex_addr.substr(0, colon_pos);
    std::string port_hex = hex_addr.substr(colon_pos + 1);

    // Convert IP (little endian hex to big endian)
    if (ip_hex.length() == 8) {
        std::string ip_str;
        for (int i = 6; i >= 0; i -= 2) {
            std::string byte_str = ip_hex.substr(i, 2);
            int byte_val = std::stoi(byte_str, nullptr, 16);
            ip_str += std::to_string(byte_val);
            if (i > 0) ip_str += ".";
        }
        return ip_str;
    }

    return hex_addr;
}

bool DLPMonitor::checkFileAgainstPolicies(const std::string& file_path) {
    for (const auto& policy : policies_) {
        // Check file extensions
        for (const auto& ext : policy.file_extensions) {
            if (file_path.find(ext) != std::string::npos) {
                return true;
            }
        }

        // Check restricted paths
        for (const auto& path : policy.restricted_paths) {
            if (file_path.find(path) == 0) {
                return true;
            }
        }

        // Check file content if it's accessible
        if (checkContentAgainstPolicies(file_path)) {
            return true;
        }
    }
    return false;
}

bool DLPMonitor::checkContentAgainstPolicies(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Cannot open file for content check: " << file_path << std::endl;
        return false;
    }

    // Read file content
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

    if (content.empty()) {
        std::cout << "File is empty or unreadable: " << file_path << std::endl;
        return false;
    }

    std::cout << "Checking content of file: " << file_path << " (size: " << content.size() << " bytes)" << std::endl;

    // Convert content to lowercase for case-insensitive matching
    std::string lower_content = content;
    std::transform(lower_content.begin(), lower_content.end(), lower_content.begin(), ::tolower);

    for (const auto& policy : policies_) {
        for (const auto& pattern : policy.content_patterns) {
            try {
                // Try case-sensitive match first
                if (std::regex_search(content, pattern)) {
                    std::cout << "Content pattern matched (case-sensitive): " << file_path << std::endl;
                    return true;
                }

                // For case-insensitive matching, we'll use a simple string search approach
                // since we can't easily extract the pattern string from std::regex
                std::string search_content = lower_content;

                // Try simple string matching for common patterns
                // This is a simplified approach - in production you'd want more sophisticated pattern matching
                if (search_content.find("confidential") != std::string::npos ||
                    search_content.find("secret") != std::string::npos ||
                    search_content.find("internal") != std::string::npos ||
                    search_content.find("password") != std::string::npos ||
                    search_content.find("api_key") != std::string::npos ||
                    search_content.find("token") != std::string::npos) {
                    std::cout << "Content pattern matched (string search): " << file_path << std::endl;
                    return true;
                }

            } catch (const std::regex_error& e) {
                std::cerr << "Regex error for pattern: " << e.what() << std::endl;
                // Continue with other patterns
            }
        }
    }

    std::cout << "No content patterns matched for file: " << file_path << std::endl;
    return false;
}
