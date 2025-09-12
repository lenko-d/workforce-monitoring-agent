# API Documentation

This document provides comprehensive documentation for the Workforce Monitoring and Insider Risk Management Agent REST API and WebSocket interfaces.

## Overview

The API provides programmatic access to all monitoring data, configuration management, and real-time events. It follows RESTful principles and uses JSON for data exchange.

**Base URL**: `http://localhost:5000/api`

**Authentication**: API key or session-based authentication

## Authentication

### API Key Authentication
```bash
curl -H "X-API-Key: your-api-key" http://localhost:5000/api/dashboard
```

### Session Authentication
```bash
# Login to get session
curl -X POST http://localhost:5000/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username": "admin", "password": "password"}'

# Use session cookie in subsequent requests
curl -H "Cookie: session=session-id" http://localhost:5000/api/dashboard
```

## Dashboard Endpoints

### GET /api/dashboard

Get dashboard overview with key metrics and recent activity.

**Response:**
```json
{
  "status": "success",
  "data": {
    "active_users": 45,
    "total_alerts": 12,
    "risk_score": 0.23,
    "productivity_rate": 78.5,
    "recent_activities": [
      {
        "id": "act_123",
        "user": "john.doe",
        "type": "file_access",
        "timestamp": "2024-01-15T10:30:00Z",
        "details": {
          "file": "/home/john.doe/documents/secret.pdf",
          "action": "read"
        }
      }
    ],
    "system_health": {
      "agent_status": "healthy",
      "cpu_usage": 15.2,
      "memory_usage": 234,
      "disk_usage": 45.8
    }
  }
}
```

### GET /api/dashboard/metrics

Get detailed metrics for dashboard charts.

**Query Parameters:**
- `period`: Time period (1h, 24h, 7d, 30d)
- `resolution`: Data resolution (1m, 5m, 1h, 1d)

**Response:**
```json
{
  "status": "success",
  "data": {
    "timestamps": ["2024-01-15T00:00:00Z", "2024-01-15T01:00:00Z"],
    "active_users": [23, 45],
    "alerts": [2, 5],
    "productivity": [75.2, 82.1],
    "risk_scores": [0.15, 0.23]
  }
}
```

## Activity Monitoring

### GET /api/activities

Get recent user activities with filtering and pagination.

**Query Parameters:**
- `user`: Filter by username
- `type`: Activity type (keyboard, mouse, file_access, network, etc.)
- `start_time`: Start timestamp (ISO 8601)
- `end_time`: End timestamp (ISO 8601)
- `limit`: Maximum number of results (default: 100)
- `offset`: Pagination offset (default: 0)

**Response:**
```json
{
  "status": "success",
  "data": {
    "activities": [
      {
        "id": "act_123",
        "user": "john.doe",
        "type": "keyboard",
        "timestamp": "2024-01-15T10:30:15Z",
        "details": {
          "keys_pressed": 45,
          "active_window": "Google Chrome",
          "application": "chrome"
        },
        "risk_score": 0.05
      },
      {
        "id": "act_124",
        "user": "john.doe",
        "type": "file_access",
        "timestamp": "2024-01-15T10:30:20Z",
        "details": {
          "file": "/home/john.doe/documents/report.docx",
          "action": "write",
          "size": 245760
        },
        "risk_score": 0.12
      }
    ],
    "total": 1250,
    "limit": 100,
    "offset": 0
  }
}
```

### GET /api/activities/{id}

Get detailed information about a specific activity.

**Response:**
```json
{
  "status": "success",
  "data": {
    "id": "act_123",
    "user": "john.doe",
    "type": "file_access",
    "timestamp": "2024-01-15T10:30:20Z",
    "details": {
      "file": "/home/john.doe/documents/report.docx",
      "action": "write",
      "size": 245760,
      "content_hash": "a1b2c3d4...",
      "permissions": "rw-r--r--"
    },
    "context": {
      "user_agent": "Mozilla/5.0...",
      "ip_address": "192.168.1.100",
      "session_id": "sess_456"
    },
    "analysis": {
      "risk_score": 0.12,
      "anomaly_score": 0.85,
      "patterns": ["after_hours_access", "large_file_modification"]
    }
  }
}
```

### GET /api/activities/summary

Get aggregated activity statistics.

**Query Parameters:**
- `period`: Time period (1h, 24h, 7d, 30d)
- `group_by`: Grouping field (user, type, application)

