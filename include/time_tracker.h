#ifndef TIME_TRACKER_H
#define TIME_TRACKER_H

#include <string>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <atomic>
#include <functional>

struct TimeEntry {
    std::string user;
    std::string application;
    std::string window_title;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::chrono::seconds duration;
    bool active;
};

struct ProductivityMetrics {
    std::string user;
    std::chrono::hours total_time;
    std::chrono::hours productive_time;
    std::chrono::hours unproductive_time;
    double productivity_score;  // 0.0 to 1.0
    std::unordered_map<std::string, std::chrono::seconds> app_usage;
};

class TimeTracker {
public:
    TimeTracker();
    ~TimeTracker();

    void startTracking();
    void stopTracking();
    void setCallback(std::function<void(const TimeEntry&)> callback);
    ProductivityMetrics getProductivityMetrics(const std::string& user);
    std::vector<TimeEntry> getTimeEntries(const std::string& user,
                                         std::chrono::system_clock::time_point start,
                                         std::chrono::system_clock::time_point end);
    std::string getCurrentUser();
    bool isProductiveApplication(const std::string& app_name);

private:
    void trackActiveWindow();
    void calculateProductivity();
    std::string getActiveWindowTitle();
    std::string getActiveApplication();

    std::thread tracking_thread_;
    std::atomic<bool> running_;
    std::unordered_map<std::string, TimeEntry> current_sessions_;
    std::vector<TimeEntry> time_entries_;
    std::function<void(const TimeEntry&)> callback_;
};

#endif // TIME_TRACKER_H
