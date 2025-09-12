# Troubleshooting Guide

This guide provides solutions to common issues and problems that may occur with the Workforce Monitoring and Insider Risk Management Agent.

## Quick Diagnosis

### System Health Check

```bash
# Check overall system status
sudo workforce-agent --diagnostics

# Check service status
sudo systemctl status workforce-agent

# Check backend service
sudo systemctl status workforce-backend

# View recent logs
sudo journalctl -u workforce-agent -n 20
```

### Log Analysis

```bash
# View agent logs
sudo journalctl -u workforce-agent -f

# View backend logs
sudo journalctl -u workforce-backend -f

# Search for errors in logs
sudo journalctl -u workforce-agent | grep -i error

# Check log file directly
sudo tail -f /var/log/workforce-agent/agent.log
```

## Installation Issues

### Build Failures

#### CMake Configuration Errors

**Problem**: CMake fails to configure the build.

**Symptoms**:
```
CMake Error: Could not find CMAKE_ROOT
CMake Error: Could not find CMAKE_MODULE_PATH
```

**Solutions**:
```bash
# Clean build directory
cd build
rm -rf *

# Reconfigure with verbose output
cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON

# Check CMake version
cmake --version

# Install CMake if missing
sudo apt-get install cmake
```

#### Missing Dependencies

**Problem**: Build fails due to missing libraries.

**Symptoms**:
```
fatal error: wayland-client.h: No such file or directory
```

**Solutions**:
```bash
# Install missing Wayland libraries
sudo apt-get install libwayland-dev libwayland-client0 wayland-protocols

# Install all required dependencies
sudo apt-get install \
    build-essential \
    libssl-dev \
    libcurl4-openssl-dev \
    nlohmann-json3-dev \
    libevdev-dev

# For BCC/eBPF support
sudo apt-get install bcc-tools libbcc-dev
```

#### Compiler Errors

**Problem**: C++ compilation fails.

**Symptoms**:
```
error: 'std::filesystem' has not been declared
```

**Solutions**:
```bash
# Check compiler version
g++ --version

# Use C++17 standard explicitly
cd build
cmake .. -DCMAKE_CXX_STANDARD=17

# Update compiler if too old
sudo apt-get install g++-9
export CC=gcc-9 CXX=g++-9
```

### Runtime Installation Issues

#### Permission Denied

**Problem**: Agent fails to start due to permission issues.

**Symptoms**:
```
Permission denied: /dev/input/event0
```

**Solutions**:
```bash
# Add user to input group
sudo usermod -a -G input $USER

# For eBPF capabilities
sudo usermod -a -G bcc $USER

# Restart session or reboot
# Check device permissions
ls -la /dev/input/

# Fix device permissions if needed
sudo chmod 666 /dev/input/event*
```

#### Service Startup Failures

**Problem**: Systemd service fails to start.

**Symptoms**:
```
workforce-agent.service: Failed to start
```

**Solutions**:
```bash
# Check service status
sudo systemctl status workforce-agent

# View detailed error logs
sudo journalctl -u workforce-agent -n 50

# Try manual startup
sudo -u workforce-agent /opt/workforce-agent/bin/workforce_agent

# Check configuration files
sudo workforce-agent --validate-config

# Fix service file if corrupted
sudo systemctl daemon-reload
```

## Monitoring Issues

### No Activity Data Collected

**Problem**: Agent runs but no activity is recorded.

**Symptoms**:
- Dashboard shows no activity
- API returns empty results
- Logs show no activity events

**Diagnostic Steps**:
```bash
# Check agent process
ps aux | grep workforce_agent

# Verify monitoring is enabled
curl http://localhost:5000/api/config | jq .monitoring.enabled

# Check input device access
ls -la /dev/input/

# Test activity API
curl http://localhost:5000/api/activities?limit=5
```

**Solutions**:

1. **Enable Monitoring**:
```json
{
  "monitoring": {
    "enabled": true,
    "sample_rate": 1000
  }
}
```

2. **Fix Input Permissions**:
```bash
sudo usermod -a -G input workforce-agent
sudo systemctl restart workforce-agent
```

