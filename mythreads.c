#include "mythreads.h"
#include "stdio.h"
#include "string.h"
#include <sys/time.h>
#include "stdlib.h"
#include "assert.h"

typedef struct thread {
  int id;
  ucontext_t context;
  thFuncPtr func;
  void *arg;
  void *result;
  int done;
} thread_t;

typedef struct node {
  thread_t *t;
  struct node *next;
} node_t;

node_t *start, *current;

int locks[NUM_LOCKS];
int conditions[NUM_LOCKS][CONDITIONS_PER_LOCK];

int thread_count = 1;
int interruptsAreDisabled = 0;

static void interruptDisable() {
  // Disable interrupts
  interruptsAreDisabled = 1;
}

static void interruptEnable() {
  // Enable interrupts
  interruptsAreDisabled = 0;
}

void append_node(node_t *node) {
  // Add a node to the list
  node_t *tmp2 = current->next;
  current->next = node;
  node->next = tmp2;
}

void remove_node(node_t *node) {
  // Remove a node from the list
  node_t *tmp = start;
  while (tmp->next != node) {
    tmp = tmp->next;
  }
  tmp = tmp->next;
}

void runThread(thread_t *t) {
  // Wrapper function for running the user function
  interruptEnable();
  t->result = t->func(t->arg);
  t->done = 1;
  interruptDisable();
}

void threadYieldInternal() {
  // Advance to the next unfinished thread
  node_t *old = current;
  do {
    current = current->next;
  } while (current->t->done);

  // Switch contexts the the next thread
  if (old != current) {
    // assert(interruptsAreDisabled);
    swapcontext(&old->t->context, &current->t->context);
  } else {
  }
}

extern void threadYield() {
  interruptDisable();
  threadYieldInternal();
  interruptEnable();
}

extern void threadInit() {
  // Create a thread structure for the main thread
  thread_t *main_thread = malloc(sizeof(thread_t));
  getcontext(&main_thread->context);
  main_thread->id = 1;
  main_thread->done = 0;

  // And a node structure
  node_t *main_node = malloc(sizeof(node_t));
  main_node->t = main_thread;
  main_node->next = main_node;

  // Initialize the list
  start = current = main_node;
}

extern int threadCreate(thFuncPtr funcPtr, void *argPtr) {
  interruptDisable();

  // Create a thread structure for the user function
  thread_t *t = (thread_t *) malloc(sizeof(thread_t));
  getcontext(&t->context);
  // Make a stack for it
  t->context.uc_stack.ss_sp = malloc(STACK_SIZE);
  t->context.uc_stack.ss_size = STACK_SIZE;
  t->context.uc_stack.ss_flags = 0;
  t->context.uc_link = &start->t->context;
  // Add other details
  t->func = funcPtr;
  t->arg = argPtr;
  t->done = 0;
  t->id = ++thread_count;
  makecontext(&t->context, (void (*)(void)) runThread, 1, t);

  // Create the node structure
  node_t *newnode = (node_t *) malloc(sizeof(node_t));
  newnode->t = t;
  // Add it to the list
  append_node(newnode);
  // Run it (or something else)
  threadYieldInternal();

  interruptEnable();
  return newnode->t->id;
}

extern void threadJoin(int thread_id, void **result) {
  interruptDisable();

  // Find the thread
  node_t *tmp = start;
  while (tmp->t->id != thread_id) {
    tmp = tmp->next;
  }

  // Switch threads until it's done
  while (!tmp->t->done) {
    threadYieldInternal();
  }

  // Store the result
  if (result)
    *result = tmp->t->result;

  interruptEnable();
}

extern void threadExit(void *result) {
  interruptDisable();
  // Store the result and mark thread as complete
  current->t->result = result;
  current->t->done = 1;
  // Remove it from the list
  remove_node(current);
  // Move on to other threads
  threadYieldInternal();
  interruptEnable();
}

void threadLockInternal(int lockNum) {
  // Attempt to grab the lock
  int gotlock = 0;
  while (!gotlock) {
    if (!locks[lockNum]) {
      // Lock has been established
      locks[lockNum] = 1;
      gotlock = 1;
    } else {
      // Switch threads until it can be grabbed
      threadYield();
    }
  }
}

extern void threadLock(int lockNum) {
  interruptDisable();
  threadLockInternal(lockNum);
  interruptEnable();
}

extern void threadUnlock(int lockNum) {
  // Release the lock
  locks[lockNum] = 0;
}

extern void threadWait(int lockNum, int conditionNum) {
  interruptDisable();

  // Prevent erroroneous calls
  if (!locks[lockNum]) {
    printf("threadWait called with unlocked mutex\n");
    exit(1);
  }

  threadUnlock(lockNum);
  // Take the current thread out of the rotation
  node_t *old = current;
  remove_node(current);
  // Wait for a condition variable signal
  while (!conditions[lockNum][conditionNum]) {
    threadYield();
  }
  // Put the thread back into the rotation
  append_node(old);
  threadLockInternal(lockNum);

  interruptEnable();
}

extern void threadSignal(int lockNum, int conditionNum) {
  interruptDisable();
  // Signal the condition variable
  conditions[lockNum][conditionNum]++;
  interruptEnable();
}
