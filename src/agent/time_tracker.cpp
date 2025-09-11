#include "time_tracker.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <wayland-client.h>
#include <unistd.h>
#include <pwd.h>

TimeTracker::TimeTracker() : running_(false) {}

TimeTracker::~TimeTracker() {
    stopTracking();
}

void TimeTracker::startTracking() {
    if (running_) return;
    running_ = true;

    tracking_thread_ = std::thread(&TimeTracker::trackActiveWindow, this);
}

void TimeTracker::stopTracking() {
    if (!running_) return;
    running_ = false;

    if (tracking_thread_.joinable()) {
        tracking_thread_.join();
    }

    // Finalize any active sessions
    for (auto& [user, entry] : current_sessions_) {
        if (entry.active) {
            entry.end_time = std::chrono::system_clock::now();
            entry.duration = std::chrono::duration_cast<std::chrono::seconds>(
                entry.end_time - entry.start_time);
            entry.active = false;
            time_entries_.push_back(entry);

            if (callback_) {
                callback_(entry);
            }
        }
    }
}

void TimeTracker::setCallback(std::function<void(const TimeEntry&)> callback) {
    callback_ = callback;
}

ProductivityMetrics TimeTracker::getProductivityMetrics(const std::string& user) {
    ProductivityMetrics metrics;
    metrics.user = user;

    std::chrono::hours total_time(0);
    std::chrono::hours productive_time(0);
    std::chrono::hours unproductive_time(0);

    for (const auto& entry : time_entries_) {
        if (entry.user == user) {
            auto hours = std::chrono::duration_cast<std::chrono::hours>(entry.duration);
            total_time += hours;

            // Simple productivity classification (can be enhanced with ML)
            if (isProductiveApplication(entry.application)) {
                productive_time += hours;
            } else {
                unproductive_time += hours;
            }

            metrics.app_usage[entry.application] += entry.duration;
        }
    }

    metrics.total_time = total_time;
    metrics.productive_time = productive_time;
    metrics.unproductive_time = unproductive_time;

    if (total_time.count() > 0) {
        metrics.productivity_score = static_cast<double>(productive_time.count()) /
                                   static_cast<double>(total_time.count());
    } else {
        metrics.productivity_score = 0.0;
    }

    return metrics;
}

std::vector<TimeEntry> TimeTracker::getTimeEntries(const std::string& user,
    std::chrono::system_clock::time_point start, std::chrono::system_clock::time_point end) {

    std::vector<TimeEntry> filtered_entries;

    for (const auto& entry : time_entries_) {
        if (entry.user == user && entry.start_time >= start && entry.end_time <= end) {
            filtered_entries.push_back(entry);
        }
    }

    return filtered_entries;
}

void TimeTracker::trackActiveWindow() {
    // Wayland-compatible window tracking
    // Since direct Wayland window access is restricted, we'll use polling
    // with system tools to track active windows

    std::string previous_app;
    std::string previous_title;
    auto session_start = std::chrono::system_clock::now();

    while (running_) {
        std::string current_app = getActiveApplication();
        std::string current_title = getActiveWindowTitle();

        // Check if the active window/application has changed
        if ((current_app != previous_app || current_title != previous_title) &&
            (!current_app.empty() || !current_title.empty())) {

            // End previous session
            if (!previous_app.empty() || !previous_title.empty()) {
                auto now = std::chrono::system_clock::now();
                std::string user = getCurrentUser();

                TimeEntry entry{
                    user,
                    previous_app,
                    previous_title,
                    session_start,
                    now,
                    std::chrono::duration_cast<std::chrono::seconds>(now - session_start),
                    false
                };

                time_entries_.push_back(entry);

                if (callback_) {
                    callback_(entry);
                }
            }

            // Start new session
            previous_app = current_app;
            previous_title = current_title;
            session_start = std::chrono::system_clock::now();
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Finalize the last session
    if (!previous_app.empty() || !previous_title.empty()) {
        auto now = std::chrono::system_clock::now();
        std::string user = getCurrentUser();

        TimeEntry entry{
            user,
            previous_app,
            previous_title,
            session_start,
            now,
            std::chrono::duration_cast<std::chrono::seconds>(now - session_start),
            false
        };

        time_entries_.push_back(entry);

        if (callback_) {
            callback_(entry);
        }
    }
}

void TimeTracker::calculateProductivity() {
    // This method can be enhanced with machine learning models
    // For now, it's a placeholder
}

std::string TimeTracker::getActiveWindowTitle() {
    // Use system tools to get active window title on Wayland
    // This is a fallback approach since direct Wayland access is restricted

    // Check if Sway is available before using swaymsg
    bool sway_available = (getenv("SWAYSOCK") != nullptr) &&
                         (system("swaymsg -t get_version >/dev/null 2>&1") == 0);

    std::string command = "xdotool getactivewindow getwindowname 2>/dev/null || ";
    if (sway_available) {
        command += "swaymsg -t get_tree | jq -r '.. | select(.focused? == true).name' 2>/dev/null || ";
    }
    command += "echo ''";

    FILE* fp = popen(command.c_str(), "r");
    if (!fp) return "";

    char buffer[256];
    std::string result = "";
    if (fgets(buffer, sizeof(buffer), fp) != nullptr) {
        result = buffer;
        // Remove trailing newline
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }
        // Clean up empty or null results
        if (result == "null" || result.empty()) {
            result = "";
        }
    }
    pclose(fp);
    return result;
}

std::string TimeTracker::getActiveApplication() {
    // Get the process name of the active window on Wayland
    // This uses system tools as a fallback

    // Check if Sway is available before using swaymsg
    bool sway_available = (getenv("SWAYSOCK") != nullptr) &&
                         (system("swaymsg -t get_version >/dev/null 2>&1") == 0);

    std::string command = "xdotool getactivewindow getwindowpid 2>/dev/null | "
                         "xargs -I {} ps -p {} -o comm= 2>/dev/null || ";
    if (sway_available) {
        command += "swaymsg -t get_tree | jq -r '.. | select(.focused? == true).app_id' 2>/dev/null || ";
    }
    command += "echo 'unknown'";

    FILE* fp = popen(command.c_str(), "r");
    if (!fp) return "unknown";

    char buffer[256];
    std::string result = "unknown";
    if (fgets(buffer, sizeof(buffer), fp) != nullptr) {
        result = buffer;
        // Remove trailing newline
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }
        // Clean up the result
        if (result.empty() || result == "null") {
            result = "unknown";
        }
    }
    pclose(fp);
    return result;
}

std::string TimeTracker::getCurrentUser() {
    uid_t uid = getuid();
    struct passwd* pw = getpwuid(uid);
    return pw ? pw->pw_name : "unknown";
}

bool TimeTracker::isProductiveApplication(const std::string& app_name) {
    // Simple classification - can be enhanced with configuration or ML
    std::vector<std::string> productive_apps = {
        "code", "vscode", "sublime", "vim", "emacs",
        "chrome", "firefox", "edge",
        "libreoffice", "soffice", "excel", "word"
    };

    std::vector<std::string> unproductive_apps = {
        "facebook", "twitter", "instagram", "youtube",
        "netflix", "spotify", "games"
    };

    for (const auto& productive : productive_apps) {
        if (app_name.find(productive) != std::string::npos) {
            return true;
        }
    }

    for (const auto& unproductive : unproductive_apps) {
        if (app_name.find(unproductive) != std::string::npos) {
            return false;
        }
    }

    return true;  // Default to productive for unknown apps
}
