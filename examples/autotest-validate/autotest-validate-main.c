#include <stdio.h>
#include "autotest-validate.h"

const char * git_username = "david68cu";

int main(int argc, char**argv)
{
    printf("this_function_returns_true returned %s\n",
                this_function_returns_true() ? "true" : "false");
    printf("this_function_returns_false returned %s\n",
                this_function_returns_false() ? "true" : "false");
    printf("my_username %s\n", my_username() == git_username ? "true" : "false");
}
