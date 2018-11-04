#include "syscall.h"

void nada(int);
int id;

int main(){
	Fork(nada);
	Write("basura", 6, 1);
}


void nada(int dummy){
	Write( "hola, nuevo fork2\n", 4, 1 );
}

