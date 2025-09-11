#include "upgrade_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <chrono>
#include <thread>
#include <curl/curl.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <nlohmann/json.hpp>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

// Current version - should be updated with each release
const VersionInfo CURRENT_VERSION = {1, 0, 0, "dev", "2025-01-06"};

// cURL write callback
size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append(static_cast<char*>(contents), total_size);
    return total_size;
}

// Progress callback for downloads
int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    UpgradeManager* manager = static_cast<UpgradeManager*>(clientp);
    if (manager && dltotal > 0) {
        int percentage = static_cast<int>((dlnow * 100) / dltotal);
        manager->reportProgressPublic(percentage, "Downloading update...");
    }
    return 0;
}

UpgradeManager::UpgradeManager()
    : status_(UpgradeStatus::IDLE),
      current_version_(CURRENT_VERSION),
      update_available_(false),
      auto_update_running_(false),
      auto_update_interval_(60), // 1 hour default
      backup_enabled_(true),
      update_server_url_("http://localhost:5000"),
      temp_directory_("/tmp/workforce_agent_updates"),
      backup_directory_("$HOME/.workforce_agent/backups") {
}

UpgradeManager::~UpgradeManager() {
    stopAutoUpdateCheck();
}

void UpgradeManager::initialize(const std::string& config_file) {
    // Only load configuration if a valid config file path is provided
    if (!config_file.empty()) {
        loadConfiguration(config_file);
    }

    // Expand environment variables in paths
    expandEnvironmentVariables(temp_directory_);
    expandEnvironmentVariables(backup_directory_);

    updateStatus(UpgradeStatus::IDLE, "Upgrade manager initialized");

    // Create necessary directories with error handling
    try {
        fs::create_directories(temp_directory_);
        if (backup_enabled_) {
            fs::create_directories(backup_directory_);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Warning: Failed to create directories: " << e.what() << std::endl;
        std::cerr << "Upgrade functionality may be limited." << std::endl;
        backup_enabled_ = false;
    }
}

void UpgradeManager::startAutoUpdateCheck() {
    if (auto_update_running_) return;

    auto_update_running_ = true;
    auto_update_thread_ = std::thread(&UpgradeManager::autoUpdateCheckLoop, this);
}

void UpgradeManager::stopAutoUpdateCheck() {
    if (auto_update_running_) {
        auto_update_running_ = false;
        if (auto_update_thread_.joinable()) {
            auto_update_thread_.join();
        }
    }
}

bool UpgradeManager::checkForUpdates() {
    updateStatus(UpgradeStatus::CHECKING, "Checking for updates...");

    try {
        std::string version_info = getLatestVersionInfo();
        if (version_info.empty()) {
            updateStatus(UpgradeStatus::FAILED, "Failed to retrieve version information - backend server may not be running or network connection failed");
            return false;
        }

        json version_json = json::parse(version_info);
        VersionInfo latest_version = {
            version_json["major"],
            version_json["minor"],
            version_json["patch"],
            version_json.value("build", ""),
            version_json.value("release_date", "")
        };

        if (latest_version > current_version_) {
            available_update_.version = latest_version;
            available_update_.download_url = version_json["download_url"];
            available_update_.checksum = version_json["checksum"];
            available_update_.release_notes = version_json.value("release_notes", "");
            available_update_.file_size = version_json.value("file_size", 0);
            available_update_.signature = version_json.value("signature", "");
            update_available_ = true;

            updateStatus(UpgradeStatus::IDLE, "Update available: " + latest_version.toString());

            if (update_available_callback_) {
                update_available_callback_(available_update_);
            }

            return true;
        } else {
            updateStatus(UpgradeStatus::IDLE, "No updates available - current version " + current_version_.toString() + " is up to date");
            return false;
        }
    } catch (const json::parse_error& e) {
        updateStatus(UpgradeStatus::FAILED, "Failed to parse version information from backend - invalid JSON response: " + std::string(e.what()));
        return false;
    } catch (const json::type_error& e) {
        updateStatus(UpgradeStatus::FAILED, "Failed to parse version information from backend - missing required fields: " + std::string(e.what()));
        return false;
    } catch (const std::exception& e) {
        updateStatus(UpgradeStatus::FAILED, "Error checking for updates: " + std::string(e.what()));
        return false;
    }
}

bool UpgradeManager::downloadUpdate(const UpdateInfo& update) {
    updateStatus(UpgradeStatus::DOWNLOADING, "Downloading update...");

    std::string download_path = temp_directory_ + "/update_" + update.version.toString() + ".tar.gz";

    std::string result = downloadFile(update.download_url, download_path);
    if (result.empty()) {
        updateStatus(UpgradeStatus::FAILED, "Download failed");
        return false;
    }

    // Verify checksum
    if (!verifyUpdateFile(download_path, update.checksum)) {
        updateStatus(UpgradeStatus::FAILED, "Checksum verification failed");
        fs::remove(download_path);
        return false;
    }

    // Verify signature if provided
    if (!update.signature.empty() && !validateUpdateSignature(download_path, update.signature)) {
        updateStatus(UpgradeStatus::FAILED, "Signature verification failed");
        fs::remove(download_path);
        return false;
    }

    updateStatus(UpgradeStatus::IDLE, "Update downloaded and verified");
    return true;
}

bool UpgradeManager::installUpdate() {
    updateStatus(UpgradeStatus::INSTALLING, "Installing update...");

    std::string update_archive = temp_directory_ + "/update_" + available_update_.version.toString() + ".tar.gz";
    std::string extract_path = temp_directory_ + "/extracted_update";

    // Create backup if enabled
    if (backup_enabled_ && !backupCurrentVersion()) {
        updateStatus(UpgradeStatus::FAILED, "Failed to create backup");
        return false;
    }

    // Extract update
    if (!extractUpdate(update_archive, extract_path)) {
        updateStatus(UpgradeStatus::FAILED, "Failed to extract update");
        return false;
    }

    // Find new executable
    std::string new_executable;
    for (const auto& entry : fs::recursive_directory_iterator(extract_path)) {
        if (entry.is_regular_file() && entry.path().filename() == "workforce_agent") {
            new_executable = entry.path().string();
            break;
        }
    }

    if (new_executable.empty()) {
        updateStatus(UpgradeStatus::FAILED, "New executable not found in update");
        return false;
    }

    // Replace executable
    if (!replaceExecutable(new_executable)) {
        updateStatus(UpgradeStatus::FAILED, "Failed to replace executable");
        rollbackUpdate();
        return false;
    }

    // Cleanup
    fs::remove_all(extract_path);
    fs::remove(update_archive);

    updateStatus(UpgradeStatus::SUCCESS, "Update installed successfully");
    return true;
}

bool UpgradeManager::rollbackUpdate() {
    updateStatus(UpgradeStatus::ROLLBACK, "Rolling back update...");

    if (!backup_enabled_ || !restoreBackup()) {
        updateStatus(UpgradeStatus::FAILED, "Rollback failed");
        return false;
    }

    updateStatus(UpgradeStatus::IDLE, "Rollback completed");
    return true;
}

void UpgradeManager::loadConfiguration(const std::string& config_file) {
    try {
        if (fs::exists(config_file)) {
            std::ifstream file(config_file);
            json config;
            file >> config;

            update_server_url_ = config.value("update_server_url", update_server_url_);
            auto_update_interval_ = config.value("auto_update_interval", auto_update_interval_);
            backup_enabled_ = config.value("backup_enabled", backup_enabled_);
            backup_directory_ = config.value("backup_directory", "/var/backups/workforce_agent");
            temp_directory_ = config.value("temp_directory", "/tmp/workforce_agent_updates");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading configuration: " << e.what() << std::endl;
    }
}

void UpgradeManager::saveConfiguration(const std::string& config_file) {
    try {
        json config = {
            {"update_server_url", update_server_url_},
            {"auto_update_interval", auto_update_interval_},
            {"backup_enabled", backup_enabled_},
            {"backup_directory", backup_directory_},
            {"temp_directory", temp_directory_}
        };

        std::ofstream file(config_file);
        file << config.dump(4);
    } catch (const std::exception& e) {
        std::cerr << "Error saving configuration: " << e.what() << std::endl;
    }
}

bool UpgradeManager::verifyUpdateFile(const std::string& file_path, const std::string& expected_checksum) {
    std::string actual_checksum = calculateChecksum(file_path);
    return actual_checksum == expected_checksum;
}

bool UpgradeManager::backupCurrentVersion() {
    try {
        std::string backup_path = backup_directory_ + "/workforce_agent_" + current_version_.toString() + "_backup";

        // Get current executable path
        char exe_path[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
        if (len == -1) return false;
        exe_path[len] = '\0';

        fs::copy(exe_path, backup_path, fs::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Backup failed: " << e.what() << std::endl;
        return false;
    }
}

bool UpgradeManager::restoreBackup() {
    try {
        std::string backup_path = backup_directory_ + "/workforce_agent_" + current_version_.toString() + "_backup";

        if (!fs::exists(backup_path)) return false;

        // Get current executable path
        char exe_path[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
        if (len == -1) return false;
        exe_path[len] = '\0';

        fs::copy(backup_path, exe_path, fs::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Restore failed: " << e.what() << std::endl;
        return false;
    }
}

void UpgradeManager::cleanupBackup() {
    try {
        // Keep only the last 3 backups
        std::vector<fs::path> backups;
        for (const auto& entry : fs::directory_iterator(backup_directory_)) {
            if (entry.is_regular_file() && entry.path().filename().string().find("backup") != std::string::npos) {
                backups.push_back(entry.path());
            }
        }

        std::sort(backups.begin(), backups.end(),
                 [](const fs::path& a, const fs::path& b) {
                     return fs::last_write_time(a) > fs::last_write_time(b);
                 });

        for (size_t i = 3; i < backups.size(); ++i) {
            fs::remove(backups[i]);
        }
    } catch (const std::exception& e) {
        std::cerr << "Cleanup failed: " << e.what() << std::endl;
    }
}

void UpgradeManager::updateStatus(UpgradeStatus status, const std::string& message) {
    status_ = status;
    status_message_ = message;

    if (status_callback_) {
        status_callback_(status, message);
    }
}

void UpgradeManager::reportProgress(int percentage, const std::string& message) {
    if (progress_callback_) {
        progress_callback_(percentage, message);
    }
}

std::string UpgradeManager::downloadFile(const std::string& url, const std::string& output_path) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string response;
    FILE* fp = fopen(output_path.c_str(), "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return "";
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);  // 30 second timeout
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);  // 10 second connection timeout

    // Set up progress callback
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    CURLcode res = curl_easy_perform(curl);

    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        // Provide detailed error information for debugging
        std::string error_msg = "Network error: ";
        switch (res) {
            case CURLE_COULDNT_RESOLVE_HOST:
                error_msg += "Could not resolve hostname - check network connection and DNS";
                break;
            case CURLE_COULDNT_CONNECT:
                error_msg += "Could not connect to server - backend may not be running";
                break;
            case CURLE_OPERATION_TIMEDOUT:
                error_msg += "Connection timed out - server may be unresponsive";
                break;
            case CURLE_SSL_CONNECT_ERROR:
                error_msg += "SSL connection failed - certificate or TLS issue";
                break;
            case CURLE_HTTP_RETURNED_ERROR: {
                long response_code;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
                error_msg += "HTTP error " + std::to_string(response_code) + " - endpoint may not exist";
                break;
            }
            default:
                error_msg += curl_easy_strerror(res);
        }
        // Store error for later retrieval but don't print to stderr
        last_network_error_ = error_msg;
        return "";
    }

    return response;
}

std::string UpgradeManager::getLatestVersionInfo() {
    std::string url = update_server_url_ + "/latest";
    return downloadFile(url, "/dev/null"); // We don't need to save the response to a file
}

bool UpgradeManager::validateUpdateSignature(const std::string& file_path, const std::string& signature) {
    // This is a simplified signature validation
    // In production, you would use proper cryptographic signature verification
    return !signature.empty();
}

std::string UpgradeManager::calculateChecksum(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) return "";

    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    char buffer[8192];
    while (file.read(buffer, sizeof(buffer))) {
        SHA256_Update(&sha256, buffer, file.gcount());
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return ss.str();
}

bool UpgradeManager::extractUpdate(const std::string& archive_path, const std::string& extract_path) {
    // This is a simplified extraction - in production you'd use libarchive or similar
    std::string command = "tar -xzf " + archive_path + " -C " + extract_path;
    int result = system(command.c_str());
    return result == 0;
}

bool UpgradeManager::replaceExecutable(const std::string& new_executable_path) {
    // Get current executable path
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1) return false;
    exe_path[len] = '\0';

    // Create backup of current executable
    std::string backup_path = std::string(exe_path) + ".old";
    if (fs::exists(backup_path)) {
        fs::remove(backup_path);
    }
    fs::copy(exe_path, backup_path);

    // Replace executable
    try {
        fs::copy(new_executable_path, exe_path, fs::copy_options::overwrite_existing);
        fs::permissions(exe_path, fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
                       fs::perm_options::add);
        return true;
    } catch (const std::exception& e) {
        // Restore backup on failure
        fs::copy(backup_path, exe_path, fs::copy_options::overwrite_existing);
        return false;
    }
}

void UpgradeManager::expandEnvironmentVariables(std::string& path) {
    // Simple environment variable expansion for $HOME
    if (path.find("$HOME") != std::string::npos) {
        const char* home = getenv("HOME");
        if (home) {
            size_t pos = path.find("$HOME");
            path.replace(pos, 5, home);
        }
    }
}

void UpgradeManager::autoUpdateCheckLoop() {
    while (auto_update_running_) {
        checkForUpdates();
        std::this_thread::sleep_for(std::chrono::minutes(auto_update_interval_));
    }
}