3. **Check Wayland/X11 Session**:
```bash
# Verify display session
echo $DISPLAY
echo $WAYLAND_DISPLAY

# For Wayland sessions
export WAYLAND_DISPLAY=wayland-0
```

### High CPU/Memory Usage

**Problem**: Agent consumes excessive system resources.

**Symptoms**:
- High CPU usage (>50%)
- Memory usage keeps growing
- System becomes slow

**Diagnostic Steps**:
```bash
# Monitor resource usage
top -p $(pgrep workforce_agent)

# Check memory usage
ps aux --sort=-%mem | head

# View agent configuration
curl http://localhost:5000/api/config | jq .monitoring
```

**Solutions**:

1. **Adjust Sample Rate**:
```json
{
  "monitoring": {
    "sample_rate": 2000,
    "idle_threshold": 600
  }
}
```

2. **Enable Compression**:
```json
{
  "monitoring": {
    "compression_enabled": true,
    "batch_size": 500
  }
}
```

3. **Limit Buffer Size**:
```json
{
  "monitoring": {
    "max_buffer_size": 5000,
    "async_processing": true
  }
}
```

### Data Loss Prevention Not Working

**Problem**: DLP policies don't trigger or block transfers.

**Symptoms**:
- No DLP alerts generated
- Unauthorized transfers succeed
- DLP events API returns empty

**Diagnostic Steps**:
```bash
# Check DLP status
curl http://localhost:5000/api/dlp-events?limit=5

# Verify DLP configuration
curl http://localhost:5000/api/config | jq .dlp

# Test file system monitoring
sudo inotifywait -m /home/user/Documents -t 10
```

**Solutions**:

1. **Enable DLP Features**:
```json
{
  "dlp": {
    "enabled": true,
    "real_time_scanning": true,
    "file_system_monitoring": true
  }
}
```

2. **Check File Permissions**:
```bash
# Ensure agent can read monitored directories
sudo -u workforce-agent ls /home/user/Documents

# Add agent to necessary groups
sudo usermod -a -G users workforce-agent
```

3. **Verify Policy Configuration**:
```json
{
  "policies": [
    {
      "name": "test_policy",
      "enabled": true,
      "rules": {
        "file_extensions": [".txt", ".docx"]
      },
      "actions": {
        "alert": true,
        "log": true
      }
    }
  ]
}
```

## Backend Issues

### Flask Application Not Starting

**Problem**: Python backend fails to start.

**Symptoms**:
```
ImportError: No module named 'flask'
ModuleNotFoundError: No module named 'sklearn'
```

**Solutions**:
```bash
# Install missing dependencies
pip3 install -r requirements.txt

# Check Python path
python3 -c "import sys; print(sys.path)"

# Use virtual environment
source venv/bin/activate
pip install -r requirements.txt

# Check Python version compatibility
python3 --version
```

### Database Connection Issues

**Problem**: Backend cannot connect to database.

**Symptoms**:
```
sqlalchemy.exc.OperationalError: (sqlite3.OperationalError) unable to open database file
```

**Solutions**:
```bash
# Check database file permissions
ls -la /var/lib/workforce-agent/

# Create database directory if missing
sudo mkdir -p /var/lib/workforce-agent
sudo chown workforce-agent:workforce-agent /var/lib/workforce-agent

# Fix database path in configuration
{
  "database": {
    "path": "/var/lib/workforce-agent/data.db"
  }
}
```

### Socket.IO Connection Problems

**Problem**: Real-time updates not working.

**Symptoms**:
- Dashboard doesn't update live
- WebSocket connection fails
- Browser console shows connection errors

**Diagnostic Steps**:
```bash
# Check Socket.IO server status
curl http://localhost:5000/socket.io/?EIO=4&transport=polling

# Verify CORS settings
curl -H "Origin: http://localhost:3000" http://localhost:5000/api/config
```

**Solutions**:

1. **Fix CORS Configuration**:
```json
{
  "socketio": {
    "cors_allowed_origins": ["http://localhost:3000", "http://localhost:5000"]
  }
}
```

