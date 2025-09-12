#!/usr/bin/env python3
import requests
import json
import time
from datetime import datetime

# Test activity data to send to backend
test_activities = [
    {
        "type": "activity",
        "timestamp": datetime.now().isoformat(),
        "activity_type": "file_access",
        "details": "Opened document.docx",
        "user": "test_user"
    },
    {
        "type": "activity",
        "timestamp": datetime.now().isoformat(),
        "activity_type": "application_start",
        "details": "Started Microsoft Word",
        "user": "test_user"
    },
    {
        "type": "activity",
        "timestamp": datetime.now().isoformat(),
        "activity_type": "web_access",
        "details": "Visited https://example.com",
        "user": "test_user"
    },
    {
        "type": "activity",
        "timestamp": datetime.now().isoformat(),
        "activity_type": "file_save",
        "details": "Saved report.pdf",
        "user": "test_user"
    },
    {
        "type": "activity",
        "timestamp": datetime.now().isoformat(),
        "activity_type": "keyboard_activity",
        "details": "High typing activity detected",
        "user": "test_user"
    }
]

def send_test_activities():
    backend_url = "http://localhost:5000/agent_data"

    print("Sending test activity data to backend...")

    for i, activity in enumerate(test_activities):
        try:
            response = requests.post(backend_url, json=activity, headers={'Content-Type': 'application/json'})
            if response.status_code == 200:
                print(f"✓ Sent activity {i+1}/{len(test_activities)}: {activity['activity_type']}")
            else:
                print(f"✗ Failed to send activity {i+1}: {response.status_code}")
        except Exception as e:
            print(f"✗ Error sending activity {i+1}: {e}")

        time.sleep(0.5)  # Small delay between requests

    print("\nTest activities sent successfully!")
    print("Check the Recent Activities panel in the UI to see if activities are now showing.")

if __name__ == "__main__":
    send_test_activities()
