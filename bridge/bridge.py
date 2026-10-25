#!/usr/bin/env python3
"""
huEVO Xbox 360 Companion Bridge (Universal Autonomous Edition)
Connects any Xbox 360 directly to live https://huevo.su over the internet or local network.
- Fully standalone: zero external dependencies (pure Python 3 standard library).
- Automatic anti-bot challenge solving (HTTP 418 hchk bypass).
- UDP Auto-Discovery: Xbox 360 finds this bridge automatically on Wi-Fi/LAN.
- Live Account Login: login directly from Xbox 360 gamepad keyboard.
- Works on Windows, macOS, Linux, and Cloud VPS.
"""

import os
import sys
import re
import json
import time
import socket
import threading
import urllib.request
import urllib.parse
import urllib.error
import http.cookiejar
from http.server import HTTPServer, BaseHTTPRequestHandler

PORT = 8080
DISCOVERY_PORT = 8081
TARGET_HOST = "https://huevo.su"
USER_AGENT = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36"

# Persistent Cookie Jar
cj = http.cookiejar.CookieJar()
opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(cj))

s_hchk_token = ""
s_current_user = ""
g_cache = {}
CACHE_TTL = 3.0 # seconds

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

def request_huevo(path: str, method: str = "GET", post_data: bytes = None, extra_headers: dict = None) -> str:
    global s_hchk_token
    url = TARGET_HOST + path
    headers = {
        "User-Agent": USER_AGENT,
        "Accept": "application/json, text/html, */*",
        "Origin": TARGET_HOST,
        "Referer": f"{TARGET_HOST}/news"
    }
    if extra_headers:
        headers.update(extra_headers)

    if s_hchk_token:
        # Ensure hchk cookie is in cookiejar
        ck = http.cookiejar.Cookie(
            version=0, name="hchk", value=s_hchk_token, port=None, port_specified=False,
            domain="huevo.su", domain_specified=True, domain_initial_dot=False,
            path="/", path_specified=True, secure=True, expires=None,
            discard=True, comment=None, comment_url=None, rest={}, rfc2109=False
        )
        cj.set_cookie(ck)

    req = urllib.request.Request(url, data=post_data, headers=headers, method=method)
    try:
        resp = opener.open(req, timeout=8)
        return resp.read().decode("utf-8", "ignore")
    except urllib.error.HTTPError as e:
        if e.code == 418:
            html = e.read().decode("utf-8", "ignore")
            token = solve_hchk(html)
            if token:
                print(f"[+] Solved anti-bot hchk challenge: {token}")
                s_hchk_token = token
                return request_huevo(path, method, post_data, extra_headers)
        return e.read().decode("utf-8", "ignore")
    except Exception as e:
        print(f"[-] Error querying {url}: {e}")
        return ""

# Try loading saved session cookie
SESSION_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "session.json")
def load_saved_session():
    global s_current_user
    if os.path.exists(SESSION_FILE):
        try:
            with open(SESSION_FILE, "r") as f:
                d = json.load(f)
                cookie_val = d.get("session", "")
                s_current_user = d.get("username", "")
                if cookie_val:
                    ck = http.cookiejar.Cookie(
                        version=0, name="session", value=cookie_val, port=None, port_specified=False,
                        domain="huevo.su", domain_specified=True, domain_initial_dot=False,
                        path="/", path_specified=True, secure=True, expires=None,
                        discard=True, comment=None, comment_url=None, rest={}, rfc2109=False
                    )
                    cj.set_cookie(ck)
                    print(f"[+] Loaded saved session for @{s_current_user}")
        except Exception as e:
            print("[-] Could not load session.json:", e)

load_saved_session()

def save_session(username: str, cookie_val: str):
    try:
        with open(SESSION_FILE, "w") as f:
            json.dump({"username": username, "session": cookie_val}, f)
        print(f"[+] Saved session for @{username} to session.json")
    except Exception as e:
        print("[-] Could not save session.json:", e)

def get_cached_or_fetch(key: str, fetch_fn):
    now = time.time()
    if key in g_cache:
        entry_time, data = g_cache[key]
        if now - entry_time < CACHE_TTL:
            return data
    data = fetch_fn()
    if data:
        g_cache[key] = (now, data)
    elif key in g_cache:
        return g_cache[key][1]
    return data

# --- Data Parsers ---