2. **Check Network Configuration**:
```bash
# Verify port availability
netstat -tlnp | grep 5000

# Check firewall settings
sudo ufw status
```

## Web Dashboard Issues

### Dashboard Not Loading

**Problem**: Web interface doesn't load or shows errors.

**Symptoms**:
- Blank page
- JavaScript errors in browser console
- 404 errors for static files

**Diagnostic Steps**:
```bash
# Check backend server status
curl http://localhost:5000/

# Verify static file serving
curl http://localhost:5000/static/js/app.js

# Check browser console for errors
# Open Developer Tools (F12) in browser
```

**Solutions**:

1. **Fix Static File Paths**:
```python
# In app.py
app.static_folder = 'src/frontend'
app.static_url_path = '/static'
```

2. **Enable CORS for Frontend**:
```python
from flask_cors import CORS
CORS(app, origins=["http://localhost:3000"])
```

3. **Check File Permissions**:
```bash
# Ensure web server can read frontend files
sudo chown -R workforce-agent:workforce-agent src/frontend/
```

### Real-time Updates Not Working

**Problem**: Dashboard doesn't update in real-time.

**Symptoms**:
- Data doesn't refresh automatically
- No live activity feed
- WebSocket connection errors

**Solutions**:

1. **Verify Socket.IO Client**:
```javascript
// In frontend JavaScript
const socket = io('http://localhost:5000', {
  transports: ['websocket', 'polling']
});

socket.on('connect', () => {
  console.log('Connected to server');
});

socket.on('agent_data', (data) => {
  updateDashboard(data);
});
```

2. **Check Network Connectivity**:
```bash
# Test WebSocket connection
curl -I -N -H "Connection: Upgrade" \
     -H "Upgrade: websocket" \
     http://localhost:5000/socket.io/
```

## Behavioral Analysis Issues

### LLM Analysis Not Working

**Problem**: AI-powered analysis fails or produces no results.

**Symptoms**:
- No LLM insights in reports
- API calls to LLM providers fail
- Error messages about missing API keys

**Diagnostic Steps**:
```bash
# Check LLM configuration
curl http://localhost:5000/api/config | jq .llm

# Test API key validity
curl -H "Authorization: Bearer YOUR_API_KEY" \
     https://api.openai.com/v1/models

# Check LLM service logs
sudo journalctl -u workforce-agent | grep -i llm
```

**Solutions**:

1. **Configure API Keys**:
```bash
export OPENAI_API_KEY="your-api-key-here"
# Or set in configuration
{
  "llm": {
    "provider": "openai",
    "api_key": "your-api-key-here"
  }
}
```

2. **Check Rate Limits**:
```json
{
  "llm": {
    "rate_limit": 60,
    "retry_attempts": 3,
    "timeout": 30
  }
}
```

3. **Fallback to Traditional Analysis**:
```json
{
  "behavior_analysis": {
    "llm_enabled": false,
    "anomaly_detection": true
  }
}
```

### False Positive Alerts

**Problem**: Too many false positive security alerts.

**Symptoms**:
- Excessive alert notifications
- Legitimate activities flagged as suspicious
- Alert fatigue

**Solutions**:

1. **Adjust Risk Thresholds**:
```json
{
  "behavior_analysis": {
    "risk_thresholds": {
      "low": 0.3,
      "medium": 0.6,
      "high": 0.8
    }
  }
}
```

2. **Add Exceptions**:
```json
{
  "dlp": {
    "exceptions": {
      "users": ["admin", "auditor"],
      "applications": ["backup_software"],
      "file_patterns": ["*.tmp", "*.log"]
    }
  }
}
```

3. **Tune Anomaly Detection**:
```json
{
  "behavior_analysis": {
    "model_update_interval": 3600,
    "sensitivity": "medium"
  }
}
```

## Performance Issues

### Slow Dashboard Loading

**Problem**: Web interface loads slowly.

**Symptoms**:
- Long page load times
- Slow chart rendering
- Delayed data updates

**Solutions**:

1. **Enable Caching**:
```json
{
  "performance": {
    "caching": {
      "enabled": true,
      "ttl": 300
    }
  }
}
```

