# socket_chat
This project is about a chat service implemented using the Linux Sockets interface in C. It's composed by two applications, a server and a client.

## Usage
Both server and client are C applications that run on Linux terminal. The communication between clients and server is based in several commands.

To run server aplication you must execute the next two commands:<br>
   1- '$ gcc server.c -o server'<br>
   2- '$ ./server'

To run a client you just need to execute the makefile by runnig: '$ make'. If you get some error after executing this command, you must run the next two commands:<br>
    1- '$ gcc -pthread -no-pie client.c terminal.o -lncurses -o client'<br>
    2- '$ ./client'

## Server Commands
* /start [PORT]: Starts the server listenig at port: [PORT]. Ex: /start 8000
* /write [MSG]: Sends a message to all active users in the chat. Ex: /write Hello everyone!
* /private [NICK] [MSG]: Sends a message to the user with nickname [NICK]. Ex: /private leo Hi Leonard, how are you?
* /list: Enumerates all active user and their IP addresses.
* /kick [NICK]: Ejects the user with nickname [NICK]. Ex: /kick leo
* /block [WORD]: Blocks the word [WORD] and prohibits its use in the chat's messages. Ex: /block politics
* /ban [IP]: Ejects the client with IP address [IP] and do not allows more connections from that IP. Ex: /ban 162.38.6.26
* /unban [IP]: Allows connections from [IP] again and remove it from list of banished clients. Ex: /unban 162.38.6.26
* /stop, /exit: Stops the server and notify clients.

## Client Commands
* /connect [IP] [PORT]: Tries connect the server at [IP]:[PORT]. Ex: /connect 127.0.0.1 8000
* /nick [NICK]: Requests the server to change user nickname. Ex: /nick leo
* /list: Enumerates all active user and their IP addresses.
* /private [NICK] [MSG]: Sends a private message to user with nickname [NICK]. Ex: /private shelly Hi Sheldon, how is String Theory going?
* [MSG]: Sends a public message to all active users. No command required. Ex: Hi everyone! I'am Leonard, my nick is leo. 
* /sendfile [NICK] [FILE_PATH]: Notify user [NICK] about send a file and wait for an answer. Ex: /sendfile shelly /home/Pictures/card.jpg
* /accept: Accepts /sendfile request and start recieving the file.
* /reject: Rejects /sendfile request and notify sender.
* /disconnect: Closes the connection with the server.