**Response:**
```json
{
  "status": "success",
  "data": {
    "period": "24h",
    "total_activities": 15420,
    "by_user": {
      "john.doe": 2340,
      "jane.smith": 1890,
      "bob.johnson": 1567
    },
    "by_type": {
      "keyboard": 8920,
      "mouse": 4560,
      "file_access": 1200,
      "network": 740
    },
    "by_application": {
      "chrome": 3450,
      "vscode": 2890,
      "slack": 1230
    }
  }
}
```

## Data Loss Prevention (DLP)

### GET /api/dlp-events

Get DLP violation events.

**Query Parameters:**
- `severity`: Filter by severity (low, medium, high, critical)
- `policy`: Filter by DLP policy name
- `status`: Event status (detected, blocked, allowed, quarantined)
- `start_time`: Start timestamp
- `end_time`: End timestamp
- `limit`: Maximum results
- `offset`: Pagination offset

**Response:**
```json
{
  "status": "success",
  "data": {
    "events": [
      {
        "id": "dlp_123",
        "user": "john.doe",
        "timestamp": "2024-01-15T10:30:20Z",
        "policy": "confidential_files",
        "severity": "high",
        "status": "blocked",
        "details": {
          "file": "/home/john.doe/documents/secret.pdf",
          "destination": "external-drive",
          "size": 2097152,
          "content_matches": ["confidential", "internal"],
          "risk_factors": ["large_file", "external_destination"]
        },
        "actions_taken": ["blocked_transfer", "alert_generated"],
        "analysis": {
          "confidence": 0.95,
          "false_positive_probability": 0.02
        }
      }
    ],
    "total": 45,
    "limit": 100,
    "offset": 0
  }
}
```

### GET /api/dlp-policies

Get all DLP policies.

**Response:**
```json
{
  "status": "success",
  "data": {
    "policies": [
      {
        "id": "pol_123",
        "name": "confidential_files",
        "description": "Protect confidential documents",
        "enabled": true,
        "rules": {
          "file_extensions": [".docx", ".xlsx", ".pdf"],
          "content_patterns": ["confidential", "internal", "sensitive"],
          "file_size_threshold": "10MB",
          "destinations": {
            "allowed": ["internal.company.com"],
            "blocked": ["external.*", "*.dropbox.com"]
          }
        },
        "actions": {
          "block": true,
          "alert": true,
          "quarantine": false,
          "encrypt": true
        },
        "exceptions": {
          "users": ["admin"],
          "groups": ["executives"]
        }
      }
    ]
  }
}
```

### POST /api/dlp-policies

Create a new DLP policy.

**Request Body:**
```json
{
  "name": "financial_data",
  "description": "Protect financial documents and data",
  "enabled": true,
  "rules": {
    "file_extensions": [".xlsx", ".pdf", ".csv"],
    "content_patterns": ["salary", "budget", "financial", "revenue"],
    "file_size_threshold": "50MB"
  },
  "actions": {
    "block": true,
    "alert": true,
    "quarantine": true
  }
}
```

### PUT /api/dlp-policies/{id}

Update an existing DLP policy.

### DELETE /api/dlp-policies/{id}

Delete a DLP policy.

## Behavioral Analytics

### GET /api/behavior-analysis

Get behavioral analysis results.

**Query Parameters:**
- `user`: Specific user to analyze
- `period`: Analysis time period
- `model`: Analysis model (ml, llm, hybrid)

**Response:**
```json
{
  "status": "success",
  "data": {
    "user": "john.doe",
    "period": "7d",
    "overall_risk_score": 0.34,
    "anomalies": [
      {
        "id": "anom_123",
        "type": "unusual_hours",
        "severity": "medium",
        "timestamp": "2024-01-15T02:30:00Z",
        "description": "Access during unusual hours (2:30 AM)",
        "confidence": 0.87,
        "patterns": ["after_hours", "single_user"]
      }
    ],
    "patterns": [
      {
        "name": "after_hours_access",
        "frequency": 0.15,
        "risk_level": "medium",
        "description": "Frequent access outside business hours"
      }
    ],
    "recommendations": [
      {
        "type": "policy",
        "action": "time_restrictions",
        "description": "Implement time-based access restrictions",
        "priority": "high"
      }
    ]
  }
}
```

### GET /api/behavior-analysis/llm-insights

Get LLM-powered behavioral insights.

