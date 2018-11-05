// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "console.h"
#include "synch.h"
#include <sched.h>


void returnFromSystemCall() {

        int pc, npc;

        pc = machine->ReadRegister( PCReg );
        npc = machine->ReadRegister( NextPCReg );
        machine->WriteRegister( PrevPCReg, pc );        // PrevPC <- PC
        machine->WriteRegister( PCReg, npc );           // PC <- NextPC
        machine->WriteRegister( NextPCReg, npc + 4 );   // NextPC <- NextPC + 4

}       // returnFromSystemCall


/*
#define SC_Halt		0
#define SC_Exit		1 - 
#define SC_Exec		2
#define SC_Join		3
#define SC_Create	4
#define SC_Open		5
#define SC_Read		6 -
#define SC_Write	7
#define SC_Close	8
#define SC_Fork		9
#define SC_Yield	10 -	
#define SC_SemCreate	11
#define SC_SemDestroy	12
#define SC_SemSignal	13
#define SC_SemWait	14

*/

void Nachos_Halt() {                    // System call 0
	DEBUG('a', "Shutdown, initiated by user program.\n");
    interrupt->Halt();

}       // Nachos_Halt

void Nachos_Exit(){			//System call 1

	
	//matar archivos
	
	filesTable->delThread();
	if(filesTable->getUsage() == 0){	//ultimo hilo
		for(int i = 0; i<filesTable->getSize(); ++i){	//cierra todos los archivos que hayan quedado abiertos
			if(filesTable->isOpened(i)){
				close(filesTable->getUnixHandle(i));
			}
		}
		//delete filesTable;
	}
		
	//matar semaforos
	
	semTable->delThread();
	if(semTable->getUsage() == 0){	//ultimo hilo
		for(int j = 0; j<semTable->getSize(); ++j){	//cierra todos los semaforos que hayan quedado abiertos
			if(semTable->isActive(j)){
				// elimina el semaforo
				semTable->DelSemaphore(j);
			}
		}
		//delete semTable;
	}
		
	//devolver memoria
	//delete currentThread->space;	
	
	//Revisar si hay alguien que esta esperandome (join)
	int currPid = 	currentThread->pid;
	//guarda return 
	if(semaforosJoin[currPid] != NULL){
		Semaphore* sem = (Semaphore*)semaforosJoin[currPid]; 
		sem->V();
		semaforosJoin[currPid] = NULL;
		exitResult[currPid]=machine->ReadRegister(4);
	}
	
	processId->Clear(currentThread->pid);
   	currentThread->Finish();
	returnFromSystemCall();
}


void NachosExecThread(void * p) { // for 64 bits version
	//ED donde se guarda el lo que viene en el registro 4 para que el hijo pueda accederlo
	int nameDir = reg4[currentThread->pid]; 
	char * filename = new char[100];
	//leer input 
	int num = 99;
	int i = 0;
	while(num != '\0'){	
		machine->ReadMem(nameDir+i , 1, &num);
		filename[i] = (char)num;
		++i;
	}
	printf("Nombre archivo: %s\n", filename);
	OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;

    if (executable == NULL) {
		printf("Unable to open file %s\n", filename);
		return;
    }
    space = new AddrSpace(executable);    
    currentThread->space = space;

    delete executable;			// close file
    delete filename;

    space->InitRegisters();		// set the initial register values
    space->RestoreState();		// load page table register

	machine->Run();			// jump to the user progam
    ASSERT(false);			// machine->Run never returns;
	
}


void Nachos_Exec(){			//System call 2
	//Con procesos
	Thread * newTh = new Thread( "child to execute Exec code" );
	newTh->space = new AddrSpace(currentThread->space);
	reg4[newTh->pid] = machine->ReadRegister(4);
	newTh->Fork(NachosExecThread,(void*)(machine->ReadRegister( 4 )) );
	
	machine->WriteRegister(2, newTh->pid);
	returnFromSystemCall();
}

void Nachos_Join(){		//System call 3
	int newPid = machine->ReadRegister(4);
	int result = 0;
	if(processId->Test(newPid)){
		// esta activo el hilo
			joinSem->P();	
		//current thread crea semaforo y hace wait 
		Semaphore* wait = new Semaphore((char*)(currentThread->getName()), 0);

		semaforosJoin[newPid] = wait;
		joinSem->V();
		wait->P();	
		
		//devuelve el retorno del exit del hilo que estaba esperando
		result = exitResult[newPid];
	}
	machine->WriteRegister(2, result);	
	returnFromSystemCall();
}

