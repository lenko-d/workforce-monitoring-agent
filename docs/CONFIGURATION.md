# Configuration Guide

This guide provides detailed information about configuring the Workforce Monitoring and Insider Risk Management Agent, including all available configuration options, best practices, and examples.

## Configuration Overview

The agent uses multiple configuration files for different components:

- **Agent Configuration** (`/etc/workforce-agent/agent_config.json`) - Core monitoring settings
- **DLP Policies** (`/etc/workforce-agent/dlp_policies.json`) - Data loss prevention rules
- **Upgrade Configuration** (`/etc/workforce-agent/upgrade_config.json`) - Update management settings
- **Backend Configuration** (`/etc/workforce-agent/backend_config.json`) - Python backend settings

## Agent Configuration

### Core Settings

```json
{
  "agent": {
    "version": "1.0.0",
    "name": "workforce-agent-01",
    "description": "Primary monitoring agent",
    "enabled": true,
    "debug_mode": false,
    "log_level": "info"
  }
}
```

### Monitoring Configuration

```json
{
  "monitoring": {
    "enabled": true,
    "sample_rate": 1000,
    "idle_threshold": 300,
    "batch_size": 100,
    "max_buffer_size": 10000,
    "compression_enabled": true,
    "async_processing": true
  }
}
```

#### Sample Rate
- **Type**: Integer (milliseconds)
- **Default**: 1000
- **Range**: 100-10000
- **Description**: How often to sample user activity
- **Impact**: Lower values increase accuracy but use more resources

#### Idle Threshold
- **Type**: Integer (seconds)
- **Default**: 300
- **Range**: 60-3600
- **Description**: Time before user is considered idle
- **Impact**: Affects productivity calculations

### Activity Monitoring

```json
{
  "activity_monitor": {
    "keyboard_enabled": true,
    "mouse_enabled": true,
    "window_focus_enabled": true,
    "application_tracking": true,
    "screenshot_enabled": false,
    "screenshot_interval": 300,
    "privacy_mode": false
  }
}
```

#### Privacy Mode
- **Type**: Boolean
- **Default**: false
- **Description**: Limits data collection for privacy compliance
- **Impact**: Reduces monitoring scope when enabled

### Data Loss Prevention

```json
{
  "dlp": {
    "enabled": true,
    "real_time_scanning": true,
    "network_monitoring": true,
    "file_system_monitoring": true,
    "content_analysis": true,
    "max_file_size": "100MB",
    "scan_temp_files": false,
    "scan_hidden_files": false
  }
}
```

### Behavioral Analysis

```json
{
  "behavior_analysis": {
    "enabled": true,
    "anomaly_detection": true,
    "llm_enabled": true,
    "llm_provider": "openai",
    "analysis_interval": 300,
    "risk_thresholds": {
      "low": 0.2,
      "medium": 0.5,
      "high": 0.8,
      "critical": 0.95
    },
    "model_update_interval": 3600
  }
}
```

#### LLM Configuration
```json
{
  "llm": {
    "provider": "openai",
    "model": "gpt-4",
    "api_key": "your-api-key-here",
    "max_tokens": 1000,
    "temperature": 0.3,
    "timeout": 30,
    "retry_attempts": 3,
    "rate_limit": 60
  }
}
```

### Logging Configuration

```json
{
  "logging": {
    "level": "info",
    "format": "json",
    "output": "file",
    "file_path": "/var/log/workforce-agent/agent.log",
    "max_file_size": "100MB",
    "max_files": 10,
    "compress_old_logs": true,
    "syslog_enabled": false,
    "syslog_facility": "local0"
  }
}
```

#### Log Levels
- **DEBUG**: Detailed diagnostic information
- **INFO**: General information about operation
- **WARNING**: Warning messages for potential issues
- **ERROR**: Error messages for failures
- **CRITICAL**: Critical errors requiring immediate attention

### Alert Configuration

```json
{
  "alerts": {
    "enabled": true,
    "real_time_alerts": true,
    "batch_alerts": true,
    "batch_interval": 300,
    "channels": {
      "email": {
        "enabled": true,
        "smtp_server": "smtp.company.com",
        "smtp_port": 587,
        "use_tls": true,
        "username": "alerts@company.com",
        "password": "smtp-password",
        "recipients": ["security@company.com", "it@company.com"],
        "sender": "workforce-agent@company.com"
      },
      "webhook": {
        "enabled": true,
        "url": "https://api.company.com/webhooks/security",
        "method": "POST",
        "headers": {
          "Authorization": "Bearer token123",
          "Content-Type": "application/json"
        },
        "timeout": 10
      },
      "slack": {
        "enabled": false,
        "webhook_url": "https://hooks.slack.com/...",
        "channel": "#security-alerts",
        "username": "Workforce Agent"
      }
    }
  }
}
```

