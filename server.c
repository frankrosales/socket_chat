#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "chat.h"

/*Gloval variables for server*/

    token_t tokens[MAX_TOKENS_NUMBER];
    client_t clients[MAX_CLIENTS_NUMBER];
    file_t files[ 2 * MAX_CLIENTS_NUMBER];
    int n_files = 0;
    fd_set read_set, ready_set;
    char buffer[4096];
    int listen_fd = -1;
    int n_clients = 0;// the actual number of connected clients
    int running = 0;
    int max_fd;

/*****************************/

int main(int argc, char const *argv[])
{
    signal(SIGINT, handler);
    FD_ZERO(&read_set);
    FD_SET(STDIN_FILENO, &read_set);
    max_fd = STDIN_FILENO;
   
    while(1)
    {
        if(running && n_clients == 0)   printf("...waiting for connections...\n");
        // update ready_set
        ready_set = read_set;
        select(max_fd + 1, &ready_set, NULL, NULL, NULL);
        
        if(running)
        {
            // check all connected clients (check them with FD_ISSET)
            for(int i = 0; i < n_clients; i++)
            {
                if(FD_ISSET(clients[i].c_fd, &ready_set))
                {
                    Check_client(i);
                }
            }
            // There is a new connection request...
            if(FD_ISSET(listen_fd, &ready_set))
            {
                puts("There is a new connection request.");
                int client_fd = Accept_new_client();
                if(client_fd < 0)
                {
                    printf("ERROR calling accept function.\n");
                }
                else
                {
                    //check if It's baned or not
                    char *IP = inet_ntoa(clients[n_clients].addr.sin_addr);
                    printf("IP: %s\n", IP);
                    if(is_baned(IP))
                    {
                        Send_message(clients[n_clients].c_fd, "Sorry, you may not connect to this chat.");
                        Send_message(clients[n_clients].c_fd, "/kick");
                        FD_CLR(clients[n_clients].c_fd, &read_set);
                        close(clients[n_clients].c_fd);
                        printf( "Client <%s> has been ejected.\n", IP);
                        bzero(clients[n_clients].nick_name, 10);
                        clients[n_clients].c_fd = -1;
                        clients[n_clients].connected = 0;
                    }
                    else
                    {
                        n_clients++;
                        // insert the new clientfd into read_set
                        FD_SET(client_fd, &read_set);
                        // update max_fd
                        if(client_fd > max_fd) max_fd = client_fd;
                        char *msg = malloc(256);
                        strcpy(msg, "-->Client <");
                        strcat(msg, clients[n_clients - 1].nick_name);
                        strcat(msg, "> has connected.");
                        puts(msg);
                        Send_multi_message(msg);
                        logmsg(msg);
                        free(msg);
                    }
                }
            }
        }
        // The standard input is ready for reading
        if(FD_ISSET(STDIN_FILENO, &ready_set))
        {
            // read and parse the input line
            int tok_count = Parse(buffer, ReadLine(STDIN_FILENO,buffer), tokens);
            if (strcmp((char*)tokens[0].string, "/start") == 0)
            {
                if(running) printf("Server is running, It cant be restarted.");
                else
                {
                    bzero(clients, MAX_CLIENTS_NUMBER*sizeof(client_t));                    
                    int port = atoi(tokens[1].string);
                    // start the server in this port
                    listen_fd = Start(port);
                    if(listen_fd > max_fd) max_fd = listen_fd;
                    running = 1;
                    // introduce the socket descriptor of the server in the fd_set
                    FD_SET(listen_fd, &read_set);
                    printf("Server is running at port <%d>.\n", port);
                }
            }
            if(running)
            {
                if (strcmp((char*)tokens[0].string, "/write") == 0)
                {
                    if(tok_count < 2)
                        Error("Invaid Command.");
                    else if(n_clients == 0) puts("There is no connected clients.");
                    else
                    {
                        char *msg = malloc(500);
                        bzero(msg, 500);
                        strcat(msg, "admin:");
                        char *c = (char *)&buffer[6];
                        strcat(msg, c);
                        Send_multi_message(msg);    
                        free(msg);        
                    }                
                }
                else if (strcmp((char*)tokens[0].string, "/private") == 0)
                {
                    if(tok_count < 3)   Error("Invaid Command.");
                    else
                    {
                        int found = 0;
                        for(int i = 0; i < n_clients; i++)
                        {
                            if(clients[i].connected && strcmp(tokens[1].string, clients[i].nick_name) == 0)
                            {
                                found = 1;
                                char *msg = malloc(500);
                                bzero(msg, 500);
                                msg = strcat(msg, "admin: [PRIVATE] ");
                                for(int i = 2; i < tok_count; i++)
                                {
                                    strcat(msg, tokens[i].string);
                                    strcat(msg, " ");
                                }
                                Send_message(clients[i].c_fd, msg);
                                free(msg);
                            }
                        }
                        if(found == 0) 
                            Error("there is no such client ID.");
                        
                    }
                    
                }
                else if (strcmp((char*)tokens[0].string, "/stop") == 0)
                {
                    //printf("Command: %s\n", "stop");
                    logmsg("Server has Stoped");
                    Stop();
                }
                else if (strcmp((char*)tokens[0].string, "/exit") == 0)
                {
                    logmsg("Server has exit.");
                    Stop();
                    _exit(0);
                }
                else if (strcmp((char*)tokens[0].string, "/list") == 0)
                {
                    for(int i = 0; i < n_clients; i++)
                    {
                        if(clients[i].connected)
                        {
                            char *msg = malloc(256);
                            strcpy(msg, clients[i].nick_name);
                            strcat(msg, " : ");
                            char *IP = inet_ntoa(clients[i].addr.sin_addr);
                            strcat(msg,IP);
                            printf("%s\n", msg);
                            free(msg);
                        }
                    }
                    
                }
                else if (strcmp((char*)tokens[0].string, "/kick") == 0)
                {
                    if(tok_count != 2) printf("ERROR: Invalid command.");
                    else
                    {
                        int found = 0;
                        for(int i = 0; i < n_clients; i++)
                        {
                            if(clients[i].connected && strcmp(tokens[1].string, clients[i].nick_name) == 0)
                            {
                                found = 1;
                                Send_message(clients[i].c_fd, "/kick");
                                FD_CLR(clients[i].c_fd, &read_set);
                                close(clients[i].c_fd);
                                char *msg = malloc(50);
                                strcpy(msg, "Cliente <");
                                strcat(msg, tokens[1].string);
                                strcat(msg, ">  has been ejected.");
                                logmsg(msg);
                                printf( "Client <%s> has been ejected.\n", tokens[1].string);
                                bzero(clients[i].nick_name, 10);
                                clients[i].c_fd = -1;
                                clients[i].connected = 0;
                                free(msg);
                            }
                        }
                        if(found == 0) 
                            printf( "ERROR: There is no such client ID.");
                    }
                }
                else if (strcmp((char*)tokens[0].string, "/block") == 0)
                {
                    // commnad /block
                    if(tok_count == 1) block();
                    else if (tok_count == 2) // commnad /block word
                    {
                        block_word(tokens[1].string);
                        char *msg = malloc(50);
                        strcpy(msg, "Word: '");
                        strcat(msg, tokens[1].string);
                        strcat(msg, "' has been blocked");
                        free(msg);
                    }
                    else printf( "ERROR: Invalid command.");
                }
                else if (strcmp((char*)tokens[0].string, "/ban") == 0)
                {
                    if(tok_count != 2) printf("ERROR: Invalid command\n");
                    else
                    {
                        printf("IP: %s\n", tokens[1].string);
                        if(!ban(tokens[1].string)) printf("There is no such client IP.\n");
                        else
                        {
                            char *msg = malloc(50);
                            strcpy(msg, "User with IP: '");
                            strcat(msg, tokens[1].string);
                            strcat(msg, "' has been baned.");
                            logmsg(msg);
                            free(msg);
                        }
                    }
                }
                else if (strcmp((char*)tokens[0].string, "/unban") == 0)
                {
                    if(tok_count != 2) printf("ERROR: Invalid commnad.\n");
                    else
                    {
                        unban(tokens[1].string);
                        char *msg = malloc(50);
                        strcpy(msg, "User with IP: '");
                        strcat(msg, tokens[1].string);
                        strcat(msg, "' has been unbaned.");
                        logmsg(msg);
                        free(msg);
                    }
                }
            }
        }

    }
    return 0;
}

