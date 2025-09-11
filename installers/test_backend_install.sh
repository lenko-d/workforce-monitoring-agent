#!/bin/bash

# Test script for Workforce Monitoring Backend Installation
# This script verifies that the backend installation is working correctly

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
BACKEND_DIR="/opt/workforce-backend"
CONFIG_DIR="/etc/workforce-backend"
LOG_DIR="/var/log/workforce-backend"
DATA_DIR="/var/lib/workforce-backend"
SERVICE_NAME="workforce-backend"
USER_NAME="workforce-backend"

print_header() {
    echo -e "${BLUE}================================================${NC}"
    echo -e "${BLUE}  Backend Installation Test${NC}"
    echo -e "${BLUE}================================================${NC}"
    echo
}

print_test() {
    echo -e "${BLUE}[TEST]${NC} $1"
}

print_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
}

print_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

test_directories() {
    print_test "Checking installation directories..."

    local dirs=("$BACKEND_DIR" "$CONFIG_DIR" "$LOG_DIR" "$DATA_DIR")
    local all_exist=true

    for dir in "${dirs[@]}"; do
        if [[ -d "$dir" ]]; then
            print_pass "Directory exists: $dir"
        else
            print_fail "Directory missing: $dir"
            all_exist=false
        fi
    done

    if $all_exist; then
        print_pass "All installation directories exist"
    else
        print_fail "Some installation directories are missing"
        return 1
    fi
}

test_user() {
    print_test "Checking service user..."

    if id "$USER_NAME" &>/dev/null; then
        print_pass "Service user exists: $USER_NAME"
    else
        print_fail "Service user missing: $USER_NAME"
        return 1
    fi
}

test_files() {
    print_test "Checking installation files..."

    local files=(
        "$BACKEND_DIR/app.py"
        "$BACKEND_DIR/venv/bin/python"
        "$BACKEND_DIR/venv/bin/pip"
        "$BACKEND_DIR/start_backend.sh"
        "$CONFIG_DIR/backend_config.json"
        "$CONFIG_DIR/.env"
        "/etc/systemd/system/$SERVICE_NAME.service"
        "/etc/logrotate.d/$SERVICE_NAME"
    )

    local all_exist=true

    for file in "${files[@]}"; do
        if [[ -f "$file" ]]; then
            print_pass "File exists: $file"
        else
            print_fail "File missing: $file"
            all_exist=false
        fi
    done

    if $all_exist; then
        print_pass "All installation files exist"
    else
        print_fail "Some installation files are missing"
        return 1
    fi
}

test_permissions() {
    print_test "Checking file permissions..."

    local issues=0

    # Check if backend directory is owned by service user
    if [[ $(stat -c '%U' "$BACKEND_DIR") == "$USER_NAME" ]]; then
        print_pass "Backend directory owned by service user"
    else
        print_fail "Backend directory not owned by service user"
        issues=$((issues + 1))
    fi

    # Check if config files have correct permissions
    if [[ $(stat -c '%a' "$CONFIG_DIR/backend_config.json") == "644" ]]; then
        print_pass "Config file has correct permissions"
    else
        print_fail "Config file has incorrect permissions"
        issues=$((issues + 1))
    fi

    # Check if env file is secure
    if [[ $(stat -c '%a' "$CONFIG_DIR/.env") == "600" ]]; then
        print_pass "Environment file has secure permissions"
    else
        print_fail "Environment file has insecure permissions"
        issues=$((issues + 1))
    fi

    # Check if startup script is executable
    if [[ -x "$BACKEND_DIR/start_backend.sh" ]]; then
        print_pass "Startup script is executable"
    else
        print_fail "Startup script is not executable"
        issues=$((issues + 1))
    fi

    if [[ $issues -eq 0 ]]; then
        print_pass "All permissions are correct"
    else
        print_fail "$issues permission issues found"
        return 1
    fi
}

test_python_environment() {
    print_test "Testing Python virtual environment..."

    local python_bin="$BACKEND_DIR/venv/bin/python"
    local pip_bin="$BACKEND_DIR/venv/bin/pip"

    if [[ ! -x "$python_bin" ]]; then
        print_fail "Python binary not found or not executable"
        return 1
    fi

    # Test Python version
    local python_version
    python_version=$("$python_bin" --version 2>&1)
    if [[ $? -eq 0 ]]; then
        print_pass "Python executable works: $python_version"
    else
        print_fail "Python executable failed"
        return 1
    fi

    # Test pip
    if "$pip_bin" --version &>/dev/null; then
        print_pass "Pip is working"
    else
        print_fail "Pip is not working"
        return 1
    fi

    # Test key packages
    local packages=("flask" "pandas" "numpy" "scikit-learn")
    for package in "${packages[@]}"; do
        if "$python_bin" -c "import $package" 2>/dev/null; then
            print_pass "Package $package is installed"
        else
            print_fail "Package $package is not installed"
            return 1
        fi
    done

    print_pass "Python environment is working correctly"
}

