from flask import Flask, request, jsonify, send_from_directory
from flask_socketio import SocketIO, emit
from flask_cors import CORS
from werkzeug.exceptions import BadRequest
from datetime import datetime, timedelta
import threading
import time
import os

app = Flask(__name__)
app.config['SECRET_KEY'] = 'workforce_monitoring_secret_key'
CORS(app)
socketio = SocketIO(app, cors_allowed_origins="*")

# Global data storage
activity_data = []
dlp_events = []
time_entries = []
behavior_patterns = []
alerts = []

@app.route('/')
def index():
    current_dir = os.path.dirname(os.path.abspath(__file__))
    frontend_dir = os.path.join(current_dir, '..', 'frontend')
    return send_from_directory(frontend_dir, 'index.html')

@app.route('/index.html')
def index_html():
    current_dir = os.path.dirname(os.path.abspath(__file__))
    frontend_dir = os.path.join(current_dir, '..', 'frontend')
    return send_from_directory(frontend_dir, 'index.html')

@app.route('/api/dashboard')
def get_dashboard_data():
    """Get dashboard overview data"""
    total_users = len(set(entry.get('user', 'unknown') for entry in activity_data))
    total_alerts = len(alerts)
    active_sessions = len([entry for entry in time_entries if entry.get('active', False)])

    # Calculate productivity metrics
    productivity_data = calculate_productivity_metrics()

    return jsonify({
        'total_users': total_users,
        'total_alerts': total_alerts,
        'active_sessions': active_sessions,
        'productivity_score': productivity_data.get('average_score', 0),
        'risk_score': calculate_overall_risk_score()
    })

@app.route('/api/activities')
def get_activities():
    """Get recent activities"""
    limit = int(request.args.get('limit', 100))
    user = request.args.get('user')

    filtered_activities = activity_data
    if user:
        filtered_activities = [a for a in activity_data if a.get('user') == user]

    return jsonify(filtered_activities[-limit:])

@app.route('/api/dlp-events')
def get_dlp_events():
    """Get DLP events"""
    limit = int(request.args.get('limit', 50))
    return jsonify(dlp_events[-limit:])

@app.route('/api/productivity')
def get_productivity_data():
    """Get productivity analytics"""
    user = request.args.get('user')
    days = int(request.args.get('days', 7))

    end_date = datetime.now()
    start_date = end_date - timedelta(days=days)

    filtered_entries = []
    for entry in time_entries:
        try:
            # Try different timestamp keys (start_time for regular entries, timestamp for productivity)
            timestamp_str = entry.get('start_time') or entry.get('timestamp')
            if not timestamp_str:
                continue

            timestamp_str = str(timestamp_str)

            # Handle both ISO format and Unix timestamp
            if timestamp_str.replace('.', '').isdigit():
                # Unix timestamp
                entry_time = datetime.fromtimestamp(float(timestamp_str))
            else:
                # ISO format
                entry_time = datetime.fromisoformat(timestamp_str.replace('Z', '+00:00'))

            # Make both datetimes naive for comparison
            if entry_time.tzinfo is not None:
                entry_time = entry_time.replace(tzinfo=None)
            if start_date <= entry_time <= end_date:
                filtered_entries.append(entry)
        except (ValueError, KeyError, TypeError, OSError, AttributeError):
            # Skip entries with invalid timestamps
            continue

    if user:
        filtered_entries = [e for e in filtered_entries if e.get('user') == user]

    return jsonify(calculate_productivity_metrics(filtered_entries))

@app.route('/api/risk-analysis')
def get_risk_analysis():
    """Get risk analysis data"""
    user = request.args.get('user')

    patterns = behavior_patterns
    if user:
        patterns = [p for p in patterns if p.get('user') == user]

    risk_distribution = {
        'low': len([p for p in patterns if p.get('confidence_score', 0) < 0.3]),
        'medium': len([p for p in patterns if 0.3 <= p.get('confidence_score', 0) < 0.7]),
        'high': len([p for p in patterns if p.get('confidence_score', 0) >= 0.7])
    }

    return jsonify({
        'risk_distribution': risk_distribution,
        'recent_anomalies': patterns[-10:],
        'overall_risk_score': calculate_overall_risk_score()
    })