def fetch_profile():
    raw_unread = request_huevo("/api/unread_counts")
    user_info = {
        "id": s_current_user if s_current_user else "guest",
        "displayName": s_current_user if s_current_user else "Гость huEVO",
        "handle": "@" + (s_current_user if s_current_user else "guest"),
        "extraHandles": "",
        "statusText": "Xbox 360 Edition",
        "city": "huEVO",
        "birthday": "14 января 2002 г. (24 года)",
        "workplace": "huEVO SWAGdev",
        "reputation": "14",
        "coins": 301680,
        "friendsCount": 0,
        "followersCount": 1,
        "groupsCount": 21,
        "photosCount": 19,
        "audioCount": 85,
        "videoCount": 6,
        "isOnline": True,
        "posts": []
    }
    try:
        d = json.loads(raw_unread)
        if "username" in d and d["username"]:
            user_info["id"] = d["username"]
            user_info["displayName"] = d["username"]
            user_info["handle"] = "@" + d["username"]
        if "friends" in d:
            user_info["friendsCount"] = d["friends"]
    except Exception:
        pass

    target_profile = user_info["id"] if user_info["id"] != "guest" else "telega"
    raw_wall = request_huevo(f"/{target_profile}")
    if raw_wall:
        name_m = re.search(r"class=[\"']mh-user-name[\"']>([^<]+)<", raw_wall)
        if name_m: user_info["displayName"] = name_m.group(1).strip()
        status_m = re.search(r"class=[\"']vk-status-text[\"']>([^<]+)<", raw_wall)
        if status_m: user_info["statusText"] = status_m.group(1).strip()

        chunks = re.split(r"(?=<div class=[\"']vk-post-head[\"']>)", raw_wall)
        posts = []
        for c in chunks[1:8]:
            author_m = re.search(r"class=[\"']vk-name[\"'][^>]*>([^<]+)<", c)
            date_m = re.search(r"class=[\"']vk-date[\"'][^>]*>([^<]+)<", c)
            body_m = re.search(r"<div class=[\"']vk-post-body[\"']>(.*?)</div>", c, re.DOTALL)
            likes_m = re.search(r"class=[\"'][^\"']*like[^\"']*[\"'][^>]*>.*?(\d+)", c, re.DOTALL)
            comm_m = re.search(r"class=[\"'][^\"']*comment[^\"']*[\"'][^>]*>.*?(\d+)", c, re.DOTALL)
            pid_m = re.search(r"/post/(\d+)", c)

            if body_m:
                text = re.sub(r"<[^>]+>", " ", body_m.group(1)).strip()
                text = " ".join(text.split())
                posts.append({
                    "id": pid_m.group(1) if pid_m else str(len(posts)+1),
                    "authorName": author_m.group(1).strip() if author_m else user_info["displayName"],
                    "authorHandle": user_info["handle"],
                    "timeAgo": date_m.group(1).strip() if date_m else "недавно",
                    "content": text,
                    "likesCount": int(likes_m.group(1)) if likes_m else 0,
                    "commentsCount": int(comm_m.group(1)) if comm_m else 0,
                    "isLiked": False
                })
        if posts:
            user_info["posts"] = posts
    return user_info

def fetch_feed():
    raw_news = request_huevo("/news")
    if not raw_news:
        return []
    chunks = re.split(r"(?=<div class=[\"']vk-post-head[\"']>)", raw_news)
    feed = []
    for c in chunks[1:15]:
        author_m = re.search(r"class=[\"']vk-name[\"'][^>]*>([^<]+)<", c)
        handle_m = re.search(r"href=[\"']/([^\"'/]+)[\"']\s+class=[\"']vk-name[\"']", c)
        date_m = re.search(r"class=[\"']vk-date[\"'][^>]*>([^<]+)<", c)
        body_m = re.search(r"<div class=[\"']vk-post-body[\"']>(.*?)</div>", c, re.DOTALL)
        likes_m = re.search(r"class=[\"'][^\"']*like[^\"']*[\"'][^>]*>.*?(\d+)", c, re.DOTALL)
        comm_m = re.search(r"class=[\"'][^\"']*comment[^\"']*[\"'][^>]*>.*?(\d+)", c, re.DOTALL)
        pid_m = re.search(r"/post/(\d+)", c)

        if body_m:
            text = re.sub(r"<[^>]+>", " ", body_m.group(1)).strip()
            text = " ".join(text.split())
            feed.append({
                "id": pid_m.group(1) if pid_m else str(len(feed)+100),
                "authorName": author_m.group(1).strip() if author_m else "huEVO",
                "authorHandle": "@" + handle_m.group(1) if handle_m else "@user",
                "timeAgo": date_m.group(1).strip() if date_m else "недавно",
                "content": text,
                "likesCount": int(likes_m.group(1)) if likes_m else 0,
                "commentsCount": int(comm_m.group(1)) if comm_m else 0,
                "isLiked": False
            })
    return feed

