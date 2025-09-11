#include "activity_monitor.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <set>
#include <wayland-client.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>

ActivityMonitor::ActivityMonitor() : running_(false) {
    // Initialize Wayland display (silently handle failure)
    struct wl_display* display = wl_display_connect(nullptr);
    if (display) {
        wl_display_disconnect(display);
    }
    // Note: Wayland connection failure is expected in some environments
    // and will be handled gracefully by the monitoring functions
}

ActivityMonitor::~ActivityMonitor() {
    stopMonitoring();
}

void ActivityMonitor::startMonitoring() {
    if (running_) return;

    running_ = true;

    keyboard_thread_ = std::thread(&ActivityMonitor::monitorKeyboard, this);
    mouse_thread_ = std::thread(&ActivityMonitor::monitorMouse, this);
    window_thread_ = std::thread(&ActivityMonitor::monitorWindowFocus, this);
    app_thread_ = std::thread(&ActivityMonitor::monitorApplications, this);
}

void ActivityMonitor::stopMonitoring() {
    if (!running_) return;

    running_ = false;

    if (keyboard_thread_.joinable()) keyboard_thread_.join();
    if (mouse_thread_.joinable()) mouse_thread_.join();
    if (window_thread_.joinable()) window_thread_.join();
    if (app_thread_.joinable()) app_thread_.join();
}

void ActivityMonitor::setCallback(std::function<void(const ActivityEvent&)> callback) {
    callback_ = callback;
}

