#include<stdio.h>
#include<stdlib.h>

extern "C" void hello()
{
    printf("Hello from Verdande\n");
}

extern "C" int get_int()
{
    int int_var = 9;
    return int_var;
}

