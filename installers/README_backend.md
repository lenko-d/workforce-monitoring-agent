# Workforce Monitoring Backend Installer

This directory contains the installer for the Workforce Monitoring Backend Server component.

## Overview

The backend installer (`install_backend.sh`) provides a complete installation solution for the Python Flask-based backend server that powers the workforce monitoring system. It handles:

- System dependency installation
- Python virtual environment setup
- Package dependency installation
- Service configuration and management
- Security hardening
- Log rotation setup

## Features

### Complete Installation
- **System Dependencies**: Automatically installs Python 3, pip, and required system libraries
- **Virtual Environment**: Creates an isolated Python environment to avoid conflicts
- **Dependencies**: Installs all required Python packages from `requirements.txt`
- **File Management**: Copies backend and frontend files to appropriate locations

### Service Management
- **Systemd Service**: Creates and configures a systemd service for automatic startup
- **User Isolation**: Runs under a dedicated service user for security
- **Resource Limits**: Configures memory and CPU limits for stability
- **Auto-restart**: Automatically restarts the service on failure

### Security Features
- **Non-privileged User**: Service runs under a dedicated user account
- **Filesystem Protection**: Restricts file system access using systemd security features
- **Environment Isolation**: Uses private temporary directories and restricted kernel access

### Configuration Management
- **JSON Configuration**: Comprehensive configuration file for all backend settings
- **Environment Variables**: Secure environment file for sensitive settings
- **Log Management**: Automatic log rotation and cleanup

## Installation

### Prerequisites

- Linux operating system (Ubuntu/Debian, CentOS/RHEL, or Fedora)
- Root or sudo access
- Internet connection for downloading dependencies

### Quick Installation

```bash
# Navigate to the installers directory
cd installers

# Run the installer with sudo
sudo ./install_backend.sh
```

### Installation Process

The installer will:

1. **Check Requirements**: Verify root access and detect operating system
2. **Install Dependencies**: Install Python 3, pip, and system libraries
3. **Create Directories**: Set up installation directories with proper permissions
4. **Setup Python Environment**: Create virtual environment and install packages
5. **Copy Files**: Install backend application files
6. **Configure Service**: Create systemd service and configuration files
7. **Setup Logging**: Configure log rotation
8. **Create Uninstaller**: Generate uninstallation script

## Configuration

After installation, configuration files are located in `/etc/workforce-backend/`:

- `backend_config.json`: Main configuration file
- `.env`: Environment variables (secure file)

### Key Configuration Options

```json
{
  "server": {
    "host": "0.0.0.0",
    "port": 5000,
    "debug": false,
    "ssl_enabled": false
  },
  "logging": {
    "level": "INFO",
    "file": "/var/log/workforce-backend/backend.log"
  },
  "ml_models": {
    "isolation_forest_contamination": 0.1,
    "autoencoder_epochs": 50
  }
}
```

## Service Management

### Start the Service
```bash
sudo systemctl start workforce-backend
```

### Stop the Service
```bash
sudo systemctl stop workforce-backend
```

### Check Status
```bash
sudo systemctl status workforce-backend
```

### View Logs
```bash
# System logs
sudo journalctl -u workforce-backend

# Application logs
sudo tail -f /var/log/workforce-backend/backend.log
```

### Enable Auto-start
```bash
sudo systemctl enable workforce-backend
```

## Web Interface

Once the service is running, access the web interface at:

- **Dashboard**: http://localhost:5000
- **API Documentation**: http://localhost:5000/api/

## API Endpoints

The backend provides RESTful API endpoints for:

- `/api/dashboard` - Dashboard overview data
- `/api/activities` - User activity data
- `/api/dlp-events` - Data Loss Prevention events
- `/api/productivity` - Productivity analytics
- `/api/risk-analysis` - Risk assessment data

## WebSocket Support

The backend includes SocketIO support for real-time communication:

- Real-time data updates
- Live alerts and notifications
- Agent data streaming

## Troubleshooting

### Service Won't Start

Check the service status and logs:
```bash
sudo systemctl status workforce-backend
sudo journalctl -u workforce-backend -n 50
```

Common issues:
- Port 5000 already in use
- Missing dependencies
- Permission issues

### Python Package Installation Issues

If some Python packages fail to install:
1. Check internet connection
2. Verify package names in `requirements.txt`
3. Try installing problematic packages manually:
   ```bash
   sudo /opt/workforce-backend/venv/bin/pip install <package_name>
   ```

### Permission Issues

Ensure the service user has access to required directories:
```bash
sudo chown -R workforce-backend:workforce-backend /opt/workforce-backend
sudo chown -R workforce-backend:workforce-backend /var/log/workforce-backend
sudo chown -R workforce-backend:workforce-backend /var/lib/workforce-backend
```

## Uninstallation

To completely remove the backend:

```bash
# Stop the service
sudo systemctl stop workforce-backend

# Run the uninstaller
sudo /opt/workforce-backend/uninstall_backend.sh
```

The uninstaller will:
- Stop and disable the service
- Remove all installed files and directories
- Delete the service user
- Clean up configuration files

## File Locations

| Component | Location |
|-----------|----------|
| Backend Application | `/opt/workforce-backend/` |
| Configuration | `/etc/workforce-backend/` |
| Logs | `/var/log/workforce-backend/` |
| Data | `/var/lib/workforce-backend/` |
| Virtual Environment | `/opt/workforce-backend/venv/` |
| Service File | `/etc/systemd/system/workforce-backend.service` |
| Log Rotation | `/etc/logrotate.d/workforce-backend` |

## Security Considerations

- The service runs under a dedicated user account
- File system access is restricted using systemd security features
- Sensitive configuration is stored in protected files
- Network access is limited to the configured port
- Memory and CPU usage is limited to prevent resource exhaustion

## Performance Tuning

### Memory Configuration
Adjust memory limits in the systemd service file:
```ini
MemoryLimit=1G
```

### CPU Limits
Modify CPU quota in the service file:
```ini
CPUQuota=50%
```

### Python Performance
For high-load environments, consider:
- Increasing worker threads in the configuration
- Using a production WSGI server (gunicorn)
- Implementing connection pooling for databases

## Support

For issues or questions:
1. Check the logs for error messages
2. Verify configuration files
3. Ensure all dependencies are installed
4. Review system resource usage

## Development

To modify the backend after installation:
1. Edit files in `/opt/workforce-backend/`
2. Restart the service: `sudo systemctl restart workforce-backend`
3. Check logs for any startup errors

## Backup and Recovery

### Configuration Backup
```bash
sudo cp -r /etc/workforce-backend /path/to/backup/
```

### Data Backup
```bash
sudo cp -r /var/lib/workforce-backend /path/to/backup/
```

### Log Backup
```bash
sudo cp -r /var/log/workforce-backend /path/to/backup/
