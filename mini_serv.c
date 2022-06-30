#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

int fd_arr[999];
int max_conn = 0;
int client_id = 0;
fd_set active;
fd_set readyRead;
fd_set readyWrite;
char bufRead[9999];
char bufWrite[9999];

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

void wrong_arg_cont()
{
    write(2, "Wrong number of arguments\n", 26);
    exit(1);
}

int main(int ac, char **av)
{
    if (ac != 2)
        wrong_arg_cnt();

    int server_fd;
	struct sockaddr_in servaddr;

	// socket create and verification 
	server_fd = socket(AF_INET, SOCK_STREAM, 0); 
	if (server_fd == -1) { 
        fatal();
	} 

	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));

    if ((bind(server_fd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
        fatal();

    if (listen(server_fd, 128) != 0)
        fatal();

    FD_ZERO(&active);
    FD_SET(server_fd, &active);
    bzero(&fd_arr, sizeof(fd_arr));
    
    while (1)
    {
        readyRead = readyWrite = active;
        if (select(max_conn + 1, &readyRead, &readyWrite, 0 , 0) < 0)
            continue; // or exit?
        
        for (int x = 0; x <= max_conn; x++)
        {
            if (FD_ISSET(x, &readyRead))
            {
                if (x == server_fd) // new connection
                {
                    int client_fd = accept(server_fd, (struct sockaddr *)&servaddr, sizeof(servaddr));
                    if (client_fd < 0)
                        continue;

                    fd_arr[client_fd] = client_id++;
                    FD_SET(client_fd, &active); // add fd to the active set
                    // server: client %d just arrived\n"
                    break;
                }
                if (x != server_fd) // read
                {
                    int read_bytes = recv(x, bufRead, 9999, 0);

                    if (read_bytes <= 0) // client dropped
                    {
                        FD_CLR(x, &active); // remove fd from the active set
                        // server: client %d just left\n
                        close(x);
                        break;
                    }
                    else
                    {
                        char s[9999];
                        bzero(&s, sizeof(s));
                        for (int i = 0, j = 0; i < read_bytes; i++, j++) // send line for line
                        {
                            s[j] = bufRead[i];
                            if (s[j] == '\n')
                            {
                                s[j] = '\0';
                                // send to others
                                j = -1;
                            }
                        }
                    }
                }
            }
        }
    }
}