test_configuration() {
    print_test "Testing configuration files..."

    local config_file="$CONFIG_DIR/backend_config.json"
    local env_file="$CONFIG_DIR/.env"

    # Test JSON configuration
    if "$BACKEND_DIR/venv/bin/python" -c "import json; json.load(open('$config_file'))" 2>/dev/null; then
        print_pass "Configuration JSON is valid"
    else
        print_fail "Configuration JSON is invalid"
        return 1
    fi

    # Test environment file
    if [[ -f "$env_file" ]]; then
        print_pass "Environment file exists"
    else
        print_fail "Environment file missing"
        return 1
    fi

    # Check for required environment variables
    local required_vars=("FLASK_ENV" "SECRET_KEY" "WORKFORCE_CONFIG")
    for var in "${required_vars[@]}"; do
        if grep -q "^$var=" "$env_file"; then
            print_pass "Environment variable $var is set"
        else
            print_fail "Environment variable $var is missing"
            return 1
        fi
    done

    print_pass "Configuration files are valid"
}

test_service() {
    print_test "Testing systemd service..."

    # Check if service file exists
    if [[ -f "/etc/systemd/system/$SERVICE_NAME.service" ]]; then
        print_pass "Service file exists"
    else
        print_fail "Service file missing"
        return 1
    fi

    # Check systemd status
    if systemctl list-units --all | grep -q "$SERVICE_NAME.service"; then
        print_pass "Service is registered with systemd"
    else
        print_fail "Service is not registered with systemd"
        return 1
    fi

    # Check if service can be started (don't actually start it)
    if sudo systemctl status "$SERVICE_NAME" &>/dev/null; then
        print_pass "Service status can be checked"
    else
        print_warning "Service status check failed (might be normal if not started)"
    fi

    print_pass "Service configuration is correct"
}

test_network() {
    print_test "Testing network configuration..."

    # Check if port 5000 is available (should be if service not running)
    if ! netstat -tln 2>/dev/null | grep -q ":5000 "; then
        print_pass "Port 5000 is available"
    else
        print_warning "Port 5000 is already in use (service might be running)"
    fi

    # Test localhost connectivity (if service is running)
    if curl -s --max-time 5 http://localhost:5000 &>/dev/null; then
        print_pass "Backend service is responding on localhost:5000"
    else
        print_warning "Backend service is not responding (might not be started)"
    fi
}

test_logrotate() {
    print_test "Testing log rotation configuration..."

    local logrotate_file="/etc/logrotate.d/$SERVICE_NAME"

    if [[ -f "$logrotate_file" ]]; then
        print_pass "Logrotate configuration exists"

        # Test logrotate configuration syntax
        if logrotate -d "$logrotate_file" 2>/dev/null; then
            print_pass "Logrotate configuration is valid"
        else
            print_fail "Logrotate configuration is invalid"
            return 1
        fi
    else
        print_fail "Logrotate configuration missing"
        return 1
    fi
}

run_all_tests() {
    local tests_passed=0
    local total_tests=8

    print_header

    echo "Running installation tests..."
    echo

    if test_directories; then
        tests_passed=$((tests_passed + 1))
    fi

    if test_user; then
        tests_passed=$((tests_passed + 1))
    fi

    if test_files; then
        tests_passed=$((tests_passed + 1))
    fi

    if test_permissions; then
        tests_passed=$((tests_passed + 1))
    fi

    if test_python_environment; then
        tests_passed=$((tests_passed + 1))
    fi

    if test_configuration; then
        tests_passed=$((tests_passed + 1))
    fi

    if test_service; then
        tests_passed=$((tests_passed + 1))
    fi

    if test_logrotate; then
        tests_passed=$((tests_passed + 1))
    fi

    echo
    echo -e "${BLUE}================================================${NC}"
    echo -e "Test Results: $tests_passed/$total_tests tests passed"
    echo -e "${BLUE}================================================${NC}"

    if [[ $tests_passed -eq $total_tests ]]; then
        echo -e "${GREEN}✓ All tests passed! Backend installation is working correctly.${NC}"
        return 0
    else
        echo -e "${RED}✗ $((total_tests - tests_passed)) tests failed. Please check the installation.${NC}"
        return 1
    fi
}

show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo
    echo "Test the Workforce Monitoring Backend installation"
    echo
    echo "Options:"
    echo "  --help     Show this help message"
    echo "  --verbose  Show detailed test output"
    echo
    echo "Examples:"
    echo "  $0                    # Run all tests"
    echo "  sudo $0              # Run with sudo if needed"
}

main() {
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --help)
                show_usage
                exit 0
                ;;
            --verbose)
                set -x
                shift
                ;;
            *)
                echo "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done

    run_all_tests
}

# Run main function
main "$@"