**Response:**
```json
{
  "status": "success",
  "data": {
    "insights": [
      {
        "id": "llm_123",
        "user": "john.doe",
        "timestamp": "2024-01-15T10:30:00Z",
        "analysis_type": "pattern_recognition",
        "severity": "medium",
        "confidence": 0.92,
        "summary": "Detected unusual file access pattern - accessing sensitive documents outside normal hours",
        "details": "User typically accesses sensitive files during business hours (9AM-5PM) but recent activity shows access at 2:30 AM. This pattern has occurred 3 times in the last week.",
        "recommendations": [
          "Enable additional monitoring for after-hours access",
          "Review access logs for similar patterns",
          "Consider implementing time-based access restrictions"
        ],
        "context": {
          "normal_behavior": "9AM-5PM access pattern",
          "anomalous_behavior": "2:30 AM access",
          "frequency": "3 occurrences this week"
        }
      }
    ]
  }
}
```

## Productivity Analytics

### GET /api/productivity

Get productivity metrics and time tracking data.

**Query Parameters:**
- `user`: Specific user
- `period`: Time period
- `category`: Productivity category (productive, unproductive, neutral)

**Response:**
```json
{
  "status": "success",
  "data": {
    "user": "john.doe",
    "period": "24h",
    "total_time": 28800,
    "productive_time": 21600,
    "unproductive_time": 4320,
    "neutral_time": 2880,
    "productivity_rate": 75.0,
    "by_application": [
      {
        "application": "vscode",
        "category": "productive",
        "time_spent": 14400,
        "percentage": 50.0
      },
      {
        "application": "chrome",
        "category": "neutral",
        "time_spent": 7200,
        "percentage": 25.0
      },
      {
        "application": "slack",
        "category": "unproductive",
        "time_spent": 3600,
        "percentage": 12.5
      }
    ],
    "trends": {
      "daily_average": 74.2,
      "weekly_trend": "increasing",
      "peak_productivity_hour": 14
    }
  }
}
```

### GET /api/productivity/categories

Get productivity categories configuration.

**Response:**
```json
{
  "status": "success",
  "data": {
    "categories": {
      "productive": [
        "vscode",
        "intellij",
        "eclipse",
        "word",
        "excel"
      ],
      "unproductive": [
        "facebook",
        "twitter",
        "youtube",
        "netflix",
        "games"
      ],
      "neutral": [
        "chrome",
        "firefox",
        "outlook",
        "slack",
        "teams"
      ]
    },
    "custom_rules": [
      {
        "pattern": ".*work.*",
        "category": "productive"
      },
      {
        "pattern": ".*social.*",
        "category": "unproductive"
      }
    ]
  }
}
```

## Configuration Management

### GET /api/config

Get current agent configuration.

**Response:**
```json
{
  "status": "success",
  "data": {
    "agent": {
      "version": "1.0.0",
      "monitoring_enabled": true,
      "sample_rate": 1000,
      "idle_threshold": 300
    },
    "backend": {
      "host": "localhost",
      "port": 5000,
      "database_url": "sqlite:///workforce.db"
    },
    "logging": {
      "level": "info",
      "max_file_size": "100MB",
      "retention_days": 30
    }
  }
}
```

### PUT /api/config

Update agent configuration.

**Request Body:**
```json
{
  "agent": {
    "sample_rate": 2000,
    "idle_threshold": 600
  },
  "logging": {
    "level": "debug"
  }
}
```

### GET /api/config/schema

Get configuration schema for validation.

## User Management

### GET /api/users

Get list of monitored users.

**Response:**
```json
{
  "status": "success",
  "data": {
    "users": [
      {
        "username": "john.doe",
        "full_name": "John Doe",
        "department": "Engineering",
        "role": "Developer",
        "status": "active",
        "last_activity": "2024-01-15T10:30:00Z",
        "risk_score": 0.23
      }
    ],
    "total": 50,
    "active": 45
  }
}
```

### POST /api/users

Add a user to monitoring.

**Request Body:**
```json
{
  "username": "jane.smith",
  "full_name": "Jane Smith",
  "department": "Marketing",
  "role": "Manager"
}
```

### DELETE /api/users/{username}

Remove a user from monitoring.

## Reports

### POST /api/reports

Generate a custom report.

**Request Body:**
```json
{
  "type": "activity_summary",
  "filters": {
    "users": ["john.doe", "jane.smith"],
    "date_range": "2024-01-01 to 2024-01-31",
    "applications": ["vscode", "chrome"]
  },
  "format": "pdf",
  "schedule": {
    "frequency": "weekly",
    "recipients": ["manager@company.com"]
  }
}
```

