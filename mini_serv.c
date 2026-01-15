#include <unistd.h>


int	main(int ac, char **av)
{
	if (ac != 2)
	{
		write (2, "Wrong number of arguments\n", 26);
	}

	return (0);
}
