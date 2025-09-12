# Installation Guide

This guide provides detailed instructions for installing the Workforce Monitoring and Insider Risk Management Agent on Linux systems.

## System Requirements

### Minimum Requirements
- **Operating System**: Ubuntu 18.04+ or Debian 10+
- **Architecture**: x86_64
- **RAM**: 2GB minimum, 4GB recommended
- **Disk Space**: 500MB for installation, 1GB for logs and data
- **Network**: Internet connection for updates and LLM services

### Recommended Requirements
- **Operating System**: Ubuntu 20.04+ or Debian 11+
- **RAM**: 8GB or more
- **CPU**: Multi-core processor (4+ cores)
- **Storage**: SSD with 10GB+ free space

## Quick Installation

### Automated Installation (Recommended)

The easiest way to install is using the automated installer:

```bash
# Clone the repository
git clone https://github.com/your-org/workforce-monitoring-agent.git
cd workforce-monitoring-agent

# Run the installer (requires root/sudo)
sudo ./install.sh

# Optional: Install with eBPF support for advanced network monitoring
sudo ./install.sh --with-bcc
```

The installer will:
- ✅ Install all system dependencies
- ✅ Build and install the C++ monitoring agent
- ✅ Install Python backend dependencies
- ✅ Create systemd service
- ✅ Set up configuration files
- ✅ Configure log rotation
- ✅ Create uninstaller

### Manual Installation

For advanced users or custom deployments:

#### 1. Install System Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libwayland-dev \
    libwayland-client0 \
    wayland-protocols \
    libevdev-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    nlohmann-json3-dev \
    python3 \
    python3-pip \
    python3-dev
```

**Optional: BCC for eBPF Network Monitoring**
```bash
sudo apt-get install -y bcc-tools libbcc-dev
```

**Red Hat/CentOS:**
```bash
sudo yum groupinstall "Development Tools"
sudo yum install -y \
    cmake \
    wayland-devel \
    libevdev-devel \
    openssl-devel \
    libcurl-devel \
    json-devel \
    python3 \
    python3-pip \
    python3-devel
```

#### 2. Build the C++ Agent

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the agent
make -j$(nproc)

# Optional: Install system-wide
sudo make install
```

#### 3. Install Python Dependencies

```bash
# Install Python requirements
pip3 install -r requirements.txt

# Or install in virtual environment (recommended)
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

## Post-Installation Configuration

### Service Management

After installation, the agent runs as a systemd service:

```bash
# Start the service
sudo systemctl start workforce-agent

# Enable auto-start on boot
sudo systemctl enable workforce-agent

# Check service status
sudo systemctl status workforce-agent

# View service logs
sudo journalctl -u workforce-agent -f

# Restart the service
sudo systemctl restart workforce-agent
```

### Configuration Files

Default configuration files are installed to `/etc/workforce-agent/`:

- `agent_config.json` - Main agent configuration
- `upgrade_config.json` - Update management settings
- `dlp_policies.json` - Data loss prevention rules

### User Permissions

The agent requires certain permissions for monitoring:

```bash
# Add user to input group for keyboard/mouse monitoring
sudo usermod -a -G input $USER

# For eBPF network monitoring (if using BCC)
sudo usermod -a -G bcc $USER

# Logout and login again for group changes to take effect
```

## Verification

### Check Installation

```bash
# Verify agent binary
workforce_agent --version

# Check Python backend
cd src/backend
python3 -c "import flask, sklearn; print('Dependencies OK')"

# Test service
sudo systemctl status workforce-agent
```

### Access Web Dashboard

1. Open your browser
2. Navigate to `http://localhost:5000`
3. Default credentials: `admin` / `admin` (change immediately)

### Test Monitoring Features

```bash
# Test activity monitoring
curl http://localhost:5000/api/activities

# Test DLP status
curl http://localhost:5000/api/dlp-events

# Test productivity analytics
curl http://localhost:5000/api/productivity
```

## Troubleshooting Installation Issues

### Common Issues

#### CMake Build Failures
```bash
# Clean build and retry
cd build
rm -rf *
cmake ..
make clean
make
```

#### Missing Dependencies
```bash
# Check for missing packages
sudo apt-get install -y libwayland-dev libevdev-dev

# For Python issues
pip3 install --upgrade pip
pip3 install -r requirements.txt --force-reinstall
```

#### Permission Errors
```bash
# Fix input device permissions
sudo chmod 666 /dev/input/event*

# Add user to required groups
sudo usermod -a -G input,bcc $USER
```

#### Service Won't Start
```bash
# Check service logs
sudo journalctl -u workforce-agent -n 50

# Manual start for debugging
sudo /opt/workforce-agent/bin/workforce_agent
```

## Advanced Installation Options

### Docker Installation

```dockerfile
FROM ubuntu:20.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential cmake python3 python3-pip \
    libwayland-dev libevdev-dev libssl-dev

# Copy source
COPY . /app
WORKDIR /app

# Build and install
RUN mkdir build && cd build && cmake .. && make
RUN pip3 install -r requirements.txt

# Run
CMD ["./build/workforce_agent"]
```

### Cluster Deployment

For multi-server deployments:

1. Install on each server using automated installer
2. Configure central logging server
3. Set up centralized dashboard
4. Configure shared DLP policies

### Offline Installation

For air-gapped environments:

1. Download all dependencies on internet-connected machine
2. Transfer installation packages to target system
3. Use local package repositories
4. Configure offline update mechanism

## Uninstallation

### Using Automated Uninstaller

```bash
sudo /opt/workforce-agent/uninstall.sh
```

### Manual Uninstallation

```bash
# Stop and disable service
sudo systemctl stop workforce-agent
sudo systemctl disable workforce-agent

# Remove files
sudo rm -rf /opt/workforce-agent
sudo rm -rf /etc/workforce-agent
sudo rm -rf /var/log/workforce-agent

# Remove user
sudo userdel workforce-agent

# Clean up dependencies (optional)
sudo apt-get autoremove
```

## Next Steps

After successful installation:

1. **Configure DLP Policies** - Set up data protection rules
2. **Enable LLM Analysis** - Configure AI-powered behavioral analysis
3. **Set up Alerts** - Configure notification channels
4. **Review Dashboard** - Familiarize yourself with the interface
5. **Test Monitoring** - Verify all features are working
6. **User Training** - Train staff on monitoring policies

For detailed configuration options, see the [Configuration Guide](CONFIGURATION.md).