2. **Optimize Database Queries**:
```python
# Add database indexes
from app import db
db.Index('idx_activity_user_time', Activity.user, Activity.timestamp)
```

3. **Implement Pagination**:
```javascript
// Frontend pagination
const pageSize = 50;
let currentPage = 1;

function loadActivities(page = 1) {
  fetch(`/api/activities?page=${page}&limit=${pageSize}`)
    .then(response => response.json())
    .then(data => renderActivities(data));
}
```

### High Database Usage

**Problem**: Database grows too large or queries are slow.

**Symptoms**:
- Large database file size
- Slow query performance
- High disk I/O

**Solutions**:

1. **Configure Data Retention**:
```json
{
  "database": {
    "retention_days": 90,
    "auto_cleanup": true
  }
}
```

2. **Add Database Indexes**:
```sql
CREATE INDEX idx_activity_timestamp ON activities(timestamp);
CREATE INDEX idx_activity_user ON activities(user);
CREATE INDEX idx_dlp_timestamp ON dlp_events(timestamp);
```

3. **Optimize Query Performance**:
```python
# Use select only needed columns
activities = Activity.query \
    .with_entities(Activity.user, Activity.type, Activity.timestamp) \
    .filter(Activity.timestamp >= start_date) \
    .all()
```

## Network Issues

### Connection Timeouts

**Problem**: Agent loses connection to backend or external services.

**Symptoms**:
- Intermittent connectivity
- Timeout errors in logs
- Data synchronization failures

**Solutions**:

1. **Adjust Network Timeouts**:
```json
{
  "network": {
    "connection_timeout": 60,
    "retry_attempts": 5,
    "retry_delay": 10
  }
}
```

2. **Configure Proxy Settings**:
```json
{
  "network": {
    "proxy": {
      "enabled": true,
      "http_proxy": "http://proxy.company.com:8080",
      "https_proxy": "http://proxy.company.com:8080"
    }
  }
}
```

### SSL/TLS Issues

**Problem**: HTTPS connections fail.

**Symptoms**:
- SSL certificate errors
- TLS handshake failures
- Connection refused errors

**Solutions**:

1. **Update SSL Certificates**:
```bash
# Update CA certificates
sudo apt-get install ca-certificates
sudo update-ca-certificates
```

2. **Configure SSL Settings**:
```json
{
  "network": {
    "ssl_verify": true,
    "ssl_cert_path": "/etc/ssl/certs/ca-certificates.crt"
  }
}
```

## Alert System Issues

### Alerts Not Sending

**Problem**: Email or webhook alerts are not delivered.

**Symptoms**:
- No alert notifications received
- Alert logs show send failures
- SMTP connection errors

**Solutions**:

1. **Verify Email Configuration**:
```json
{
  "alerts": {
    "channels": {
      "email": {
        "enabled": true,
        "smtp_server": "smtp.company.com",
        "smtp_port": 587,
        "use_tls": true,
        "username": "alerts@company.com",
        "password": "correct-password"
      }
    }
  }
}
```

2. **Test SMTP Connection**:
```bash
# Test SMTP server connection
telnet smtp.company.com 587

# Send test email
echo "Test" | mail -s "Test Alert" user@company.com
```

3. **Check Webhook Endpoints**:
```bash
# Test webhook URL
curl -X POST https://api.company.com/webhooks \
  -H "Content-Type: application/json" \
  -d '{"test": "alert"}'
```

## Upgrade Issues

### Failed Upgrades

**Problem**: Agent upgrade fails or causes issues.

**Symptoms**:
- Upgrade process hangs
- Service fails after upgrade
- Rollback doesn't work

**Solutions**:

1. **Manual Upgrade Process**:
```bash
# Stop services
sudo systemctl stop workforce-agent workforce-backend

# Backup current version
sudo cp -r /opt/workforce-agent /opt/workforce-agent.backup

# Download and install new version
wget https://github.com/your-org/workforce-monitoring/releases/download/v1.1.0/workforce-agent_1.1.0.deb
sudo dpkg -i workforce-agent_1.1.0.deb

# Restart services
sudo systemctl start workforce-agent workforce-backend
```

