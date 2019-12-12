// threadtest.cc
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield,
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
// ****lab4 code begin****
#include "synch.h"
// ****lab4 code end****

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;

    for (num = 0; num < 5; num++) {
// ****lab1 code begin****
	    printf("*** thread %d, threadID=%d, named \"%s\" looped %d times\n", which, currentThread->getThreadID(), currentThread->getName(), num);
        currentThread->Yield();
    }
    TS();
// ****lab1 code end****
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t1 = new Thread("forked thread", 1);
    t1->Fork(SimpleThread, 1);
    Thread *t2 = new Thread("forked thread", 2);
    t2->Fork(SimpleThread, 2);
    SimpleThread(0);
}



// ****lab4 code begin****
void sleep(int val){
    int n=0;
    while(n<val) ++n;
}
//uses condition to deal with producers and consumers problem
Condition* condc = new Condition("ConsumerCondition");
Condition* condp = new Condition("ProducerCondition");
Lock* pcLock = new Lock("producerConsumerLock");
int shareNum = 0; //share num of producers and comsumers
int maxShareNum = 5;
void Producer1(int val){
  //while(1){
  for(int i=0;i<5;++i){
    pcLock->Acquire();
    while(shareNum >= maxShareNum){
      printf("Product already full:[%d],threadId:[%d],wait consumer.\n",
      shareNum,currentThread->getThreadID());
      condp->Wait(pcLock);
    }
    printf("name:[%s],threadId:[%d],before:[%d],after:[%d]\n",
    currentThread->getName(),currentThread->getThreadID(),
    shareNum,shareNum+1);
    ++shareNum;
    condc->Signal(pcLock);
    pcLock->Release();
    sleep(val);
  }
}

void Customer1(int val){
  //while(1){
  for(int i=0;i<5;++i){
    pcLock->Acquire();
    while(shareNum <= 0){
      printf("-->Product already empty:[%d],threadId:[%d],wait producer.\n",
      shareNum,currentThread->getThreadID());
      condc->Wait(pcLock);
    }
    printf("-->name:[%s],threadId:[%d],before:[%d],after:[%d]\n",
    currentThread->getName(),currentThread->getThreadID(),
    shareNum,shareNum-1);
    --shareNum;
    condp->Signal(pcLock);
    pcLock->Release();
    sleep(val);
  }
}

void ThreadProducerConsumerTest1(){
  DEBUG('t', "Entering ThreadProducerConsumerTest1");
  Thread* p1 = new Thread("Producer1");
  Thread* p2 = new Thread("Producer2");
  p1->Fork(Producer1, 1);
  p2->Fork(Producer1, 3);

  Thread* c1 = new Thread("Consumer1");
  Thread* c2 = new Thread("Consumer2");
  c1->Fork(Customer1, 1);
  c2->Fork(Customer1, 2);
}

//uses semaphore to deal with producers and consumers problem
#define BufferNum 10 // 缓冲区大小
Semaphore* emptyBuffer = new Semaphore("emptyBuffer", BufferNum);
Semaphore* mutex = new Semaphore("lockSemaphore", 1);
Semaphore* fullBuffer = new Semaphore("fullBuffer", 0);
int msgQueue = 0;

void Producer2(int val){
  //while(1) {
  for(int i=0;i<5;++i){
    emptyBuffer->P();
    mutex->P();
    if(msgQueue >= BufferNum){
        printf("-->Product already full:[%d],wait consumer.",msgQueue);
    }else{
      printf("-->name:[%s],threadID:[%d],before:[%d],after:[%d]\n",\
      currentThread->getName(),currentThread->getThreadID(),
      msgQueue,msgQueue+1);
      ++msgQueue;
    }
    mutex->V();
    fullBuffer->V();

    sleep(val); // to relax for a while until next producing action
  }
}

void Customer2(int val){
  //while(1) {
  for(int i=0;i<5;++i){
    fullBuffer->P();
    mutex->P();
    if(msgQueue <= 0){
        printf("Product already empty:[%d],wait Producer.",msgQueue);
    }else{
      printf("name:[%s] threadID:[%d],before:[%d],after:[%d]\n",\
      currentThread->getName(),currentThread->getThreadID(),
      msgQueue,msgQueue-1);
      --msgQueue;
    }
    mutex->V();
    emptyBuffer->V();

    sleep(val); // to relax for a while until next consuming action
    }
}

void ThreadProducerConsumerTest2(){
  DEBUG('t', "Entering ThreadProducerConsumerTest");
  Thread* p1 = new Thread("Producer1");
  Thread* p2 = new Thread("Producer2");
  p1->Fork(Producer2, 1);
  p2->Fork(Producer2, 3);

  //can cut down one consumer to see results
  Thread* c1 = new Thread("Consumer1");
  Thread* c2 = new Thread("Consumer2");
  c1->Fork(Customer2, 1);
  c2->Fork(Customer2, 2);
}

// using lock to deal with readers-and-writers problem
int rCnt = 0; // readers count
Lock* rLock = new Lock("rlock"); // readers count lock
Semaphore* wLock = new Semaphore("wlock",1); // cannot use lock cause only the reader who acquires the lock can release the lock
int bufSize = 0;

void readFunc(int sleepTime){
  //while(1) {
  for(int i=0;i<5;++i){
    rLock->Acquire();
    ++rCnt;
    if(rCnt == 1){
        wLock->P();
    }
    rLock->Release();
    if(0 == bufSize){
        printf("threadName:[%s],bufSize:[%d],currently no data.\n",
        currentThread->getName(),bufSize);
    }else{
        printf("threadName:[%s],bufSize:[%d],exec read operation.\n",
        currentThread->getName(),bufSize);
    }
    rLock->Acquire();
    --rCnt;
    if(rCnt == 0){
        wLock->V();
    }
    rLock->Release();
    currentThread->Yield();
    sleep(sleepTime);
  }
}

void writeFunc(int sleepTime){
  //while(1) {
  for(int i=0;i<5;++i){
    wLock->P();
    ++bufSize;
    printf("writerThread:[%s],buffersize before:[%d],after:[%d]\n",
    currentThread->getName(), bufSize, bufSize+1);
    wLock->V();
    currentThread->Yield();
    sleep(sleepTime);
  }
}

void readWriteThreadTest()
{
  DEBUG('t', "Entering readWriteThreadTest");
  Thread * r1 = new Thread("reader1");
  Thread * r2 = new Thread("reader2");
  Thread * r3 = new Thread("reader3");
  Thread * w1 = new Thread("writer1");
  Thread * w2 = new Thread("writer2");

    // 3个读者2个写者
  r1->Fork(readFunc,1);
  w1->Fork(writeFunc,2);
  r2->Fork(readFunc,1);
  w2->Fork(writeFunc,3);
  r3->Fork(readFunc,4);
}
// ****lab4 code end****

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
	case 2:
    ThreadProducerConsumerTest1();
    break;
    case 3:
    ThreadProducerConsumerTest2();
    break;
    case 4:
    readWriteThreadTest();
    break;
    default:
	printf("No test specified.\n");
	break;
    }
}
