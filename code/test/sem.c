
#include "syscall.h"

void nada();
void algo();
int semaforoID;
int main(){
	
    semaforoID = SemCreate(0);
//	SemSignal ( semaforoID );
//	nada();
	Fork(nada);
	Write("Main\n", 5, 1);
	SemSignal(semaforoID);
	Yield();
//        Fork(algo);
//       Yield();
//	Write("Voy a destruir semaforo\n",15,1);
	SemDestroy(semaforoID);
//	Write("Fin del Programa\n",17,1);
}


void nada(){
   	Write("Entra nada",10,1);
	SemWait(semaforoID); 
	Write( "holaNada", 8, 1 );
    
}
void algo(){
//   	 Write( "hola", 4, 1 );
//     SemSignal( semaforoID );
     
    
}