void Nachos_Create(){					//System call 4
	int nameDir = machine->ReadRegister(4); 
	char * filename = new char[100];
	//leer input 
	int num = 99;
	int i = 0;
	while(num != '\0'){	
		machine->ReadMem(nameDir+i , 1, &num);
		filename[i] = (char)num;
		++i;
	}

	int result = -1;
	result = creat(filename, 0777);
//	result = open(filename, O_RDWR|O_CREAT, S_IRWXU);
	if(result>0){
		printf("Archivo %s creado\n", filename);
		//close(result);
	}
	machine->WriteRegister(2, result);		
	
	returnFromSystemCall();
}

void Nachos_Open() {                    // System call 5
/* System call definition described to user
	int Open(
		char *name	// Register 4
	);
*/
	// Read the name from the user memory, see 5 below
	// Use NachosOpenFilesTable class to create a relationship
	// between user file and unix file
	// Verify for errors
	printf("OPEN\n");
	char filename[100];
    int i=0;
	int memval = 99;
    int vaddr = machine->ReadRegister(4);
    machine->ReadMem(vaddr, 1, &memval);
    while ((*(char*)&memval) != '\0') {
        filename[i]  = (char)memval;
        ++i;
        vaddr++;
        machine->ReadMem(vaddr, 1, &memval);
    }
	filename[i] = (char)memval;
	int result = -1;
	int unixHandle = open(filename, O_RDWR);	//abre el archivo y guarda el identificador
	if(unixHandle != -1){
		result = filesTable->Open(unixHandle);
		printf("Archivo %s abierto. Id %d\n", filename,  result);
	}
	
	machine->WriteRegister(2, result);
	
	returnFromSystemCall();		// Update the PC registers
}       // Nachos_Open


void Nachos_Read(){							//system call 6
	int bufDir = machine->ReadRegister(4); 	//buffer       
    int size = machine->ReadRegister(5);	// Read size to read
   	int id = machine->ReadRegister(6);	// Read file descriptor
   	int bytesRead = 0;
   	
	printf("ID donde leer: %d\n", id);
   	if(filesTable->isOpened(id)){
   		int idUnix = filesTable->getUnixHandle(id);
		bytesRead = read(idUnix, &bufDir, size);
		printf("Lee %d bytes\n", bytesRead);	
   	}
   	
	machine->WriteRegister(2, bytesRead);
	printf("termina read\n");
	returnFromSystemCall();
}

void Nachos_Write() {                   		// System call 7
/* System call definition described to user
        void Write(
		char *buffer,	// Register 4
		int size,	// Register 5
		 OpenFileId id	// Register 6
	);
*/
	int bufDir = machine->ReadRegister(4);        
    int size = machine->ReadRegister(5);	// Read size to write
   	int id = machine->ReadRegister( 6 );	// Read file descriptor
	char * buffer = new char[size];
	printf("ID donde escribir: %d\n", id);
	//leer input 
	int num;
	for(int i =0; i<size; ++i){	
		machine->ReadMem(bufDir+i , 1, &num);
		buffer[i] = num;
	}
	
	// Need a semaphore to synchronize access to console
	consoleSem->P();
	switch (id) {
		case  ConsoleInput:	// User could not write to standard input
			machine->WriteRegister( 2, -1 );
			break;
		case  ConsoleOutput:
			buffer[ size ] = 0;
			printf( "%s", buffer );
		break;
		case ConsoleError:	// This trick permits to write integers to console
			printf( "%d\n", machine->ReadRegister( 4 ) );
			break;
		default:	// All other opened files
			// Verify if the file is opened, if not return -1 in r2
			// Get the unix handle from our table for open files
			// Do the write to the already opened Unix file
			// Return the number of chars written to user, via r2
			if(filesTable->isOpened(id)){
				int unixHandle = filesTable->getUnixHandle(id);
				int result = write(unixHandle, buffer, size);
				machine->WriteRegister(2, result);
				printf("Se escribieron %d bytes en el archivo.\n", result);
			}else{
				machine->WriteRegister( 2, -1 );
			}
			
			break;

	}
	// Update simulation stats, see details in Statistics class in machine/stats.cc
	consoleSem->V();

    returnFromSystemCall();		// Update the PC registers

}       // Nachos_Write

void Nachos_Close(){		//System call 8
	int fileId = machine->ReadRegister(4);
	int unixId = filesTable->getUnixHandle(fileId);
	close(unixId);
	filesTable->Close(fileId);
	returnFromSystemCall();
}