2. **Rollback Procedure**:
```bash
# Stop services
sudo systemctl stop workforce-agent workforce-backend

# Restore backup
sudo rm -rf /opt/workforce-agent
sudo mv /opt/workforce-agent.backup /opt/workforce-agent

# Restart services
sudo systemctl start workforce-agent workforce-backend
```

## Log Analysis

### Common Error Patterns

#### Memory Errors
```
terminate called after throwing an instance of 'std::bad_alloc'
```

**Solutions**:
- Increase system memory
- Reduce monitoring scope
- Enable memory limits

#### File System Errors
```
No space left on device
```

**Solutions**:
- Clean up log files
- Increase disk space
- Configure log rotation

#### Network Errors
```
Connection refused
```

**Solutions**:
- Check service status
- Verify network configuration
- Test connectivity

## Advanced Troubleshooting

### Debug Mode

Enable detailed logging for troubleshooting:

```json
{
  "agent": {
    "debug_mode": true,
    "log_level": "debug"
  },
  "logging": {
    "level": "debug",
    "format": "json"
  }
}
```

### Performance Profiling

```bash
# Profile CPU usage
perf record -g -p $(pgrep workforce_agent) -o perf.data
perf report -i perf.data

# Profile memory usage
valgrind --tool=massif --massif-out-file=massif.out ./workforce_agent
ms_print massif.out
```

### Core Dump Analysis

```bash
# Enable core dumps
echo "core.%e.%p.%t" | sudo tee /proc/sys/kernel/core_pattern
ulimit -c unlimited

# Analyze core dump
gdb ./workforce_agent core.workforce_agent.12345.1640995200
```

## Getting Help

### Support Resources

1. **Documentation**: Check all documentation files
2. **Logs**: Collect comprehensive logs for analysis
3. **Configuration**: Verify all configuration files
4. **System Info**: Gather system information

### Diagnostic Script

```bash
#!/bin/bash
# diagnostic.sh - Comprehensive system diagnostic

echo "=== Workforce Agent Diagnostic Report ==="
echo "Date: $(date)"
echo "System: $(uname -a)"
echo ""

echo "=== Service Status ==="
sudo systemctl status workforce-agent --no-pager
sudo systemctl status workforce-backend --no-pager
echo ""

echo "=== Process Information ==="
ps aux | grep workforce
echo ""

echo "=== Resource Usage ==="
top -b -n 1 | head -20
echo ""

echo "=== Network Connections ==="
netstat -tlnp | grep :5000
echo ""

echo "=== Log Analysis ==="
sudo journalctl -u workforce-agent --since "1 hour ago" | tail -20
echo ""

echo "=== Configuration Check ==="
curl -s http://localhost:5000/api/config | jq .agent 2>/dev/null || echo "Backend not responding"
echo ""

echo "=== Disk Usage ==="
df -h /var/lib/workforce-agent 2>/dev/null || echo "Data directory not found"
echo ""
```

### Support Bundle Creation

```bash
# Create support bundle
sudo workforce-agent --create-support-bundle

# Or manually
mkdir support-bundle
cp /etc/workforce-agent/*.json support-bundle/
sudo journalctl -u workforce-agent > support-bundle/agent.log
sudo journalctl -u workforce-backend > support-bundle/backend.log
cp /var/log/workforce-agent/*.log support-bundle/ 2>/dev/null
tar -czf support-bundle.tar.gz support-bundle/
```

## Prevention

### Best Practices

1. **Regular Monitoring**: Monitor system resources and logs
2. **Configuration Backup**: Backup configuration before changes
3. **Staged Updates**: Test updates in staging environment
4. **Documentation**: Keep detailed change logs
5. **Alert Tuning**: Regularly review and tune alert thresholds

### Proactive Maintenance

```bash
# Daily health check
sudo workforce-agent --health-check

# Weekly log rotation
sudo logrotate /etc/logrotate.d/workforce-agent

# Monthly performance review
sudo workforce-agent --performance-report

# Quarterly security audit
sudo workforce-agent --security-audit
