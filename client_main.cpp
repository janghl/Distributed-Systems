#include "client_utils.h"
#include "controller.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: client pattern num_machines\n");
        return 1;
    }

    char *pattern = argv[1];
    size_t num_machines;
    sscanf(argv[2], "%zu", &num_machines);
    
    Controller controller{pattern, num_machines};
    controller.DistributedGrep();
    return 0;
}