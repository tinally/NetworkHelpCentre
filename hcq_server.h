#ifndef HCQ_SERVER_H
#define HCQ_SERVER_H

#define BUF_SIZE 30


/* Clients are kept in this data structure for its sock_fd, name, role, and course if the client is a Student. */
typedef struct client {
    int sock_fd;
    char *name;
    char *role;
    char *course;
    struct client *next;
    
    char buf[BUF_SIZE];
    int inbuf;
    int room;  // How many bytes remaining in buffer?
    char *after;       // Pointer to position after the data in buf
} Client;


// instantiate the course list and the client linked list
void configure_lists();

// print the perror message for system calls and terminates the program
void error_message(char *msg);

// Write the message str to the client at client_fd and close the client if the writing was unsuccessful.
void write_and_clean(int client_fd, char *str);

// Free the memory, reset the client so each field has its initial values when instantiation,
// and close the client from connecting to this server.
void cleanup(Client *client);

// Search the first n characters of buf for a network newline (\r\n).
int find_network_newline(const char *buf, int n);

//  Accept a connection.
int accept_connection(int fd);

// Read a message from the client and execute a client's command according to whether the client
// is a Student of a TA.
void read_from(Client *client);

#endif
