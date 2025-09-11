#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <sys/stat.h>
#include <curl/curl.h>
#include <sstream>
#include <iomanip>
#ifdef HAS_NLOHMANN_JSON
#include <nlohmann/json.hpp>
#endif
#include "activity_monitor.h"
#include "dlp_monitor.h"
#include "time_tracker.h"
#include "behavior_analyzer.h"
#include "upgrade_manager.h"

std::atomic<bool> running(true);

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    running = false;
}

// Callback function for cURL to write response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool sendDataToBackend(const std::string& json_data) {
    CURL* curl;
    CURLcode res;
    std::string response_string;
    long response_code = 0;

    // Get backend URL from environment variable or use default
    const char* backend_url = std::getenv("BACKEND_URL");
    if (!backend_url) {
        backend_url = "http://localhost:5000/agent_data";
    }

    curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize cURL" << std::endl;
        return false;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, backend_url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_data.length());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);  // 10 second timeout
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);  // Disable SSL verification for development
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << "Failed to send data to backend: " << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        return false;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    if (response_code >= 200 && response_code < 300) {
        //std::cout << "Data sent successfully to backend" << std::endl;
        return true;
    } else {
        std::cerr << "Backend returned error code: " << response_code << std::endl;
        std::cerr << "Response: " << response_string << std::endl;
        return false;
    }
}

void sendApplicationUsageData(const std::string& user, const ProductivityMetrics& productivity, TimeTracker& timeTracker) {
    // Convert timestamp to ISO string
    auto now = std::chrono::system_clock::now();
    auto now_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream timestamp_ss;
    timestamp_ss << std::put_time(std::gmtime(&now_t), "%Y-%m-%dT%H:%M:%SZ");

#ifdef HAS_NLOHMANN_JSON
    // Create application usage array
    nlohmann::json app_usage_array = nlohmann::json::array();
    for (const auto& [app_name, duration] : productivity.app_usage) {
        nlohmann::json app_data = {
            {"application", app_name},
            {"total_time_seconds", duration.count()},
            {"is_productive", timeTracker.isProductiveApplication(app_name)}
        };
        app_usage_array.push_back(app_data);
    }

    nlohmann::json usage_json = {
        {"type", "app_usage"},
        {"timestamp", timestamp_ss.str()},
        {"user", user},
        {"session_duration_hours", productivity.total_time.count()},
        {"productive_time_hours", productivity.productive_time.count()},
        {"productivity_score", productivity.productivity_score},
        {"application_usage", app_usage_array}
    };

    sendDataToBackend(usage_json.dump());
#else
    // Manual JSON construction for systems without nlohmann/json
    std::stringstream usage_json;
    usage_json << "{\"type\":\"app_usage\",\"timestamp\":\"" << timestamp_ss.str()
               << "\",\"user\":\"" << user
               << "\",\"session_duration_hours\":" << productivity.total_time.count()
               << ",\"productive_time_hours\":" << productivity.productive_time.count()
               << ",\"productivity_score\":" << productivity.productivity_score
               << ",\"application_usage\":[";

    bool first = true;
    for (const auto& [app_name, duration] : productivity.app_usage) {
        if (!first) usage_json << ",";
        usage_json << "{\"application\":\"" << app_name
                   << "\",\"total_time_seconds\":" << duration.count()
                   << ",\"is_productive\":" << (timeTracker.isProductiveApplication(app_name) ? "true" : "false") << "}";
        first = false;
    }

    usage_json << "]}";
    sendDataToBackend(usage_json.str());
#endif
}

