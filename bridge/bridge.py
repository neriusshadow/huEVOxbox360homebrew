#!/usr/bin/env python3
"""
huEVO Xbox 360 Companion Bridge (Standard Library Edition)
Connects Xbox 360 directly to live https://huevo.su with automatic anti-bot challenge solving.
Zero external dependencies required (runs on any Mac/PC with standard Python 3).
"""

import sys
import re
import json
import socket
from http.server import HTTPServer, BaseHTTPRequestHandler
import urllib.request
import urllib.parse
import urllib.error

PORT = 8080
TARGET_HOST = "https://huevo.su"
USER_AGENT = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36"

s_session_cookie = ""
s_hchk_token = ""

def solve_hchk(html_text: str) -> str:
    match_a = re.search(r'data-hchk-a="([^"]+)"', html_text)
    match_b = re.search(r'data-hchk-b="([^"]+)"', html_text)
    if not match_a or not match_b:
        return ""
    a = match_a.group(1)
    b = match_b.group(1)
    if len(a) != len(b):
        return ""
    token = "".join([chr(int(a[i:i+2], 16) ^ int(b[i:i+2], 16)) for i in range(0, len(a), 2)])
    return token

def forward_request(method: str, path: str, body: bytes = None, extra_headers: dict = None) -> tuple:
    global s_hchk_token, s_session_cookie
    url = TARGET_HOST + path

    headers = {
        "User-Agent": USER_AGENT,
        "Accept": "text/html,application/xhtml+xml,application/json,*/*",
        "Accept-Language": "ru-RU,ru;q=0.9,en;q=0.8"
    }
    if extra_headers:
        headers.update(extra_headers)

    cookies = []
    if s_hchk_token:
        cookies.append(f"hchk={s_hchk_token}")
    if s_session_cookie:
        cookies.append(s_session_cookie)
    if cookies:
        headers["Cookie"] = "; ".join(cookies)

    req = urllib.request.Request(url, data=body, headers=headers, method=method)

    try:
        resp = urllib.request.urlopen(req, timeout=10)
        resp_data = resp.read()
        resp_headers = resp.headers
        code = resp.status
    except urllib.error.HTTPError as e:
        code = e.code
        resp_data = e.read()
        resp_headers = e.headers
    except Exception as e:
        print(f"[-] Connection error to huevo.su: {e}")
        return 502, b"Error connecting to huevo.su", {}

    # Check if challenge page returned
    html_sample = resp_data.decode("utf-8", errors="replace")
    if 'data-hchk-a="' in html_sample:
        token = solve_hchk(html_sample)
        if token:
            print(f"[+] Solved huevo.su anti-bot challenge: {token}")
            s_hchk_token = token
            # Retry request with solved cookie
            return forward_request(method, path, body, extra_headers)

    # Save any new cookies
    for h, v in resp_headers.items():
        if h.lower() == "set-cookie":
            if "huevo" in v or "session" in v:
                s_session_cookie = v.split(";")[0]
                print(f"[+] Saved session cookie: {s_session_cookie}")

    return code, resp_data, resp_headers

class BridgeHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        print(f"[*] Xbox 360 -> {self.command} {self.path}")

    def do_GET(self):
        code, data, headers = forward_request("GET", self.path)
        self.send_response(code)
        self.send_header("Content-Type", headers.get("Content-Type", "text/html; charset=utf-8"))
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length) if length > 0 else None
        code, data, headers = forward_request("POST", self.path, body, {
            "Content-Type": self.headers.get("Content-Type", "application/x-www-form-urlencoded")
        })
        self.send_response(code)
        self.send_header("Content-Type", headers.get("Content-Type", "text/html; charset=utf-8"))
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
    except Exception:
        ip = '127.0.0.1'
    finally:
        s.close()
    return ip

def main():
    local_ip = get_local_ip()
    server = HTTPServer(('0.0.0.0', PORT), BridgeHandler)
    print("=" * 65)
    print("  huEVO Xbox 360 Companion Bridge (Standalone Edition)")
    print(f"  Мост запущен на: http://{local_ip}:{PORT}/")
    print(f"  Этот IP ({local_ip}) уже прописан в приложении huEVO!")
    print("  Xbox 360 будет получать реальные страницы https://huevo.su")
    print("=" * 65)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nОстановка моста...")
        server.server_close()

if __name__ == "__main__":
    main()
