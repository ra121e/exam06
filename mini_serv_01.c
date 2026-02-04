#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <stdio.h>

int	sockfd;
fd_set	activefd;
fd_set	readfd;
fd_set	writefd;
int	count;
int	id[FD_SETSIZE];
char	*msgs[FD_SETSIZE];
int	maxfd;

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

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
	char	*newbuf;
	int		len;

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

void	fatal_error(void)
{
	write (2, "Fatal error\n", 12);
	exit (1);
}

void	broadcast(int sender_fd, char *str)
{
	for (int fd = 0; fd < maxfd; ++fd)
	{
		if (FD_ISSET(fd, &writefd) && fd != sender_fd)
			send(fd, str, strlen(str), 0);
	}
}

int	main(int ac, char **av)
{
	if (ac != 2)
	{
		write (2, "Wrong number of arguments\n", 26);
		exit (1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		fatal_error();
	FD_ZERO(&activefd);
	FD_SET(sockfd, &activefd);
	struct sockaddr_in	servaddr;
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		fatal_error();
	if (listen(sockfd, 10) != 0)
		fatal_error();

	maxfd = sockfd;
	while (1)
	{
		readfd = activefd;
		writefd = activefd;

		if (select(maxfd + 1, &readfd, &writefd, NULL, NULL) < 0)
			fatal_error();

		for (int fd = 0; fd <=maxfd; ++fd)
		{
			if (!FD_ISSET(fd, &readfd))
				continue ;
			if (fd == sockfd)
			{
				struct sockaddr_in	clientaddr;
				socklen_t	addrlen;
				int clientfd = accept(fd, (struct sockaddr *)&clientaddr, &addrlen);
				if (clientfd >= 0)
				{
					char buf_write[1001];
					maxfd = clientfd > maxfd ? clientfd : maxfd;
					FD_SET(clientfd, &activefd);
					id[clientfd] = count++;
					msgs[clientfd] = NULL;
					sprintf(buf_write, "server: client %d just arrived\n", id[clientfd]);
					broadcast(clientfd, buf_write);
					break ;
				}

			}
			else
			{

			}
		}
	}
	return (0);
}