// Pass the user routine address as a parameter for this function
// This function is similar to "StartProcess" in "progtest.cc" file under "userprog"
// Requires a correct AddrSpace setup to work well

void NachosForkThread( void * p ) { // for 64 bits version
    AddrSpace *space;

    space = currentThread->space;
    space->InitRegisters();             // set the initial register values
    space->RestoreState();              // load page table register

// Set the return address for this thread to the same as the main thread
// This will lead this thread to call the exit system call and finish
    machine->WriteRegister( RetAddrReg, 4 );

    machine->WriteRegister( PCReg, (long) p );
    machine->WriteRegister( NextPCReg, (long)p + 4 );

    machine->Run();                     // jump to the user progam
    ASSERT(false);
}



void Nachos_Fork() {			// System call 9
	DEBUG( 'u', "Entering Fork System call\n" );
	// We need to create a new kernel thread to execute the user thread
	Thread * newT = new Thread( "child to execute Fork code" );
	
	// We need to share the Open File Table structure with this new child

	// Child and father will also share the same address space, except for the stack
	// Text, init data and uninit data are shared, a new stack area must be created
	// for the new child
	// We suggest the use of a new constructor in AddrSpace class,
	// This new constructor will copy the shared segments (space variable) from currentThread, passed
	// as a parameter, and create a new stack for the new child
	newT->space = new AddrSpace( currentThread->space );

	// We (kernel)-Fork to a new method to execute the child code
	// Pass the user routine address, now in register 4, as a parameter
	// Note: in 64 bits register 4 need to be casted to (void *)
	newT->Fork(NachosForkThread,(void*)(machine->ReadRegister( 4 )) );

	returnFromSystemCall();	// This adjust the PrevPC, PC, and NextPC registers

	DEBUG( 'u', "Exiting Fork System call\n" );
}	// Kernel_Fork


void Nachos_Yield(){	//System call 10
	/*int r = sched_yield();
	machine->WriteRegister(2, r);
	*/
	currentThread->Yield();
	returnFromSystemCall();
}


void Nachos_SemCreate(){			//System call 11
	int ini = machine->ReadRegister(4);
	int ret = semTable->CreateSemaphore(ini);
	printf("Crea semaforo %d\n", ret);
	machine->WriteRegister(2, ret);
	returnFromSystemCall();
}


void Nachos_SemDestroy(){		//System call 12
	int semId = machine->ReadRegister(4);
	int ret = semTable->DelSemaphore(semId);
	printf("Destruye semaforo\n");
	machine->WriteRegister(2, ret);
	returnFromSystemCall();
}


void Nachos_SemSignal(){		//System call 13
	int ini = machine->ReadRegister(4);
	int ret = semTable->signalSem(ini);
	printf("Signal\n");
	machine->WriteRegister(2, ret);
	returnFromSystemCall();
}



void Nachos_SemWait(){		//System call 14
	int ini = machine->ReadRegister(4);
	int ret = semTable->waitSem(ini);
	printf("Wait\n");
	machine->WriteRegister(2, ret);
	returnFromSystemCall();
}

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions s
//	are in machine.h.
//----------------------------------------------------------------------


void ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
	
    switch ( which ) {
		
       case SyscallException:
          switch ( type ) {
             case SC_Halt:
                Nachos_Halt();             // System call #0
                break;
             case SC_Exit:					// System call #1
             	//devolver memoria
				Nachos_Exit();
             	break;
             case SC_Exec:
             	Nachos_Exec();
             	break;
             case SC_Join:
             	Nachos_Join();
             	break;
             case SC_Create:				
                Nachos_Create();             // System call #4
                break;
             case SC_Open:
                Nachos_Open();             // System call #5
                break;
             case SC_Read:
                Nachos_Read();             // System call #6
                break;
             case SC_Write:
                Nachos_Write();             // System call #7
                break;
             case SC_Close:
                Nachos_Close();             // System call #
                break;
             case SC_Fork:					//System call #9
             	Nachos_Fork();
             	break;
             case SC_Yield:
             	Nachos_Yield();
             	break;
             case SC_SemCreate:
             	Nachos_SemCreate();
             	break;
             case SC_SemDestroy:
             	Nachos_SemDestroy();
             	break;
             case SC_SemSignal:
             	Nachos_SemSignal();
             	break;
             case SC_SemWait:
             	Nachos_SemWait();
             	break;
             default:
                printf("Unexpected syscall exception %d\n", type );
                ASSERT(false);
                break;
          }
       break;
       default:
          printf( "Unexpected exception %d\n", which );
          ASSERT(false);
          break;
    }
}

