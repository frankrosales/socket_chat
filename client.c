#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include "chat.h"
#include "terminal.h"


int read_end;
int write_end;

int client_fd = -1;
int max_fd;
char buffer[4096];
token_t tokens[MAX_TOKENS_NUMBER];
fd_set read_fd, ready_fd;
int connected = 0;
file_t send_files[MAX_CLIENTS_NUMBER];
int send_count = 0;
int send_fd;
file_t recieve_files[MAX_CLIENTS_NUMBER];
int recieve_count = 0;


int main()
{
    signal(SIGINT, handler);
    int terminal_fds[2];
    //Initialize the terminal
    init_terminal(terminal_fds);
    //Get the file descriptors
    int read_end =  terminal_fds[0];
    int write_end =  terminal_fds[1];
	FD_ZERO(&read_fd);
    FD_SET(read_end, &read_fd);
    max_fd = read_end;

	while(1)
	{		
        ready_fd = read_fd;
        if(Send_Files())
        {
            fd_set writer;
            FD_SET(write_end, &writer);
            select(max_fd + 1, &ready_fd, &writer, NULL, NULL);
        }
        else  select(max_fd + 1, &ready_fd, NULL, NULL, NULL);

		if(FD_ISSET(read_end, &ready_fd) )
		{
            int rd = ReadLine(read_end, buffer);
            if(connected && buffer[0] != '/')
            {
                Send_message(client_fd, buffer);
            }
            else
            {
                int tok_count = Parse(buffer, rd, tokens);            
                if (strcmp((char*)tokens[0].string, "/connect") == 0)
                {
                   if(tok_count != 3) dprintf(write_end, "ERROR: Invalid command.");
                   else if(connected == 1) dprintf(write_end, "ERROR: You are already connected.");
                   else 
                   {
                       client_fd = Connect((char*)tokens[1].string, atoi(tokens[2].string));
                       if(client_fd < 0 ) dprintf(write_end, "ERROR: Unable to connect to this server.");
                       else
                       {
                            if(client_fd > max_fd) max_fd = client_fd;
                            FD_SET(client_fd, &read_fd);
                            dprintf(write_end,"-->Connection established with server at port <%s>\n", tokens[2].string);
                            connected = 1;
                            char *msg = malloc(50);
                            strcpy(msg, "-->Connection established with server at port <");
                            strcat(msg, tokens[2].string);
                            strcat(msg, ">.");
                            logmsg(msg);
                            free(msg);
                       }
                   }
                }
                else if (strcmp((char*)tokens[0].string, "/disconnect") == 0)
                {
                    connected = -1;
                    Send_message(client_fd, "/disconnect");
                    close(client_fd);
                    FD_CLR(client_fd, &read_fd);
                    client_fd = -1;
                    max_fd = read_end;
                    dprintf(write_end,"-->You are now disconnected\n");
                    logmsg("You are now disconnected.");
                }
                else if (strcmp((char*)tokens[0].string, "/private") == 0)
                {
                    if(tok_count < 3)   dprintf(write_end, "Invaid Command.");
                    else
                    {
                        Send_message(client_fd, buffer);
                    }
                }
                else if (strcmp((char*)tokens[0].string, "/nick") == 0)
                {
                    if(tok_count != 2)   dprintf(write_end, "ERROR: Invaid Command.");
                    else
                    {
                        Send_message(client_fd, buffer);
                    }
                }
                else if (strcmp((char*)tokens[0].string, "/exit") == 0)
                {
                    connected = -1;
                    Send_message(client_fd, "/disconnect");
                    client_fd = -1;
                    close(client_fd);
                    max_fd = read_end;
                    dprintf(write_end, "-->You are now disconnected");
                    logmsg("You are now disconnected.");
                    _exit(0);
                }
                else if (strcmp((char*)tokens[0].string, "/list") == 0)
                {
                    if(tok_count != 1) dprintf(write_end, "ERROR: Invaid Command.");
                    else    
                        Send_message(client_fd, "/list");
                }
                else if (strcmp((char*)tokens[0].string, "/sendfile") == 0)
                {
                    if(tok_count != 3) dprintf(write_end, "ERROR: Invaid Command.");
                    else
                    {
                        int fs = Get_File_Size(tokens[2].string);
                        if(fs < 0) dprintf(write_end, "ERROR: No such file path.");
                        else Send_message(client_fd, buffer);
                    }
                }
                else if (strcmp((char*)tokens[0].string, "/accept") == 0)
                {
                    if(tok_count != 2) dprintf(write_end, "ERROR: Invaid Command.");
                    else
                    {
                        Send_message(client_fd, buffer);
                    }
                }
                else if (strcmp((char*)tokens[0].string, "/reject") == 0)
                {
                    if(tok_count != 2) dprintf(write_end, "ERROR: Invaid Command.");
                    else
                    {
                        Send_message(client_fd, buffer);
                    }
                }
            }
            
		}
		if(FD_ISSET(client_fd, &ready_fd))
		{
            // the server has sent a message
            int rd = ReadMessage(client_fd, buffer);
            if(rd)
            {
                if(strncmp(buffer, "/stop", 5) == 0)
                {
                    bzero(buffer, 4096);
                    char *msg =  "-->Server has stoped.\n";
                    write(write_end, msg, strlen(msg));
                    connected = -1;
                    logmsg(msg);
                    FD_CLR(client_fd, &read_fd);
                    close(client_fd);
                    max_fd = read_end;
                    msg = "-->You are now disconnected.\n";
                    write(write_end, msg, strlen(msg));
                }
                else if(strncmp(buffer, "/kick", 5) == 0)
                {
                    bzero(buffer, 4096);
                    char *msg =  "-->You has been ejected by server.";
                    write(write_end, msg, strlen(msg));
                    write(write_end, "\n", 1);
                    logmsg(msg);
                    connected = -1;
                    FD_CLR(client_fd, &read_fd);
                    close(client_fd);
                    max_fd = read_end;
                    msg = "-->You are now disconnected.\n";
                    write(write_end, msg, strlen(msg));
                }
                else if (strncmp(buffer, "/newfile", 8) == 0)
                {
                    // create the new file
                    Parse(buffer, rd, tokens);
                    // tokens[1] is path and [2] is the id
                    int fd = open(tokens[1].string, O_CREAT | O_APPEND | O_RDWR, S_IRWXU);
                    if(fd < 0) dprintf(write_end, "ERROR creating the file.\n");
                    else
                    {
                        recieve_files[recieve_count].f_fd = fd;               
                        recieve_files[recieve_count].f_fd = atoi(tokens[2].string);
                        strcpy(recieve_files[recieve_count].f_path, tokens[1].string);
                        recieve_count++;
                    }
                }
                else if (strncmp(buffer, "/sendfile", 9) == 0)
                {
                    // send a file
                    Parse(buffer, rd, tokens); // 4 must be returned by Parse(...)
                    // tok 1 = nick
                    // tok 2 = path
                    // tok 3 = ID
                    for(int i = 0; i < send_count; i++)
                    {
                        if(!strcmp(tokens[2].string, send_files[i].f_path))
                        {
                            send_files[i].f_ID = atoi(tokens[3].string);
                            strcpy(send_files[i].f_reciever, tokens[1].string);
                            strcpy(send_files[i].f_path, tokens[2].string);
                            send_files[i].f_sending = 1;
                            break;
                        }
                    }
                }
                else if (strncmp(buffer, "/file", 5) == 0)
                {
                    char number[4];
                    strncpy(number, &buffer[5], 4);
                    int f_ID = atoi(number);
                    strncpy(number, &buffer[13], 4);
                    int M = atoi(number);
                    strncpy(number, &buffer[13], 4);
                    // I have the file ID and M, I can now copy
                    for(int i = 0; i < send_count; i++)
                    {
                        if(send_files[i].f_ID == f_ID) // this is the file one
                        {
                            write(send_files[i].f_fd, &buffer[29], M);
                            break;
                        }
                    }                    

                }
                else write(write_end, buffer, 4096);
            }
		}
    }

    /*end terminal*/ 
        end_terminal();
        _exit(0);
    /**/
}


