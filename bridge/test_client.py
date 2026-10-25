#!/usr/bin/env python3
"""
Test client to verify bridge connectivity and endpoints.
"""
import urllib.request
import json

def test():
    base = "http://127.0.0.1:8080"
    print(f"Connecting to {base}/api/status...")
    try:
        with urllib.request.urlopen(f"{base}/api/status", timeout=3) as resp:
            data = json.loads(resp.read().decode())
            print("Status response:", json.dumps(data, indent=2, ensure_ascii=False))

        with urllib.request.urlopen(f"{base}/api/feed", timeout=3) as resp:
            data = json.loads(resp.read().decode())
            print(f"Feed fetched: {len(data)} posts")
            for post in data:
                print(f" - [{post['author']}]: {post['content']} ({post['likes']} likes)")
    except Exception as e:
        print(f"Connection test failed: {e}")

if __name__ == "__main__":
    test()
