# Workforce Monitoring and Insider Risk Management Agent

A comprehensive system for monitoring employee productivity, ensuring compliance, and detecting insider risks using Linux endpoint monitoring and AI-powered analysis.

## Features

### Core Monitoring Components
- **User Activity Tracking**: Real-time monitoring of keyboard, mouse, and window focus events
- **Data Loss Prevention (DLP)**: Prevents unauthorized transfer of sensitive data
  - File system monitoring with inotify
  - Network transfer monitoring with eBPF
- **Time Tracking**: Application usage and productivity analysis
- **Behavior Analytics**: AI-driven anomaly detection and risk assessment
- **LLM-Powered Behavioral Analysis**: Advanced AI analysis using OpenAI GPT-4 or Anthropic Claude
  - Real-time behavioral pattern analysis
  - Intelligent risk assessment and anomaly detection
  - Automated security recommendations
  - Contextual threat analysis

### Backend Features
- **Real-time Data Processing**: Flask-SocketIO for live data streaming
- **Machine Learning**: Isolation Forest and Autoencoder for anomaly detection
- **RESTful API**: Comprehensive endpoints for data access
- **Data Persistence**: In-memory storage with cleanup mechanisms

### Frontend Dashboard
- **Real-time Visualization**: Live charts and metrics updates
- **Alert System**: Real-time notifications for security events
- **Risk Analysis**: Interactive risk distribution charts
- **Activity Monitoring**: Live activity feed and trends

## Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   C++ Agent     │    │  Python Backend │    │   Web Frontend  │
│                 │    │                 │    │                 │
│ • Activity Mon. │◄──►│ • Flask Server  │◄──►│ • Dashboard     │
│ • DLP Monitor   │    │ • Socket.IO     │    │ • Real-time     │
│ • Time Tracker  │    │ • ML Models     │    │ • Charts        │
│ • Behavior Ana. │    │ • REST API      │    │ • Alerts        │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## Prerequisites

### System Requirements
- Linux (Ubuntu/Debian recommended)
- CMake 3.16+
- C++17 compatible compiler
- Python 3.8+
- Wayland development libraries

### Dependencies
```bash
# C++ Dependencies
sudo apt-get update
sudo apt-get install libwayland-dev libwayland-client0 wayland-protocols libevdev-dev libssl-dev

# Required for upgrade functionality
sudo apt-get install libcurl4-openssl-dev libssl-dev nlohmann-json3-dev

# Optional: BCC for eBPF network monitoring
sudo apt-get install bcc-tools libbcc-dev

# Python Dependencies (handled by requirements.txt)
pip install -r requirements.txt
```

## Installation

### Quick Install (Recommended)

The easiest way to install the Workforce Monitoring Agent is using the automated installer:

```bash
# Clone the repository
git clone <repository-url>
cd workforce-monitoring-agent

# Run the installer (requires root/sudo)
sudo ./install.sh

# Or with BCC support for eBPF network monitoring
sudo ./install.sh --with-bcc
```

The installer will:
- ✅ Install all system dependencies
- ✅ Build and install the C++ agent
- ✅ Install Python dependencies
- ✅ Create systemd service
- ✅ Set up configuration files
- ✅ Configure log rotation
- ✅ Create uninstaller

### Manual Installation

If you prefer manual installation:

1. **Clone the repository**
   ```bash
   git clone <repository-url>
   cd workforce-monitoring-agent
   ```

2. **Install system dependencies**
   ```bash
   # Ubuntu/Debian
   sudo apt-get update
   sudo apt-get install build-essential cmake libwayland-dev libwayland-client0 wayland-protocols libevdev-dev libssl-dev libcurl4-openssl-dev nlohmann-json3-dev python3 python3-pip

   # Optional: BCC for eBPF
   sudo apt-get install bcc-tools libbcc-dev
   ```

3. **Build the C++ Agent**
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

4. **Install Python Dependencies**
   ```bash
   pip3 install -r requirements.txt
   ```

### Using Makefile

The project includes a Makefile for easier building:

```bash
# Build the agent
make build

# Install system-wide (requires sudo)
make install

# Setup development environment
make dev-setup

# Build and run in development mode
make dev-run

# Clean build files
make clean

# Show all available targets
make help
```

## Usage

### Service Management (After Installation)

After installation, the agent runs as a systemd service:

```bash
# Start the service
sudo systemctl start workforce-agent

# Stop the service
sudo systemctl stop workforce-agent

# Check service status
sudo systemctl status workforce-agent

# Enable auto-start on boot
sudo systemctl enable workforce-agent

# View service logs
sudo journalctl -u workforce-agent

# Restart the service
sudo systemctl restart workforce-agent
```

### Manual Usage (Development)

For development and testing:

