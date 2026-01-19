#include <stdlib.h> // atoi
#include <unistd.h> // read, write
#include <sys/select.h> //fd_set
#include <sys/socket.h> // socket, SOMAXCONN

int	main(int ac, char **av)
{
	int	port;
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


	struct sockaddr_in servaddr; 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(av[1]); 

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		write (2, "Fatal error\n", 12);
		exit (1);
	} 
	if (listen(sockfd, SOMAXCONN) != 0) {
		write (2, "Fatal error\n", 12);
		exit (1);
	}


	return (0);
}
