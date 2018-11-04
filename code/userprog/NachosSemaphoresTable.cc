#include "NachosSemaphoresTable.h"

NachosSemaphoresTable::NachosSemaphoresTable(){
	this->usage = 1;
	this->activeSemaphoresMap = new BitMap(_SIZE_SEM);
}

NachosSemaphoresTable::~NachosSemaphoresTable(){
	delete activeSemaphoresMap;
	delete semaphores;
}

// Register the file handle
int NachosSemaphoresTable::CreateSemaphore(int value){
	int posicion = activeSemaphoresMap->Find();
	semaphores[posicion] = new Semaphore((char*)posicion, value);
	return posicion;
}

int NachosSemaphoresTable::DelSemaphore(int id){
	Semaphore* s = (Semaphore*)(semaphores[id]);
	s->Destroy();
	activeSemaphoresMap->Clear(id);
	return 0;
}

bool NachosSemaphoresTable::isActive(int id){
	return activeSemaphoresMap->Test(id);
}

int NachosSemaphoresTable::signalSem(int id){
	Semaphore* s = (Semaphore*)(semaphores[id]);
	s->V();
	return s->getValue();
}

int NachosSemaphoresTable::waitSem(int id){
	Semaphore* s = (Semaphore*)(semaphores[id]);
	s->P();
	return s->getValue();
}

void NachosSemaphoresTable::addThread(){
	usage++;
}

void NachosSemaphoresTable::delThread(){
	usage--;
}

int NachosSemaphoresTable::getUsage(){
	return usage;
}

int NachosSemaphoresTable::getSize(){
	return _SIZE_SEM;
}
