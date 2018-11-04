// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create several threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustrate the inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.
//

#include <unistd.h>
#include <stdlib.h>
#include "copyright.h"
#include "system.h"
#include "dinningph.h"
#include "synch.h"

DinningPh * dp;

void Philo( void * p ) {

    int eats, thinks;
    long who = (long) p;

    currentThread->Yield();

    for ( int i = 0; i < 10; i++ ) {

        printf(" Philosopher %ld will try to pickup sticks\n", who + 1);

        dp->pickup( who );
        dp->print();
        eats = Random() % 6;

        currentThread->Yield();
        sleep( eats );

        dp->putdown( who );

        thinks = Random() % 6;
        currentThread->Yield();
        sleep( thinks );
    }

}

Semaphore * semH;
Semaphore * semO;
//----------------------------------------------------------------------
// SimpleThread
// 	Loop 10 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"name" points to a string with a thread name, just for
//      debugging purposes.
//----------------------------------------------------------------------

void
SimpleThread(void* name)
{
    // Reinterpret arg "name" as a string
    char* threadName = (char*)name;
    
    // If the lines dealing with interrupts are commented,
    // the code will behave incorrectly, because
    // printf execution may cause race conditions.
    for (int num = 0; num < 10; num++) {
        //IntStatus oldLevel = interrupt->SetLevel(IntOff);
		printf("*** thread %s looped %d times\n", threadName, num);
		//interrupt->SetLevel(oldLevel);
        //currentThread->Yield();
    }
    
    //IntStatus oldLevel = interrupt->SetLevel(IntOff);
    printf(">>> Thread %s has finished\n", threadName);
    //interrupt->SetLevel(oldLevel);
}

//agua
int cantO = 0;
int cantH = 0;

void O(void* name){
	if(cantH>1){ //puede crear agua
		printf("Creando agua desde O\n");
		cantH -= 2;
		semH->V();
		semH->V();
	}else{
		cantO++;
		semO->P();
	}
	printf("Termino oxigeno\n");
}

void H(void* name){
	if(cantH>0 && cantO > 0){ //puede crear agua
		printf("Creando agua desde H\n");
		cantH -= 1;
		cantO -= 1;
		semO->V();
		semH->V();
	}else{
		cantH++;
		semH->P();
	}
	printf("Termino hidrogeno\n");
}



//----------------------------------------------------------------------
// ThreadTest
// 	Set up a ping-pong between several threads, by launching
//	ten threads which call SimpleThread, and finally calling 
//	SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest()
{
    Thread * Ph;

    DEBUG('t', "Entering SimpleTest");

/*
    dp = new DinningPh();

    for ( long k = 0; k < 5; k++ ) {
        Ph = new Thread( "dp" );
        Ph->Fork( Philo, (void *) k );
    }

    return;
*/

	semH = new Semaphore("Sem1", 0);
	semO = new Semaphore("Sem2", 0);
	//sem->P(); es wait
	//sem->V(); es signal

/*
    for ( int k=1; k<=6; k++) {
      char* threadname = new char[100];
      sprintf(threadname, "Hilo %d", k);
      Thread* newThread = new Thread (threadname);
      newThread->Fork (SimpleThread, (void*)threadname);
    }
    
    SimpleThread( (void*)"Hilo 0");
*/

	for(int i = 0; i < 10; ++i){
		int r = rand();
		char* threadname = new char[100];
      	sprintf(threadname, "Hilo %d", i);
      	Thread* newThread = new Thread (threadname);
      	
		if(r%2){
			printf("Se creo oxigeno\n");
	      	newThread->Fork (O, (void*)threadname);
	      	
		}else{
			printf("Se creo hidrogeno\n");
	      	newThread->Fork (H, (void*)threadname);
		}
	}


}


