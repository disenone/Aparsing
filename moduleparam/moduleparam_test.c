#include "moduleparam.h"
#include <stdio.h>


static int test = 0;
static bool btest = 1;
static unsigned int latest_num = 0;
static long latest[10] = {0};
static char strtest[10] = "\0";

void usage()
{
    char *msg = "usage: moduleparam_test [test=int] [btest[=bool]] [latest=int array] [strtest=string]\n";
    printf(msg);
}

int main (int argc, char **argv)
{
    init_module_param(10);
    module_param(test, int);
    module_param(btest, bool);
    module_param_array(latest, long, &latest_num);
    module_param_string(strtest, strtest, sizeof(strtest));

    int ret = parse_params(argc - 1, &argv[1], 0);

    if(ret != 0)
    {
        usage();
        return 0;
    }

    char buf[1024];
    for(int i=0; i < MODULE_INIT_VARIABLE_NUM; ++i)
    {
        MODULE_INIT_VARIABLE[i].get(buf, &MODULE_INIT_VARIABLE[i]);
        printf("%s = %s\n", MODULE_INIT_VARIABLE[i].name, buf);
    }
    return 0;
}
