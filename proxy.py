#!/usr/bin/env python3
"""
Proxy HTTP simples para conectar Arduino ao servidor remoto
Este script permite que o Arduino acesse o servidor através da VPN do computador
"""

from http.server import HTTPServer, BaseHTTPRequestHandler
import urllib.request
import urllib.error
import json
import sys
import time

# Configurações
LOCAL_PORT = 8080  # Porta onde o Arduino vai conectar
REMOTE_SERVER = "http://150.162.244.124"  # Servidor remoto

class ProxyHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        """Trata requisições GET do Arduino"""
        try:
            # Monta URL completa para o servidor remoto
            remote_url = f"{REMOTE_SERVER}{self.path}"
            print(f"🔄 Proxy GET: {self.path} → {remote_url}")

            # Faz requisição para o servidor remoto
            with urllib.request.urlopen(remote_url, timeout=10) as response:
                # Lê resposta
                data = response.read().decode('utf-8')

                # Envia resposta para o Arduino
                self.send_response(response.getcode())
                self.send_header('Content-Type', response.headers.get('Content-Type', 'application/json'))
                self.send_header('Access-Control-Allow-Origin', '*')
                self.end_headers()
                self.wfile.write(data.encode('utf-8'))

                print(f"✅ Proxy: {response.getcode()} - {len(data)} bytes")

        except urllib.error.HTTPError as e:
            print(f"❌ HTTP Error: {e.code} - {e.reason}")
            self.send_error(e.code, str(e.reason))
        except Exception as e:
            print(f"❌ Proxy Error: {str(e)}")
            self.send_error(500, str(e))

    def do_POST(self):
        """Trata requisições POST do Arduino"""
        try:
            # Lê dados do Arduino
            content_length = int(self.headers.get('Content-Length', 0))
            post_data = self.rfile.read(content_length) if content_length > 0 else b''

            # Monta URL completa para o servidor remoto
            remote_url = f"{REMOTE_SERVER}{self.path}"
            print(f"🔄 Proxy POST: {self.path} → {remote_url}")
            print(f"📤 Dados: {post_data.decode('utf-8', errors='ignore')}")

            # Faz requisição POST para o servidor remoto
            req = urllib.request.Request(
                remote_url,
                data=post_data,
                headers={
                    'Content-Type': self.headers.get('Content-Type', 'application/json'),
                    'User-Agent': 'Arduino-Proxy/1.0'
                },
                method='POST'
            )

            with urllib.request.urlopen(req, timeout=10) as response:
                # Lê resposta
                data = response.read().decode('utf-8')

                # Envia resposta para o Arduino
                self.send_response(response.getcode())
                self.send_header('Content-Type', response.headers.get('Content-Type', 'application/json'))
                self.send_header('Access-Control-Allow-Origin', '*')
                self.end_headers()
                self.wfile.write(data.encode('utf-8'))

                print(f"✅ Proxy POST: {response.getcode()} - {len(data)} bytes")

        except urllib.error.HTTPError as e:
            print(f"❌ HTTP Error: {e.code} - {e.reason}")
            self.send_error(e.code, str(e.reason))
        except Exception as e:
            print(f"❌ Proxy Error: {str(e)}")
            self.send_error(500, str(e))

    def do_OPTIONS(self):
        """Trata preflight CORS"""
        self.send_response(200)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()

    def log_message(self, format, *args):
        """Desabilita logs padrão do servidor"""
        return

def main():
    print("🚀 Iniciando Proxy HTTP para Arduino")
    print(f"📡 Servidor remoto: {REMOTE_SERVER}")
    print(f"🏠 Servidor local: http://localhost:{LOCAL_PORT}")
    print("📱 Arduino deve conectar para: http://192.168.1.XXX:8080")
    print("   (Substitua XXX pelo IP do seu computador)")
    print("\nAguardando conexões...\n")

    try:
        server = HTTPServer(('0.0.0.0', LOCAL_PORT), ProxyHandler)
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n⏹  Proxy interrompido pelo usuário")
    except Exception as e:
        print(f"\n❌ Erro ao iniciar proxy: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()