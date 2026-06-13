#include <stdio.h>
#include <stdlib.h>

extern "C" void hello_() {
    printf("Hello from Verdande\n");
}

extern "C" int get_int_() {
    int int_var = 10;
    return int_var;
}


