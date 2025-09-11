#ifndef UPGRADE_MANAGER_H
#define UPGRADE_MANAGER_H

#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <memory>

struct VersionInfo {
    int major;
    int minor;
    int patch;
    std::string build;
    std::string release_date;

    std::string toString() const {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch) +
               (build.empty() ? "" : "-" + build);
    }

    bool operator<(const VersionInfo& other) const {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        return patch < other.patch;
    }

    bool operator>(const VersionInfo& other) const {
        return other < *this;
    }

    bool operator==(const VersionInfo& other) const {
        return major == other.major && minor == other.minor && patch == other.patch;
    }

    bool operator!=(const VersionInfo& other) const {
        return !(*this == other);
    }

    bool operator<=(const VersionInfo& other) const {
        return *this < other || *this == other;
    }

    bool operator>=(const VersionInfo& other) const {
        return *this > other || *this == other;
    }
};

struct UpdateInfo {
    VersionInfo version;
    std::string download_url;
    std::string checksum;
    std::string release_notes;
    size_t file_size;
    std::string signature;
};

enum class UpgradeStatus {
    IDLE,
    CHECKING,
    DOWNLOADING,
    VERIFYING,
    INSTALLING,
    SUCCESS,
    FAILED,
    ROLLBACK
};

class UpgradeManager {
public:
    UpgradeManager();
    ~UpgradeManager();

    void initialize(const std::string& config_file = "");
    void startAutoUpdateCheck();
    void stopAutoUpdateCheck();

    // Manual update operations
    bool checkForUpdates();
    bool downloadUpdate(const UpdateInfo& update);
    bool installUpdate();
    bool rollbackUpdate();

    // Status and information
    UpgradeStatus getStatus() const { return status_; }
    VersionInfo getCurrentVersion() const { return current_version_; }
    UpdateInfo getAvailableUpdate() const { return available_update_; }
    std::string getStatusMessage() const { return status_message_; }
    std::string getLastNetworkError() const { return last_network_error_; }

    // Configuration
    void setUpdateServerUrl(const std::string& url) { update_server_url_ = url; }
    void setAutoUpdateInterval(int minutes) { auto_update_interval_ = minutes; }
    void setBackupEnabled(bool enabled) { backup_enabled_ = enabled; }

    // Callbacks
    void setUpdateAvailableCallback(std::function<void(const UpdateInfo&)> callback) {
        update_available_callback_ = callback;
    }
    void setProgressCallback(std::function<void(int, const std::string&)> callback) {
        progress_callback_ = callback;
    }
    void setStatusCallback(std::function<void(UpgradeStatus, const std::string&)> callback) {
        status_callback_ = callback;
    }

    // Public method for progress reporting (accessible from callbacks)
    void reportProgressPublic(int percentage, const std::string& message = "") {
        reportProgress(percentage, message);
    }

private:
    void loadConfiguration(const std::string& config_file);
    void saveConfiguration(const std::string& config_file);
    bool verifyUpdateFile(const std::string& file_path, const std::string& expected_checksum);
    bool backupCurrentVersion();
    bool restoreBackup();
    void cleanupBackup();
    void updateStatus(UpgradeStatus status, const std::string& message = "");
    void reportProgress(int percentage, const std::string& message = "");

    // Network operations
    std::string downloadFile(const std::string& url, const std::string& output_path);
    std::string getLatestVersionInfo();
    bool validateUpdateSignature(const std::string& file_path, const std::string& signature);

    // File operations
    std::string calculateChecksum(const std::string& file_path);
    bool extractUpdate(const std::string& archive_path, const std::string& extract_path);
    bool replaceExecutable(const std::string& new_executable_path);
    void expandEnvironmentVariables(std::string& path);

    // Auto-update thread
    void autoUpdateCheckLoop();
    std::thread auto_update_thread_;
    std::atomic<bool> auto_update_running_;
    int auto_update_interval_; // minutes

    // Status
    std::atomic<UpgradeStatus> status_;
    std::string status_message_;
    VersionInfo current_version_;
    UpdateInfo available_update_;
    bool update_available_;

    // Configuration
    std::string update_server_url_;
    std::string backup_directory_;
    bool backup_enabled_;
    std::string temp_directory_;

    // Error tracking
    std::string last_network_error_;

    // Callbacks
    std::function<void(const UpdateInfo&)> update_available_callback_;
    std::function<void(int, const std::string&)> progress_callback_;
    std::function<void(UpgradeStatus, const std::string&)> status_callback_;
};

#endif // UPGRADE_MANAGER_H
