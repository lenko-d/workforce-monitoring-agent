#!/bin/bash

# Workforce Monitoring Backend Installer
# This script installs only the backend server component

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BACKEND_DIR="/opt/workforce-backend"
CONFIG_DIR="/etc/workforce-backend"
LOG_DIR="/var/log/workforce-backend"
DATA_DIR="/var/lib/workforce-backend"
TEMP_DIR="/tmp/workforce-backend-install"
SERVICE_NAME="workforce-backend"
USER_NAME="workforce-backend"

# Functions
print_header() {
    echo -e "${BLUE}================================================${NC}"
    echo -e "${BLUE}  Workforce Monitoring Backend Installer${NC}"
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
                python3 \
                python3-pip \
                python3-dev \
                python3-venv \
                build-essential \
                pkg-config \
                libssl-dev \
                libffi-dev
            ;;
        redhat)
            $PACKAGE_MANAGER update -y
            $PACKAGE_MANAGER install -y \
                python3 \
                python3-pip \
                python3-devel \
                python3-virtualenv \
                gcc \
                gcc-c++ \
                make \
                pkgconfig \
                openssl-devel \
                libffi-devel
            ;;
        fedora)
            $PACKAGE_MANAGER update -y
            $PACKAGE_MANAGER install -y \
                python3 \
                python3-pip \
                python3-devel \
                python3-virtualenv \
                gcc \
                gcc-c++ \
                make \
                pkgconfig \
                openssl-devel \
                libffi-devel
            ;;
    esac

    print_success "System dependencies installed"
}

create_directories() {
    print_step "Creating directories..."

    mkdir -p "$BACKEND_DIR"
    mkdir -p "$CONFIG_DIR"
    mkdir -p "$LOG_DIR"
    mkdir -p "$DATA_DIR"
    mkdir -p "$TEMP_DIR"

    # Set proper permissions
    chown -R root:root "$BACKEND_DIR"
    chown -R root:root "$CONFIG_DIR"
    chmod -R 755 "$BACKEND_DIR"
    chmod -R 755 "$CONFIG_DIR"
    chmod -R 755 "$LOG_DIR"
    chmod -R 700 "$DATA_DIR"

    print_success "Directories created"
}

create_user() {
    print_step "Creating service user..."

    if ! id "$USER_NAME" &>/dev/null; then
        useradd -r -s /bin/false -d "$BACKEND_DIR" "$USER_NAME"
        print_success "User $USER_NAME created"
    else
        print_warning "User $USER_NAME already exists"
    fi
}

