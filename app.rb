require 'eventmachine'
require 'em-websocket'

@sockets = []
EventMachine.run do
  puts 'start'
  EventMachine::WebSocket.start(:host => '192.168.2.102', :port => 3000) do |socket|
    puts 'start webserver'
    socket.onopen do
      puts 'onopen'
      @sockets << socket
    end
    socket.onmessage do |mess|
      puts "onmessage = #{mess}"
      @sockets.each {|s| s.send mess}
    end
    socket.onclose do
      pust 'onclose'
      @sockets.delete socket
    end
  end
end
