#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "hcq.h"
#include "hcq_server.h"

#ifndef PORT
#define PORT 56535
#endif
#define MAX_BACKLOG 5
#define BUF_SIZE 30


/* Use global variables so we can have exactly one TA list, one student list, one course list,
 * one clients linked list, and one all_fds.
 */
Client *clients = NULL;
Ta *ta_list = NULL;
Student *stu_list = NULL;
Course *courses;
int num_courses = 0;
fd_set all_fds;


/* Instantiate the course list and the client linked list.
 */
void configure_lists() {
    // Configure the list of available courses for the help centre queue.
    num_courses = config_course_list(&courses, NULL);
    
    // Instantiate the clients linked list.
    clients = malloc(sizeof(Client));
	if (clients == NULL) {
		error_message("malloc for head of clients linked list");
	}
    clients->sock_fd = -1;
    clients->name = NULL;
    clients->role = NULL;
    clients->course = NULL;
    clients->next = NULL;
    
    (clients->buf)[0] = '\0';
    clients->inbuf = 0;
    clients->room = sizeof(clients->buf);  // How many bytes remaining in buffer?
    clients->after = clients->buf;       // Pointer to position after the data in buf.
}


/* Print the perror message for system calls and terminates the program.
 */
void error_message(char *msg) {
    perror(msg);
    exit(1);
}


/* Write the message str to the client at client_fd and close the client if the writing was unsuccessful.
 */
void write_and_clean(int client_fd, char *str) {
    int writing = write(client_fd, str, strlen(str));
    if (writing != strlen(str)) {
        Client *client = clients;
        while (client != NULL) {
            if (client->sock_fd == client_fd) {
                cleanup(client);
                break;
            }
            client = client->next;
        }
    }
}


/* Free the memory, reset the client so each field has its initial values when instantiation,
 * and close the client from connecting to this server.
 */
void cleanup(Client *client) {
	if (client->role != NULL) {	
		if (strcmp(client->role, "S") == 0) {
			give_up_waiting(&stu_list, client->name);
		} else if (strcmp(client->role, "T") == 0) {
			remove_ta(&ta_list, client->name);
		}
		free(client->role);
	}
	if (client->name != NULL) {	
		free(client->name);
	}
	if (client->course != NULL) {
		free(client->course);
	}
	FD_CLR(client->sock_fd, &all_fds);
	close(client->sock_fd);
	client->sock_fd = -1;
	client->name = NULL;
	client->role = NULL;
	client->course = NULL;

	(client->buf)[0] = '\0';
    client->inbuf = 0;
    client->room = sizeof(client->buf);  // How many bytes remaining in buffer?
    client->after = client->buf;      // Pointer to position after the data in buf.
}


/* Search the first n characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found.
 * Definitely do not use strchr or other string functions to search here. (Why not?)
 */
int find_network_newline(const char *buf, int n) {
    for (int i = 0; i < n-1; i++) {
        if (buf[i] == '\r') {
            if (buf[i+1] == '\n') {
                return i+2;
            }
        }
    }
    return -1;
}


/* Accept a connection. Note that a new file descriptor is created for
 * communication with the client. The initial socket descriptor is used
 * to accept connections, but the new socket is used to communicate.
 * Return the new client's file descriptor or -1 on error.
 */
int accept_connection(int fd) {
    int client_fd = accept(fd, NULL, NULL);
    if (client_fd < 0) {
        perror("server: accept");
        close(fd);
        exit(1);
    }
	Client *prev = NULL;
	Client *client = clients;	
	while (client != NULL) {
		if (client->sock_fd == -1) {
			client->sock_fd = client_fd;
			return client_fd;
		}
		prev = client;
		client = client->next;
	}
	client = malloc(sizeof(Client));
	if (client == NULL) {
		error_message("malloc for connecting a new client");
	}
	client->sock_fd = client_fd;
	client->name = NULL;
	client->role = NULL;
	client->course = NULL;
	client->next = NULL;

	(client->buf)[0] = '\0';
    client->inbuf = 0;
    client->room = sizeof(client->buf);  // How many bytes remaining in buffer?
    client->after = client->buf;       // Pointer to position after the data in buf.
	if (prev != NULL) {
		prev->next = client;
	} else {
		clients = client;
	}
    return client_fd;
}