1. **Start the Backend Server**
   ```bash
   cd src/backend
   python3 app.py
   ```
   The server will start on `http://localhost:5000`

2. **Run the C++ Agent**
   ```bash
   ./build/workforce_agent
   ```

3. **Access the Dashboard**
   Open your browser and navigate to `http://localhost:5000`

### Configuration Files

After installation, configuration files are located at:
- **Agent Config**: `/etc/workforce-agent/agent_config.json`
- **Upgrade Config**: `/etc/workforce-agent/upgrade_config.json`
- **Logs**: `/var/log/workforce-agent/`
- **Backups**: `/var/backups/workforce-agent/`

### Uninstallation

To uninstall the agent:

```bash
# Using the uninstaller (created during installation)
sudo /opt/workforce-agent/uninstall.sh

# Or manually
sudo systemctl stop workforce-agent
sudo systemctl disable workforce-agent
sudo rm -rf /opt/workforce-agent
sudo rm -rf /etc/workforce-agent
sudo rm -rf /var/log/workforce-agent
sudo userdel workforce-agent
```

## API Endpoints

### Dashboard
- `GET /api/dashboard` - Overview metrics
- `GET /api/activities` - Recent user activities
- `GET /api/dlp-events` - DLP violation events
- `GET /api/productivity` - Productivity analytics
- `GET /api/risk-analysis` - Risk assessment data

### Real-time Events (Socket.IO)
- `agent_data` - Receive monitoring data from agents
- `alert` - Real-time alert notifications
- `data_update` - Live data updates

## Configuration

### DLP Policies
Configure data loss prevention rules in the agent:
```cpp
DLPPolicy policy;
policy.name = "confidential_files";
policy.file_extensions = {".docx", ".xlsx", ".pdf"};
policy.content_patterns = {std::regex("confidential")};
policy.block_transfer = true;
dlp_monitor.addPolicy(policy);
```

### ML Model Parameters
Adjust machine learning settings in `app.py`:
```python
isolation_forest = IsolationForest(contamination=0.1, random_state=42)
```

### eBPF Network Monitoring
The DLP monitor includes optional eBPF-based network monitoring for detecting data exfiltration attempts:

**Features:**
- Real-time monitoring of TCP/UDP network transfers
- Detection of large file transfers (>1MB)
- Monitoring of suspicious protocols (FTP, SSH, SMTP, etc.)
- Integration with DLP policies for restricted destinations

**Requirements:**
- BCC (BPF Compiler Collection) installed
- Linux kernel with eBPF support
- Root privileges for eBPF operations

**Automatic Fallback:**
If BCC is not available, the system automatically falls back to basic network monitoring.

**eBPF Program Details:**
- Monitors `tcp_sendmsg` and `udp_sendmsg` kernel functions
- Captures source/destination IPs, ports, and transfer sizes
- Uses perf buffers for efficient event delivery
- Filters out small transfers (<100 bytes) for performance

### Upgrade Management
The agent includes a comprehensive upgrade system for seamless updates:

**Features:**
- **Automatic Update Checking**: Configurable intervals for update checks
- **Secure Downloads**: HTTPS downloads with checksum verification
- **Atomic Upgrades**: Backup and rollback capabilities
- **Progress Tracking**: Real-time progress reporting
- **Signature Verification**: Optional cryptographic signature validation

**Configuration:**
```json
{
  "update_server_url": "https://api.example.com/updates",
  "auto_update_interval": 60,
  "backup_enabled": true,
  "backup_directory": "/var/backups/workforce_agent",
  "temp_directory": "/tmp/workforce_agent_updates"
}
```

**Upgrade Process:**
1. Check for available updates from configured server
2. Download and verify update package integrity
3. Create backup of current version
4. Install new version atomically
5. Automatic rollback on failure
6. Cleanup temporary files and old backups

### LLM-Powered Behavioral Analysis

The agent includes advanced AI-powered behavioral analysis using Large Language Models (LLMs) for intelligent threat detection and risk assessment:

**Features:**
- **Intelligent Pattern Recognition**: Uses GPT-4 or Claude to analyze complex behavioral patterns
- **Contextual Risk Assessment**: Understands context and intent behind user actions
- **Automated Recommendations**: Generates specific security recommendations based on analysis
- **Real-time Analysis**: Processes behavior data in real-time with configurable intervals
- **Multi-Provider Support**: Supports OpenAI GPT-4 and Anthropic Claude models

**Supported LLM Providers:**
- **OpenAI GPT-4**: Industry-leading language model for behavioral analysis
- **Anthropic Claude**: Advanced AI model with strong reasoning capabilities
- **Local Models**: Support for local LLM deployments (future enhancement)

**Configuration:**

