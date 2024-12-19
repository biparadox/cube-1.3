import os
import sys
import cgi
import argparse
#from functools import partial
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import unquote

save_path = './'

class SimpleHTTPRequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        # Parse the URL to get the file path
        # file_path = unquote(self.path.strip("/"))
        file_path = save_path
        if os.path.isfile(file_path):
            # If the requested path is a file, serve the file as an attachment
            self.send_response(200)
            self.send_header('Content-Type', 'application/octet-stream')
            # self.send_header('Content-Disposition', f'attachment; filename="{os.path.basename(file_path)}"')
            self.end_headers()
            with open(file_path, 'rb') as file:
                self.wfile.write(file.read())
        else:
            # Otherwise, show the upload form and file list
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            # List files in the current directory
            file_list = os.listdir(save_path)
            file_list_html = ''.join(f'<li><a href="/{file}" rel="external nofollow" >{file}</a></li>' for file in file_list)
            self.wfile.write(f'''
                <html>
                <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
                <body>
                    <form enctype="multipart/form-data" method="post">
                        <input type="file" name="file">
                        <input type="submit" value="Upload">
                    </form>
                    <h2>Files in current directory:</h2>
                    <ul>
                        {file_list_html}
                    </ul>
                </body>
                </html>
            '''.encode())
    def do_POST(self):
        try:
            content_type = self.headers['Content-Type']
            if 'multipart/form-data' in content_type:
                form = cgi.FieldStorage(
                    fp=self.rfile,
                    headers=self.headers,
                    environ={'REQUEST_METHOD': 'POST'}
                )
                file_field = form['file']
                if file_field.filename:
                    # Save the file
                    with open(os.path.join(save_path, file_field.filename), 'wb') as f:
                        f.write(file_field.file.read())
                    self.send_response(200)
                    self.send_header('Content-type', 'text/html')
                    self.end_headers()
                    self.wfile.write(b'''
                        <html>
                        <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
                        <body>
                            <p>File uploaded successfully.</p>
                            <script>
                                setTimeout(function() {
                                    window.location.href = "/";
                                }, 2000);
                            </script>
                        </body>
                        </html>
                    ''')
                else:
                    self.send_response(400)
                    self.send_header('Content-type', 'text/html')
                    self.end_headers()
                    self.wfile.write(b'''
                        <html>
                        <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
                        <body>
                            <p>No file uploaded.</p>
                            <script>
                                setTimeout(function() {
                                    window.location.href = "/";
                                }, 2000);
                            </script>
                        </body>
                        </html>
                    ''')
            else:
                self.send_response(400)
                self.send_header('Content-type', 'text/html')
                self.end_headers()
                self.wfile.write(b'''
                    <html>
                    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
                    <body>
                        <p>Invalid content type.</p>
                        <script>
                            setTimeout(function() {
                                window.location.href = "/";
                            }, 2000);
                        </script>
                    </body>
                    </html>
                ''')
        except Exception as e:
            self.send_response(500)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.wfile.write(f'''
                <html>
                <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
                <body>
                    <p>Error occurred: {str(e)}</p>
                    <script>
                        setTimeout(function() {{
                            window.location.href = "/";
                        }}, 2000);
                    </script>
                </body>
                </html>
            '''.encode())
def run(server_class, handler_class, ip, port):
    server_address = (ip, int(port))
    httpd = server_class(server_address, handler_class)
    # Uncomment the following lines if you need HTTPS support
    # httpd.socket = ssl.wrap_socket(httpd.socket,
    #                                keyfile="path/to/key.pem",
    #                                certfile='path/to/cert.pem', server_side=True)
    print(f'Starting httpd server on port {port}...')
    httpd.serve_forever()
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-ip_addr',default='',help='default ip address is 127.0.0.1')
    parser.add_argument('-port',default='8000',help='default port is 8000')
    parser.add_argument('-path',default='./',help='default path is current path ./ ')
    args = parser.parse_args()
    print("args:", args)
    print("type(args):", type(args))
    print("args.ip_addr:", args.ip_addr)
    print("args.port:", args.port)
    print("args.path:",args.path)
    # handler = partial(SimpleHTTPRequestHandler,directory=args.path)
    save_path = args.path
    run(HTTPServer,SimpleHTTPRequestHandler,args.ip_addr,args.port)