### Database Configuration

```json
{
  "database": {
    "type": "sqlite",
    "path": "/var/lib/workforce-agent/data.db",
    "connection_pool_size": 10,
    "connection_timeout": 30,
    "backup_enabled": true,
    "backup_interval": 86400,
    "retention_days": 365
  }
}
```

### Network Configuration

```json
{
  "network": {
    "backend_host": "localhost",
    "backend_port": 5000,
    "use_ssl": false,
    "ssl_cert_path": "/etc/ssl/certs/workforce-agent.crt",
    "ssl_key_path": "/etc/ssl/private/workforce-agent.key",
    "connection_timeout": 30,
    "retry_attempts": 3,
    "retry_delay": 5
  }
}
```

## DLP Policy Configuration

### Policy Structure

```json
{
  "policies": [
    {
      "id": "confidential_files",
      "name": "Confidential Documents",
      "description": "Monitor access to confidential files",
      "enabled": true,
      "priority": "high",
      "rules": {
        "file_extensions": [".docx", ".xlsx", ".pdf", ".pptx"],
        "file_patterns": ["*confidential*", "*internal*", "*sensitive*"],
        "content_patterns": ["confidential", "internal use only", "proprietary"],
        "file_size_min": "1KB",
        "file_size_max": "100MB"
      },
      "destinations": {
        "allowed": ["internal.company.com", "company.sharepoint.com"],
        "blocked": ["*.dropbox.com", "*.google.com", "external.*"],
        "monitored": ["*.usb", "*.external"]
      },
      "actions": {
        "monitor": true,
        "alert": true,
        "block": false,
        "quarantine": false,
        "encrypt": false,
        "log": true
      },
      "exceptions": {
        "users": ["admin", "auditor"],
        "groups": ["executives", "legal"],
        "departments": ["IT", "Security"]
      },
      "sensitivity": "high"
    }
  ]
}
```

### Advanced DLP Rules

#### Content-Based Rules
```json
{
  "content_rules": {
    "credit_card_patterns": {
      "regex": "\\b\\d{4}[ -]?\\d{4}[ -]?\\d{4}[ -]?\\d{4}\\b",
      "description": "Credit card number detection"
    },
    "ssn_patterns": {
      "regex": "\\b\\d{3}-\\d{2}-\\d{4}\\b",
      "description": "Social Security Number detection"
    },
    "email_patterns": {
      "regex": "\\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Z|a-z]{2,}\\b",
      "description": "Email address detection"
    }
  }
}
```

#### File Type Signatures
```json
{
  "file_signatures": {
    "pdf": ["255044462D"],
    "docx": ["504B030414000600"],
    "xlsx": ["504B030414000600"],
    "zip": ["504B0304"]
  }
}
```

### Network DLP Rules

```json
{
  "network_rules": {
    "protocols": ["ftp", "sftp", "http", "https"],
    "ports": [21, 22, 80, 443, 8080],
    "domains": ["*.dropbox.com", "*.google.com", "*.onedrive.com"],
    "ip_ranges": ["192.168.0.0/16", "10.0.0.0/8"],
    "file_size_threshold": "10MB",
    "transfer_rate_threshold": "1MB/s"
  }
}
```

## User and Group Management

### User Configuration

```json
{
  "users": {
    "include": ["john.doe", "jane.smith"],
    "exclude": ["admin", "system", "guest"],
    "groups": {
      "employees": ["john.doe", "jane.smith", "bob.johnson"],
      "contractors": ["alice.contractor", "charlie.temp"],
      "executives": ["ceo.company", "cfo.company"]
    },
    "departments": {
      "engineering": ["john.doe", "jane.smith"],
      "marketing": ["bob.johnson"],
      "sales": ["alice.contractor"]
    }
  }
}
```

### Role-Based Access

```json
{
  "roles": {
    "admin": {
      "permissions": ["read", "write", "delete", "configure"],
      "users": ["admin"]
    },
    "analyst": {
      "permissions": ["read", "write"],
      "users": ["analyst1", "analyst2"],
      "restrictions": {
        "max_query_results": 1000,
        "allowed_time_range": "9:00-17:00"
      }
    },
    "viewer": {
      "permissions": ["read"],
      "users": ["viewer1", "viewer2"],
      "restrictions": {
        "max_query_results": 100,
        "allowed_reports": ["summary", "activity"]
      }
    }
  }
}
```

## Productivity Configuration

### Application Categories

