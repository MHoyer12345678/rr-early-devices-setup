#include "Controller.h"

#include <string.h>

int main(int argc, char *argv[])
{
	int result;
	bool debug_mode=false;

	if (argc > 1)
		debug_mode=(strcmp(argv[1],"-d")==0);

	if (controller_init(debug_mode))
	{
		controller_run();
		result=controller_get_exit_result();
	}
	else
		result=1;

	controller_deinit();

	return result;
}
