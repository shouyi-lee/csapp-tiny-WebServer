#include "config.h"
#include "server.h"
#include "sigset.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stdout, "usage: %s <port>\n", argv[0]);
        return -1;
    }
    init_signo();

    serve(argv[1]);
}