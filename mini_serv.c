#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

# define HUGER 4096
# define HUGE 4096 * 2

int fd_arr[1024 * 64];
int max_conn = 0;
int client_id = 0;
fd_set active;
fd_set readyRead;
fd_set readyWrite;
char bufRead[HUGER];
char bufWrite[HUGER + 1];
char s[HUGE];

void fatal()
{
    write(2, "Fatal error\n", 12);
    exit(1);
}

void send_to_all(int author)
{
    for (int i = 0; i < max_conn; i++)
    {
        if (FD_ISSET(i, &readyWrite) && i != author)
            send(i, bufWrite, strlen(bufWrite), 0);
    }
}

void wrong_arg_cnt()
{
    write(2, "Wrong number of arguments\n", 26);
    exit(1);
}

int main(int ac, char **av)
{
    if (ac != 2)
        wrong_arg_cnt();


    bzero(&fd_arr, sizeof(fd_arr));
    FD_ZERO(&active);
	// socket create and verification 
	int server_fd = socket(AF_INET, SOCK_STREAM, 0); 
	if (server_fd == -1) { 
        fatal();
	} 
    FD_SET(server_fd, &active);

    max_conn = server_fd;

	// assign IP, PORT 
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr)); 
    socklen_t len = sizeof(servaddr);
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));

    if ((bind(server_fd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
        fatal();

    if (listen(server_fd, 128) != 0)
        fatal();

    while (1)
    {
        readyRead = readyWrite = active;
        if (select(max_conn + 1, &readyRead, &readyWrite, 0 , 0) < 0)
            continue; // or exit?

        for (int x = 0; x <= max_conn; x++)
        {
            if (x == server_fd && FD_ISSET(x, &readyRead)) // new client
            {
                int client_fd = accept(server_fd, (struct sockaddr *)&servaddr, &len);
                if (client_fd < 0)
                    continue;

                max_conn = (client_fd > max_conn) ? client_fd : max_conn;
                fd_arr[client_fd] = client_id++;
                FD_SET(client_fd, &active); // add fd to the active set
                sprintf(bufWrite, "server: client %d just arrived\n", fd_arr[client_fd]);
                send_to_all(x);
                break;
            }
            else if (x != server_fd && FD_ISSET(x, &readyRead))
            {
                int read_bytes = recv(x, bufRead, HUGE, 0);

                if (read_bytes <= 0) // client dropped
                {
                    sprintf(bufWrite, "server client %d just left\n", fd_arr[x]);
                    send_to_all(x);
                    FD_CLR(x, &active); // remove fd from the active set
                    close(x);
                    break;
                }
                else
                {
                    //char s[HUGE];
                    //bzero(&s, sizeof(s));
                    for (int i = 0, j = 0; i < read_bytes; i++, j++)
                    {
                        s[j] = bufRead[i];
                        if (s[j] == '\n') // send to clients line by line
                        {
                            s[j] = '\0';
                            sprintf(bufWrite, "client %d: %s\n", fd_arr[x], s);
                            send_to_all(x);
                            j = -1;
                        }
                    }
                }
            }
        }

    }
}