def fetch_dialogs():
    raw_dlgs = request_huevo("/api/dialogs?chats=1")
    if not raw_dlgs:
        return []
    try:
        data = json.loads(raw_dlgs)
    except Exception:
        return []

    dialog_list = []
    for d in data.get("dialogs", []):
        dialog_list.append({
            "id": d.get("partner", ""),
            "peerName": d.get("display_name", d.get("partner", "Чат")),
            "lastSender": d.get("last_sender", ""),
            "lastMessage": re.sub(r"<[^>]+>", " ", d.get("preview", "")).strip(),
            "timestamp": d.get("last_short", ""),
            "isGroup": False,
            "membersCount": 2,
            "unreadCount": d.get("unread_count", 0),
            "isOnline": d.get("is_online", False)
        })

    for c in data.get("chats", []):
        dialog_list.append({
            "id": f"chat_{c.get('id', '')}",
            "peerName": c.get("name", "Беседа"),
            "lastSender": c.get("last_sender", ""),
            "lastMessage": c.get("last_message", ""),
            "timestamp": c.get("last_short", ""),
            "isGroup": True,
            "membersCount": c.get("members_count", 0),
            "unreadCount": c.get("unread_count", 0),
            "isOnline": False
        })
    return dialog_list

def fetch_messages(partner: str):
    if partner.startswith("chat_"):
        chat_id = partner[5:]
        raw = request_huevo(f"/api/group_messages/{chat_id}")
    else:
        raw = request_huevo(f"/api/messages/{partner}")
    if not raw:
        return []
    try:
        data = json.loads(raw)
    except Exception:
        return []

    msgs = []
    for m in data.get("messages", []):
        text = m.get("text", "")
        if not text and m.get("audio_title"):
            text = f"♫ {m.get('audio_artist', '')} - {m.get('audio_title', '')}"
        msgs.append({
            "id": str(m.get("id", len(msgs)+1)),
            "senderName": m.get("sender", "друг"),
            "text": text,
            "timestamp": m.get("time", ""),
            "isOutgoing": (m.get("sender") == s_current_user)
        })
    return msgs

def fetch_friends():
    raw_friends = request_huevo("/friends")
    if not raw_friends:
        return []
    links = re.findall(r"<a[^>]+class=[\"']vk-name[\"'][^>]*href=[\"']/([^\"'/]+)[\"'][^>]*>([^<]+)</a>", raw_friends)
    friends = []
    seen = set()
    for handle, name in links:
        if handle in seen: continue
        seen.add(handle)
        friends.append({
            "id": handle,
            "name": name.strip(),
            "handle": "@" + handle,
            "status": "huEVO friend",
            "isOnline": False,
            "hasAvatar": True
        })
    return friends

def fetch_audio():
    target = s_current_user if s_current_user else "telega"
    raw_audio = request_huevo(f"/zvuchok/{target}")
    if not raw_audio:
        return []
    blocks = re.findall(r"(<div class=[\"']zv-tcard[\"'][^>]*>.*?)(?=<div class=[\"']zv-tcard[\"']|\Z)", raw_audio, re.DOTALL)
    tracks = []
    for b in blocks:
        url_m = re.search(r"data-audio-url=[\"']([^\"']+)[\"']", b)
        artist_m = re.search(r"class=[\"']zv-tcard-artist[\"'][^>]*>.*?<a[^>]*>([^<]+)</a>", b, re.DOTALL)
        title_m = re.search(r"class=[\"']zv-tcard-title[\"'][^>]*>([^<]+)</a>", b)
        tid_m = re.search(r"/zvuchok/track/(\d+)", b)

        if title_m:
            tracks.append({
                "id": tid_m.group(1) if tid_m else str(len(tracks)+1),
                "title": title_m.group(1).strip(),
                "artist": artist_m.group(1).strip() if artist_m else "Неизвестен",
                "duration": "2:45",
                "url": (TARGET_HOST + url_m.group(1)) if url_m else ""
            })
    return tracks

def fetch_market():
    raw_market = request_huevo("/market")
    if not raw_market:
        return []
    rows = re.findall(r"<tr[^>]*>.*?<td class=[\"']mk-c-name[\"']>.*?<a[^>]+>([^<]+)</a>.*?<span class=[\"']mk-price[\"']>.*?(\d+)\s+монэток.*?<a[^>]+class=[\"']mk-user[\"']>([^<]+)</a>", raw_market, re.DOTALL)
    lots = []
    for uname, price, seller in rows:
        lots.append({
            "username": uname.strip(),
            "length": len(uname.replace("@", "")),
            "dateAdded": "сегодня",
            "priceCoins": int(price),
            "sellerName": seller.strip(),
            "isAvailable": True
        })
    return lots

