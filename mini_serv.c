#include <stdlib.h>
#include <unistd.h>


int	main(int ac, char **av)
{
	int	port;

	if (ac != 2)
	{
		write (2, "Wrong number of arguments\n", 26);
		exit (1);
	}

	port = atoi(av[1]);
	if (!(port > 1024 && port < 49151))
	{
		write (2, "Fatal error\n", 12);
		return (1);
	}




	return (0);
}