void sendRecentBehaviorPatterns(BehaviorAnalyzer& behavior_analyzer, const std::string& user) {
    // Get recent behavior patterns using getRecentPatterns
    std::vector<BehaviorPattern> recent_patterns = behavior_analyzer.getRecentPatterns(user, 10); // Get last 10 patterns

    if (recent_patterns.empty()) {
        return; // No patterns to send
    }

    // Convert timestamp to ISO string for the batch
    auto now = std::chrono::system_clock::now();
    auto now_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream batch_timestamp_ss;
    batch_timestamp_ss << std::put_time(std::gmtime(&now_t), "%Y-%m-%dT%H:%M:%SZ");

#ifdef HAS_NLOHMANN_JSON
    // Create patterns array
    nlohmann::json patterns_array = nlohmann::json::array();
    for (const auto& pattern : recent_patterns) {
        // Convert pattern timestamp to ISO string
        auto pattern_time_t = std::chrono::system_clock::to_time_t(pattern.timestamp);
        std::stringstream pattern_timestamp_ss;
        pattern_timestamp_ss << std::put_time(std::gmtime(&pattern_time_t), "%Y-%m-%dT%H:%M:%SZ");

        nlohmann::json pattern_data = {
            {"pattern_type", pattern.pattern_type},
            {"description", pattern.description},
            {"confidence_score", pattern.confidence_score},
            {"timestamp", pattern_timestamp_ss.str()},
            {"user", pattern.user}
        };
        patterns_array.push_back(pattern_data);
    }

    nlohmann::json patterns_json = {
        {"type", "behavior_patterns"},
        {"batch_timestamp", batch_timestamp_ss.str()},
        {"user", user},
        {"patterns", patterns_array},
        {"pattern_count", recent_patterns.size()}
    };

    sendDataToBackend(patterns_json.dump());
#else
    // Manual JSON construction for systems without nlohmann/json
    std::stringstream patterns_json;
    patterns_json << "{\"type\":\"behavior_patterns\",\"batch_timestamp\":\"" << batch_timestamp_ss.str()
                  << "\",\"user\":\"" << user << "\",\"patterns\":[";

    bool first = true;
    for (const auto& pattern : recent_patterns) {
        if (!first) patterns_json << ",";
        // Convert pattern timestamp to ISO string
        auto pattern_time_t = std::chrono::system_clock::to_time_t(pattern.timestamp);
        std::stringstream pattern_timestamp_ss;
        pattern_timestamp_ss << std::put_time(std::gmtime(&pattern_time_t), "%Y-%m-%dT%H:%M:%SZ");

        patterns_json << "{\"pattern_type\":\"" << pattern.pattern_type
                      << "\",\"description\":\"" << pattern.description
                      << "\",\"confidence_score\":" << pattern.confidence_score
                      << ",\"timestamp\":\"" << pattern_timestamp_ss.str()
                      << "\",\"user\":\"" << pattern.user << "\"}";
        first = false;
    }

    patterns_json << "],\"pattern_count\":" << recent_patterns.size() << "}";
    sendDataToBackend(patterns_json.str());
#endif
}

