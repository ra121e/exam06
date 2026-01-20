#include <stdlib.h> // atoi
#include <unistd.h> // read, write
#include <sys/select.h> //fd_set
#include <sys/socket.h> // socket, SOMAXCONN
#include <strings.h> // bzero
#include <arpa/inet.h> // hotonl

#include <stdio.h>

int	main(int ac, char **av)
{
	int		port;
	int		sockfd;
	int		max_fd;
	int		clientfd;
	fd_set	activefd;
	fd_set	readfd;
	fd_set	writefd;
	
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

	max_fd = 0;
	while (1)
	{
		readfd = activefd;
		writefd = activefd;

		if (select(max_fd + 1, &readfd, &writefd, NULL, NULL) < 0)
		{
			write (2, "Fatal error\n", 12);
			exit (1);
		}

		printf("max_fd: %d\n", max_fd);

		for (int fd =0; fd <= max_fd; ++fd)
		{
			if (fd == sockfd)
			{
				struct sockaddr_in	clientaddr;
				socklen_t			addrlen;
				addrlen = sizeof (clientaddr);
				clientfd = accept(sockfd, (struct sockaddr *)&servaddr, &addrlen);

				printf("clientfd: %d\n", clientfd);

				if (clientfd < 0)
				{
					// register client
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
				// send message
				write (1, buf_read, read_bytes);
			}
		}
	}
	return (0);
}
