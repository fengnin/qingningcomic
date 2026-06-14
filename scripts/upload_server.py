#!/usr/bin/env python3
"""
参考图上传服务 - 供即梦AI 4.0使用
接收 multipart/form-data 请求，保存图片到本地，返回公网可访问的 URL。

部署路径: /data/comic/upload_service/upload_server.py
启动命令: python3 upload_server.py &
"""

import os
import uuid
import hmac
import json
from datetime import datetime
from http.server import HTTPServer, BaseHTTPRequestHandler
from email import message_from_bytes
from email.policy import HTTP

# ===== 配置 =====
UPLOAD_DIR = "/data/comic/ref_uploads"
PUBLIC_BASE_URL = "http://101.33.201.174/ref_uploads"
AUTH_TOKEN = "your-secret-token-here"
PORT = 18765
MAX_FILE_SIZE = 10 * 1024 * 1024  # 10MB


def verify_token(authorization_header: str) -> bool:
    if not AUTH_TOKEN:
        return True
    if not authorization_header:
        return False
    parts = authorization_header.strip().split(" ", 1)
    if len(parts) != 2 or parts[0].lower() != "bearer":
        return False
    return hmac.compare_digest(parts[1].strip(), AUTH_TOKEN)


def error_response(handler, code: int, message: str):
    body = json.dumps({"error": message}).encode("utf-8")
    handler.send_response(code)
    handler.send_header("Content-Type", "application/json")
    handler.send_header("Content-Length", str(len(body)))
    handler.end_headers()
    handler.wfile.write(body)


def parse_multipart(content_type: str, body: bytes):
    """解析 multipart/form-data，返回 {field_name: (filename, data)} 字典"""
    # 构造一个符合 email 解析格式的消息头
    msg = message_from_bytes(
        f"Content-Type: {content_type}\r\n\r\n".encode() + body,
        policy=HTTP,
    )
    result = {}
    for part in msg.get_payload():
        if not hasattr(part, 'get_param'):
            continue
        disposition = part.get("Content-Disposition", "")
        name = None
        filename = None
        for item in disposition.split(";"):
            item = item.strip()
            if item.startswith("name="):
                name = item[5:].strip('"')
            elif item.startswith("filename="):
                filename = item[9:].strip('"')
        if name is None:
            continue
        payload = part.get_payload(decode=True)
        result[name] = (filename, payload)
    return result


class UploadHandler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"[{ts}] {fmt % args}")

    def do_POST(self):
        if not self.path.rstrip("/").endswith("/upload") and self.path != "/upload":
            error_response(self, 404, "Not found")
            return

        if not verify_token(self.headers.get("Authorization", "")):
            error_response(self, 401, "Unauthorized")
            return

        content_length = int(self.headers.get("Content-Length", 0))
        if content_length > MAX_FILE_SIZE:
            error_response(self, 413, "File too large")
            return

        content_type = self.headers.get("Content-Type", "")
        if "multipart/form-data" not in content_type:
            error_response(self, 400, "Expected multipart/form-data")
            return

        body = self.rfile.read(content_length)

        try:
            fields = parse_multipart(content_type, body)
        except Exception as e:
            error_response(self, 400, f"Failed to parse multipart: {e}")
            return

        if "file" not in fields:
            error_response(self, 400, "Missing file field")
            return

        filename, file_data = fields["file"]
        if not file_data:
            error_response(self, 400, "Empty file")
            return

        ext = ".png"
        if filename:
            raw_ext = os.path.splitext(filename)[1].lower()
            if raw_ext in (".png", ".jpg", ".jpeg", ".webp"):
                ext = raw_ext

        save_name = f"{uuid.uuid4().hex}{ext}"
        save_path = os.path.join(UPLOAD_DIR, save_name)

        os.makedirs(UPLOAD_DIR, exist_ok=True)
        with open(save_path, "wb") as f:
            f.write(file_data)

        public_url = f"{PUBLIC_BASE_URL.rstrip('/')}/{save_name}"
        self.log_message("Saved %s (%d bytes) -> %s", save_name, len(file_data), public_url)

        body = json.dumps({"url": public_url}).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path.rstrip("/") in ("/upload/health", "/health"):
            body = b'{"status":"ok"}'
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        else:
            error_response(self, 404, "Not found")


if __name__ == "__main__":
    os.makedirs(UPLOAD_DIR, exist_ok=True)
    server = HTTPServer(("127.0.0.1", PORT), UploadHandler)
    print(f"Upload service listening on 127.0.0.1:{PORT}")
    print(f"Upload dir: {UPLOAD_DIR}")
    print(f"Public base URL: {PUBLIC_BASE_URL}")
    server.serve_forever()
