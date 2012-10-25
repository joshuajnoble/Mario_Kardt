import struct
import SocketServer
import time
import threading
from SocketServer import ThreadingTCPServer, BaseRequestHandler
from base64 import b64encode
from hashlib import sha1
from mimetools import Message
from StringIO import StringIO

class WebSocketsHandler(SocketServer.StreamRequestHandler):
    magic = '258EAFA5-E914-47DA-95CA-C5AB0DC85B11'
    hasSecKey = True
    receivedMessage = ""
    handlers = {}

    def setup(self):
        SocketServer.StreamRequestHandler.setup(self)
        print "connection established", self.client_address
        self.server.clients.append(self)
        self.handshake_done = False

    def handle(self):
        while True:
            if not self.handshake_done:
                self.handshake()
            else:
                self.read_next_message()
                #time.sleep(5)

    def read_next_message(self):
        print "has message"
        if self.hasSecKey:
            length = ord(self.rfile.read(2)[1]) & 127
            if length == 126:
                length = struct.unpack(">H", self.rfile.read(2))[0]
            elif length == 127:
                length = struct.unpack(">Q", self.rfile.read(8))[0]

            masks = [ord(byte) for byte in self.rfile.read(4)]
            decoded = ""
            for char in self.rfile.read(length):
                decoded += chr(ord(char) ^ masks[len(decoded) % 4])
            self.on_message(decoded)
        else:
            self.on_message(self.rfile.read(8))

    def send_message(self, message):
        print "sending"
        print message
        if self.hasSecKey:
            self.request.send(chr(129))
            length = len(message)
            if length <= 125:
                self.request.send(chr(length))
            elif length >= 126 and length <= 65535:
                self.request.send(126)
                self.request.send(struct.pack(">H", length))
            else:
                self.request.send(127)
                self.request.send(struct.pack(">Q", length))
            self.request.send(message)
        else:
            self.request.send('\x00' + message + '\xff')

    def handshake(self):
        data = self.request.recv(1024).strip()

        print data
        dSplit = data.split('\r\n', 1)
        if len(dSplit) > 1 :
            headers = Message(StringIO(data.split('\r\n', 1)[1]))
        else:
            headers = Message(StringIO(data.split('\r\n', 1)))

        if headers.get("Upgrade", None).lower() != "websocket":
            print "no upgrade"
            return

        print 'Handshaking...'

        try:
            key = headers['Sec-WebSocket-Key']
            digest = b64encode(sha1(key + self.magic).hexdigest().decode('hex'))
        except KeyError:
            self.hasSecKey = False
            print "no Sec-WebSocket-Key"

        response = 'HTTP/1.1 101 Switching Protocols\r\n'
        response += 'Upgrade: websocket\r\n'
        response += 'Connection: Upgrade\r\n'

        if(self.hasSecKey):
            response += 'Sec-WebSocket-Accept: %s\r\n\r\n' % digest

        self.handshake_done = self.request.send(response)

    def on_message(self, message):
        self.receivedMessage += message.strip("\r")
        if self.receivedMessage.find("Hello, Wifly") != -1:
            self.server.handleWiFlyMessage("Hello, Wifly")
            self.receivedMessage = ""
        if self.receivedMessage.find("Hello, Browser") != -1:
            self.server.handleBrowserMessage("Hello, Browser")
            self.receivedMessage = ""

    def set_handlers(self, wHandler, bHandler):
        print "calling set handle"
        #self.handlers.append(wHandler)
        #self.handlers.append(bHandler)
        self.handlers['wifly'] = wHandler
        self.handlers['browser'] = bHandler


class WSServer(ThreadingTCPServer):


    clients = []

    def handle_request(self):
        print " handle request "
        handler = ThreadingTCPServer.handle_request(self)
        handler.set_handlers(self.handleWiFlyMessage, self.handleBrowserMessage)
        clients.append(handler)
        return handler

    def finish_request(self, *args, **kws):
        print "process request"
        handler = ThreadingTCPServer.finish_request(self, *args, **kws)
        #handler.set_handlers(self.handleWiFlyMessage, self.handleBrowserMessage)

    def handleWiFlyMessage(self, message):
        print "handleWiFlyMessage"
        self.clients[0].send_message(message)

    def handleBrowserMessage(self, message):
        print "handleBrowserMessage"
        self.clients[1].send_message(message)


if __name__ == "__main__":
    server = WSServer(("", 3000), WebSocketsHandler)

    server_thread = threading.Thread(target=server.serve_forever)
    ip, port = server.server_address

    server_thread.setDaemon(True)
    server_thread.start()

    while 1:
        num = 0

    server_thread.join()
    server.shutdown()