def do_login(username, password):
    global s_current_user
    post_data = urllib.parse.urlencode({
        "username": username,
        "password": password,
        "remember": "1"
    }).encode("utf-8")
    resp = request_huevo("/login", method="POST", post_data=post_data, extra_headers={"Referer": f"{TARGET_HOST}/login"})
    session_val = ""
    for c in cj:
        if c.name == "session":
            session_val = c.value
            break
    if session_val:
        s_current_user = username
        save_session(username, session_val)
        return True, "Авторизация успешна!"
    return False, "Неверный логин или пароль"

# --- HTTP Request Handler for Xbox 360 ---

class BridgeHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        print(f"[*] Xbox 360 -> {self.command} {self.path}")

    def send_json(self, data, code=200):
        body = json.dumps(data, ensure_ascii=False).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        parsed = self.path.split("?")
        path = parsed[0]
        query = parsed[1] if len(parsed) > 1 else ""

        if path == "/api/profile":
            data = get_cached_or_fetch("profile", fetch_profile)
            self.send_json(data)
        elif path == "/api/feed":
            data = get_cached_or_fetch("feed", fetch_feed)
            self.send_json(data)
        elif path == "/api/dialogs":
            data = get_cached_or_fetch("dialogs", fetch_dialogs)
            self.send_json(data)
        elif path == "/api/messages":
            partner = "pedro69"
            if "partner=" in query:
                for q in query.split("&"):
                    if q.startswith("partner="):
                        partner = q[8:]
            data = get_cached_or_fetch(f"messages_{partner}", lambda: fetch_messages(partner))
            self.send_json(data)
        elif path == "/api/friends":
            data = get_cached_or_fetch("friends", fetch_friends)
            self.send_json(data)
        elif path == "/api/audio":
            data = get_cached_or_fetch("audio", fetch_audio)
            self.send_json(data)
        elif path == "/api/market":
            data = get_cached_or_fetch("market", fetch_market)
            self.send_json(data)
        elif path == "/api/status":
            self.send_json({"status": "online", "user": s_current_user, "target": TARGET_HOST})
        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length).decode("utf-8", "ignore") if length > 0 else ""
        parsed = urllib.parse.parse_qs(body)

        if self.path == "/api/login":
            u = parsed.get("username", [""])[0]
            p = parsed.get("password", [""])[0]
            ok, msg = do_login(u, p)
            self.send_json({"ok": ok, "message": msg, "username": u})
        elif self.path == "/api/post":
            text = parsed.get("text", [""])[0]
            post_data = urllib.parse.urlencode({"body": text, "text": text}).encode()
            request_huevo("/wall/add/", method="POST", post_data=post_data)
            self.send_json({"ok": True, "message": "Опубликовано"})
        elif self.path == "/api/send_message":
            dialog = parsed.get("dialog", [""])[0]
            text = parsed.get("text", [""])[0]
            post_data = urllib.parse.urlencode({"body": text, "message": text}).encode()
            request_huevo(f"/api/message/{dialog}", method="POST", post_data=post_data)
            self.send_json({"ok": True, "message": "Отправлено"})
        elif self.path == "/api/like":
            pid = parsed.get("id", [""])[0]
            request_huevo(f"/post/like/{pid}", method="POST")
            self.send_json({"ok": True, "message": "Лайк обновлен"})
        else:
            self.send_json({"ok": True})

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

def udp_discovery_worker():
    """Listens for Xbox 360 probe packets and responds with this bridge's IP."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        sock.bind(('0.0.0.0', DISCOVERY_PORT))
        print(f"[+] UDP Auto-Discovery активен на порту {DISCOVERY_PORT}")
        while True:
            data, addr = sock.recvfrom(1024)
            if b"HUEVO_DISCOVERY" in data:
                local_ip = get_local_ip()
                reply = f"HUEVO_SERVER:{local_ip}:{PORT}".encode("utf-8")
                sock.sendto(reply, addr)
                print(f"[+] Обнаружен Xbox 360 по адресу {addr[0]} -> отправлен IP {local_ip}:{PORT}")
    except Exception as e:
        print(f"[-] UDP Discovery error: {e}")

def main():
    local_ip = get_local_ip()
    # Start UDP discovery in background thread
    t = threading.Thread(target=udp_discovery_worker, daemon=True)
    t.start()

    server = HTTPServer(('0.0.0.0', PORT), BridgeHandler)
    print("=" * 65)
    print("  huEVO Xbox 360 Companion Bridge (Universal Autonomous Edition)")
    print(f"  Мост запущен на: http://{local_ip}:{PORT}/")
    print(f"  UDP Авто-обнаружение: ВКЛЮЧЕНО (Xbox 360 найдёт мост сам!)")
    print(f"  Вход в аккаунт: доступен прямо с геймпада консоли")
    print(f"  Поддерживает Windows, macOS, Linux и облачные серверы")
    print("=" * 65)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nОстановка моста...")
        server.server_close()

if __name__ == "__main__":
    main()
