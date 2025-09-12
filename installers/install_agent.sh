#!/bin/bash

# Workforce Monitoring Agent Installer
# This script installs the workforce monitoring agent and all its dependencies

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
INSTALL_DIR="/opt/wm-agent"
CONFIG_DIR="/etc/wm-agent"
LOG_DIR="/var/log/wm-agent"
BACKUP_DIR="/var/backups/wm-agent"
TEMP_DIR="/tmp/wm-agent-install"
SERVICE_NAME="wm-agent"
USER_NAME="wm-agent"

# Functions
print_header() {
    echo -e "${BLUE}================================================${NC}"
    echo -e "${BLUE}  Workforce Monitoring Agent Installer${NC}"
    echo -e "${BLUE}================================================${NC}"
    echo
}

print_step() {
    echo -e "${GREEN}[STEP]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "This installer must be run as root (sudo)"
        exit 1
    fi
}

detect_os() {
    if [[ -f /etc/debian_version ]]; then
        OS="debian"
        PACKAGE_MANAGER="apt-get"
    elif [[ -f /etc/redhat-release ]]; then
        OS="redhat"
        PACKAGE_MANAGER="yum"
    elif [[ -f /etc/fedora-release ]]; then
        OS="fedora"
        PACKAGE_MANAGER="dnf"
    else
        print_error "Unsupported operating system"
        exit 1
    fi
}

install_system_dependencies() {
    print_step "Installing system dependencies..."

    case $OS in
        debian)
            $PACKAGE_MANAGER update
            $PACKAGE_MANAGER install -y \
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
                python3-dev \
                pkg-config \
                sway \
                jq \
                xdotool
            ;;
        redhat)
            $PACKAGE_MANAGER update -y
            $PACKAGE_MANAGER install -y \
                gcc \
                gcc-c++ \
                make \
                cmake \
                wayland-devel \
                wayland-protocols-devel \
                libevdev-devel \
                openssl-devel \
                libcurl-devel \
                json-devel \
                python3 \
                python3-pip \
                python3-devel \
                pkgconfig \
                sway \
                jq \
                xdotool
            ;;
        fedora)
            $PACKAGE_MANAGER update -y
            $PACKAGE_MANAGER install -y \
                gcc \
                gcc-c++ \
                make \
                cmake \
                wayland-devel \
                wayland-protocols-devel \
                libevdev-devel \
                openssl-devel \
                libcurl-devel \
                json-devel \
                python3 \
                python3-pip \
                python3-devel \
                pkgconfig \
                sway \
                jq \
                xdotool
            ;;
    esac

    # Optional BCC installation for eBPF
    if [[ "$INSTALL_BCC" == "yes" ]]; then
        print_step "Installing BCC for eBPF support..."
        case $OS in
            debian)
                $PACKAGE_MANAGER install -y bcc-tools libbcc-dev
                ;;
            redhat|fedora)
                $PACKAGE_MANAGER install -y bcc-tools bcc-devel
                ;;
        esac
    fi

    print_success "System dependencies installed"
}

create_directories() {
    print_step "Creating directories..."

    # Check if /opt is writable, if not use alternative location
    if ! mkdir -p "$INSTALL_DIR" 2>/dev/null; then
        print_warning "/opt is read-only, using /usr/local as alternative installation directory"
        INSTALL_DIR="/usr/local/wm-agent"
        mkdir -p "$INSTALL_DIR" || {
            print_error "Failed to create installation directory in /usr/local either"
            exit 1
        }
    fi

    # Check if /etc is writable
    if ! mkdir -p "$CONFIG_DIR" 2>/dev/null; then
        print_warning "/etc is read-only, using /usr/local/etc as alternative config directory"
        CONFIG_DIR="/usr/local/etc/wm-agent"
        mkdir -p "$CONFIG_DIR" || {
            print_error "Failed to create config directory in /usr/local/etc either"
            exit 1
        }
    fi

    # Check if /var/log is writable
    if ! mkdir -p "$LOG_DIR" 2>/dev/null; then
        print_warning "/var/log is read-only, using /usr/local/var/log as alternative log directory"
        LOG_DIR="/usr/local/var/log/wm-agent"
        mkdir -p "$LOG_DIR" || {
            print_error "Failed to create log directory in /usr/local/var/log either"
            exit 1
        }
    fi

    # Check if /var/backups is writable
    if ! mkdir -p "$BACKUP_DIR" 2>/dev/null; then
        print_warning "/var/backups is read-only, using /usr/local/var/backups as alternative backup directory"
        BACKUP_DIR="/usr/local/var/backups/wm-agent"
        mkdir -p "$BACKUP_DIR" || {
            print_error "Failed to create backup directory in /usr/local/var/backups either"
            exit 1
        }
    fi

    # /tmp should always be writable
    mkdir -p "$TEMP_DIR" || {
        print_error "Failed to create temporary directory"
        exit 1
    }

    # Set proper permissions
    chown -R root:root "$INSTALL_DIR" 2>/dev/null || true
    chown -R root:root "$CONFIG_DIR" 2>/dev/null || true
    chmod -R 755 "$INSTALL_DIR" 2>/dev/null || true
    chmod -R 755 "$CONFIG_DIR" 2>/dev/null || true
    chmod -R 755 "$LOG_DIR" 2>/dev/null || true
    chmod -R 700 "$BACKUP_DIR" 2>/dev/null || true

    print_success "Directories created"
}