**Response:**
```json
{
  "status": "success",
  "data": {
    "report_id": "rep_123",
    "status": "generating",
    "estimated_completion": "2024-01-15T10:35:00Z",
    "download_url": "/api/reports/rep_123/download"
  }
}
```

### GET /api/reports/{id}

Get report status and metadata.

### GET /api/reports/{id}/download

Download the generated report.

## Real-time Events (WebSocket)

### Connection

```javascript
const socket = io('http://localhost:5000', {
  auth: {
    token: 'your-api-key'
  }
});
```

### Events

#### agent_data
Real-time activity data from agents.

```javascript
socket.on('agent_data', (data) => {
  console.log('New activity:', data);
  // data: { user, type, timestamp, details, risk_score }
});
```

#### alert
Security alerts and notifications.

```javascript
socket.on('alert', (alert) => {
  console.log('Alert:', alert);
  // alert: { id, type, severity, message, user, timestamp }
});
```

#### data_update
Dashboard data updates.

```javascript
socket.on('data_update', (update) => {
  console.log('Data update:', update);
  // update: { type, data, timestamp }
});
```

#### system_status
System health and status updates.

```javascript
socket.on('system_status', (status) => {
  console.log('System status:', status);
  // status: { component, status, metrics, timestamp }
});
```

## Error Handling

All API endpoints return standardized error responses:

```json
{
  "status": "error",
  "error": {
    "code": "VALIDATION_ERROR",
    "message": "Invalid request parameters",
    "details": {
      "field": "user",
      "reason": "User not found"
    }
  }
}
```

### Common Error Codes
- `VALIDATION_ERROR`: Invalid request parameters
- `AUTHENTICATION_ERROR`: Authentication failed
- `AUTHORIZATION_ERROR`: Insufficient permissions
- `NOT_FOUND`: Resource not found
- `RATE_LIMITED`: Too many requests
- `INTERNAL_ERROR`: Server error

## Rate Limiting

API requests are rate limited to prevent abuse:

- **Authenticated requests**: 1000 requests per hour
- **Anonymous requests**: 100 requests per hour
- **Real-time connections**: 10 concurrent connections per user

Rate limit headers are included in responses:
```
X-RateLimit-Limit: 1000
X-RateLimit-Remaining: 999
X-RateLimit-Reset: 1640995200
```

## SDKs and Libraries

### Python SDK
```python
from workforce_monitoring import Client

client = Client(api_key='your-api-key', base_url='http://localhost:5000')

# Get dashboard data
dashboard = client.dashboard.get()

# Get activities
activities = client.activities.list(user='john.doe', limit=100)

# Create DLP policy
policy = client.dlp.create_policy({
    'name': 'test_policy',
    'rules': {'file_extensions': ['.pdf']}
})
```

### JavaScript SDK
```javascript
import { WorkforceClient } from 'workforce-monitoring-sdk';

const client = new WorkforceClient({
  apiKey: 'your-api-key',
  baseURL: 'http://localhost:5000'
});

// Real-time updates
client.on('alert', (alert) => {
  console.log('New alert:', alert);
});

// Get activities
const activities = await client.activities.list({
  user: 'john.doe',
  limit: 50
});
```

## Best Practices

### Efficient API Usage
- Use appropriate filters to limit result sets
- Implement pagination for large datasets
- Cache frequently accessed data
- Use WebSocket for real-time updates instead of polling

### Error Handling
```javascript
try {
  const response = await fetch('/api/activities');
  if (!response.ok) {
    const error = await response.json();
    handleError(error);
  }
  const data = await response.json();
} catch (error) {
  console.error('Network error:', error);
}
```

### Authentication Security
- Store API keys securely
- Rotate keys regularly
- Use HTTPS in production
- Implement proper session management

## Versioning

API versioning follows semantic versioning:

- **v1**: Current stable version
- **Breaking changes**: New major version
- **Additions**: Minor version bump
- **Bug fixes**: Patch version bump

Specify version in requests:
```
Accept: application/vnd.workforce.v1+json
```

## Support

For API support and questions:
- **Documentation**: This API documentation
- **Community**: GitHub issues and discussions
- **Enterprise**: Contact support for premium assistance
- **Status Page**: API status and incident updates
