#!/usr/bin/env python3
import requests
import json
import time
from datetime import datetime

# Test data to send to backend
test_data = [
    {
        "type": "time",
        "start_time": datetime.now().isoformat(),
        "application": "vscode",
        "duration": 1800,  # 30 minutes
        "user": "test_user",
        "active": False
    },
    {
        "type": "time",
        "start_time": datetime.now().isoformat(),
        "application": "chrome",
        "duration": 1200,  # 20 minutes
        "user": "test_user",
        "active": False
    },
    {
        "type": "time",
        "start_time": datetime.now().isoformat(),
        "application": "facebook",
        "duration": 600,  # 10 minutes
        "user": "test_user",
        "active": False
    },
    {
        "type": "productivity",
        "timestamp": datetime.now().isoformat(),
        "user": "test_user",
        "productivity_score": 0.75,
        "productive_time": 2700,  # 45 minutes
        "total_time": 3600  # 60 minutes
    }
]

def send_test_data():
    backend_url = "http://localhost:5000/agent_data"

    print("Sending test productivity data to backend...")

    for i, data in enumerate(test_data):
        try:
            response = requests.post(backend_url, json=data, headers={'Content-Type': 'application/json'})
            if response.status_code == 200:
                print(f"✓ Sent data {i+1}/{len(test_data)}: {data['type']}")
            else:
                print(f"✗ Failed to send data {i+1}: {response.status_code}")
        except Exception as e:
            print(f"✗ Error sending data {i+1}: {e}")

        time.sleep(0.5)  # Small delay between requests

    print("\nTest data sent successfully!")
    print("Check the UI at http://localhost:8000 to see if productivity is now showing a real value.")

if __name__ == "__main__":
    send_test_data()