@app.route('/api/application-usage')
def get_application_usage():
    """Get application usage statistics"""
    user = request.args.get('user')
    limit = int(request.args.get('limit', 10))

    # Aggregate application usage from time_entries
    app_usage = {}
    for entry in time_entries:
        if entry.get('type') == 'time':  # Only count actual time tracking entries
            app_name = entry.get('application', 'Unknown')
            duration = entry.get('duration', 0)

            if user and entry.get('user') != user:
                continue

            if app_name not in app_usage:
                app_usage[app_name] = 0
            app_usage[app_name] += duration

    # Convert to sorted list
    usage_list = []
    for app, total_time in app_usage.items():
        usage_list.append({
            'application': app,
            'total_time_seconds': total_time,
            'total_time_formatted': format_duration(total_time),
            'is_productive': is_productive_app(app)
        })

    # Sort by total time (descending) and limit results
    usage_list.sort(key=lambda x: x['total_time_seconds'], reverse=True)
    usage_list = usage_list[:limit]

    return jsonify({
        'application_usage': usage_list,
        'total_applications': len(usage_list)
    })

@app.route('/api/activity-trends')
def get_activity_trends():
    """Get activity trends data for the last 7 days"""
    user = request.args.get('user')
    days = int(request.args.get('days', 7))

    # Get data for the last N days
    end_date = datetime.now()
    start_date = end_date - timedelta(days=days)

    # Filter activities by date and user
    filtered_activities = []
    for activity in activity_data:
        try:
            activity_time = datetime.fromisoformat(activity.get('timestamp', ''))
            if activity_time.tzinfo is not None:
                activity_time = activity_time.replace(tzinfo=None)
            if start_date <= activity_time <= end_date:
                if not user or activity.get('user') == user:
                    filtered_activities.append(activity)
        except (ValueError, KeyError, TypeError):
            continue

    # Aggregate activities by day
    daily_counts = {}
    for i in range(days):
        date = (start_date + timedelta(days=i)).date()
        daily_counts[date] = 0

    for activity in filtered_activities:
        try:
            activity_time = datetime.fromisoformat(activity.get('timestamp', ''))
            if activity_time.tzinfo is not None:
                activity_time = activity_time.replace(tzinfo=None)
            date = activity_time.date()
            if date in daily_counts:
                daily_counts[date] += 1
        except (ValueError, KeyError, TypeError):
            continue

    # Convert to list format for frontend (last 7 days)
    trend_data = []
    labels = []
    for i in range(days):
        date = (start_date + timedelta(days=i)).date()
        count = daily_counts.get(date, 0)
        trend_data.append(count)
        labels.append(date.strftime('%a'))  # Mon, Tue, etc.

    return jsonify({
        'labels': labels,
        'data': trend_data,
        'total_activities': len(filtered_activities)
    })

@app.route('/api/alerts')
def get_alerts():
    """Get recent alerts"""
    limit = int(request.args.get('limit', 50))
    user = request.args.get('user')

    filtered_alerts = alerts
    if user:
        filtered_alerts = [a for a in alerts if a.get('user') == user]

    # Return most recent alerts
    return jsonify(filtered_alerts[-limit:])

@app.route('/api/behavior-patterns')
def get_behavior_patterns():
    """Get behavior patterns data"""
    limit = int(request.args.get('limit', 50))
    user = request.args.get('user')
    pattern_type = request.args.get('pattern_type')

    filtered_patterns = behavior_patterns
    if user:
        filtered_patterns = [p for p in filtered_patterns if p.get('user') == user]
    if pattern_type:
        filtered_patterns = [p for p in filtered_patterns if p.get('pattern_type') == pattern_type]

    # Return most recent patterns
    return jsonify({
        'patterns': filtered_patterns[-limit:],
        'total_count': len(filtered_patterns),
        'filtered_count': len(filtered_patterns[-limit:])
    })

