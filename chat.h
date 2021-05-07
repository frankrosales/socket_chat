
/*******************Gloval variables********************/

    #define MAX_CLIENTS_NUMBER  20

    #define MAX_TOKENS_NUMBER 4
    
/*******************************************************/

/***********************Structures**********************/

    typedef struct sockaddr SA;

    typedef struct client
    {
        // the name of the client
        char nick_name[10];
        // the address information
        struct sockaddr_in addr;
        // the client file descriptor asigned in the fd_set
        int c_fd;
        //If client is connected or he's not
        int connected;
    } client_t;

    typedef struct token
    {
        char string[255];
        // the actual length of the token
        int tok_len; 
    } token_t;

    typedef struct file
    {
        int f_ID;  // the ID asigned by server
        int f_fd;  // the file descrptor to read from the file
        int f_size;  // the number of bytes of the file
        int f_count; // the current number of parts which has been sended
        int f_sending; // 1 means tha this file is been sending now and 0 It's not
        char f_sender[10]; // the nick name of the sender client
        char f_reciever[10]; // the nick name of the client who will recieve the file
        char f_path[1024]; // the absolute path of the file
    } file_t;

/*******************************************************/

/***********************Functions***********************/

    int ReadLine(int fd, char *buffer);

    int Parse(char *line, int buf_len, token_t *tokens);   

    void Error(char* msg);

    int Start(int port);

    void Stop();

    int Accept_new_client();

    int Send_message(int fd, char *msg);

    void Send_multi_message(char *msg);

    void handler(int sig);

    int ToString(int n, char* string);

    int Connect(char *hostname, int port);

    int ReadMessage(int fd, char *buffer);

    void Check_client(int index);

    void PrintLine(char *line);

    int block_word(char *word);

    void block();

    int is_blocked(char *word);

    int ban(char *IP);

    int is_baned(char *IP);

    void unban(char *IP);

    int Send_Files();

    int Get_File_Size(char *path);

    void logmsg(char *msg);

/*******************************************************/