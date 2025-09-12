# User Manual

This manual provides comprehensive guidance for using the Workforce Monitoring and Insider Risk Management Agent, including daily operations, configuration, and best practices.

## Getting Started

### First Login

After installation, access the web dashboard:

1. Open your browser and navigate to `http://localhost:5000`
2. Log in with default credentials:
   - **Username**: `admin`
   - **Password**: `admin`
3. **Important**: Change the default password immediately after first login

### Dashboard Overview

The main dashboard provides:

- **Real-time Activity Feed**: Live monitoring of user actions
- **Risk Assessment Panel**: Current threat levels and alerts
- **Productivity Metrics**: Application usage and time tracking
- **DLP Event Log**: Data loss prevention incidents
- **System Status**: Agent health and connectivity

## Core Features

### Activity Monitoring

#### Real-time Tracking
- **Keyboard/Mouse Events**: Monitor input activity patterns
- **Window Focus**: Track application usage and context switching
- **Idle Detection**: Identify periods of inactivity
- **Application Usage**: Detailed time spent in each application

#### Viewing Activity Data
```bash
# Get recent activities via API
curl http://localhost:5000/api/activities

# Filter by user and time range
curl "http://localhost:5000/api/activities?user=john.doe&hours=24"
```

#### Activity Reports
- **Daily Summary**: Aggregated activity by user
- **Application Usage**: Time spent in different applications
- **Productivity Analysis**: Active vs idle time ratios
- **Trend Analysis**: Activity patterns over time

### Data Loss Prevention (DLP)

#### Policy Configuration
DLP policies define what data to protect and how:

```json
{
  "name": "confidential_files",
  "file_extensions": [".docx", ".xlsx", ".pdf"],
  "content_patterns": ["confidential", "sensitive", "internal"],
  "block_transfer": true,
  "alert_threshold": "medium"
}
```

#### Monitoring Capabilities
- **File System Monitoring**: Real-time file access tracking
- **Network Transfer Detection**: Monitor data exfiltration attempts
- **Content Analysis**: Pattern matching in files and transfers
- **Automatic Blocking**: Prevent unauthorized data transfers

#### DLP Events
View and manage DLP incidents:
- **Event Details**: What was attempted, when, and by whom
- **Risk Assessment**: Severity and potential impact
- **Response Actions**: Allow, block, or quarantine
- **Audit Trail**: Complete history of DLP decisions

### Behavioral Analytics

#### AI-Powered Analysis
The system uses machine learning and LLM analysis:

- **Anomaly Detection**: Identify unusual behavior patterns
- **Risk Scoring**: Calculate threat levels for activities
- **Contextual Analysis**: Understand intent behind actions
- **Predictive Alerts**: Anticipate potential security issues

#### LLM Integration
Configure AI analysis for deeper insights:

```bash
# Enable LLM analysis
export LLM_PROVIDER=openai
export OPENAI_API_KEY=your-api-key

# Or use Anthropic Claude
export LLM_PROVIDER=anthropic
export ANTHROPIC_API_KEY=your-api-key
```

#### Analysis Reports
- **Behavioral Patterns**: Common activity sequences
- **Risk Trends**: Changes in user behavior over time
- **Recommendations**: AI-suggested security measures
- **Confidence Scores**: Reliability of analysis results

### Time Tracking and Productivity

#### Application Monitoring
Track time spent in applications:

- **Active Usage**: Time with application in focus
- **Background Activity**: Applications running but not active
- **Productivity Categories**: Classify applications by productivity level
- **Custom Categories**: Define your own productivity rules

#### Productivity Reports
- **Daily Productivity**: Hours of productive vs unproductive time
- **Application Rankings**: Most used applications
- **Time Distribution**: How time is spent across categories
- **Trend Analysis**: Productivity changes over time

## Configuration

### Basic Settings

#### Agent Configuration
Main configuration file: `/etc/workforce-agent/agent_config.json`

```json
{
  "monitoring": {
    "enabled": true,
    "sample_rate": 1000,
    "idle_threshold": 300
  },
  "logging": {
    "level": "info",
    "max_file_size": "100MB",
    "retention_days": 30
  },
  "alerts": {
    "email_enabled": true,
    "smtp_server": "smtp.company.com",
    "alert_recipients": ["security@company.com"]
  }
}
```

