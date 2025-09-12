# Developer Guide

This guide provides comprehensive information for developers who want to contribute to, extend, or integrate with the Workforce Monitoring and Insider Risk Management Agent.

## Development Environment Setup

### Prerequisites

**System Requirements:**
- Linux (Ubuntu 18.04+ or Debian 10+)
- GCC 7.0+ or Clang 6.0+
- CMake 3.16+
- Python 3.8+
- Git

**Development Tools:**
```bash
# Install development dependencies
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    clang-format \
    clang-tidy \
    valgrind \
    gdb \
    python3-dev \
    python3-venv \
    git \
    libssl-dev \
    libcurl4-openssl-dev \
    nlohmann-json3-dev
```

### Clone and Setup Repository

```bash
# Clone the repository
git clone https://github.com/your-org/workforce-monitoring-agent.git
cd workforce-monitoring-agent

# Create Python virtual environment
python3 -m venv venv
source venv/bin/activate

# Install Python dependencies
pip install -r requirements.txt
pip install -r requirements-dev.txt

# Build the C++ agent
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Development Workflow

```bash
# Create a feature branch
git checkout -b feature/new-monitoring-module

# Make your changes
# ... development work ...

# Format code
find src -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Run linting
clang-tidy src/agent/*.cpp -- -I include/

# Run tests
cd build
ctest --output-on-failure

# Commit changes
git add .
git commit -m "Add new monitoring module"

# Push and create PR
git push origin feature/new-monitoring-module
```

## Project Structure

```
├── CMakeLists.txt              # Main build configuration
├── Makefile                    # Convenience build targets
├── requirements.txt            # Python dependencies
├── requirements-dev.txt        # Development dependencies
├── include/                    # C++ header files
│   ├── activity_monitor.h      # Activity monitoring interface
│   ├── behavior_analyzer.h     # Behavioral analysis
│   ├── dlp_monitor.h          # Data loss prevention
│   ├── time_tracker.h         # Time tracking
│   ├── llm_behavior_analyzer.h # LLM-powered analysis
│   └── upgrade_manager.h      # Update management
├── src/
│   ├── agent/                 # C++ agent source
│   │   ├── main.cpp          # Agent entry point
│   │   ├── activity_monitor.cpp
│   │   ├── behavior_analyzer.cpp
│   │   ├── dlp_monitor.cpp
│   │   ├── time_tracker.cpp
│   │   ├── llm_behavior_analyzer.cpp
│   │   └── upgrade_manager.cpp
│   ├── backend/               # Python backend
│   │   ├── app.py            # Flask application
│   │   ├── models.py         # Data models
│   │   ├── routes.py         # API routes
│   │   └── utils.py          # Utility functions
│   └── frontend/              # Web frontend
│       ├── index.html        # Main dashboard
│       ├── css/              # Stylesheets
│       └── js/               # JavaScript
├── tests/                     # Test files
│   ├── test_activities.py    # Activity tests
│   ├── test_productivity.py  # Productivity tests
│   └── test_dlp.py          # DLP tests
├── docs/                      # Documentation
├── installers/               # Installation scripts
└── lib/                      # External libraries
```

## Architecture Overview

### Component Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   C++ Agent     │    │  Python Backend │    │   Web Frontend  │
│                 │    │                 │    │                 │
│ • Activity Mon. │◄──►│ • Flask Server  │◄──►│ • Dashboard     │
│ • DLP Monitor   │    │ • Socket.IO     │    │ • Real-time     │
│ • Behavior Ana. │    │ • ML Models     │    │ • Charts        │
│ • Time Tracker  │    │ • REST API      │    │ • Alerts        │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### C++ Agent Components

#### Activity Monitor
Responsible for tracking user interactions:

```cpp
class ActivityMonitor {
public:
    ActivityMonitor();
    ~ActivityMonitor();

    void startMonitoring();
    void stopMonitoring();
    void setCallback(ActivityCallback callback);

    // Configuration
    void setSampleRate(int rate);
    void setIdleThreshold(int seconds);

private:
    void monitorKeyboard();
    void monitorMouse();
    void monitorWindowFocus();
    void processActivity(const ActivityData& data);
};
```

#### DLP Monitor
Handles data loss prevention:

```cpp
class DLPMonitor {
public:
    void addPolicy(const DLPPolicy& policy);
    void removePolicy(const std::string& policyId);
    std::vector<DLPEvent> getEvents() const;

private:
    void monitorFileSystem();
    void monitorNetwork();
    void checkPolicy(const FileAccess& access);
    void generateAlert(const DLPPolicy& policy, const FileAccess& access);
};
```

#### Behavior Analyzer
Performs anomaly detection and risk assessment:

```cpp
class BehaviorAnalyzer {
public:
    BehaviorAnalyzer();
    void analyzeActivity(const ActivityData& data);
    RiskAssessment assessRisk(const UserProfile& user);
    void setLLMProvider(const std::string& provider);

private:
    void updateUserProfile(const ActivityData& data);
    double calculateAnomalyScore(const ActivityData& data);
    void generateInsights(const UserProfile& user);
};
```

### Python Backend Components

#### Flask Application Structure
```python
# app.py
from flask import Flask
from flask_socketio import SocketIO
from models import db
from routes import api_bp

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*")

# Database
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///workforce.db'
db.init_app(app)

# Register blueprints
app.register_blueprint(api_bp, url_prefix='/api')

# Socket.IO events
@socketio.on('connect')
def handle_connect():
    print('Client connected')

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000)
```

#### Data Models
```python
# models.py
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime

db = SQLAlchemy()

class Activity(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    user = db.Column(db.String(100), nullable=False)
    type = db.Column(db.String(50), nullable=False)
    timestamp = db.Column(db.DateTime, default=datetime.utcnow)
    details = db.Column(db.JSON)
    risk_score = db.Column(db.Float, default=0.0)

class DLPEvent(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    user = db.Column(db.String(100), nullable=False)
    policy_id = db.Column(db.String(100), nullable=False)
    severity = db.Column(db.String(20), nullable=False)
    status = db.Column(db.String(20), default='detected')
    timestamp = db.Column(db.DateTime, default=datetime.utcnow)
    details = db.Column(db.JSON)

class UserProfile(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(100), unique=True, nullable=False)
    department = db.Column(db.String(100))
    risk_score = db.Column(db.Float, default=0.0)
    last_activity = db.Column(db.DateTime)
    behavior_patterns = db.Column(db.JSON)
```

## Adding New Monitoring Features

### 1. Define the Interface

Create a header file in `include/`:

```cpp
// include/new_monitor.h
#ifndef NEW_MONITOR_H
#define NEW_MONITOR_H

#include <string>
#include <functional>

struct NewMonitorData {
    std::string user;
    std::string type;
    double timestamp;
    // Add specific fields for your monitor
};

using NewMonitorCallback = std::function<void(const NewMonitorData&)>;

class NewMonitor {
public:
    NewMonitor();
    ~NewMonitor();

    void start();
    void stop();
    void setCallback(NewMonitorCallback callback);

private:
    void monitoringLoop();
    void processData(const NewMonitorData& data);
};

#endif // NEW_MONITOR_H
```

### 2. Implement the Monitor

Create the implementation in `src/agent/`:

```cpp
// src/agent/new_monitor.cpp
#include "new_monitor.h"
#include <thread>
#include <atomic>
#include <chrono>

NewMonitor::NewMonitor() : running_(false) {}

NewMonitor::~NewMonitor() {
    stop();
}

void NewMonitor::start() {
    if (running_) return;

    running_ = true;
    monitor_thread_ = std::thread(&NewMonitor::monitoringLoop, this);
}

void NewMonitor::stop() {
    if (!running_) return;

    running_ = false;
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
}

void NewMonitor::setCallback(NewMonitorCallback callback) {
    callback_ = callback;
}

void NewMonitor::monitoringLoop() {
    while (running_) {
        // Your monitoring logic here
        NewMonitorData data = collectData();

        if (callback_) {
            callback_(data);
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void NewMonitor::processData(const NewMonitorData& data) {
    // Process and analyze the data
    // This could include filtering, aggregation, etc.
}
```

### 3. Integrate with Main Agent

Update `main.cpp` to include your new monitor:

```cpp
// src/agent/main.cpp
#include "new_monitor.h"

int main(int argc, char* argv[]) {
    // Initialize existing monitors
    ActivityMonitor activity_monitor;
    DLPMonitor dlp_monitor;
    BehaviorAnalyzer behavior_analyzer;

    // Initialize new monitor
    NewMonitor new_monitor;

    // Set up callbacks
    activity_monitor.setCallback([](const ActivityData& data) {
        // Handle activity data
        behavior_analyzer.analyzeActivity(data);
    });

    new_monitor.setCallback([](const NewMonitorData& data) {
        // Handle new monitor data
        std::cout << "New data: " << data.type << std::endl;
    });

    // Start all monitors
    activity_monitor.startMonitoring();
    dlp_monitor.startMonitoring();
    new_monitor.start();

    // Main event loop
    while (true) {
        // Process events, check health, etc.
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
```

### 4. Update Build Configuration

Add your new files to `CMakeLists.txt`:

```cmake
# CMakeLists.txt
set(SOURCES
    src/agent/main.cpp
    src/agent/activity_monitor.cpp
    src/agent/dlp_monitor.cpp
    src/agent/behavior_analyzer.cpp
    src/agent/new_monitor.cpp  # Add your new source file
)

set(HEADERS
    include/activity_monitor.h
    include/dlp_monitor.h
    include/behavior_analyzer.h
    include/new_monitor.h  # Add your new header file
)
```

### 5. Add Backend API Support

Create API endpoints in the Python backend:

```python
# src/backend/routes.py
from flask import Blueprint, request, jsonify
from models import db, NewMonitorData

api_bp = Blueprint('api', __name__)

@api_bp.route('/new-monitor-data', methods=['GET'])
def get_new_monitor_data():
    """Get new monitor data with filtering"""
    user = request.args.get('user')
    limit = int(request.args.get('limit', 100))

    query = NewMonitorData.query
    if user:
        query = query.filter_by(user=user)

    data = query.limit(limit).all()

    return jsonify({
        'status': 'success',
        'data': [item.to_dict() for item in data]
    })

@api_bp.route('/new-monitor-config', methods=['POST'])
def update_new_monitor_config():
    """Update new monitor configuration"""
    config = request.get_json()

    # Validate and update configuration
    # ...

    return jsonify({'status': 'success'})
```

### 6. Update Frontend

Add UI components for your new monitor:

```html
<!-- src/frontend/index.html -->
<div id="new-monitor-section" class="dashboard-section">
    <h3>New Monitor</h3>
    <div id="new-monitor-chart"></div>
    <div id="new-monitor-events"></div>
</div>
```

```javascript
// JavaScript for new monitor
function updateNewMonitorData() {
    fetch('/api/new-monitor-data')
        .then(response => response.json())
        .then(data => {
            updateNewMonitorChart(data);
            updateNewMonitorEvents(data);
        });
}

// Initialize real-time updates
const socket = io();
socket.on('new_monitor_data', (data) => {
    updateNewMonitorDisplay(data);
});
```

## Testing

### Unit Tests

Create unit tests for your components:

```cpp
// tests/test_new_monitor.cpp
#include <gtest/gtest.h>
#include "new_monitor.h"

class NewMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        monitor = std::make_unique<NewMonitor>();
    }

    void TearDown() override {
        monitor->stop();
    }

    std::unique_ptr<NewMonitor> monitor;
};

TEST_F(NewMonitorTest, StartsAndStopsCorrectly) {
    EXPECT_NO_THROW(monitor->start());
    EXPECT_NO_THROW(monitor->stop());
}

TEST_F(NewMonitorTest, ProcessesDataCorrectly) {
    bool callback_called = false;
    NewMonitorData received_data;

    monitor->setCallback([&](const NewMonitorData& data) {
        callback_called = true;
        received_data = data;
    });

    monitor->start();

    // Wait for some data to be processed
    std::this_thread::sleep_for(std::chrono::seconds(2));

    monitor->stop();

    EXPECT_TRUE(callback_called);
    EXPECT_FALSE(received_data.user.empty());
}
```

### Integration Tests

Test component interactions:

```python
# tests/test_integration.py
import pytest
from app import app, socketio
from models import db, Activity

def test_full_monitoring_pipeline():
    """Test the complete monitoring pipeline"""
    with app.app_context():
        # Create test data
        activity = Activity(
            user='test_user',
            type='keyboard',
            details={'keys_pressed': 100}
        )
        db.session.add(activity)
        db.session.commit()

        # Test API endpoint
        response = app.test_client().get('/api/activities')
        assert response.status_code == 200

        data = response.get_json()
        assert data['status'] == 'success'
        assert len(data['data']['activities']) > 0

def test_socketio_realtime_updates():
    """Test real-time updates via Socket.IO"""
    client = socketio.test_client(app)

    # Connect to socket
    assert client.is_connected()

    # Emit test data
    client.emit('test_data', {'user': 'test', 'type': 'activity'})

    # Check if data was received
    received = client.get_received()
    assert len(received) > 0
```

### Performance Testing

```python
# tests/test_performance.py
import time
import psutil
from app import app

def test_monitoring_performance():
    """Test monitoring performance under load"""
    process = psutil.Process()

    start_time = time.time()
    start_memory = process.memory_info().rss

    # Simulate high activity
    with app.app_context():
        for i in range(1000):
            # Create test activities
            pass

    end_time = time.time()
    end_memory = process.memory_info().rss

    duration = end_time - start_time
    memory_used = end_memory - start_memory

    # Assert performance requirements
    assert duration < 10.0  # Should complete within 10 seconds
    assert memory_used < 50 * 1024 * 1024  # Less than 50MB memory usage
```

## Code Quality

### Code Formatting

Use clang-format for consistent code style:

```bash
# Format all C++ files
find src include -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Check formatting without modifying files
find src include -name "*.cpp" -o -name "*.hpp" | xargs clang-format --dry-run --Werror
```

### Static Analysis

Use clang-tidy for static analysis:

```bash
# Run clang-tidy on all source files
clang-tidy src/agent/*.cpp -- -I include/

# Fix auto-fixable issues
clang-tidy src/agent/*.cpp --fix -- -I include/
```

### Code Coverage

Generate coverage reports:

```bash
# Build with coverage
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCOVERAGE=ON
make -j$(nproc)

# Run tests with coverage
ctest --output-on-failure

# Generate coverage report
gcovr -r .. -e "tests/" -e "build/" --html --html-details -o coverage.html
```

## Debugging

### Logging

Configure comprehensive logging:

```cpp
// Include logging header
#include <spdlog/spdlog.h>

// Initialize logger
auto logger = spdlog::stdout_color_mt("monitor");

// Log different levels
logger->info("Monitor started");
logger->debug("Processing activity: {}", activity.type);
logger->error("Failed to connect to backend: {}", error.message);
```

### Debugging with GDB

```bash
# Build with debug symbols
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Run with gdb
gdb ./workforce_agent

# Set breakpoints
(gdb) break main
(gdb) break ActivityMonitor::processActivity

# Run the program
(gdb) run

# Inspect variables
(gdb) print activity
(gdb) print data.details
```

### Memory Debugging with Valgrind

```bash
# Run with valgrind
valgrind --leak-check=full --show-leak-kinds=all ./workforce_agent

# Profile memory usage
valgrind --tool=massif ./workforce_agent
ms_print massif.out.*
```

## Performance Optimization

### Profiling

Use perf for CPU profiling:

```bash
# Profile the application
perf record -g ./workforce_agent

# Analyze results
perf report

# Generate flame graph
perf script | stackcollapse-perf.pl | flamegraph.pl > flamegraph.svg
```

### Memory Optimization

```cpp
// Use memory pools for frequent allocations
class ActivityPool {
public:
    ActivityData* allocate() {
        if (free_list_.empty()) {
            return new ActivityData();
        }
        ActivityData* data = free_list_.back();
        free_list_.pop_back();
        return data;
    }

    void deallocate(ActivityData* data) {
        free_list_.push_back(data);
    }

private:
    std::vector<ActivityData*> free_list_;
};
```

### Async Processing

```cpp
// Use thread pools for CPU-intensive tasks
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

class ThreadPool {
public:
    ThreadPool(size_t num_threads);
    ~ThreadPool();

    template<class F>
    void enqueue(F&& f);

private:
    void worker();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_;
};
```

## Contributing Guidelines

### Pull Request Process

1. **Fork** the repository
2. **Create** a feature branch from `main`
3. **Make** your changes with tests
4. **Run** all tests and checks
5. **Update** documentation if needed
6. **Commit** with clear messages
7. **Push** to your fork
8. **Create** a Pull Request

### Commit Message Format

```
type(scope): description

[optional body]

[optional footer]
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation
- `style`: Code style changes
- `refactor`: Code refactoring
- `test`: Testing
- `chore`: Maintenance

### Code Review Checklist

- [ ] Code compiles without warnings
- [ ] Unit tests pass
- [ ] Integration tests pass
- [ ] Code is properly formatted
- [ ] Documentation is updated
- [ ] No security vulnerabilities
- [ ] Performance impact assessed
- [ ] Backward compatibility maintained

## Security Considerations

### Secure Coding Practices

```cpp
// Avoid buffer overflows
std::string safe_string(const char* input, size_t max_len) {
    if (!input) return "";
    size_t len = strnlen(input, max_len);
    return std::string(input, len);
}

// Use secure random number generation
#include <random>
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(0, 100);

// Validate input data
bool validate_user_input(const std::string& input) {
    // Check length
    if (input.length() > MAX_INPUT_LENGTH) return false;

    // Check for malicious patterns
    if (input.find("..") != std::string::npos) return false;

    // Use regex for pattern validation
    static const std::regex valid_pattern(R"(^[a-zA-Z0-9_]+$)");
    return std::regex_match(input, valid_pattern);
}
```

### API Security

```python
# Rate limiting
from flask_limiter import Limiter
limiter = Limiter(app, key_func=get_remote_address)

@app.route("/api/activities")
@limiter.limit("1000 per hour")
def get_activities():
    pass

# Input validation
from marshmallow import Schema, fields, ValidationError

class ActivitySchema(Schema):
    user = fields.Str(required=True, validate=lambda x: len(x) <= 100)
    type = fields.Str(required=True, validate=lambda x: x in ['keyboard', 'mouse'])
    timestamp = fields.DateTime(required=True)

# Authentication
from flask_jwt_extended import JWTManager, jwt_required

jwt = JWTManager(app)

@app.route("/api/secure-endpoint")
@jwt_required()
def secure_endpoint():
    pass
```

## Deployment

### Containerization

```dockerfile
# Dockerfile
FROM ubuntu:20.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential cmake python3 python3-pip \
    libssl-dev libcurl4-openssl-dev

# Copy source
COPY . /app
WORKDIR /app

# Build
RUN mkdir build && cd build && cmake .. && make -j$(nproc)
RUN pip3 install -r requirements.txt

# Expose port
EXPOSE 5000

# Run
CMD ["python3", "src/backend/app.py"]
```

### Kubernetes Deployment

```yaml
# deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: workforce-agent
spec:
  replicas: 3
  selector:
    matchLabels:
      app: workforce-agent
  template:
    metadata:
      labels:
        app: workforce-agent
    spec:
      containers:
      - name: agent
        image: workforce-agent:latest
        ports:
        - containerPort: 5000
        env:
        - name: DATABASE_URL
          value: "postgresql://db:5432/workforce"
        resources:
          requests:
            memory: "256Mi"
            cpu: "250m"
          limits:
            memory: "512Mi"
            cpu: "500m"
```

## Support and Resources

### Getting Help

- **Documentation**: Check this developer guide and API docs
- **Community**: Join GitHub Discussions for peer support
- **Issues**: Report bugs and request features on GitHub
- **Security**: Report security issues privately

### Useful Resources

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Flask Documentation](https://flask.palletsprojects.com/)
- [Socket.IO Documentation](https://socketio.readthedocs.io/)
- [CMake Documentation](https://cmake.org/documentation/)

### Development Tools

- **IDE**: Visual Studio Code with C++ extensions
- **Version Control**: Git with GitHub
- **CI/CD**: GitHub Actions
- **Code Quality**: clang-format, clang-tidy, cppcheck
- **Testing**: Google Test, pytest
- **Documentation**: MkDocs, Doxygen