@app.route('/latest')
def get_latest_version():
    """Get latest version information for upgrade manager"""
    return jsonify({
        'major': 1,
        'minor': 0,
        'patch': 0,
        'build': 'stable',
        'release_date': datetime.now().isoformat(),
        'download_url': 'https://example.com/download/workforce-agent-1.0.0.tar.gz',
        'checksum': 'dummy_checksum_for_demo',
        'release_notes': 'Latest stable release of Workforce Monitoring Agent',
        'file_size': 1024000,
        'signature': 'dummy_signature_for_demo'
    })

@socketio.on('connect')
def handle_connect():
    print('Client connected')
    emit('status', {'message': 'Connected to Workforce Monitoring Server'})

@socketio.on('disconnect')
def handle_disconnect():
    print('Client disconnected')

@app.route('/agent_data', methods=['POST'])
def handle_agent_data_http():
    """Handle HTTP POST data from monitoring agent"""
    try:
        # Check if request has JSON content type
        if not request.is_json:
            return jsonify({'error': 'Content-Type must be application/json'}), 400

        # Check if request has data
        if not request.data:
            return jsonify({'error': 'Request body is empty'}), 400

        data = request.get_json()
        if not data:
            return jsonify({'error': 'Invalid JSON data provided'}), 400

        return process_agent_data(data)
    except BadRequest as e:
        print(f"Bad request error: {e}")
        return jsonify({'error': 'Invalid JSON format in request body'}), 400
    except Exception as e:
        print(f"Error processing agent data: {e}")
        return jsonify({'error': f'Failed to process request: {str(e)}'}), 500

@socketio.on('agent_data')
def handle_agent_data_socketio(data):
    """Handle SocketIO data from monitoring agent"""
    try:
        return process_agent_data(data)
    except Exception as e:
        print(f"Error processing agent data: {e}")
        return {'error': 'Internal server error'}, 500

