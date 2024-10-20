import http.server
import socketserver

PORT = 8080

class SimpleHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        # Handle GET request
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        #self.wfile.write(b"Hello, this is a response from your server!")

    def do_POST(self):
        # Handle POST request
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        response = f"Received POST request with data: {post_data.decode('utf-8')}"
        print("Warning: Capacitor charge levels have dropped by 10%, please check sensor dashboard")
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.end_headers()
        self.wfile.write(response.encode('utf-8'))

def run(server_class=socketserver.TCPServer, handler_class=SimpleHTTPRequestHandler):
    server_address = ('', PORT)
    httpd = server_class(server_address, handler_class)
    print(f"Server started on localhost:{PORT}")
    httpd.serve_forever()

if __name__ == "__main__":
    run()