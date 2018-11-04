#include "bitmap.h"
#include "synch.h"

#define _SIZE_SEM 100	//cantidad maxima de semaforos que se pueden crear

class NachosSemaphoresTable {
  public:
    NachosSemaphoresTable();       // Initialize
    ~NachosSemaphoresTable();      // De-allocate
    
    int CreateSemaphore(int value); 
    int DelSemaphore( int id );   
    int signalSem(int id);
    int waitSem(int id);
    bool isActive(int id);

    void addThread();		// If a user thread is using this table, add it
    void delThread();		// If a user thread is using this table, delete it
	int getUsage();
	int getSize();
    //void Print();               // Print contents
    
  private:
    BitMap * activeSemaphoresMap;	// A bitmap to control our vector
    void* semaphores[_SIZE_SEM];
    int usage;						// How many threads are using this table

};