def process_agent_data(data):
    """Process agent data (shared between HTTP and SocketIO)"""
    data_type = data.get('type')

    if data_type == 'activity':
        activity_data.append(data)

    elif data_type == 'dlp':
        dlp_events.append(data)
        # Check if this should trigger an alert
        if data.get('blocked', False):
            create_alert('DLP Violation', f"Blocked: {data.get('policy_violated', 'Unknown')}", 'high')

    elif data_type == 'time':
        time_entries.append(data)

    elif data_type == 'anomaly':
        behavior_patterns.append(data)
        create_alert('Behavior Anomaly', data.get('description', 'Anomalous behavior detected'), 'medium')

    elif data_type == 'productivity':
        # Handle productivity metrics data
        productivity_data = {
            'user': data.get('user', 'unknown'),
            'productivity_score': data.get('productivity_score', 0),
            'productive_time': data.get('productive_time', 0),
            'total_time': data.get('total_time', 0),
            'timestamp': data.get('timestamp', datetime.now().isoformat())
        }
        # Store in time_entries for now, or we could create a separate list
        time_entries.append(productivity_data)

    elif data_type == 'app_usage':
        # Handle application usage data from agent
        app_usage_data = {
            'type': 'app_usage',
            'user': data.get('user', 'unknown'),
            'timestamp': data.get('timestamp', datetime.now().isoformat()),
            'session_duration_hours': data.get('session_duration_hours', 0),
            'productive_time_hours': data.get('productive_time_hours', 0),
            'productivity_score': data.get('productivity_score', 0),
            'application_usage': data.get('application_usage', [])
        }
        # Store application usage data
        time_entries.append(app_usage_data)

    elif data_type == 'alert':
        # Handle alert data from agent
        alert_data = {
            'type': 'alert',
            'alert_type': data.get('alert_type', 'unknown'),
            'title': data.get('title', 'Alert'),
            'description': data.get('description', ''),
            'severity': data.get('severity', 'low'),
            'user': data.get('user', 'unknown'),
            'timestamp': data.get('timestamp', datetime.now().isoformat()),
            'acknowledged': False
        }
        # Store alert data and emit real-time alert
        alerts.append(alert_data)
        print(f"Alert processed and stored: {alert_data}")
        socketio.emit('alert', alert_data)
        print(f"Alert emitted via Socket.IO: {alert_data}")

    elif data_type == 'behavior_patterns':
        # Handle behavior patterns data from agent
        patterns_data = {
            'type': 'behavior_patterns',
            'batch_timestamp': data.get('batch_timestamp', datetime.now().isoformat()),
            'user': data.get('user', 'unknown'),
            'patterns': data.get('patterns', []),
            'pattern_count': data.get('pattern_count', 0)
        }

        # Process each pattern in the batch
        for pattern in patterns_data['patterns']:
            pattern_entry = {
                'user': pattern.get('user', 'unknown'),
                'pattern_type': pattern.get('pattern_type', 'unknown'),
                'description': pattern.get('description', ''),
                'confidence_score': pattern.get('confidence_score', 0.0),
                'timestamp': pattern.get('timestamp', datetime.now().isoformat()),
                'batch_processed': True
            }
            behavior_patterns.append(pattern_entry)

        print(f"Behavior patterns batch processed: {patterns_data['pattern_count']} patterns for user {patterns_data['user']}")

        # Emit real-time update for behavior patterns
        socketio.emit('behavior_patterns_update', {
            'user': patterns_data['user'],
            'pattern_count': patterns_data['pattern_count'],
            'batch_timestamp': patterns_data['batch_timestamp']
        })

    # Keep data size manageable
    if len(activity_data) > 10000:
        activity_data.pop(0)
    if len(dlp_events) > 1000:
        dlp_events.pop(0)
    if len(time_entries) > 5000:
        time_entries.pop(0)
    if len(behavior_patterns) > 2000:
        behavior_patterns.pop(0)

    # Emit real-time updates
    socketio.emit('data_update', {'type': data_type, 'data': data})

    return jsonify({'status': 'success', 'message': 'Data processed successfully'})



def calculate_productivity_metrics(entries=None):
    """Calculate productivity metrics"""
    if entries is None:
        entries = time_entries

    if not entries:
        return {'average_score': 0, 'total_productive_time': 0, 'total_time': 0}

    total_time = 0
    productive_time = 0

    for entry in entries:
        # Handle different entry types
        if entry.get('type') == 'time':
            # Regular time tracking entry
            duration = entry.get('duration', 0)
            total_time += duration
            if is_productive_app(entry.get('application', '')):
                productive_time += duration
        elif entry.get('type') == 'productivity':
            # Direct productivity data from agent
            total_time += entry.get('total_time', 0)
            productive_time += entry.get('productive_time', 0)
        elif entry.get('type') == 'app_usage':
            # Application usage data
            total_time += entry.get('session_duration_hours', 0) * 3600  # Convert hours to seconds
            productive_time += entry.get('productive_time_hours', 0) * 3600

    productivity_score = productive_time / total_time if total_time > 0 else 0

    return {
        'average_score': productivity_score,
        'total_productive_time': productive_time,
        'total_time': total_time,
        'entries_count': len(entries)
    }