```json
{
  "productivity": {
    "categories": {
      "productive": [
        "vscode",
        "intellij-idea",
        "eclipse",
        "word",
        "excel",
        "powerpoint",
        "outlook",
        "slack",
        "teams"
      ],
      "unproductive": [
        "facebook",
        "twitter",
        "instagram",
        "youtube",
        "netflix",
        "games",
        "minecraft"
      ],
      "neutral": [
        "chrome",
        "firefox",
        "safari",
        "edge",
        "terminal",
        "file-explorer"
      ]
    },
    "custom_rules": [
      {
        "pattern": ".*work.*",
        "category": "productive",
        "weight": 1.0
      },
      {
        "pattern": ".*meeting.*",
        "category": "productive",
        "weight": 0.8
      },
      {
        "pattern": ".*social.*",
        "category": "unproductive",
        "weight": 1.0
      }
    ],
    "time_tracking": {
      "track_idle_time": true,
      "idle_threshold": 300,
      "auto_pause_tracking": true,
      "pause_threshold": 1800
    }
  }
}
```

### Working Hours Configuration

```json
{
  "working_hours": {
    "timezone": "America/New_York",
    "business_days": ["monday", "tuesday", "wednesday", "thursday", "friday"],
    "business_hours": {
      "start": "09:00",
      "end": "17:00",
      "break_start": "12:00",
      "break_end": "13:00"
    },
    "holidays": [
      "2024-01-01",
      "2024-07-04",
      "2024-12-25"
    ],
    "flexible_hours": false
  }
}
```

## Security Configuration

### Authentication

```json
{
  "authentication": {
    "method": "local",
    "session_timeout": 3600,
    "max_login_attempts": 5,
    "lockout_duration": 900,
    "password_policy": {
      "min_length": 8,
      "require_uppercase": true,
      "require_lowercase": true,
      "require_numbers": true,
      "require_symbols": true,
      "prevent_reuse": true,
      "max_age": 90
    },
    "two_factor_auth": {
      "enabled": false,
      "method": "totp",
      "issuer": "Workforce Agent"
    }
  }
}
```

### Encryption

```json
{
  "encryption": {
    "data_at_rest": {
      "enabled": true,
      "algorithm": "AES-256-GCM",
      "key_rotation": 90
    },
    "data_in_transit": {
      "enabled": true,
      "protocol": "TLS 1.3",
      "cipher_suites": ["TLS_AES_256_GCM_SHA384"]
    },
    "database_encryption": {
      "enabled": true,
      "key_file": "/etc/workforce-agent/db.key"
    }
  }
}
```

### Audit Configuration

```json
{
  "audit": {
    "enabled": true,
    "log_all_access": true,
    "log_configuration_changes": true,
    "log_admin_actions": true,
    "retention_period": 2555,
    "immutable_logs": true,
    "external_audit": {
      "enabled": false,
      "endpoint": "https://audit.company.com/api/events",
      "api_key": "audit-api-key"
    }
  }
}
```

## Performance Tuning

### Resource Limits

```json
{
  "performance": {
    "cpu_limit": 50,
    "memory_limit": "512MB",
    "disk_iops_limit": 1000,
    "network_bandwidth_limit": "10MB/s",
    "thread_pool_size": 4,
    "queue_size": 1000,
    "batch_processing": {
      "enabled": true,
      "batch_size": 100,
      "flush_interval": 30
    }
  }
}
```

### Monitoring Optimization

```json
{
  "optimization": {
    "sampling": {
      "adaptive_enabled": true,
      "min_sample_rate": 500,
      "max_sample_rate": 5000,
      "activity_threshold": 10
    },
    "caching": {
      "enabled": true,
      "ttl": 300,
      "max_size": "100MB"
    },
    "compression": {
      "enabled": true,
      "algorithm": "lz4",
      "level": 1
    }
  }
}
```

## Backend Configuration

### Flask Application Settings

```json
{
  "flask": {
    "debug": false,
    "testing": false,
    "secret_key": "your-secret-key-here",
    "session_cookie_secure": true,
    "session_cookie_httponly": true,
    "session_cookie_samesite": "Strict",
    "max_content_length": "16MB"
  }
}
```

### Database Settings

```json
{
  "sqlalchemy": {
    "database_uri": "sqlite:///workforce.db",
    "echo": false,
    "pool_pre_ping": true,
    "pool_recycle": 3600,
    "pool_size": 10,
    "max_overflow": 20
  }
}
```

### Socket.IO Configuration

```json
{
  "socketio": {
    "cors_allowed_origins": ["http://localhost:3000"],
    "ping_timeout": 60,
    "ping_interval": 25,
    "max_http_buffer_size": "1GB",
    "allow_upgrades": true,
    "transports": ["polling", "websocket"]
  }
}
```

## Upgrade Configuration

### Update Management

```json
{
  "upgrade": {
    "auto_update": true,
    "update_check_interval": 86400,
    "update_server": "https://updates.company.com/api",
    "channel": "stable",
    "backup_before_upgrade": true,
    "rollback_enabled": true,
    "maintenance_window": {
      "enabled": true,
      "start_time": "02:00",
      "duration": 120
    }
  }
}
```

## Configuration Validation

