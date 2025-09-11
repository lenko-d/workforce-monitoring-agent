#ifndef ACTIVITY_MONITOR_H
#define ACTIVITY_MONITOR_H

#include <string>
#include <vector>
#include <set>
#include <thread>
#include <atomic>
#include <functional>

struct ActivityEvent {
    std::string timestamp;
    std::string type;  // "keyboard", "mouse", "window", "application"
    std::string details;
    std::string user;
};

class ActivityMonitor {
public:
    ActivityMonitor();
    ~ActivityMonitor();

    void startMonitoring();
    void stopMonitoring();
    void setCallback(std::function<void(const ActivityEvent&)> callback);

private:
    void monitorKeyboard();
    void monitorMouse();
    void monitorWindowFocus();
    void monitorApplications();
    std::string getActiveWindowTitle();
    std::string getActiveApplication();
    std::set<std::string> getRunningApplications();

    std::thread keyboard_thread_;
    std::thread mouse_thread_;
    std::thread window_thread_;
    std::thread app_thread_;

    std::atomic<bool> running_;
    std::function<void(const ActivityEvent&)> callback_;
};

#endif // ACTIVITY_MONITOR_H
