# Workforce Monitoring Agent Makefile

.PHONY: all build install clean test uninstall help

# Default target
all: build

# Build the agent
build:
	@echo "Building Workforce Monitoring Agent..."
	@mkdir -p build
	@cd build && cmake ..
	@cd build && make -j$(nproc)
	@echo "Build completed successfully!"

# Install the agent (requires root)
install:
	@echo "Installing Workforce Monitoring Agent..."
	@sudo ./install.sh

# Install with BCC support
install-with-bcc:
	@echo "Installing Workforce Monitoring Agent with BCC support..."
	@sudo ./install.sh --with-bcc

# Clean build files
clean:
	@echo "Cleaning build files..."
	@rm -rf build
	@rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake
	@echo "Clean completed!"

# Run tests (if any)
test:
	@echo "Running tests..."
	@echo "No tests implemented yet"

# Uninstall the agent
uninstall:
	@echo "Uninstalling Workforce Monitoring Agent..."
	@if [ -f "/opt/workforce-agent/uninstall.sh" ]; then \
		sudo /opt/workforce-agent/uninstall.sh; \
	elif [ -f "/usr/local/workforce-agent/uninstall.sh" ]; then \
		sudo /usr/local/workforce-agent/uninstall.sh; \
	else \
		echo "Uninstaller not found. Please check installation directory."; \
	fi

# Development setup
dev-setup:
	@echo "Setting up development environment..."
	@sudo apt-get update
	@sudo apt-get install -y build-essential cmake libx11-dev libxtst-dev libevdev-dev libssl-dev libcurl4-openssl-dev nlohmann-json3-dev python3 python3-pip
	@pip3 install -r requirements.txt
	@echo "Development environment ready!"

# Quick build and run (for development)
dev-run: build
	@echo "Starting agent in development mode..."
	@./build/wm-agent

# Show help
help:
	@echo "Workforce Monitoring Agent Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all              - Build the agent (default)"
	@echo "  build            - Build the C++ agent"
	@echo "  install          - Install the agent system-wide"
	@echo "  install-with-bcc - Install with BCC/eBPF support"
	@echo "  clean            - Clean build files"
	@echo "  test             - Run tests"
	@echo "  uninstall        - Uninstall the agent"
	@echo "  dev-setup        - Setup development environment"
	@echo "  dev-run          - Build and run in development mode"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  make build          # Build the agent"
	@echo "  make install        # Install system-wide"
	@echo "  make dev-setup      # Setup development environment"
	@echo "  make dev-run        # Build and run for development"

# Development target with BCC
dev-with-bcc: dev-setup
	@echo "Installing BCC for eBPF support..."
	@sudo apt-get install -y bcc-tools libbcc-dev
	@echo "BCC support added to development environment"