/* Read a message from the client and execute a client's command according to whether the client is a Student of a TA.
 */
void read_from(Client *client) {
    int client_fd = client->sock_fd;
    char *buf = client->buf;
    int num_read;
    if ((num_read = read(client_fd, client->after, client->room)) > 0) {
        // Step 1: update inbuf (how many bytes were just added?)
        client->inbuf += num_read;
        int where;
        
        // Step 2: the loop condition below calls find_network_newline
        // to determine if a full line has been read from the client.
        // Your next task should be to implement find_network_newline
        // (found at the bottom of this file).
        //
        // Note: we use a loop here because a single read might result in
        // more than one full line.
        while ((where = find_network_newline(buf, client->inbuf)) > 0) {
            // Step 3: Okay, we have a full line.
            // Output the full line, not including the "\r\n",
            // using print statement below.
            // Be sure to put a '\0' in the correct place first;
            // otherwise you'll get junk in the output.
            buf[where-2] = '\0';
	        if (client->name == NULL) {
                // read this client's name
				client->name = malloc(sizeof(char)*BUF_SIZE);
				if (client->name == NULL) {
					error_message("malloc for client name");
				}
    			(client->name)[0] = '\0';
    			strcpy(client->name, buf);
                char *str = "Are you a TA or a Student (enter T or S)?\r\n";
				write_and_clean(client_fd, str);
			} else if (client->role == NULL) {
                // read this client's role
				client->role = malloc(sizeof(char)*BUF_SIZE);
				if (client->role == NULL) {
					error_message("malloc for client role");
				}
    			(client->role)[0] = '\0';
				strcpy(client->role, buf);
				if (strcmp(client->role, "S") == 0) {
                    char *str = "Valid courses: CSC108, CSC148, CSC209\r\nWhich course are you asking about?\r\n";
					write_and_clean(client_fd, str);
				} else if (strcmp(client->role, "T") == 0) {
                    char *str = "Valid commands for TA:\r\n\t\tstats\r\n\t\tnext\r\n\t\t(or use Ctrl-C to leave)\r\n";
					add_ta(&ta_list, client->name);
					write_and_clean(client_fd, str);
				} else {
                    char *str ="Invalid role (enter T or S)\r\n";
					write_and_clean(client_fd, str);
					free(client->role);
					client->role = NULL;
				}
		    } else if (strcmp(client->role, "S") == 0 && client->course == NULL) {
                // read in this Student's course
				if (strcmp(buf, "CSC108") != 0 && strcmp(buf, "CSC148") != 0 && strcmp(buf, "CSC209") != 0) {
                    char *str = "This is not a valid course. Good-bye.\r\n";
					write_and_clean(client_fd, str);
					cleanup(client);
					return;
				}
				client->course = malloc(sizeof(char)*BUF_SIZE);
				if (client->course == NULL) {
					error_message("malloc for client course");
				}
				(client->course)[0] = '\0';
				strcpy(client->course, buf);     
                if (add_student(&stu_list, client->name, client->course, courses, num_courses)) {
                    char *err_str = "You are already in the queue and cannot be added again for any course. Good-bye.\r\n";
                    write_and_clean(client_fd, err_str);
					cleanup(client);
					return;
				}
				char *str = "You have been entered into the queue. While you wait, you can use the command stats to see which TAs are currently serving students.\r\n";
				write_and_clean(client_fd, str);
			} else if (strcmp(client->role, "T") == 0) {
                // execute valid commands for a TA
				if (strcmp(buf, "stats") == 0) {
					char *str = print_full_queue(stu_list);
					write_and_clean(client_fd, str);
					free(str);
				} else if (strcmp(buf, "next") == 0) {
					next_overall(client->name, &ta_list, &stu_list);
                    Ta *curr_ta = find_ta(ta_list, client->name);
					Student *stu = curr_ta->current_student;
					if (stu != NULL) {
						Client *my_student_client = clients;
						while (my_student_client != NULL) {
							if (my_student_client->name != NULL && strcmp(my_student_client->name, stu->name) == 0 && strcmp(my_student_client->role, "S") == 0) {
		                        char *str = "Your turn to see the TA.\r\nWe are disconnecting you from the server now. Press Ctrl-C to close nc\r\n";		                        
								write_and_clean(my_student_client->sock_fd, str);
								cleanup(my_student_client);
								break;
							}
							my_student_client = my_student_client->next;
						}
					}
				} else {
                    char *str = "Incorrect syntax\r\n";
                    write_and_clean(client_fd, str);
				}
			} else if (strcmp(client->role, "S") == 0) {
                // execute valid commands for a Student
				if (strcmp(buf, "stats") == 0) {
                    char *str = print_currently_serving(ta_list);
                    write_and_clean(client_fd, str);
                    free(str);
				} else {
					char *str = "Incorrect syntax\r\n";
                    write_and_clean(client_fd, str);
				}
			}
            // Step 4: update inbuf and remove the full line from the buffer
            // There might be stuff after the line, so don't just do inbuf = 0.
            client->inbuf = client->inbuf - where;
            // You want to move the stuff after the full line to the beginning
            // of the buffer.  A loop can do it, or you can use memmove.
            // memmove(destination, source, number_of_bytes)
            memmove(buf, &(buf[where]), client->inbuf);
        }
        // Step 5: update after and room, in preparation for the next read.
        client->room = BUF_SIZE - client->inbuf;
        client->after = &(buf[client->inbuf]);
        
        // Step 6: If the input is longer than BUF_SIZE, we close this client.
        if (client->room == 0) {
            cleanup(client);
        }
    } else if (num_read == -1) {
		error_message("read in read_from");
	} else if (num_read == 0) {
        // The client has closed and we are not reading anything.
        cleanup(client);
    }
}