create_user() {
    print_step "Creating service user..."

    if ! id "$USER_NAME" &>/dev/null; then
        useradd -r -s /bin/false -d "$INSTALL_DIR" "$USER_NAME"
        print_success "User $USER_NAME created"
    else
        print_warning "User $USER_NAME already exists"
    fi
}

build_agent() {
    local source_dir="$1"
    print_step "Building C++ agent..."

    # Create build directory in temp
    mkdir -p "$TEMP_DIR/build"
    cd "$TEMP_DIR/build"

    # Configure and build from source directory
    cmake -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" "$source_dir"
    make -j$(nproc)

    # Install
    make install

    print_success "C++ agent built and installed"
}

install_python_dependencies() {
    local source_dir="$1"
    local requirements_file="$2"
    print_step "Installing Python dependencies..."

    echo "Source directory: $source_dir"
    echo "Requirements file: $requirements_file"

    # Verify requirements file exists
    if [[ ! -f "$requirements_file" ]]; then
        print_error "Requirements file not found: $requirements_file"
        exit 1
    fi

    # First, try to install available packages with apt
    print_step "Installing Python packages via apt..."
    case $OS in
        debian)
            $PACKAGE_MANAGER install -y \
                python3-flask \
                python3-requests \
                python3-psutil \
                python3-numpy \
                python3-pandas \
                python3-matplotlib \
                python3-seaborn \
                python3-scipy \
                python3-pip \
                python3-venv \
                2>/dev/null || true
            ;;
        redhat)
            $PACKAGE_MANAGER install -y \
                python3-flask \
                python3-requests \
                python3-psutil \
                python3-numpy \
                python3-pandas \
                python3-matplotlib \
                python3-seaborn \
                python3-scipy \
                python3-pip \
                2>/dev/null || true
            ;;
        fedora)
            $PACKAGE_MANAGER install -y \
                python3-flask \
                python3-requests \
                python3-psutil \
                python3-numpy \
                python3-pandas \
                python3-seaborn \
                python3-scipy \
                python3-pip \
                2>/dev/null || true
            ;;
    esac

    # Create a virtual environment for the remaining packages
    print_step "Creating Python virtual environment..."
    python3 -m venv "$INSTALL_DIR/venv"

    # Activate virtual environment and install packages
    print_step "Installing remaining Python packages..."
    source "$INSTALL_DIR/venv/bin/activate"

    # Ensure we're using the correct pip and navigate to source directory
    cd "$source_dir"

    # Upgrade pip and install setuptools first
    python -m pip install --upgrade pip setuptools wheel

    # Install packages with more robust error handling
    if python -m pip install -r requirements.txt; then
        print_success "All Python packages installed successfully"
    else
        print_warning "Some packages failed to install, trying alternative approach..."

        # Try installing packages individually with fallback
        while IFS= read -r package || [[ -n "$package" ]]; do
            # Skip empty lines and comments
            [[ -z "$package" || "$package" =~ ^[[:space:]]*# ]] && continue

            echo "Installing $package..."
            if ! python -m pip install "$package"; then
                print_warning "Failed to install $package, skipping..."
            fi
        done < requirements.txt

        print_success "Python package installation completed (with some potential skips)"
    fi

    # Deactivate virtual environment
    deactivate

    print_success "Python dependencies installed"
}

setup_configuration() {
    local source_dir="$1"
    local upgrade_config_file="$2"
    print_step "Setting up configuration files..."

    # Copy configuration files from source directory
    if [[ -f "$upgrade_config_file" ]]; then
        cp "$upgrade_config_file" "$CONFIG_DIR/"
    fi

    # Create main configuration
    cat > "$CONFIG_DIR/agent_config.json" << EOF
{
  "agent": {
    "name": "wm-agent",
    "version": "1.0.0",
    "log_level": "info",
    "log_file": "$LOG_DIR/agent.log"
  },
  "monitoring": {
    "enable_activity_monitoring": true,
    "enable_dlp_monitoring": true,
    "enable_time_tracking": true,
    "enable_behavior_analysis": true
  },
  "dlp": {
    "policies": [
      {
        "name": "confidential_files",
        "file_extensions": [".docx", ".xlsx", ".pdf", ".txt"],
        "content_patterns": ["confidential", "secret", "internal"],
        "block_transfer": false
      }
    ]
  },
  "backend": {
    "host": "localhost",
    "port": 5000,
    "ssl_enabled": false
  }
}
EOF

    # Set permissions
    chown root:root "$CONFIG_DIR"/*.json
    chmod 644 "$CONFIG_DIR"/*.json

    print_success "Configuration files created"
}

create_service_file() {
    print_step "Creating systemd service..."

    # Check if systemd is available and writable
    if command -v systemctl >/dev/null 2>&1 && mkdir -p "/etc/systemd/system" 2>/dev/null; then
        cat > "/etc/systemd/system/$SERVICE_NAME.service" << EOF
[Unit]
Description=Workforce Monitoring Agent
After=network.target
Wants=network.target

[Service]
Type=simple
User=$USER_NAME
Group=$USER_NAME
ExecStart=$INSTALL_DIR/bin/wm-agent
Restart=always
RestartSec=5
Environment=HOME=$INSTALL_DIR

# Security settings
NoNewPrivileges=yes
PrivateTmp=yes
ProtectSystem=strict
ProtectHome=yes
ReadWritePaths=$LOG_DIR $BACKUP_DIR
ProtectKernelTunables=yes
ProtectControlGroups=yes

# Logging
StandardOutput=journal
StandardError=journal
SyslogIdentifier=$SERVICE_NAME

[Install]
WantedBy=multi-user.target
EOF

        # Reload systemd
        systemctl daemon-reload 2>/dev/null || print_warning "Failed to reload systemd"

        print_success "Systemd service created"
    else
        print_warning "Systemd not available or /etc read-only, skipping service creation"
        print_warning "You can manually create the service or run the agent directly:"
        print_warning "  $INSTALL_DIR/bin/wm-agent"
    fi
}

setup_logrotate() {
    print_step "Setting up log rotation..."

    # Check if logrotate is available and /etc/logrotate.d is writable
    if command -v logrotate >/dev/null 2>&1 && mkdir -p "/etc/logrotate.d" 2>/dev/null; then
        cat > "/etc/logrotate.d/$SERVICE_NAME" << EOF
$LOG_DIR/*.log {
    daily
    missingok
    rotate 52
    compress
    delaycompress
    notifempty
    create 644 $USER_NAME $USER_NAME
    postrotate
        systemctl reload $SERVICE_NAME 2>/dev/null || true
    endscript
}
EOF

        print_success "Log rotation configured"
    else
        print_warning "Logrotate not available or /etc read-only, skipping log rotation setup"
    fi
}

start_service() {
    print_step "Starting service..."

    # Check if systemd is available
    if command -v systemctl >/dev/null 2>&1; then
        systemctl enable "$SERVICE_NAME" 2>/dev/null || print_warning "Failed to enable service"
        systemctl start "$SERVICE_NAME" 2>/dev/null || print_warning "Failed to start service"

        # Wait a moment and check status
        sleep 2
        if systemctl is-active --quiet "$SERVICE_NAME" 2>/dev/null; then
            print_success "Service started successfully"
        else
            print_warning "Service failed to start. Check logs with: journalctl -u $SERVICE_NAME"
        fi
    else
        print_warning "Systemd not available, cannot start service automatically"
        print_warning "You can run the agent manually: $INSTALL_DIR/bin/wm-agent"
    fi
}

create_uninstaller() {
    print_step "Creating uninstaller..."

    cat > "$INSTALL_DIR/uninstall.sh" << EOF
#!/bin/bash
set -e

SERVICE_NAME="wm-agent"
INSTALL_DIR="$INSTALL_DIR"
CONFIG_DIR="$CONFIG_DIR"
LOG_DIR="$LOG_DIR"
BACKUP_DIR="$BACKUP_DIR"
USER_NAME="wm-agent"

echo "Stopping and disabling service..."
systemctl stop "\$SERVICE_NAME" 2>/dev/null || true
systemctl disable "\$SERVICE_NAME" 2>/dev/null || true

echo "Removing systemd service..."
rm -f "/etc/systemd/system/\$SERVICE_NAME.service"
systemctl daemon-reload 2>/dev/null || true

echo "Removing files and directories..."
rm -rf "\$INSTALL_DIR"
rm -rf "\$CONFIG_DIR"
rm -rf "\$LOG_DIR"
rm -rf "\$BACKUP_DIR"

echo "Removing logrotate configuration..."
rm -f "/etc/logrotate.d/\$SERVICE_NAME"

echo "Removing user..."
userdel "\$USER_NAME" 2>/dev/null || true

echo "Uninstallation completed!"
echo "Note: Configuration backups may remain in \$BACKUP_DIR"
EOF

    chmod +x "$INSTALL_DIR/uninstall.sh"

    print_success "Uninstaller created at $INSTALL_DIR/uninstall.sh"
}

cleanup() {
    print_step "Cleaning up temporary files..."
    rm -rf "$TEMP_DIR"
    print_success "Cleanup completed"
}

show_completion_message() {
    echo
    echo -e "${GREEN}================================================${NC}"
    echo -e "${GREEN}  Installation Completed Successfully!${NC}"
    echo -e "${GREEN}================================================${NC}"
    echo
    echo -e "Installation Details:"
    echo -e "  • Agent installed in: ${BLUE}$INSTALL_DIR${NC}"
    echo -e "  • Configuration: ${BLUE}$CONFIG_DIR${NC}"
    echo -e "  • Logs: ${BLUE}$LOG_DIR${NC}"
    echo -e "  • Backups: ${BLUE}$BACKUP_DIR${NC}"
    echo
    echo -e "Service Management:"
    echo -e "  • Start: ${BLUE}sudo systemctl start $SERVICE_NAME${NC}"
    echo -e "  • Stop: ${BLUE}sudo systemctl stop $SERVICE_NAME${NC}"
    echo -e "  • Status: ${BLUE}sudo systemctl status $SERVICE_NAME${NC}"
    echo -e "  • Logs: ${BLUE}sudo journalctl -u $SERVICE_NAME${NC}"
    echo
    echo -e "Uninstallation:"
    echo -e "  • Run: ${BLUE}sudo $INSTALL_DIR/uninstall.sh${NC}"
    echo
    echo -e "${YELLOW}Next Steps:${NC}"
    echo -e "  1. Configure the agent in $CONFIG_DIR/agent_config.json"
    echo -e "  2. Start the backend server: cd src/backend && python3 app.py"
    echo -e "  3. Access dashboard at: http://localhost:5000"
    echo
}

main() {
    # Parse command line arguments
    INSTALL_BCC="no"
    while [[ $# -gt 0 ]]; do
        case $1 in
            --with-bcc)
                INSTALL_BCC="yes"
                shift
                ;;
            --help)
                echo "Usage: $0 [--with-bcc] [--help]"
                echo "  --with-bcc    Install BCC for eBPF network monitoring"
                echo "  --help        Show this help message"
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done

    print_header
    check_root
    detect_os

    # IMPORTANT: Get the source directory BEFORE any directory changes
    SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    REQUIREMENTS_FILE="$SOURCE_DIR/requirements.txt"
    UPGRADE_CONFIG_FILE="$SOURCE_DIR/upgrade_config.json"

    echo "Installation Configuration:"
    echo "  • Operating System: $OS"
    echo "  • Install Directory: $INSTALL_DIR"
    echo "  • Config Directory: $CONFIG_DIR"
    echo "  • Source Directory: $SOURCE_DIR"
    echo "  • Requirements File: $REQUIREMENTS_FILE"
    echo "  • Service Name: $SERVICE_NAME"
    echo "  • BCC Support: $INSTALL_BCC"
    echo

    # Verify source files exist
    if [[ ! -f "$REQUIREMENTS_FILE" ]]; then
        print_error "Requirements file not found: $REQUIREMENTS_FILE"
        print_error "Please ensure you're running the installer from the source directory"
        exit 1
    fi

    read -p "Continue with installation? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Installation cancelled."
        exit 0
    fi

    # Run installation steps
    install_system_dependencies
    create_directories
    create_user
    build_agent "$SOURCE_DIR"
    install_python_dependencies "$SOURCE_DIR" "$REQUIREMENTS_FILE"
    setup_configuration "$SOURCE_DIR" "$UPGRADE_CONFIG_FILE"
    create_service_file
    setup_logrotate
    create_uninstaller
    cleanup

    # Ask about starting service
    echo
    read -p "Start the service now? (Y/n): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Nn]$ ]]; then
        start_service
    else
        echo "You can start the service later with: sudo systemctl start $SERVICE_NAME"
    fi

    show_completion_message
}

# Run main function
main "$@"