**Method 1: Environment Variables (Recommended)**
```bash
# Set LLM provider and API keys
export LLM_PROVIDER=openai
export OPENAI_API_KEY=your-openai-api-key-here

# Or use Anthropic
export LLM_PROVIDER=anthropic
export ANTHROPIC_API_KEY=your-anthropic-api-key-here
```

**Method 2: Direct Configuration in Code**
```cpp
behavior_analyzer.enableLLMAnalysis(true);
behavior_analyzer.setLLMProvider("openai");
behavior_analyzer.setLLMAPIKey("openai", "your-api-key");
behavior_analyzer.setLLMModel("openai", "gpt-4");
behavior_analyzer.startLLMAnalysis();
```

**Analysis Capabilities:**

**Behavioral Pattern Analysis:**
- Detects unusual activity sequences
- Identifies potential insider threats
- Analyzes productivity patterns
- Recognizes suspicious data access patterns

**Risk Assessment:**
- **Low**: Normal behavior with minor deviations
- **Medium**: Unusual patterns requiring attention
- **High**: Suspicious activities needing immediate review
- **Critical**: Potential security incidents requiring action

**Automated Recommendations:**
- Specific security measures to implement
- Policy adjustments based on observed behavior
- Monitoring suggestions for similar patterns
- Risk mitigation strategies

**Example LLM Analysis Output:**
```
[LLM Analysis] User: john.doe | Type: pattern | Severity: medium | Confidence: 0.85
Description: Detected unusual file access pattern - accessing sensitive documents outside normal hours
Analysis: User typically accesses sensitive files during business hours (9AM-5PM) but recent activity shows access at 2:30 AM
Recommendations: Enable additional monitoring for after-hours access | Review access logs for similar patterns | Consider implementing time-based access restrictions
```

**Performance Considerations:**
- **API Rate Limits**: Respects LLM provider rate limits with intelligent queuing
- **Cost Optimization**: Configurable analysis intervals (default: 5 minutes)
- **Caching**: Avoids redundant analysis of similar patterns
- **Fallback Mode**: Continues traditional analysis if LLM is unavailable

**Security Features:**
- **API Key Protection**: Keys stored securely, not logged in plain text
- **Data Minimization**: Only sends necessary behavioral data to LLM
- **Audit Trail**: All LLM interactions logged for compliance
- **Offline Capability**: Functions without LLM when API unavailable

**Integration with Existing Systems:**
- **Seamless Integration**: Works alongside traditional ML-based analysis
- **Unified Interface**: All insights presented through same callback system
- **Consistent Output**: LLM insights formatted as standard behavior patterns
- **Combined Intelligence**: Traditional ML + LLM analysis for comprehensive coverage

**Monitoring and Maintenance:**
- **Health Checks**: Automatic verification of LLM API connectivity
- **Usage Tracking**: Monitors API usage and costs
- **Error Handling**: Graceful degradation when LLM services unavailable
- **Configuration Updates**: Runtime reconfiguration without restart

## Security Considerations

1. **Data Encryption**: All data transmission should use HTTPS in production
2. **Access Control**: Implement proper authentication and authorization
3. **Data Retention**: Configure appropriate data cleanup policies
4. **Privacy Compliance**: Ensure compliance with local data protection regulations

## Development

### Project Structure
```
├── CMakeLists.txt          # C++ build configuration
├── requirements.txt        # Python dependencies
├── include/               # C++ header files
│   ├── activity_monitor.h
│   ├── dlp_monitor.h
│   ├── time_tracker.h
│   └── behavior_analyzer.h
├── src/
│   ├── agent/            # C++ agent source
│   ├── backend/          # Python backend
│   └── frontend/         # Web frontend
├── docs/                 # Documentation
└── lib/                  # External libraries
```

### Adding New Monitoring Features
1. Create header file in `include/`
2. Implement source file in `src/agent/`
3. Update `CMakeLists.txt`
4. Integrate with main agent in `main.cpp`
5. Add backend API endpoints
6. Update frontend dashboard

## Troubleshooting

### Common Issues

1. **Wayland Libraries Not Found**
   ```bash
   sudo apt-get install libwayland-dev libwayland-client0 wayland-protocols
   ```

2. **Permission Denied for Input Devices**
   ```bash
   sudo usermod -a -G input $USER
   # Logout and login again
   ```

3. **Port Already in Use**
   ```bash
   lsof -ti:5000 | xargs kill -9
   ```

4. **ML Model Training Issues**
   - Ensure sufficient data for training
   - Check feature extraction logic
   - Verify scikit-learn and TensorFlow versions

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Disclaimer

This software is for monitoring and security purposes. Ensure compliance with local laws and regulations regarding employee monitoring. Always inform users about monitoring activities and obtain necessary consents.
