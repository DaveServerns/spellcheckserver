//David Severns
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <semaphore.h>



/*structure to hold the sd and provide mutual exclusion*/
struct sock_buf{
  int* sock_d; // hold the socket descriptors
  int n; // number of spots
  int first; // first spot in queue
  int last; // last spot in queue
  sem_t mutex; // mutual exclusion
  sem_t spots; // syn prim to mark open spots init to 6
  sem_t items; // num items in the queue init to 0
};


void sbuf_init(sock_buf *sb, int n)
{
  sb->sock_d = (int*)calloc(n, sizeof(int)); // allocate memory
  sb->n = n; //size of queue
  sb->first = sb->last = 0; // right now the first and last spot will 0 before something is added
  sem_init(&sb->mutex, 0, 1); // set mutex lock to 1
  sem_init(&sb->spots, 0, n); //set the open socks to n
  sem_init(&sb->items, 0, 0);// init items in queue to 0
}

void sbuf_deinit(sock_buf *sb)
{
  free(sb->sock_d); // free memory
}

/* Insert item onto the rear of shared buffer sb */
void sbuf_insert(sock_buf *sb, int item)
{
  sem_wait(&sb->spots); // if there are open spots decrement if not go to sleep
  sem_wait(&sb->mutex); // lock critical section with mutex
  sb->sock_d[(++sb->last)%(sb->n)] = item; // add item to "end" of structure
  sem_post(&sb->mutex); // give up locl
  sem_post(&sb->items); // wake up workers, let them know item present
}

/* Remove and return the first item from buffer sb */
int sbuf_remove(sock_buf *sb){
  int item;
  sem_wait(&sb->items); // icrement num items, if none go to sleep
  sem_wait(&sb->mutex); // take the lock
  item = sb->sock_d[(++sb->first)%(sb->n)]; /* Remove the item */
  sem_post(&sb->mutex); // give up lock
  sem_post(&sb->spots);//if producer is asleep wake them up
  return item; //return the socket descriptors
}
//severns
