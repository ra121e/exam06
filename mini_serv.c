#include <stdlib.h> // atoi
#include <unistd.h> // read, write
#include <sys/select.h> //fd_set
#include <sys/socket.h> // socket, SOMAXCONN
#include <strings.h> // bzero
#include <string.h> //strlen
#include <arpa/inet.h> // hotonl

#include <stdio.h>

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
void	broadcast(int exc_fd, int max_fd, fd_set *wfds, int listen_fd, char *str)
{
	for (int fd = 0; fd <= max_fd; ++fd)
	{
		if (FD_ISSET(fd, wfds) && fd != exc_fd && fd != listen_fd)
			send(fd, str, strlen(str), 0);
	}
}

void	send_msg(int fd, int id, int sockfd, char **buf, fd_set *activefd, int max_fd)
{
	char	*msg;

	while (extract_message(buf, &msg))
	{
		char	buf_write[1001];
		sprintf(buf_write, "client %d: %s", id, msg);
		broadcast(fd, max_fd, activefd, sockfd, buf_write);
		free(msg);
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

	if (ac != 2)
	{
		write (2, "Wrong number of arguments\n", 26);
		exit (1);
	}

	port = atoi(av[1]);
	if (!(port > 1024 && port < 49151))
	{
		write (2, "Fatal error\n", 12);
		exit (1);
	}

	FD_ZERO(&activefd);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		write (2, "Fatal error\n", 12);
		exit (1);
	}
	FD_SET(sockfd, &activefd);

	printf("socket fd: %d\n", sockfd);

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
		write (2, "Fatal error\n", 12);
		exit (1);
	}
	if (listen(sockfd, SOMAXCONN) != 0) {
		write (2, "Fatal error\n", 12);
		exit (1);
	}

	count = 0;
	max_fd = sockfd;
	while (1)
	{
		readfd = activefd;
		writefd = activefd;

//		printf("max_fd: %d\n", max_fd);

		if (select(max_fd + 1, &readfd, &writefd, NULL, NULL) < 0)
		{
			write (2, "Fatal error\n", 12);
			exit (1);
		}

//		printf("max_fd: %d\n", max_fd);

		for (int fd = 0; fd <= max_fd; ++fd)
		{
			if (!(FD_ISSET(fd, &readfd)))
				continue ;

			if (fd == sockfd)
			{
				struct sockaddr_in	clientaddr;
				socklen_t			addrlen;
				addrlen = sizeof (clientaddr);
				clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &addrlen);

				printf("clientfd: %d\n", clientfd);

				if (clientfd >= 0)
				{
					// register client
					FD_SET(clientfd, &activefd);
					max_fd = clientfd > max_fd ? clientfd : max_fd;
					ids[clientfd] = count++;
					sprintf(buf_write, "server: client %d just arrived\n", ids[clientfd]);
					broadcast(fd, max_fd, &writefd, sockfd, buf_write);
					break ;
				}
			}
			else
			{
				int	read_bytes;
				char	buf_read[1001];

				read_bytes = recv(fd, buf_read, 1000, 0);
				if (read_bytes < 0)
				{
					// remove client
					break ;
				}
				buf_read[read_bytes] = '\0';
				// making message with str_join
				msgs[fd] = str_join(msgs[fd], buf_read);
				// send message
				send_msg(fd, ids[fd], sockfd, &msgs[fd], &activefd, max_fd);
				write (1, buf_read, read_bytes);
			}
		}
	}
	return (0);
}