int main(int argc, char* argv[]) {
    std::cout << "Workforce Monitoring Agent starting..." << std::endl;

    // Set XDG_RUNTIME_DIR if not set (needed for Wayland)
    if (!getenv("XDG_RUNTIME_DIR")) {
        const char* xdg_runtime_dir = "/tmp/xdg-runtime-dir";
        setenv("XDG_RUNTIME_DIR", xdg_runtime_dir, 1);
        // Create the directory if it doesn't exist
        mkdir(xdg_runtime_dir, 0700);
    }

    // Register signal handler
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize components
    ActivityMonitor activity_monitor;
    DLPMonitor dlp_monitor;
    TimeTracker time_tracker;
    BehaviorAnalyzer behavior_analyzer;
    UpgradeManager upgrade_manager;

    // Configure DLP Policies
    DLPPolicy confidential_policy;
    confidential_policy.name = "confidential_files";
    confidential_policy.file_extensions = {".docx", ".xlsx", ".pdf", ".txt"};
    confidential_policy.content_patterns = {std::regex("confidential"), std::regex("secret"), std::regex("internal")};
    confidential_policy.restricted_paths = {"/home", "/tmp"};
    confidential_policy.block_transfer = true;
    dlp_monitor.addPolicy(confidential_policy);

    DLPPolicy sensitive_policy;
    sensitive_policy.name = "sensitive_data";
    sensitive_policy.file_extensions = {".sql", ".db", ".key", ".pem"};
    sensitive_policy.content_patterns = {std::regex("password"), std::regex("api_key"), std::regex("token")};
    sensitive_policy.restricted_paths = {"/var", "/etc"};
    sensitive_policy.block_transfer = true;
    dlp_monitor.addPolicy(sensitive_policy);

    // Configure LLM Analysis (optional - requires API keys)
    // Uncomment and configure these lines to enable LLM-powered behavioral analysis
    /*
    behavior_analyzer.enableLLMAnalysis(true);
    behavior_analyzer.setLLMProvider("openai");  // or "anthropic"
    behavior_analyzer.setLLMAPIKey("openai", "your-openai-api-key-here");
    behavior_analyzer.setLLMModel("openai", "gpt-4");
    behavior_analyzer.startLLMAnalysis();
    */

    // Alternative: Enable with environment variables
    const char* llm_provider = std::getenv("LLM_PROVIDER");
    const char* openai_key = std::getenv("OPENAI_API_KEY");
    const char* anthropic_key = std::getenv("ANTHROPIC_API_KEY");

    if (llm_provider && (openai_key || anthropic_key)) {
        std::cout << "Enabling LLM-powered behavioral analysis..." << std::endl;
        behavior_analyzer.enableLLMAnalysis(true);
        behavior_analyzer.setLLMProvider(llm_provider);

        if (std::string(llm_provider) == "openai" && openai_key) {
            behavior_analyzer.setLLMAPIKey("openai", openai_key);
            behavior_analyzer.setLLMModel("openai", "gpt-4");
        } else if (std::string(llm_provider) == "anthropic" && anthropic_key) {
            behavior_analyzer.setLLMAPIKey("anthropic", anthropic_key);
            behavior_analyzer.setLLMModel("anthropic", "claude-3-sonnet-20240229");
        }

        behavior_analyzer.startLLMAnalysis();
        std::cout << "LLM analysis enabled with provider: " << llm_provider << std::endl;
    } else {
        std::cout << "LLM analysis disabled. Set LLM_PROVIDER and API keys to enable." << std::endl;
        std::cout << "Example: export LLM_PROVIDER=openai" << std::endl;
        std::cout << "         export OPENAI_API_KEY=your-key-here" << std::endl;
    }

    // Set up callbacks
    activity_monitor.setCallback([](const ActivityEvent& event) {
#ifdef HAS_NLOHMANN_JSON
        nlohmann::json json_data = {
            {"type", "activity"},
            {"timestamp", event.timestamp},
            {"activity_type", event.type},
            {"details", event.details},
            {"user", event.user}
        };
        sendDataToBackend(json_data.dump());
#else
        std::stringstream json_data;
        json_data << "{\"type\":\"activity\",\"timestamp\":\"" << event.timestamp
                  << "\",\"activity_type\":\"" << event.type
                  << "\",\"details\":\"" << event.details
                  << "\",\"user\":\"" << event.user << "\"}";
        sendDataToBackend(json_data.str());
#endif
    });

    dlp_monitor.setCallback([](const DLPEvent& event) {
        // Send DLP event data
#ifdef HAS_NLOHMANN_JSON
        nlohmann::json dlp_json = {
            {"type", "dlp"},
            {"timestamp", event.timestamp},
            {"dlp_type", event.type},
            {"policy_violated", event.policy_violated},
            {"user", event.user},
            {"blocked", event.blocked}
        };
        sendDataToBackend(dlp_json.dump());
#else
        std::stringstream dlp_json;
        dlp_json << "{\"type\":\"dlp\",\"timestamp\":\"" << event.timestamp
                 << "\",\"dlp_type\":\"" << event.type
                 << "\",\"policy_violated\":\"" << event.policy_violated
                 << "\",\"user\":\"" << event.user
                 << "\",\"blocked\":" << (event.blocked ? "true" : "false") << "}";
        sendDataToBackend(dlp_json.str());
#endif

        // Send alert data for all DLP events (not just blocked ones)
        std::string severity = "medium";
        if (event.blocked) {
            severity = "high";
        } else if (event.type == "file_access") {
            severity = "high";  // File access violations are always high severity
        }

        std::string alert_title = "DLP Event Detected";
        std::string alert_description = event.policy_violated;

        if (event.type == "file_access") {
            alert_title = "File Access Policy Violation";
            alert_description = "Detected: " + event.file_path + " - " + event.policy_violated;
        } else if (event.type == "suspicious_process") {
            alert_title = "Suspicious Process Detected";
            alert_description = event.policy_violated;
        } else if (event.type == "suspicious_port") {
            alert_title = "Suspicious Network Activity";
            alert_description = event.policy_violated;
        } else if (event.type == "restricted_destination") {
            alert_title = "Restricted Network Destination";
            alert_description = event.policy_violated;
        }

#ifdef HAS_NLOHMANN_JSON
        nlohmann::json alert_json = {
            {"type", "alert"},
            {"alert_type", "dlp_event"},
            {"title", alert_title},
            {"description", alert_description},
            {"severity", severity},
            {"user", event.user},
            {"timestamp", event.timestamp}
        };
        sendDataToBackend(alert_json.dump());
#else
        std::stringstream alert_json;
        alert_json << "{\"type\":\"alert\",\"alert_type\":\"dlp_event\",\"title\":\"" << alert_title
                  << "\",\"description\":\"" << alert_description << "\",\"severity\":\"" << severity
                  << "\",\"user\":\"" << event.user << "\",\"timestamp\":\"" << event.timestamp << "\"}";
        sendDataToBackend(alert_json.str());
#endif
    });

    time_tracker.setCallback([](const TimeEntry& entry) {
        // Convert time_point to ISO string
        auto start_time_t = std::chrono::system_clock::to_time_t(entry.start_time);
        std::stringstream start_time_ss;
        start_time_ss << std::put_time(std::gmtime(&start_time_t), "%Y-%m-%dT%H:%M:%SZ");

#ifdef HAS_NLOHMANN_JSON
        nlohmann::json json_data = {
            {"type", "time"},
            {"start_time", start_time_ss.str()},
            {"application", entry.application},
            {"duration", entry.duration.count()},
            {"user", entry.user},
            {"active", entry.active}
        };
        sendDataToBackend(json_data.dump());
#else
        std::stringstream json_data;
        json_data << "{\"type\":\"time\",\"start_time\":\"" << start_time_ss.str()
                  << "\",\"application\":\"" << entry.application
                  << "\",\"duration\":" << entry.duration.count()
                  << ",\"user\":\"" << entry.user
                  << "\",\"active\":" << (entry.active ? "true" : "false") << "}";
        sendDataToBackend(json_data.str());
#endif
    });

    behavior_analyzer.setAnomalyCallback([](const BehaviorPattern& pattern) {
        // Convert time_point to ISO string
        auto timestamp_t = std::chrono::system_clock::to_time_t(pattern.timestamp);
        std::stringstream timestamp_ss;
        timestamp_ss << std::put_time(std::gmtime(&timestamp_t), "%Y-%m-%dT%H:%M:%SZ");

        // Send anomaly data
#ifdef HAS_NLOHMANN_JSON
        nlohmann::json anomaly_json = {
            {"type", "anomaly"},
            {"timestamp", timestamp_ss.str()},
            {"user", pattern.user},
            {"description", pattern.description},
            {"confidence_score", pattern.confidence_score}
        };
        sendDataToBackend(anomaly_json.dump());
#else
        std::stringstream anomaly_json;
        anomaly_json << "{\"type\":\"anomaly\",\"timestamp\":\"" << timestamp_ss.str()
                    << "\",\"user\":\"" << pattern.user
                    << "\",\"description\":\"" << pattern.description
                    << "\",\"confidence_score\":" << pattern.confidence_score << "}";
        sendDataToBackend(anomaly_json.str());
#endif

        // Send alert data for anomalies
        std::string severity = "low";
        if (pattern.confidence_score > 0.7) {
            severity = "high";
        } else if (pattern.confidence_score > 0.4) {
            severity = "medium";
        }

#ifdef HAS_NLOHMANN_JSON
        nlohmann::json alert_json = {
            {"type", "alert"},
            {"alert_type", "behavior_anomaly"},
            {"title", "Behavior Anomaly Detected"},
            {"description", pattern.description},
            {"severity", severity},
            {"user", pattern.user},
            {"timestamp", timestamp_ss.str()}
        };
        sendDataToBackend(alert_json.dump());
#else
        std::stringstream alert_json;
        alert_json << "{\"type\":\"alert\",\"alert_type\":\"behavior_anomaly\",\"title\":\"Behavior Anomaly Detected\",\"description\":\""
                  << pattern.description << "\",\"severity\":\"" << severity << "\",\"user\":\""
                  << pattern.user << "\",\"timestamp\":\"" << timestamp_ss.str() << "\"}";
        sendDataToBackend(alert_json.str());
#endif
    });

    // Initialize upgrade manager
    upgrade_manager.initialize();
    upgrade_manager.setUpdateAvailableCallback([](const UpdateInfo& update) {
        std::cout << "Update available: " << update.version.toString() << std::endl;
        std::cout << "Release notes: " << update.release_notes << std::endl;
    });
    upgrade_manager.setStatusCallback([](UpgradeStatus status, const std::string& message) {
        std::cout << "Upgrade status: " << static_cast<int>(status) << " - " << message << std::endl;
    });
    upgrade_manager.setProgressCallback([](int percentage, const std::string& message) {
        std::cout << "Upgrade progress: " << percentage << "% - " << message << std::endl;
    });

    // Start auto-update checking
    upgrade_manager.startAutoUpdateCheck();

    // Start monitoring
    activity_monitor.startMonitoring();
    dlp_monitor.startMonitoring();
    time_tracker.startTracking();

    std::cout << "Monitoring started. Press Ctrl+C to stop." << std::endl;

    // Main loop
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Periodic analysis
        static int counter = 0;
        if (++counter % 60 == 0) {  // Every minute
            // Analyze current activity patterns
            std::unordered_map<std::string, double> metrics;
            metrics["activity_level"] = 0.8;  // Placeholder
            behavior_analyzer.analyzeActivity("current_user", "periodic_check", metrics);

            // Report productivity metrics
            std::string current_user = time_tracker.getCurrentUser();
            ProductivityMetrics productivity = time_tracker.getProductivityMetrics(current_user);

            // Send productivity data to backend as JSON
#ifdef HAS_NLOHMANN_JSON
            nlohmann::json productivity_json = {
                {"type", "productivity"},
                {"timestamp", std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())},
                {"user", current_user},
                {"productivity_score", productivity.productivity_score},
                {"productive_time", productivity.productive_time.count()},
                {"total_time", productivity.total_time.count()}
            };
            sendDataToBackend(productivity_json.dump());
#else
            // Convert timestamp to ISO string
            auto now = std::chrono::system_clock::now();
            auto now_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream timestamp_ss;
            timestamp_ss << std::put_time(std::gmtime(&now_t), "%Y-%m-%dT%H:%M:%SZ");

            std::stringstream productivity_json;
            productivity_json << "{\"type\":\"productivity\",\"timestamp\":\"" << timestamp_ss.str()
                            << "\",\"user\":\"" << current_user
                            << "\",\"productivity_score\":" << productivity.productivity_score
                            << ",\"productive_time\":" << productivity.productive_time.count()
                            << ",\"total_time\":" << productivity.total_time.count() << "}";
            sendDataToBackend(productivity_json.str());
#endif

            // Send application usage data to backend
            sendApplicationUsageData(current_user, productivity, time_tracker);

            // Send recent behavior patterns to backend
            sendRecentBehaviorPatterns(behavior_analyzer, current_user);
        }
    }

    // Stop monitoring
    activity_monitor.stopMonitoring();
    dlp_monitor.stopMonitoring();
    time_tracker.stopTracking();

    std::cout << "Workforce Monitoring Agent stopped." << std::endl;
    return 0;
}