setup_python_environment() {
    local source_dir="$1"
    local requirements_file="$2"
    print_step "Setting up Python virtual environment..."

    # Create virtual environment
    python3 -m venv "$BACKEND_DIR/venv"

    # Activate virtual environment
    source "$BACKEND_DIR/venv/bin/activate"

    # Upgrade pip and install setuptools
    python -m pip install --upgrade pip setuptools wheel

    # Install Python packages
    print_step "Installing Python dependencies..."
    cd "$source_dir"

    if python -m pip install -r "$requirements_file"; then
        print_success "All Python packages installed successfully"
    else
        print_warning "Some packages failed to install, trying alternative approach..."

        # Try installing packages individually
        while IFS= read -r package || [[ -n "$package" ]]; do
            # Skip empty lines and comments
            [[ -z "$package" || "$package" =~ ^[[:space:]]*# ]] && continue

            echo "Installing $package..."
            if ! python -m pip install "$package"; then
                print_warning "Failed to install $package, skipping..."
            fi
        done < "$requirements_file"

        print_success "Python package installation completed"
    fi

    # Deactivate virtual environment
    deactivate

    print_success "Python environment setup completed"
}

copy_backend_files() {
    local source_dir="$1"
    print_step "Copying backend files..."

    # Copy backend source files
    cp -r "$source_dir/src/backend"/* "$BACKEND_DIR/"

    # Copy frontend files if they exist (for serving static files)
    if [[ -d "$source_dir/src/frontend" ]]; then
        mkdir -p "$BACKEND_DIR/static"
        cp -r "$source_dir/src/frontend"/* "$BACKEND_DIR/static/"
    fi

    # Set proper ownership
    chown -R "$USER_NAME:$USER_NAME" "$BACKEND_DIR"

    print_success "Backend files copied"
}

setup_configuration() {
    local source_dir="$1"
    print_step "Setting up configuration files..."

    # Create main configuration
    cat > "$CONFIG_DIR/backend_config.json" << EOF
{
  "server": {
    "host": "0.0.0.0",
    "port": 5000,
    "debug": false,
    "ssl_enabled": false,
    "ssl_cert": null,
    "ssl_key": null
  },
  "logging": {
    "level": "INFO",
    "file": "$LOG_DIR/backend.log",
    "max_size": 10485760,
    "backup_count": 5
  },
  "database": {
    "type": "memory",
    "file": "$DATA_DIR/workforce_data.db"
  },
  "ml_models": {
    "isolation_forest_contamination": 0.1,
    "autoencoder_epochs": 50,
    "model_update_interval": 300
  },
  "security": {
    "secret_key": "$(openssl rand -hex 32)",
    "cors_origins": ["*"],
    "session_timeout": 3600
  },
  "monitoring": {
    "max_activity_records": 10000,
    "max_dlp_events": 1000,
    "max_time_entries": 5000,
    "max_behavior_patterns": 2000,
    "cleanup_interval_days": 30
  }
}
EOF

    # Create environment file
    cat > "$CONFIG_DIR/.env" << EOF
FLASK_ENV=production
FLASK_APP=app.py
SECRET_KEY=$(openssl rand -hex 32)
WORKFORCE_CONFIG=$CONFIG_DIR/backend_config.json
WORKFORCE_DATA_DIR=$DATA_DIR
WORKFORCE_LOG_DIR=$LOG_DIR
EOF

    # Set permissions
    chown root:root "$CONFIG_DIR"/*.json
    chown root:root "$CONFIG_DIR"/.env
    chmod 644 "$CONFIG_DIR"/*.json
    chmod 600 "$CONFIG_DIR"/.env

    print_success "Configuration files created"
}

create_startup_script() {
    print_step "Creating startup script..."

    cat > "$BACKEND_DIR/start_backend.sh" << EOF
#!/bin/bash
# Startup script for Workforce Monitoring Backend

# Set environment variables
export FLASK_ENV=production
export FLASK_APP=app.py
export SECRET_KEY=\$(grep SECRET_KEY $CONFIG_DIR/.env | cut -d'=' -f2)
export WORKFORCE_CONFIG=$CONFIG_DIR/backend_config.json
export WORKFORCE_DATA_DIR=$DATA_DIR
export WORKFORCE_LOG_DIR=$LOG_DIR

# Change to backend directory
cd $BACKEND_DIR

# Activate virtual environment
source venv/bin/activate

# Start the backend server
exec python app.py
EOF

    chmod +x "$BACKEND_DIR/start_backend.sh"

    print_success "Startup script created"
}

create_service_file() {
    print_step "Creating systemd service..."

    cat > "/etc/systemd/system/$SERVICE_NAME.service" << EOF
[Unit]
Description=Workforce Monitoring Backend Server
After=network.target
Wants=network.target

[Service]
Type=simple
User=$USER_NAME
Group=$USER_NAME
WorkingDirectory=$BACKEND_DIR
ExecStart=$BACKEND_DIR/start_backend.sh
Restart=always
RestartSec=5
Environment=PATH=$BACKEND_DIR/venv/bin

# Security settings
NoNewPrivileges=yes
PrivateTmp=yes
ProtectSystem=strict
ProtectHome=yes
ReadWritePaths=$LOG_DIR $DATA_DIR
ProtectKernelTunables=yes
ProtectControlGroups=yes

# Resource limits
MemoryLimit=1G
CPUQuota=50%

# Logging
StandardOutput=journal
StandardError=journal
SyslogIdentifier=$SERVICE_NAME

[Install]
WantedBy=multi-user.target
EOF

    # Reload systemd
    systemctl daemon-reload

    print_success "Systemd service created"
}

setup_logrotate() {
    print_step "Setting up log rotation..."

    cat > "/etc/logrotate.d/$SERVICE_NAME" << EOF
$LOG_DIR/*.log {
    daily
    missingok
    rotate 30
    compress
    delaycompress
    notifempty
    create 644 $USER_NAME $USER_NAME
    postrotate
        systemctl reload $SERVICE_NAME
    endscript
}
EOF

    print_success "Log rotation configured"
}

create_uninstaller() {
    print_step "Creating uninstaller..."

    cat > "$BACKEND_DIR/uninstall_backend.sh" << 'EOF'
#!/bin/bash
set -e

SERVICE_NAME="workforce-backend"
BACKEND_DIR="/opt/workforce-backend"
CONFIG_DIR="/etc/workforce-backend"
LOG_DIR="/var/log/workforce-backend"
DATA_DIR="/var/lib/workforce-backend"
USER_NAME="workforce-backend"

echo "Stopping and disabling service..."
systemctl stop "$SERVICE_NAME" 2>/dev/null || true
systemctl disable "$SERVICE_NAME" 2>/dev/null || true

echo "Removing systemd service..."
rm -f "/etc/systemd/system/$SERVICE_NAME.service"
systemctl daemon-reload

echo "Removing files and directories..."
rm -rf "$BACKEND_DIR"
rm -rf "$CONFIG_DIR"
rm -rf "$LOG_DIR"
rm -rf "$DATA_DIR"

echo "Removing logrotate configuration..."
rm -f "/etc/logrotate.d/$SERVICE_NAME"

echo "Removing user..."
userdel "$USER_NAME" 2>/dev/null || true

echo "Backend uninstallation completed!"
EOF

    chmod +x "$BACKEND_DIR/uninstall_backend.sh"

    print_success "Uninstaller created at $BACKEND_DIR/uninstall_backend.sh"
}

cleanup() {
    print_step "Cleaning up temporary files..."
    rm -rf "$TEMP_DIR"
    print_success "Cleanup completed"
}

show_completion_message() {
    echo
    echo -e "${GREEN}================================================${NC}"
    echo -e "${GREEN}  Backend Installation Completed Successfully!${NC}"
    echo -e "${GREEN}================================================${NC}"
    echo
    echo -e "Installation Details:"
    echo -e "  • Backend installed in: ${BLUE}$BACKEND_DIR${NC}"
    echo -e "  • Configuration: ${BLUE}$CONFIG_DIR${NC}"
    echo -e "  • Logs: ${BLUE}$LOG_DIR${NC}"
    echo -e "  • Data: ${BLUE}$DATA_DIR${NC}"
    echo
    echo -e "Service Management:"
    echo -e "  • Start: ${BLUE}sudo systemctl start $SERVICE_NAME${NC}"
    echo -e "  • Stop: ${BLUE}sudo systemctl stop $SERVICE_NAME${NC}"
    echo -e "  • Status: ${BLUE}sudo systemctl status $SERVICE_NAME${NC}"
    echo -e "  • Logs: ${BLUE}sudo journalctl -u $SERVICE_NAME${NC}"
    echo
    echo -e "Web Interface:"
    echo -e "  • Dashboard: ${BLUE}http://localhost:5000${NC}"
    echo -e "  • API: ${BLUE}http://localhost:5000/api/${NC}"
    echo
    echo -e "Uninstallation:"
    echo -e "  • Run: ${BLUE}sudo $BACKEND_DIR/uninstall_backend.sh${NC}"
    echo
    echo -e "${YELLOW}Next Steps:${NC}"
    echo -e "  1. Review configuration in $CONFIG_DIR/backend_config.json"
    echo -e "  2. Start the service: sudo systemctl start $SERVICE_NAME"
    echo -e "  3. Access the dashboard in your web browser"
    echo
}

main() {
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --help)
                echo "Usage: $0 [--help]"
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

    # Get the source directory
    SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
    REQUIREMENTS_FILE="$SOURCE_DIR/requirements.txt"

    echo "Installation Configuration:"
    echo "  • Operating System: $OS"
    echo "  • Backend Directory: $BACKEND_DIR"
    echo "  • Config Directory: $CONFIG_DIR"
    echo "  • Source Directory: $SOURCE_DIR"
    echo "  • Requirements File: $REQUIREMENTS_FILE"
    echo "  • Service Name: $SERVICE_NAME"
    echo

    # Verify source files exist
    if [[ ! -f "$REQUIREMENTS_FILE" ]]; then
        print_error "Requirements file not found: $REQUIREMENTS_FILE"
        exit 1
    fi

    if [[ ! -f "$SOURCE_DIR/src/backend/app.py" ]]; then
        print_error "Backend app.py not found: $SOURCE_DIR/src/backend/app.py"
        exit 1
    fi

    read -p "Continue with backend installation? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Installation cancelled."
        exit 0
    fi

    # Run installation steps
    install_system_dependencies
    create_directories
    create_user
    setup_python_environment "$SOURCE_DIR" "$REQUIREMENTS_FILE"
    copy_backend_files "$SOURCE_DIR"
    setup_configuration "$SOURCE_DIR"
    create_startup_script
    create_service_file
    setup_logrotate
    create_uninstaller
    cleanup

    # Ask about starting service
    echo
    read -p "Start the backend service now? (Y/n): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Nn]$ ]]; then
        print_step "Starting backend service..."
        systemctl enable "$SERVICE_NAME"
        systemctl start "$SERVICE_NAME"

        # Wait a moment and check status
        sleep 3
        if systemctl is-active --quiet "$SERVICE_NAME"; then
            print_success "Backend service started successfully"
        else
            print_warning "Service failed to start. Check logs with: sudo journalctl -u $SERVICE_NAME"
        fi
    else
        echo "You can start the service later with: sudo systemctl start $SERVICE_NAME"
    fi

    show_completion_message
}

# Run main function
main "$@"
