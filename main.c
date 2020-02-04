#include <vex.h>

int main(int argc, char **argv)
{
    struct proxy_config *proxy;

    if (argc < 4) usage(argv[0]);

    if (!(proxy = init_proxy(argc, argv)))
        return EXIT_FAILURE;

    event_loop(proxy);

    free_proxy_config(proxy);

    return EXIT_SUCCESS;
}
