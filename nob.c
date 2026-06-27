#define NOB_IMPLEMENTATION
#include "nob.h"

#define BUILD_FOLDER "build/"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    shift(argv, argc);

    if (!mkdir_if_not_exists(BUILD_FOLDER)) return 1;

    Cmd cmd = {0};
    cmd_append(&cmd, "gcc", "-Wall", "-Wextra", "-O2", "-o", BUILD_FOLDER"main", "main.c");
    if (!cmd_run(&cmd)) return 1;
    
    if (argc > 0 && strcmp(shift(argv, argc), "run") == 0) {
	Cmd run = {0};
	cmd_append(&run, BUILD_FOLDER"main");
	while (argc > 0) cmd_append(&run, shift(argv, argc));
	if (!cmd_run_sync(run)) return 1;
    }
    
    return 0;
}
