# Client-Server-Application

Depends on spdlog: https://github.com/gabime/spdlog.  
Compiling does not work if $PATH is not set correctly.  

Depends on the libsodium library for the security extension.  
To install the libsodium library on ubuntu, just use
sudo apt-get install libsodium-dev


log levels:  
INFO - information useful for debugging  
WARN - warnings the user should know about  
ERR - error which lead to unexpected termination  

To compile the program, just use 'make' in the terminal while on the protocol folder. You will then get a server.out and a client.out file. Start the file in the terminal to start the test. server.out automatically starts by calling LISTEN and then ACCEPT. Client.out starts with calling CONNECT after entering a PORT which the Client should connect to. After the Handshake was established, both the Server and Client are able to send messages by typing SEND into the commandprompt, followed by enter. You are then able to type the message you would like to send and send it by hitting enter again.  
The communication partner will then receive your messages and display them in the terminal.  
In order to test a lossy channel, you can type SEND LOSSY into the command line to simulate a packet loss of a message that gets read into the buffer, increases the SEQ number and then you are able to type your message again. The communication partner will then notice that the packet does not have the sequence number it is waiting for and ask for a retransmission. The original sender will then send the parts of the buffer again which had not yet been acknowledged yet.  
You can also test whether the messages account for the window size by sending a huge message by typing SEND BEE into the command line. It will then read a testfile and send its contents to the communication partner. Since some of the message will be kept in the retransmission buffer because the file is bigger than the window, the user needs to type SEND, followed by 2 punches at the enter key again to send more of the testfile.

Updates: There is also a Security Extension (cf. the Extension standard pdf file) and a multiserver file, which showcases that multiple connections are possible. The multiserver connects to 2 clients and forwards messages which it received from Client A to Client B and vice versa.

