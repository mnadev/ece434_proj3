//mypthread.c

#include "mypthread.h"

typedef enum _threadState{
	STATE_READY,
	STATE_READY2,	//This state will be set when a thread has been examined from pthread_yield (needed to ensure equal scheduling of threads)
	STATE_TERMINATED,
	STATE_BLOCKED
} threadState;

typedef struct threadNode{
	ucontext_t* threadContext;	//Context of the thread
	threadState state;		//Whether the thread is ready, blocking, or terminated.
	int callingThreadID;	//ThreadID of thread that created this thread (for mypthread_join/pthread_yield)
	void* retVal;		//When thread exits, return data will be stored here.
}Node;

//Global

// Type your globals...
int mainThreadQueued = 0;	//Used to determine if the main thread has been placed into the queue yet (as a condition for thread_create)
short activeThreadID = 0;		//Global that indicates the threadID of the currently running thread (bookkeeping needed for swapping contexts)
queue_t* threadQueue;	//Queue of all threads created.
mypthread_t* mainThread;	//Setting this pointer as global for easier access to the mainThread.
threadState stateToSearch = STATE_READY;

// Type your own functions (void)
// e.g., free up sets of data structures created in your library
int compareThreadIDs(void* checkedThread, void* tid){
	mypthread_t* thread = (mypthread_t*)checkedThread;
	short idToCheck = *(short*)tid;
	if(thread->tid == idToCheck){
		return 0;
	} else{
		return -1;
	}
}

int compareThreadStates(void* checkedThread, void* state){
	mypthread_t* thread = (mypthread_t*)checkedThread;
	threadState checkingState = *(threadState*)state;
	if(thread->mynode->state == checkingState){
		return 0;
	} else{
		return -1;
	}
}

    
// Write your thread create function here...
int mypthread_create(mypthread_t *thread, const mypthread_attr_t *attr, void *(*start_routine) (void *), void *arg){
	if(mainThreadQueued == 0){
		mainThreadQueued = 1;
		threadQueue = (queue_t*)malloc(sizeof(queue_t));
		create_queue(threadQueue);
		
		mainThread = (mypthread_t*)malloc(sizeof(mypthread_t));
		mainThread->tid = 0;
		Node* newThreadNode = (Node*)malloc(sizeof(Node));
		mainThread->mynode = newThreadNode;
		mainThread->mynode->state = STATE_READY;
		mainThread->mynode->threadContext = (ucontext_t*)malloc(sizeof(ucontext_t));
		//For later: Check if I really do need to call getcontext() here or if swapcontext() is enough in mypthread_join
		qenqueue(threadQueue, (void*)mainThread);
	}
	thread->tid = ((mypthread_t*)(threadQueue->rear->data))->tid + 1;	//Always incrementing threadID by 1.
	thread->mynode = (Node*)malloc(sizeof(Node));
	thread->mynode->threadContext = (ucontext_t*)malloc(sizeof(ucontext_t));
	if(getcontext(thread->mynode->threadContext)!=0){
		printf("Error in getting context\n");
		exit(1);
	}
	thread->mynode->threadContext->uc_stack.ss_sp = (char*)malloc(sizeof(char)*16384);	//allocate stack of 16kb
	thread->mynode->threadContext->uc_stack.ss_size = sizeof(char)*16384;
	thread->mynode->threadContext->uc_link = mainThread->mynode->threadContext;
	thread->mynode->state = STATE_READY;
	thread->mynode->callingThreadID = activeThreadID;
	makecontext(thread->mynode->threadContext, (void(*)(void))start_routine, 1, arg);
	qenqueue(threadQueue, (void*)thread);
	return 0;
}
	

// Write your thread exit function here...
void mypthread_exit(void *retval){
	mypthread_t* activeThread;
	qsearch(threadQueue, (void*)&activeThreadID, (void**)&activeThread, compareThreadIDs);	//Get active thread
	activeThread->mynode->state = STATE_TERMINATED;
	
	activeThread->mynode->retVal = retval;	//storing return value;
	mypthread_t* returningThread;
	qsearch(threadQueue, (void*)&activeThread->mynode->callingThreadID, (void**)&returningThread, compareThreadIDs);	//Getting calling thread
	returningThread->mynode->state = STATE_READY;
	activeThreadID = returningThread->tid;
	return;
}
  
    
// Write your thread yield function here...
int mypthread_yield(void){
	mypthread_t* activeThread;
	qsearch(threadQueue, (void*)&activeThreadID, (void**)&activeThread, compareThreadIDs);	//Get active thread
	mypthread_t* switchingThread;	//The thread we'll be yielding to.
	threadState searching = stateToSearch;
	qsearch(threadQueue, (void*)&searching, (void**)&switchingThread, compareThreadStates);
	if(switchingThread == NULL){
		stateToSearch = (stateToSearch == STATE_READY) ? STATE_READY2 : STATE_READY;
		searching = stateToSearch;
		qsearch(threadQueue, (void*)&searching, (void**)&switchingThread, compareThreadStates);
	}
	if(switchingThread->tid == activeThreadID){		//The thread we found is the same thread, we get to run again.
		switchingThread->mynode->state = (stateToSearch == STATE_READY) ? STATE_READY2 : STATE_READY;	//Balancing thread scheduling
		return 0;
	} else{
		activeThreadID = switchingThread->tid;
		swapcontext(activeThread->mynode->threadContext, switchingThread->mynode->threadContext);
	}
	return 0;
	
}

// Write your thread join function here...
int mypthread_join(mypthread_t thread, void **retval){
	mypthread_t* activeThread;
	qsearch(threadQueue, (void*)&activeThreadID, (void**)&activeThread, compareThreadIDs);
	if(thread.mynode->state == STATE_TERMINATED){	//If pthread_join was called and the thread is already terminated because of pthread_yielding stuff
		retval = thread.mynode->retVal;
		return 0;
	}else if(thread.mynode->state == STATE_BLOCKED){	//If joining thread ran before and was blocked (by creating new thread), then need to yield again
		mypthread_yield();
		//More stuff may be needed for retval handling.
	}else{
		activeThread->mynode->state = STATE_BLOCKED;	//Don't want pthread_yield to run this thread while it's waiting to join
		
		activeThreadID = thread.tid;	//The bookkeeping mentioned earlier. Need to keep track of current thread's ID.
		thread.mynode->threadContext->uc_link = activeThread->mynode->threadContext;	//So context returns here when it's finished
		swapcontext(activeThread->mynode->threadContext, thread.mynode->threadContext);
		//When context returns back here after exiting:
		retval = &thread.mynode->retVal;
	}
	return 0;
}
    