/* Runs the server for the help centre queue and
 */
int main(void) {
    // Instantiate the global variables
    configure_lists();
	
    // Create the socket FD.
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("server: socket");
        exit(1);
    }

    // Set information about the port (and IP) we want to be connected to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    // This should always be zero. On some systems, it won't error if you
    // forget, but on others, you'll get mysterious errors. So zero it.
    memset(&server.sin_zero, 0, 8);
    
    // Setup so the port will be released as soon as the server process terminates.
    int on = 1;
    int status = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on));
    if(status == -1) {
        perror("setsockopt -- REUSEADDR");
    }

    // Bind the selected port to the socket.
    if (bind(sock_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("server: bind");
        close(sock_fd);
        exit(1);
    }

    // Announce willingness to accept connections on this socket.
    if (listen(sock_fd, MAX_BACKLOG) < 0) {
        perror("server: listen");
        close(sock_fd);
        exit(1);
    }

    // The client accept - message accept loop. First, we prepare to listen to multiple
    // file descriptors by initializing a set of file descriptors.
    int max_fd = sock_fd;
    FD_ZERO(&all_fds);
    FD_SET(sock_fd, &all_fds);

    while (1) {
        // select updates the fd_set it receives, so we always use a copy and retain the original.
        fd_set listen_fds = all_fds;
        int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        if (nready == -1) {
            perror("server: select");
            exit(1);
        }

        // Is it the original socket? Create a new connection ...
        if (FD_ISSET(sock_fd, &listen_fds)) {
            int client_fd = accept_connection(sock_fd);
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
            FD_SET(client_fd, &all_fds);
            char *str = "Welcome to the Help Centre, what is your name?\r\n";
            write_and_clean(client_fd, str);
        }

        // Next, check the clients.
        // NOTE: We could do some tricks with nready to terminate this loop early.
        Client *client = clients;
		while(client != NULL) {
            if (client->sock_fd > -1 && FD_ISSET(client->sock_fd, &listen_fds)) {
				read_from(client);
            }
			client = client->next;
        }
    }

    // Should never get here.
    return 1;
}
