
#include "syscall.h"

int
main()
{
    char *  buff = "Hola Mundo \n";
    int id = Open( "nachos.1" );
    Write(buff, 12, id);
    Exit(0);
}