/***************************Functions's implementation**************************/

    /*///<summary>
      /// Read a command line from the descriptor fd of input and store it into buffer.
      ///<Returns> The length of buffer </Returns>
    *///</summary>
    int ReadLine(int fd, char *buffer)
    {
        int count = 0;
        char c;
        bzero(buffer, 4096);
        while(read(fd, &c, 1))
        {
            if(c == '\n') break;
            buffer[count] = c;
            count++;
        }
        return count;    
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
        if( i == buf_len || (count < MAX_TOKENS_NUMBER - 1 && buffer[i] == ' ' ) )
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

    /*///<summary>
      /// Opens and returns a listening descriptor that is
      /// ready to receive connection requests on the well-known port 'port'.
      ///<Returns> A listening descriptor to recieve connection request</Returns>
    *///</summary>
    int Start(int port)
    {
        int listenfd, optval=1;
        struct sockaddr_in serveraddr;

        // Create a socket descriptor
        if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

        // Eliminates "Address already in use" error from bind
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int)) < 0)
            return -1;

        // Listenfd will be an end point for all requests to port on any IP address for this host
        bzero((char *) &serveraddr, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serveraddr.sin_port = htons((unsigned short)port);
        if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0)
            return -1;

        // Make it a listening socket ready to accept connection requests
        if (listen(listenfd, MAX_CLIENTS_NUMBER) < 0)
            return -1;
        return listenfd;
    }

    /*///<summary>
      /// Check which command must be executed by client at index 'index'
    *///</summary>
    void Check_client(int index)
    {
        int count = ReadMessage(clients[index].c_fd, buffer);
        if(buffer[0] != '/' && count > 0)
        {
            // public message, check the blocked words
            char *msg = malloc(4096);
            strcpy(msg, clients[index].nick_name);
            strcat(msg, ": ");
            char *tok0 = strtok(buffer, " ");
            if(is_blocked(tok0)) strcat(msg, "FILTERED");
            else
            {
                strcat(msg, tok0);
                strcat(msg, " ");
            }
            while(1)
            {
                char *tok1 = strtok(NULL, " ");
                if(tok1 == NULL) break;
                else
                {
                    if(is_blocked(tok1)) strcat(msg, "FILTERED");
                    else strcat(msg, tok1);
                    strcat(msg, " ");                
                }
            }
            printf("Message: %s\n", msg);
            for(int i = 0; i < n_clients; i++)
            {
                if(clients[i].connected && i != index)
                {
                    Send_message(clients[i].c_fd, msg);
                }
            }            
            free(msg);
            return;
        }
        else if(strncmp(buffer, "/file", 5) == 0)
        {
            char number[4];
            puts(buffer);
            strncpy(number, &buffer[5], 4);
            int f_ID = atoi(number);
            for(int i = 0; i < n_files; i++)
            {
                if(files[i].f_ID == f_ID)
                {
                    for(int j = 0; j < n_clients; j++)
                    {
                        if(clients[j].connected && !strcmp(clients[j].nick_name, files[i].f_reciever))
                        {
                            write(clients[j].c_fd, buffer, count);
                            return;
                        }
                    }
                    
                }
            }
            
        }
        int tok_count = Parse(buffer, count, tokens);
        if (strcmp((char*)tokens[0].string, "/disconnect") == 0)
        {
            //puts("...disconnecting...");
            FD_CLR(clients[index].c_fd, &read_set);
            close(clients[index].c_fd);
            printf("Client <%s> has gone.\n", clients[index].nick_name);
            char *msg = malloc(50);
            strcpy(msg, "User <");
            strcat(msg, clients[index].nick_name);
            strcat(msg, "> has gone.");
            logmsg(msg);
            free(msg);
            bzero(clients[index].nick_name, 10);
            clients[index].c_fd = -1;
            clients[index].connected = 0;
        }
        else if (strcmp((char*)tokens[0].string, "/nick") == 0)
        {
            if(strlen(tokens[1].string) > 10)
            {
                Send_message(clients[index].c_fd, "The nick length must not exed 10 characters.");
                return;
            }
            for(int i = 0; i < n_clients; i++)
            {
                if(clients[i].connected && strcmp(tokens[1].string, clients[i].nick_name) == 0)
                {
                    Send_message(clients[index].c_fd, "There is another client with that nick name.");
                    return;   
                }
            }        
            bzero(clients[index].nick_name, 10);
            strcpy(clients[index].nick_name, tokens[1].string);
            char *msg = malloc(100);
            strcpy(msg, "Your nick name has been successfully changed to '");
            strcat(msg, tokens[1].string);
            strcat(msg, "'.");
            Send_message(clients[index].c_fd, msg);
            bzero(msg, 100);
            strcpy(msg, "User with IP: '");
            strcat(msg, inet_ntoa(clients[index].addr.sin_addr));
            strcat(msg, "' has changed his nick name to: '");
            strcat(msg, tokens[1].string);
            strcat(msg, "'.");
            logmsg(msg);
            free(msg);
            return;
        }
        else if (strcmp((char*)tokens[0].string, "/private") == 0)
        {
            int found = 0;
            for(int i = 0; i < n_clients; i++)
            {
                if(clients[i].connected && strcmp(tokens[1].string, clients[i].nick_name) == 0)
                {
                    found = 1;
                    char *msg = malloc(500);
                    bzero(msg, 500);
                    strcat(msg, clients[index].nick_name);
                    strcat(msg, ": [PRIVATE] ");
                    for(int i = 2; i < tok_count; i++)
                    {
                        strcat(msg, tokens[i].string);
                        strcat(msg, " ");
                    }
                    Send_message(clients[i].c_fd, msg);
                    free(msg);
                }
            }
            if(found == 0)
            {
                Send_message( clients[index].c_fd, "--> There is no such client ID.");
            }
            
        }
        else if (strcmp((char*)tokens[0].string, "/list") == 0)
        {
            char *msg = malloc(4096);
            strcpy(msg, "List:\n");
            for(int i = 0; i < n_clients; i++)
            {
                if(clients[i].connected)
                {
                    strcat(msg, clients[i].nick_name);
                    strcat(msg, " : ");
                    char *IP = inet_ntoa(clients[i].addr.sin_addr);
                    strcat(msg,IP);
                    strcat(msg, "\n");
                }
            }
            Send_message(clients[index].c_fd, msg);
            free(msg);
            
        }
        else if (strcmp((char*)tokens[0].string, "/sendfile") == 0)
        {
            // find the client
            int pos = -1;
            for(int i = 0; i < n_clients; i++)
            {
                if(clients[i].connected && strcmp(tokens[1].string, clients[i].nick_name) == 0)
                {
                    pos = i;
                    break;
                }
            }
            if( pos < 0)
            {
                Send_message(clients[index].c_fd, "There is no such client ID.");
                return;
            }
            //create the file (server just needs to fill the ID, sender and path)
            files[n_files].f_ID = n_files + 1;
            strcpy(files[n_files].f_sender, clients[index].nick_name);
            strncpy(files[n_files].f_path, tokens[2].string, tokens[2].tok_len);
            // create and send the message
                char *msg = malloc(256);
                strcpy(msg, "[NEW FILE] user '");
                strcat(msg, clients[index].nick_name);
                strcat(msg, "' has requested to send file '");
                int j = tokens[2].tok_len - 1;
                while(j >= 0 && tokens[2].string[j] != '/') j--;
                char *name = (char *)&tokens[2].string[j + 1];
                strcat(msg, name);
                strcat(msg, "'\n[NEW FILE] type '/accept ");
                char *number = malloc(2);
                int len = ToString(n_files + 1, number);
                strncat(msg, number, len);
                strcat(msg, "' to accept the file or '/reject ");
                strncat(msg, number, len);
                strcat(msg, "' to reject it.");
                free(number);
                Send_message(clients[pos].c_fd, msg);
                n_files++;
                free(msg);
            //********************
        }
        else if (strcmp((char*)tokens[0].string, "/accept") == 0)
        {
            if(tok_count != 2) Send_message(clients[index].c_fd, "Invalid command.");
            else
            {
                //notify to sender for beging to send the file
                int id = atoi(tokens[1].string);
                int pos = 0; // the index of the file that I want to send
                for(int i = 0; i < n_files; i++)
                {
                    if(files[i].f_ID == id) // this is the 'file one'
                    {
                        pos = i;
                        strcpy(files[i].f_reciever, clients[index].nick_name);
                        break;
                    }
                }
                char *msg = malloc(1024);
                strcpy(msg, "/sendfile ");
                strcat(msg, clients[index].nick_name);
                strcat(msg, " ");
                strcat(msg, files[pos].f_path);
                strcat(msg, " ");
                strcat(msg, tokens[1].string);
                
                for(int i = 0; i < n_clients; i++)
                {
                    if(clients[i].connected && strcmp(clients[i].nick_name, files[pos].f_sender) == 0)
                    {
                        Send_message(clients[i].c_fd, msg);
                        break;
                    }
                }
                // notify to reciever to create the file
                bzero(msg, 1024);
                strcpy(msg, "/newfile ");
                int i = strlen(files[pos].f_path) - 1;
                while( i >= 0 && files[pos].f_path[i] != '/') i--;
                char *name = (char *)&files[pos].f_path[i + 1];
                strcat(msg, name);
                strcat(msg, " ");
                strcat(msg, tokens[1].string);
                Send_message(clients[index].c_fd, msg);
                free(msg);
            }
        }
        else if (strcmp((char*)tokens[0].string, "/reject") == 0)
        {
            if(tok_count != 2) Send_message(clients[index].c_fd, "Invalid command.");
        }
        else if (strcmp((char*)tokens[0].string, "/finish") == 0)
        {
            int f_ID = atoi(tokens[1].string);
            for(int i = 0; i < n_files; i++)
            {
                if(files[i].f_ID == f_ID)
                {
                    for(int j = 0; j < n_clients; j++)
                    {
                        if(clients[j].connected && !strcmp(clients[j].nick_name, files[i].f_sender))
                        {
                            Send_message(clients[j].c_fd, "File recieved successfully.");
                            return;
                        }
                    }                    
                }
            }
            
        }
        

    }

    /*///<summary>
      /// Print an ERROR message
    *///</summary>
    void Error(char *msg)
    {
        printf("ERROR: %s\n", msg);
    }

    /*///<summary>
      /// Accepts a new connection from a client and store it into the list of
      /// connected clients, a standard name is asigned to new client.
      ///<Returns> In ERROR returns -1, otherwise the accept file descriptor is returned. </Returns>
    *///</summary>
    int Accept_new_client()
    {
        // the nick by default will be User_1, User_2,...,User_n
        char *number = malloc(2);
        int len = ToString(n_clients + 1, number);
        char name[10];
        strcpy(name, "User_");
        strncat(name, number, len);
        free(number);
        strcpy(clients[n_clients].nick_name, name);
        //the client file descriptor
        int size = sizeof(struct sockaddr_in);
        int c_fd = accept(listen_fd, (SA *)&clients[n_clients].addr, &size);
        if(c_fd < 0) return -1;
        clients[n_clients].c_fd = c_fd;
        clients[n_clients].connected = 1;
        return c_fd;
    }

    /*///<summary>
      /// Sends a message between the server and a client at fd following the established protocol.
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
      /// Sends a message from server to all connected clients following the established protocol.
    *///</summary>
    void Send_multi_message(char *msg)
    {
        for(int i = 0; i < n_clients; i++)
        {
            if(clients[i].connected)
            {
                Send_message(clients[i].c_fd, msg);
            }
        }        
    }

    /*///<summary>
      /// Close all opened descriptors
    *///</summary>
    void handler(int sig)
    {
        //close all opened descriptors
        Stop();
        _exit(0);
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
      /// Closes al opened descriptors, resets the set of clients and stop the server
    *///</summary>
    void Stop()
    {
        printf("Server has stoped, all connections has been closed.");
        running = 0;
        for(int i = 0; i < n_clients; i++)
        {
            if(clients[i].connected)
            {
                Send_message(clients[i].c_fd, "/stop");
                FD_CLR(clients[i].c_fd, &read_set);
                close(clients[i].c_fd);
                bzero(clients[i].nick_name, 10);
                clients[i].c_fd = -1;
                clients[i].connected = 0;           
            }
        }
        close(listen_fd);
        FD_CLR(listen_fd, &read_set);
        max_fd = STDIN_FILENO;
        int optval = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
        bzero(clients, MAX_CLIENTS_NUMBER*sizeof(client_t));
        bzero(files, 2*MAX_CLIENTS_NUMBER*sizeof(file_t));
        FD_ZERO(&read_set);
        FD_SET(STDIN_FILENO, &read_set);
        listen_fd = -1;
        n_clients = 0;
        n_files = 0;
    }

    /*///<summary>
      /// Adds the char* 'word' to the list of blocked words
      ///<Returns> On ERROR returns -1, otherwise 0 is returned.</Returns>
    *///</summary>
    int block_word(char *word)
    {
        int block = open("block.txt", O_CREAT | O_APPEND | O_RDWR, S_IRWXU);
        if(block < 0) return -1;
        write(block, word, strlen(word));
        write(block, "\n", 1);
        close(block);
        return 0;    
    }

    /*///<summary>
      /// Shows the blocked words
    *///</summary>
    void block()
    {
        int block = open("block.txt", O_RDONLY, S_IRWXU);
        if(block < 0) 
        {
            printf("There are not blocked words.\n");
            return;
        }
        char c;
        while(read(block, &c, 1))
        {
            write(STDOUT_FILENO, &c, 1);
        }
        close(block);
    }

    /*///<summary>
      ///<Returns> 1 if 'word' is blocked and 0 if is not.</Returns>
    *///</summary>
    int is_blocked(char *word)
    {
        token_t token;
        token.tok_len = 0;
        int block = open("block.txt", O_RDONLY, S_IRWXU);
        if(block < 0) return 0;
        char c;
        while(read(block, &c, 1))
        {
            if(c == '\n')
            {
                if(strcmp(token.string, word) == 0) return 1;
                bzero(token.string, 255);
                token.tok_len = 0;                
            }
            else token.string[token.tok_len++] = c;
        }
        close(block);
        return 0;
    }

    /*///<summary>
      /// Ejects the client with ip address IP and do not allows more connections from IP
      ///<Returns> 1 the action was completed and 0 if It was'nt posible.</Returns>
    *///</summary>
    int ban(char* IP)
    {
        int found = 0;
        for(int i = 0; i < n_clients; i++)
        {
            if( clients[i].connected)
            {
                char *c_IP = inet_ntoa(clients[i].addr.sin_addr);
                if(strcmp(IP, c_IP) == 0)
                {
                    found = 1;
                    // eject the client
                    Send_message(clients[i].c_fd, "/kick");
                    FD_CLR(clients[i].c_fd, &read_set);
                    close(clients[i].c_fd);
                    printf( "Client at <%s> has been ejected.\n", IP);
                    bzero(clients[i].nick_name, 10);
                    clients[i].c_fd = -1;
                    clients[i].connected = 0;
                    break;
                }
            }
        }
        if(!found) return 0;

        int ban_fd = open("ban.txt", O_CREAT | O_RDWR, S_IRWXU);
        char c;
        token_t tok;
        tok.tok_len = 0;
        found = 0;
        while (read(ban_fd, &c, 1))
        {
            if(c == ' ') //I have found a new IP
            {
                if(strcmp(tok.string, IP) == 0) // I have found the right IP
                {
                    found = 1;
                    read(ban_fd, &c, 1);
                    if(c == '1') // IP is already baned
                    {
                        close(ban_fd);
                        return 1;
                    }
                    lseek(ban_fd, -1, SEEK_CUR); // move the file descriptor 1 byte to left.
                    write(ban_fd, "1\n", 2);
                    return 1;
                }
            }
            else if(c == '\n')
            {
                bzero(tok.string, tok.tok_len);
                tok.tok_len = 0;
            }
            else tok.string[tok.tok_len++] = c;
        }
        if(!found)
        {
            write(ban_fd, IP, strlen(IP));
            write(ban_fd, " 1\n", 3);
            close(ban_fd);
            return 1;
        }        
        return 0;        
    }

    /*///<summary>
      ///<Returns> 1 if 'IP' has been 'baned' and 0 if It has not.</Returns>
    *///</summary>
    int is_baned(char *IP)
    {
        int ban_fd = open("ban.txt", O_RDONLY, S_IRWXU);
        if(ban_fd < 0) return 0;
        int rd;
        while(rd = ReadLine(ban_fd, buffer))
        {
            Parse(buffer, rd, tokens);
            if(strcmp(IP, tokens[0].string) == 0 && tokens[1].string[0] == '1')
            {
                // I have found it
                close(ban_fd);
                return 1;                
            }
        }
        return 0;
    }

    /*///<summary>
      /// Allows connections from IP again and remove it from list of baned clients.
    *///</summary>
    void unban(char *IP)
    {
        int ban_fd = open("ban.txt", O_RDWR, S_IRWXU);
        if(ban_fd < 0) return;
        token_t tok;
        tok.tok_len = 0;
        char c;
        while(read(ban_fd, &c, 1))
        {
            if(c == ' ')
            {
                if(strcmp(tok.string, IP) == 0)
                {
                    write(ban_fd, "0", 1);
                    close(ban_fd);
                    return;
                }
            }
            else if(c == '\n')
            {
                bzero(tok.string, 255);
                tok.tok_len = 0;
            }
            else tok.string[tok.tok_len++] = c;
        }
        close(ban_fd);
    }

    void logmsg(char *msg)
    {
        int logfd = open("server_log.txt", O_CREAT | O_APPEND | O_RDWR, S_IRWXU);
        write(logfd, msg, strlen(msg));
        write(logfd, "\n", 1);
        close(logfd);
    }
/*******************************************************************************/