### Schema Validation

The agent validates configuration files against JSON schemas:

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "properties": {
    "agent": {
      "type": "object",
      "properties": {
        "version": {"type": "string"},
        "enabled": {"type": "boolean"},
        "debug_mode": {"type": "boolean"}
      },
      "required": ["version", "enabled"]
    }
  }
}
```

### Configuration Testing

```bash
# Validate configuration
workforce-agent --validate-config /etc/workforce-agent/agent_config.json

# Test configuration loading
workforce-agent --test-config

# Dry-run configuration
workforce-agent --dry-run
```

## Configuration Management

### Version Control

```json
{
  "config_versioning": {
    "enabled": true,
    "git_repository": "/etc/workforce-agent/config.git",
    "auto_commit": true,
    "backup_on_change": true,
    "rollback_enabled": true
  }
}
```

### Configuration Distribution

For multi-agent deployments:

```json
{
  "config_distribution": {
    "central_config": true,
    "config_server": "https://config.company.com",
    "auto_sync": true,
    "sync_interval": 300,
    "conflict_resolution": "server_wins"
  }
}
```

## Best Practices

### Security Best Practices
1. Use strong encryption for sensitive configuration values
2. Regularly rotate API keys and passwords
3. Implement least privilege access
4. Enable audit logging for configuration changes
5. Use environment-specific configurations

### Performance Best Practices
1. Tune sampling rates based on your monitoring needs
2. Enable compression for large data volumes
3. Configure appropriate resource limits
4. Use batch processing for high-volume scenarios
5. Implement proper indexing for database queries

### Maintenance Best Practices
1. Regularly backup configuration files
2. Test configuration changes in staging environment
3. Document all configuration changes
4. Monitor configuration file integrity
5. Plan for configuration updates during maintenance windows

## Troubleshooting Configuration Issues

### Common Configuration Problems

#### Invalid JSON Syntax
```bash
# Validate JSON syntax
python3 -m json.tool /etc/workforce-agent/agent_config.json

# Check for syntax errors
jq . /etc/workforce-agent/agent_config.json
```

#### Permission Issues
```bash
# Check file permissions
ls -la /etc/workforce-agent/

# Fix permissions
sudo chown workforce-agent:workforce-agent /etc/workforce-agent/*.json
sudo chmod 600 /etc/workforce-agent/*.json
```

#### Configuration Not Loading
```bash
# Check agent logs
sudo journalctl -u workforce-agent -n 50

# Test configuration loading
sudo -u workforce-agent workforce-agent --test-config
```

#### Performance Issues
```bash
# Monitor resource usage
top -p $(pgrep workforce_agent)

# Check configuration for performance settings
grep -n "sample_rate\|batch_size" /etc/workforce-agent/agent_config.json
```

## Configuration Examples

### Basic Configuration
```json
{
  "agent": {
    "enabled": true,
    "debug_mode": false,
    "log_level": "info"
  },
  "monitoring": {
    "enabled": true,
    "sample_rate": 1000,
    "idle_threshold": 300
  },
  "alerts": {
    "enabled": true,
    "channels": {
      "email": {
        "enabled": true,
        "smtp_server": "smtp.company.com",
        "recipients": ["admin@company.com"]
      }
    }
  }
}
```

### Enterprise Configuration
```json
{
  "agent": {
    "enabled": true,
    "name": "workforce-agent-prod-01",
    "debug_mode": false,
    "log_level": "warning"
  },
  "monitoring": {
    "enabled": true,
    "sample_rate": 2000,
    "idle_threshold": 600,
    "batch_size": 500,
    "compression_enabled": true
  },
  "behavior_analysis": {
    "enabled": true,
    "llm_enabled": true,
    "llm_provider": "openai",
    "analysis_interval": 600
  },
  "security": {
    "encryption": {
      "data_at_rest": true,
      "data_in_transit": true
    },
    "audit": {
      "enabled": true,
      "immutable_logs": true
    }
  }
}
```

### High-Security Configuration
```json
{
  "agent": {
    "enabled": true,
    "privacy_mode": false,
    "debug_mode": false
  },
  "monitoring": {
    "enabled": true,
    "sample_rate": 500,
    "real_time_alerts": true
  },
  "dlp": {
    "enabled": true,
    "real_time_scanning": true,
    "content_analysis": true,
    "network_monitoring": true
  },
  "behavior_analysis": {
    "enabled": true,
    "anomaly_detection": true,
    "llm_enabled": true,
    "risk_thresholds": {
      "low": 0.1,
      "medium": 0.3,
      "high": 0.7,
      "critical": 0.9
    }
  },
  "alerts": {
    "enabled": true,
    "real_time_alerts": true,
    "channels": {
      "email": {"enabled": true},
      "webhook": {"enabled": true},
      "slack": {"enabled": true}
    }
  }
}
