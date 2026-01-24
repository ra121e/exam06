#include <stdlib.h> // atoi
#include <unistd.h> // read, write
#include <sys/select.h> //fd_set
#include <sys/socket.h> // socket, SOMAXCONN
#include <strings.h> // bzero
#include <string.h> //strlen
#include <arpa/inet.h> // hotonl

#include <stdio.h>

typedef struct s_client {
	int		id;
	char	*msg;
} t_client;

typedef struct s_server {
	int 		sockfd;
	fd_set		activefd;
	fd_set		readfd;
	fd_set		writefd;
	int			max_fd;
	t_client	clients[65536];
	int			count;
} t_server;

int extract_message(char **buf, char **msg)
{
        char    *newbuf;
        int     i;

        *msg = 0;
        if (*buf == 0)
                return (0);
        i = 0;
        while ((*buf)[i])
        {
                if ((*buf)[i] == '\n')
                {
                        newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
                        if (newbuf == 0)
                                return (-1);
                        strcpy(newbuf, *buf + i + 1);
                        *msg = *buf;
                        (*msg)[i + 1] = 0;
                        *buf = newbuf;
                        return (1);
                }
                i++;
        }
        return (0);
}

char *str_join(char *buf, char *add)
{
        char    *newbuf;
        int             len;

        if (buf == 0)
                len = 0;
        else
                len = strlen(buf);
        newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
        if (newbuf == 0)
                return (0);
        newbuf[0] = 0;
        if (buf != 0)
                strcat(newbuf, buf);
        free(buf);
        strcat(newbuf, add);
        return (newbuf);
}
void	broadcast(t_server *server, int exc_fd, char *str)
{
	for (int fd = 0; fd <= server->max_fd; ++fd)
	{
		if (FD_ISSET(fd, &server->writefd) && fd != exc_fd)
			send(fd, str, strlen(str), 0);
	}
}

void	send_msg(t_server *server, int fd)
{
	char	*msg;

	while (extract_message(&server->clients[fd].msg, &msg))
	{
		char	buf_write[1001];
		sprintf(buf_write, "client %d: %s", server->clients[fd].id, msg);
		broadcast(server, fd, buf_write);
		free(msg);
	}
}

void	init_server(t_server *server)
{
	server->sockfd = -1;
	FD_ZERO(&server->activefd);
	FD_ZERO(&server->readfd);
	FD_ZERO(&server->writefd);
	server->max_fd = -1;
	server->count = 0;
	for (int i = 0; i < 65536; ++i)
	{
		server->clients[i].id = -1;
		server->clients[i].msg = NULL;
	}
}

int	main(int ac, char **av)
{
	int		port;
	int		sockfd;
	int		max_fd;
	int		clientfd;
	fd_set	activefd;
	fd_set	readfd;
	fd_set	writefd;
	int		ids[65536];
	int		count;
	char	buf_write[42];
	char	*msgs[65536];

	t_server *server;
	server = malloc(sizeof (t_server));
	if (!server)
		return (1);

	init_server(server);

	if (ac != 2)
	{
		write (2, "Wrong number of arguments\n", 26);
		exit (1);
	}

	FD_ZERO(&server->activefd);
	server->sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (server->sockfd < 0)
	{
		write (2, "Fatal error\n", 12);
		exit (1);
	}
	FD_SET(server->sockfd, &server->activefd);

	printf("socket fd: %d\n", server->sockfd);

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));

	// Binding newly created socket to given IP and verification
	if ((bind(server->sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
		write (2, "Fatal error\n", 12);
		exit (1);
	}
	if (listen(server->sockfd, SOMAXCONN) != 0) {
		write (2, "Fatal error\n", 12);
		exit (1);
	}

//	server->count = 0;
	server->max_fd = server->sockfd;
//	for (int i = 0; i < 65536; ++i)
//	{
//		ids[i] = 0;
//		msgs[i] = NULL;
//	}
	while (1)
	{
		server->readfd = server->activefd;
		server->writefd = server->activefd;

		if (select(server->max_fd + 1, &server->readfd, &server->writefd, NULL, NULL) < 0)
		{
			write (2, "Fatal error\n", 12);
			exit (1);
		}

		for (int fd = 0; fd <= server->max_fd; ++fd)
		{
			if (!(FD_ISSET(fd, &server->readfd)))
				continue ;

			if (fd == server->sockfd)
			{
				struct sockaddr_in	clientaddr;
				socklen_t			addrlen;
				addrlen = sizeof (clientaddr);
				clientfd = accept(server->sockfd, (struct sockaddr *)&clientaddr, &addrlen);

				printf("clientfd: %d\n", clientfd);

				if (clientfd >= 0)
				{
					// register client
					FD_SET(clientfd, &server->activefd);
					server->max_fd = clientfd > server->max_fd ? clientfd : server->max_fd;
					server->clients[clientfd].id = server->count++;
					sprintf(buf_write, "server: client %d just arrived\n", server->clients[clientfd].id);
					broadcast(server, fd, buf_write);
					break ;
				}
			}
			else
			{
				int	read_bytes;
				char	buf_read[1001];

				read_bytes = recv(fd, buf_read, 1000, 0);
				if (read_bytes <= 0)
				{
					// remove client
					sprintf(buf_write, "server: client %d just left\n", server->clients[fd].id);
					broadcast(server, fd, buf_write);
					free(server->clients[fd].msg);
					FD_CLR(fd, &server->activefd);
					close(fd);
					break ;
				}
				buf_read[read_bytes] = '\0';
				// making message with str_join
				server->clients[fd].msg = str_join(server->clients[fd].msg, buf_read);
				// send message
				send_msg(server, fd);
				write (1, buf_read, read_bytes);
			}
		}
	}
	return (0);
}