#### User Permissions
Configure which users to monitor:

```json
{
  "users": {
    "include": ["user1", "user2"],
    "exclude": ["admin", "system"],
    "groups": ["employees", "contractors"]
  }
}
```

### Advanced Configuration

#### DLP Policies
Detailed policy configuration:

```json
{
  "policies": [
    {
      "name": "financial_data",
      "description": "Protect financial documents",
      "file_extensions": [".xlsx", ".pdf"],
      "content_patterns": ["salary", "budget", "financial"],
      "destinations": {
        "allowed": ["internal.company.com"],
        "blocked": ["external.*"]
      },
      "actions": {
        "block": true,
        "alert": true,
        "quarantine": false
      }
    }
  ]
}
```

#### Behavioral Analysis Settings
Configure AI analysis parameters:

```json
{
  "behavior_analysis": {
    "enabled": true,
    "llm_provider": "openai",
    "analysis_interval": 300,
    "risk_thresholds": {
      "low": 0.3,
      "medium": 0.6,
      "high": 0.8
    },
    "patterns": {
      "suspicious_hours": ["02:00", "03:00"],
      "large_transfers": "100MB",
      "unusual_applications": ["unknown.exe"]
    }
  }
}
```

## Alerts and Notifications

### Alert Types
- **Security Alerts**: DLP violations, suspicious behavior
- **System Alerts**: Agent failures, configuration issues
- **Performance Alerts**: High resource usage, connectivity problems
- **Compliance Alerts**: Policy violations, audit requirements

### Notification Channels
- **Email**: SMTP-based notifications
- **Web Dashboard**: In-app notifications
- **Syslog**: System logging integration
- **API Webhooks**: Custom integration endpoints

### Alert Configuration
```json
{
  "alerts": {
    "channels": {
      "email": {
        "enabled": true,
        "smtp_host": "smtp.company.com",
        "smtp_port": 587,
        "use_tls": true,
        "username": "alerts@company.com",
        "recipients": ["security@company.com", "it@company.com"]
      },
      "webhook": {
        "enabled": true,
        "url": "https://api.company.com/webhooks/security",
        "headers": {
          "Authorization": "Bearer token123"
        }
      }
    },
    "rules": [
      {
        "name": "high_risk_transfer",
        "condition": "dlp_event.severity == 'high'",
        "channels": ["email", "webhook"],
        "throttle": "1h"
      }
    ]
  }
}
```

## Reports and Analytics

### Standard Reports
- **Activity Summary**: Daily/weekly activity overviews
- **DLP Incident Report**: Data protection violations
- **Productivity Report**: Time usage and efficiency metrics
- **Risk Assessment**: Current threat landscape
- **Compliance Report**: Regulatory compliance status

### Custom Reports
Create tailored reports using the API:

```bash
# Generate custom activity report
curl -X POST http://localhost:5000/api/reports \
  -H "Content-Type: application/json" \
  -d '{
    "type": "activity",
    "filters": {
      "users": ["john.doe", "jane.smith"],
      "date_range": "2024-01-01 to 2024-01-31",
      "applications": ["chrome", "vscode"]
    },
    "format": "pdf"
  }'
```

### Scheduled Reports
Automate report generation:

```json
{
  "scheduled_reports": [
    {
      "name": "weekly_security_summary",
      "schedule": "0 9 * * 1",
      "type": "security",
      "recipients": ["cso@company.com"],
      "format": "pdf"
    }
  ]
}
```

## API Usage

### REST API Endpoints

#### Dashboard Data
```bash
# Get dashboard overview
GET /api/dashboard

# Get real-time activities
GET /api/activities

# Get DLP events
GET /api/dlp-events

# Get productivity data
GET /api/productivity
```

#### Configuration Management
```bash
# Get current configuration
GET /api/config

# Update configuration
PUT /api/config

# Get DLP policies
GET /api/dlp-policies

# Update DLP policies
POST /api/dlp-policies
```

#### User Management
```bash
# List monitored users
GET /api/users

# Add user to monitoring
POST /api/users

# Remove user from monitoring
DELETE /api/users/{username}
```