def format_duration(seconds):
    """Format duration in seconds to human readable format"""
    if seconds < 60:
        return f"{int(seconds)}s"
    elif seconds < 3600:
        minutes = int(seconds // 60)
        remaining_seconds = int(seconds % 60)
        return f"{minutes}m {remaining_seconds}s"
    else:
        hours = int(seconds // 3600)
        minutes = int((seconds % 3600) // 60)
        return f"{hours}h {minutes}m"

def is_productive_app(app_name):
    """Determine if an application is productive"""
    productive_apps = [
        'code', 'vscode', 'sublime', 'vim', 'emacs',
        'chrome', 'firefox', 'edge',
        'libreoffice', 'soffice', 'excel', 'word'
    ]

    unproductive_apps = [
        'facebook', 'twitter', 'instagram', 'youtube',
        'netflix', 'spotify', 'games'
    ]

    app_lower = app_name.lower()

    for productive in productive_apps:
        if productive in app_lower:
            return True

    for unproductive in unproductive_apps:
        if unproductive in app_lower:
            return False

    return False  # Default to nonproductive for unknown apps

def calculate_overall_risk_score():
    """Calculate overall system risk score"""
    if not behavior_patterns:
        # If no behavior patterns, calculate risk based on alerts and other factors
        if not alerts:
            return 0.0

        # Calculate risk based on recent alerts (last 100)
        recent_alerts = alerts[-100:]
        high_alerts = sum(1 for a in recent_alerts if a.get('severity') == 'high')
        medium_alerts = sum(1 for a in recent_alerts if a.get('severity') == 'medium')

        # Base risk on alert frequency and severity
        alert_risk = (high_alerts * 0.6 + medium_alerts * 0.3) / len(recent_alerts) if recent_alerts else 0.0

        # Add some baseline risk if there are many alerts
        baseline_risk = min(len(alerts) / 1000.0, 0.2)  # Cap at 20% for too many alerts

        return min(alert_risk + baseline_risk, 1.0)

    recent_patterns = behavior_patterns[-100:]  # Last 100 patterns
    high_risk_count = sum(1 for p in recent_patterns if p.get('confidence_score', 0) > 0.7)
    medium_risk_count = sum(1 for p in recent_patterns if 0.4 <= p.get('confidence_score', 0) <= 0.7)

    risk_score = (high_risk_count * 0.8 + medium_risk_count * 0.4) / len(recent_patterns)
    return min(risk_score, 1.0)

def create_alert(title, description, severity):
    """Create and store an alert"""
    alert = {
        'id': len(alerts) + 1,
        'title': title,
        'description': description,
        'severity': severity,
        'timestamp': datetime.now().isoformat(),
        'acknowledged': False
    }

    alerts.append(alert)

    # Emit real-time alert
    socketio.emit('alert', alert)

def background_tasks():
    """Background tasks for periodic cleanup"""
    while True:
        # Clean up old data
        cleanup_old_data()

        time.sleep(300)  # Run every 5 minutes



def cleanup_old_data():
    """Clean up old data to prevent memory issues"""
    cutoff_date = datetime.now() - timedelta(days=30)

    global activity_data, dlp_events, time_entries, behavior_patterns, alerts

    def is_recent(entry, timestamp_key):
        try:
            entry_time = datetime.fromisoformat(entry.get(timestamp_key, datetime.now().isoformat()))
            if entry_time.tzinfo is not None:
                entry_time = entry_time.replace(tzinfo=None)
            return entry_time > cutoff_date
        except (ValueError, KeyError):
            return False

    activity_data[:] = [a for a in activity_data if is_recent(a, 'timestamp')]
    dlp_events[:] = [e for e in dlp_events if is_recent(e, 'timestamp')]
    time_entries[:] = [e for e in time_entries if is_recent(e, 'start_time') or is_recent(e, 'timestamp')]
    behavior_patterns[:] = [p for p in behavior_patterns if is_recent(p, 'timestamp')]

if __name__ == '__main__':
    # Start background tasks
    threading.Thread(target=background_tasks, daemon=True).start()

    # Start server
    socketio.run(app, host='0.0.0.0', port=5000, debug=True, allow_unsafe_werkzeug=True)