/**************************************Functions*************************************/

    /*///<summary>
    /// Opens and returns a client descriptor that connect with server at port 'port'.
    ///<Returns> A client descriptor to  connect with server at port 'port'</Returns>
    *///</summary>
    int Connect(char *hostname, int port)
    {
        int clientfd;
        struct hostent *hp;
        struct sockaddr_in serveraddr;

        if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1; // Check errno for cause of error

        //Fill in the server’s IP address and port 
        if ((hp = gethostbyname(hostname)) == NULL)
            return -2; //Check h_errno for cause of error

        bzero((char *) &serveraddr, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        bcopy((char *)hp->h_addr_list[0], (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
        serveraddr.sin_port = htons(port);

        // Establish a connection with the server
        if (connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0)
            return -1;
        return clientfd;
    }

    /*///<summary>
    /// Read a command line from the descriptor fd of input and store it into buffer.
    ///<Returns> The length of buffer </Returns>
    *///</summary>
    int ReadLine(int fd, char *buffer)
    {
        bzero(buffer, 4096);
        int count = read(fd, buffer, 4096);    
        return count;    
    }

    /*///<summary>
    /// Parse buffer into tokens
    ///<Returns> The number of tokens parsed </Returns>
    *///</summary>
    int Parse(char *buffer, int buf_len, token_t *tokens)
    {
        bzero(tokens, MAX_TOKENS_NUMBER*sizeof(token_t));
        int count = 0;
        int tok_len = 0;
        for(int i = 0; i <= buf_len; i++)
        {
			if( i == buf_len || (count < MAX_TOKENS_NUMBER - 1 && buffer[i] == ' ' ) ) // because at most, I'll have 3 tokens
        {
                //I have found a token
                tokens[count++].tok_len = tok_len;
                tok_len = 0;
        }
        else 
        {
            tokens[count].string[tok_len++] = buffer[i];
        }
        }
        return count;
    }

    void PrintLine(char *line)
    {
        bzero(buffer, 4096);
        strcpy(buffer,line);
        write(write_end, buffer, 4096);
    }

    /*///<summary>
    /// Sends a message between the server and clients following the stablished protocol.
    ///<Returns> The length of msg. </Returns>
    *///</summary>
    int Send_message(int fd, char *msg)
    {
        int len1 = strlen(msg);
        write(fd, &len1, 4);
        write(fd, msg, len1);
        return len1;
    }

    /*///<summary>
    /// First, reads an 4-bytes integer from fd and them reads n characters from fd
    ///<Returns> The 4-bytes integer read from fd.</Returns>
    *///</summary>
    int ReadMessage(int fd, char *buffer)
    {
        bzero(buffer, 4096);
        int length;
        read(fd, &length, 4);
        read(fd, buffer, length);
        return length;
    }

    /*///<summary>
    /// Converts the integer into 'n' to a char* into 'string'
    ///<Returns> The length of 'string'. </Returns>
    *///</summary>
    int ToString(int n, char* string)
    {
        int length = 0;
        char digits[10] = {'0', '1', '2','3', '4', '5', '6', '7', '8', '9'};
        int* rests = calloc(10, sizeof(int));

        while(n >= 10)
        {
            rests[10-length-1] = n % 10;
            length++;
            n = (int)(n / 10);
        }
        rests[10 - length - 1] = n;
        length++;
        
        int pos = 0;
        for(int i = 10 - length; i < 10; i++)
        {
        *(string + pos) = digits[*(rests + i)];
        pos++;
        }
        free(rests);
        return length;
    }

    /*///<summary>
    /// Close all opened descriptors
    *///</summary>
    void handler(int sig)
    {
        //close all opened descriptors
        if (connected) Send_message(client_fd, "/disconnect");
        connected = -1;
        close(client_fd);
        FD_CLR(client_fd, &read_fd);
        client_fd = -1;
        max_fd = read_end;
        char *msg = "-->You are now disconnected";
        write(write_end, msg, strlen(msg));
        end_terminal();
        _exit(0);
    }

    /*///<summary>
      /// Gets the size of file with path 'path' and fills the file information
      ///<Returns> -1 if there is no such file, else returns the number of bytes in 'path'. </Returns>
    *///</summary>
    int Get_File_Size(char *path)
    {
        send_files[send_count].f_fd = open(path, O_RDONLY, S_IRWXU);
        if(send_files[send_count].f_fd < 0) return -1;
        strcpy(send_files[send_count].f_path, path);
        int count = 0;
        char c;
        while(read(send_files[send_count].f_fd, &c, 1)) count++;
        send_files[send_count++].f_size = count;
        return count;
    }

    /*///<summary>
      /// Send the current piece from all files with sending state = 1 to Its recievers
    *///</summary>
    int Send_Files()
    {
        int sending = 0;
        for(int i = 0; i < send_count; i++)
        {
            if(send_files[i].f_sending) // this file is been sended
            {
                sending = 1;
                int total = send_files[i].f_size / 4000;
                if(send_files[i].f_size % 4000) total++;
                if(send_files[i].f_count == total) // I have finished to send this one
                {
                    char *msg = malloc(50);
                    strcpy(msg, "/finish ");
                    char *number = malloc(2);
                    ToString(send_files[i].f_ID, number);
                    strcat(msg, number);
                    Send_message(client_fd, msg);
                    free(number);
                    free(msg);
                    dprintf(write_end, "File %d sended successfully.\n", send_files[i].f_ID);
                    send_files[i].f_sending = 0;
                    close(send_files[i].f_fd);
                }
                else
                {
                    char *msg = malloc(4029);
                    int rd = read(send_files[i].f_fd, &msg[29], 4000);
                    int N = rd + 29;
                    strcpy(msg, "/file");
                    strncpy(&msg[5], (char *)&send_files[i].f_ID, 4);
                    strncpy(&msg[9], (char *)&send_files[i].f_size, 4);
                    strncpy(&msg[13], (char *)&rd, 4);
                    int cant = 4000*send_files[i].f_count + rd;
                    send_files[i].f_count++;
                    strncpy(&msg[17], (char *)&cant, 4);
                    strncpy(&msg[21], (char *)&send_files[i].f_count, 4);
                    strncpy(&msg[25], (char *)&total, 4);
                    // sending the message to server
                    write(client_fd, &N, 4);
                    write(client_fd, msg, N);
                    free(msg);
                }
            }
        }
        return sending;
    }

    void logmsg(char *msg)
    {
        int logfd = open("client_log.txt", O_CREAT | O_APPEND | O_RDWR, S_IRWXU);
        write(logfd, msg, strlen(msg));
        write(logfd, "\n", 1);
        close(logfd);
    }

/************************************************************************************/
// test path: /home/frank-elier/School/Shell/history.txt