### Real-time Events (WebSocket)

Connect to real-time updates:

```javascript
const socket = io('http://localhost:5000');

socket.on('agent_data', (data) => {
  console.log('New activity:', data);
});

socket.on('alert', (alert) => {
  console.log('Security alert:', alert);
});

socket.on('data_update', (update) => {
  console.log('Data updated:', update);
});
```

## Security Best Practices

### Access Control
- **Principle of Least Privilege**: Grant minimum required permissions
- **Role-Based Access**: Different access levels for different users
- **Multi-Factor Authentication**: Enable 2FA for admin accounts
- **Session Management**: Configure appropriate session timeouts

### Data Protection
- **Encryption**: Enable data encryption at rest and in transit
- **Data Retention**: Configure appropriate data cleanup policies
- **Backup Security**: Secure backup storage and access
- **Audit Logging**: Enable comprehensive audit trails

### Monitoring Policies
- **Transparent Communication**: Inform users about monitoring
- **Clear Policies**: Document monitoring scope and purposes
- **Regular Reviews**: Periodically review and update policies
- **Compliance Checks**: Ensure compliance with local regulations

## Troubleshooting

### Common Issues

#### Agent Not Collecting Data
```bash
# Check agent status
sudo systemctl status workforce-agent

# Check agent logs
sudo journalctl -u workforce-agent -n 50

# Verify permissions
ls -la /dev/input/

# Test agent connectivity
curl http://localhost:5000/api/health
```

#### High Resource Usage
```bash
# Check system resources
top -p $(pgrep workforce_agent)

# Adjust monitoring settings
# Reduce sample rate in agent_config.json
"sample_rate": 5000

# Enable data cleanup
"cleanup_interval": 3600
```

#### False Positive Alerts
```bash
# Adjust DLP sensitivity
# Modify policy thresholds
"alert_threshold": "high"

# Add exceptions
"exclude_patterns": ["*.tmp", "*.log"]

# Update behavioral models
# Retrain with legitimate activities
```

#### Dashboard Not Loading
```bash
# Check backend service
sudo systemctl status workforce-backend

# Verify port availability
netstat -tlnp | grep 5000

# Check browser console for errors
# Clear browser cache
# Try different browser
```

### Performance Optimization

#### System Tuning
```bash
# Increase file descriptors
echo "workforce-agent soft nofile 65536" >> /etc/security/limits.conf

# Optimize kernel parameters
echo "net.core.somaxconn = 65536" >> /etc/sysctl.conf
sysctl -p
```

#### Database Optimization
```json
{
  "database": {
    "pool_size": 20,
    "max_overflow": 30,
    "pool_timeout": 30,
    "pool_recycle": 3600
  }
}
```

#### Monitoring Optimization
```json
{
  "monitoring": {
    "batch_size": 1000,
    "compression": true,
    "async_processing": true
  }
}
```

## Maintenance

### Regular Tasks
- **Log Rotation**: Ensure logs don't fill disk space
- **Database Cleanup**: Remove old data according to retention policies
- **Security Updates**: Keep system and dependencies updated
- **Performance Monitoring**: Track system resource usage
- **Backup Verification**: Test backup integrity regularly

### Backup and Recovery
```bash
# Create backup
sudo /opt/workforce-agent/backup.sh

# Restore from backup
sudo /opt/workforce-agent/restore.sh /path/to/backup

# Verify backup integrity
sudo /opt/workforce-agent/verify-backup.sh
```

### Update Management
```bash
# Check for updates
sudo workforce-agent --check-updates

# Apply updates
sudo workforce-agent --update

# Rollback if needed
sudo workforce-agent --rollback
```

## Support and Resources

### Getting Help
- **Documentation**: Check this user manual and API docs
- **Logs**: Review agent and system logs for error details
- **Community**: Join the project community for peer support
- **Professional Services**: Contact for enterprise support options

### Useful Commands
```bash
# Quick system check
sudo workforce-agent --diagnostics

# Generate support bundle
sudo workforce-agent --support-bundle

# Reset to defaults
sudo workforce-agent --reset-config

# Export configuration
sudo workforce-agent --export-config > config_backup.json