void ActivityMonitor::monitorKeyboard() {
    // Monitor keyboard events using libevdev
    // Try multiple possible keyboard devices
    const char* device_paths[] = {"/dev/input/event0", "/dev/input/event1", "/dev/input/event2", "/dev/input/event3"};
    int fd = -1;
    struct libevdev* dev = nullptr;

    // Try to find and open a keyboard device
    for (const char* device_path : device_paths) {
        fd = open(device_path, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            if (libevdev_new_from_fd(fd, &dev) >= 0) {
                // Check if this is actually a keyboard
                if (libevdev_has_event_type(dev, EV_KEY)) {
                    break;  // Found a working keyboard device
                } else {
                    libevdev_free(dev);
                    dev = nullptr;
                    close(fd);
                    fd = -1;
                }
            } else {
                close(fd);
                fd = -1;
            }
        }
    }

    if (fd < 0 || !dev) {
        // Silently exit if no keyboard device is available
        // This is expected in many containerized or headless environments
        return;
    }

    struct input_event ev;
    while (running_) {
        int rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (rc == 0 && ev.type == EV_KEY && ev.value == 1) {  // Key press
            if (callback_) {
                auto now = std::chrono::system_clock::now();
                auto time_t = std::chrono::system_clock::to_time_t(now);
                std::stringstream ss;
                ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");

                ActivityEvent event{
                    ss.str(),
                    "keyboard",
                    "Key pressed: " + std::to_string(ev.code),
                    "current_user"
                };
                callback_(event);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    libevdev_free(dev);
    close(fd);
}

void ActivityMonitor::monitorMouse() {
    // Monitor mouse events using libevdev
    // Try multiple possible mouse devices
    const char* device_paths[] = {"/dev/input/event1", "/dev/input/event2", "/dev/input/event3", "/dev/input/event4"};
    int fd = -1;
    struct libevdev* dev = nullptr;

    // Try to find and open a mouse device
    for (const char* device_path : device_paths) {
        fd = open(device_path, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            if (libevdev_new_from_fd(fd, &dev) >= 0) {
                // Check if this is actually a mouse/touchpad
                if (libevdev_has_event_type(dev, EV_REL) || libevdev_has_event_type(dev, EV_KEY)) {
                    break;  // Found a working mouse device
                } else {
                    libevdev_free(dev);
                    dev = nullptr;
                    close(fd);
                    fd = -1;
                }
            } else {
                close(fd);
                fd = -1;
            }
        }
    }

    if (fd < 0 || !dev) {
        // Silently exit if no mouse device is available
        // This is expected in many containerized or headless environments
        return;
    }

    struct input_event ev;
    while (running_) {
        int rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (rc == 0 && (ev.type == EV_REL || ev.type == EV_KEY)) {
            if (callback_) {
                auto now = std::chrono::system_clock::now();
                auto time_t = std::chrono::system_clock::to_time_t(now);
                std::stringstream ss;
                ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");

                std::string details = (ev.type == EV_REL) ? "Mouse movement" : "Mouse click";
                ActivityEvent event{
                    ss.str(),
                    "mouse",
                    details,
                    "current_user"
                };
                callback_(event);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    libevdev_free(dev);
    close(fd);
}

void ActivityMonitor::monitorWindowFocus() {
    // Wayland window focus monitoring
    // Since Wayland restricts direct window access, we'll use system tools
    // to monitor active windows via polling

    std::string last_window_title = "";
    std::string last_app_name = "";

    while (running_) {
        std::string current_window_title = getActiveWindowTitle();
        std::string current_app_name = getActiveApplication();

        // Check if window focus has changed
        if ((current_window_title != last_window_title || current_app_name != last_app_name) &&
            (!current_window_title.empty() || !current_app_name.empty())) {

            last_window_title = current_window_title;
            last_app_name = current_app_name;

            if (callback_) {
                auto now = std::chrono::system_clock::now();
                auto time_t = std::chrono::system_clock::to_time_t(now);
                std::stringstream ss;
                ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");

                std::string details = "Window focus changed - " + current_app_name;
                if (!current_window_title.empty()) {
                    details += " (" + current_window_title + ")";
                }

                ActivityEvent event{
                    ss.str(),
                    "window",
                    details,
                    "current_user"
                };
                callback_(event);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void ActivityMonitor::monitorApplications() {
    std::set<std::string> previous_applications;

    while (running_) {
        std::set<std::string> current_applications = getRunningApplications();

        // Find newly started applications
        for (const auto& app : current_applications) {
            if (previous_applications.find(app) == previous_applications.end()) {
                if (callback_) {
                    auto now = std::chrono::system_clock::now();
                    auto time_t = std::chrono::system_clock::to_time_t(now);
                    std::stringstream ss;
                    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");

                    ActivityEvent event{
                        ss.str(),
                        "application",
                        "Application started: " + app,
                        "current_user"
                    };
                    callback_(event);
                }
            }
        }

        // Find applications that have stopped
        for (const auto& app : previous_applications) {
            if (current_applications.find(app) == current_applications.end()) {
                if (callback_) {
                    auto now = std::chrono::system_clock::now();
                    auto time_t = std::chrono::system_clock::to_time_t(now);
                    std::stringstream ss;
                    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");

                    ActivityEvent event{
                        ss.str(),
                        "application",
                        "Application stopped: " + app,
                        "current_user"
                    };
                    callback_(event);
                }
            }
        }

        previous_applications = current_applications;
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

std::string ActivityMonitor::getActiveWindowTitle() {
    // Use system tools to get active window title on Wayland
    // This is a fallback approach since direct Wayland access is restricted

    // Check if Sway is available before using swaymsg
    bool sway_available = (getenv("SWAYSOCK") != nullptr) &&
                         (system("swaymsg -t get_version >/dev/null 2>&1") == 0);

    std::string command = "xdotool getactivewindow getwindowname 2>/dev/null || ";
    if (sway_available) {
        command += "swaymsg -t get_tree | jq -r '.. | select(.focused? == true).name' 2>/dev/null || ";
    }
    command += "echo 'Unknown'";

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
    }
    pclose(fp);
    return result;
}

std::set<std::string> ActivityMonitor::getRunningApplications() {
    std::set<std::string> applications;

    // Use ps command to get running processes
    // Filter out system processes and get unique application names
    std::string command = "ps -eo comm --no-headers | grep -v -E '^(ps|grep|bash|sh|systemd|init|kthreadd|kworker|migration|idle_inject|cpuhp|watchdog|ksoftirqd|rcu_|ktimersoftd|netns| khungtaskd|writeback|ksmd|khugepaged|crypto|kintegrityd|kblockd|ata_sff|md|edac|devfreq|watchdog|kswapd|ecryptfs|cryptd)$' | sort | uniq";

    FILE* fp = popen(command.c_str(), "r");
    if (!fp) return applications;

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
        std::string app_name = buffer;
        // Remove trailing newline
        if (!app_name.empty() && app_name.back() == '\n') {
            app_name.pop_back();
        }
        // Skip empty lines and system processes
        if (!app_name.empty() && app_name.length() > 2) {
            applications.insert(app_name);
        }
    }

    pclose(fp);
    return applications;
}

std::string ActivityMonitor::getActiveApplication() {
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
