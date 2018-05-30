// Barrier usage example.
// Compile with: gcc -O2 -Wall -pthread barrier-example.c -o barrier-example

// IMPORTANT NOTE: borrowed from GPU massively/fine grained parallel world,
// this reduction method is NOT suitable for real apps with pthreads!!!

#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#define COUNT_MAX 20
#define CUTOFF 10
#define MESSAGES 20
// size of array for reduction
#define N 10000
// number of threads - MUST be power of 2
#define THREADS 4

// how many elements a thread will process - NOTE: surrounding ()!
#define BLOCKSIZE  ((N+THREADS-1)/THREADS)

// struct of info passed to each thread
struct thread_params {
  int *start;	// pointer to start of thread's portion
  int *out;	// where to write partial sum
  int n;	// how many consecutive elements to process
  int id;	// thread's id
};

// thread syncing barrier
pthread_barrier_t barrier;

// the thread work function
void *thread_func(void *args) {
struct thread_params *tparm;
int *pa,*out;
int i,n,sum,id,stride;

  // thread input params
  tparm = (struct thread_params *)args;
  pa = tparm->start;
  n = tparm->n;
  out = tparm->out;
  id = tparm->id;

  // phase 1: each thread reduces its part
  sum = 0;
  for (i=0;i<n;i++) {
    sum += pa[i];
  }
  // store partial sum
  *out = sum;
  printf("Thread %d: partial sum=%d\n",id,sum);

  // phase 2: half of threads in each round sum a pair of values
  for (stride=1;stride<THREADS;stride*=2) {
    // sync on barrier, for all threads
    pthread_barrier_wait(&barrier); // after sync, barrier goes to its init() state

    if (id%(2*stride)==0) { // half of previous round
      // NOTE: this printf will show that all operations with same stride are together thanks to barrier
      printf("Thread %d: summing %d + %d (stride is %d)\n",id,*out,*(out+stride),stride);
      *out = (*out)+*(out+stride);
    }
  }

  // exit and let be joined
  pthread_exit(NULL);
}

void *consumer_thread(void *args) {
    int global_availmsg = 0;	// empty
int global_buffer;
  int i;

  // receive a predefined number of messages
  for (i=0;i<MESSAGES;i++) {
    // lock mutex
    pthread_mutex_lock(&mutex);
    while (global_availmsg<1) {	// NOTE: we use while instead of if! more than one thread may wake up
     // pthread_cond_wait(&msg_in,&mutex);
    }
    // receive message
    printf("Consumer: received msg %d\n",global_buffer);


    // signal the sender that something was removed from buffer
    pthread_cond_signal(&msg_out);

    // unlock mutex
    pthread_mutex_unlock(&mutex);
  }

  // exit and let be joined
  pthread_exit(NULL);
}

void *producer_thread(void *args) {
    
  int i;
    int global_buffer = i;
    int global_availmsg = 1;
  // send a predefined number of messages
  for (i=0;i<MESSAGES;i++) {
    // lock mutex
    pthread_mutex_lock(&mutex);
    while (global_availmsg>0) {	// NOTE: we use while instead of if! more than one thread may wake up

      pthread_cond_wait(&msg_out,&mutex);  // wait until a msg is received - NOTE: mutex MUST be locked here.
      					   // If thread is going to wait, mutex is unlocked automatically.
      					   // When we wake up, mutex will be locked by us again.
    }
    // send message
    printf("Producer: sending msg %d\n",i);


    // signal the receiver that something was put in buffer
    pthread_cond_signal(&msg_in);

    // unlock mutex
    pthread_mutex_unlock(&mutex);
  }

  // exit and let be joined
  pthread_exit(NULL);
}


// sample thread function
void *thread_func(void *args) {
    int global_count = 0;
  int myid = ((struct thread_params*)args)->myid;
  int done = 0;
  while (done==0) {
    // lock count mutex
    pthread_mutex_lock(&count_mutex);
    printf("Thread %d: got count %d",myid,global_count);
    if (global_count>=COUNT_MAX) {
      done = 1;
      printf(", terminating.\n");
    }
    else {
      global_count++;
      printf(", incrementing.\n");
    }
    // unlock count mutex
    pthread_mutex_unlock(&count_mutex);

    // simulate some work
    sleep(1);

  }

  // exit and let be joined
  pthread_exit(NULL);
}


void inssort(double *a,int n) {
int i,j;
double t;

  for (i=1;i<n;i++) {
    j = i;
    while ((j>0) && (a[j-1]>a[j])) {
      t = a[j-1];  a[j-1] = a[j];  a[j] = t;
      j--;
    }
  }

}


void quicksort(double *a,int n) {
int first,last,middle;
double t,p;
int i,j;

  // check if below cutoff limit
  if (n<=CUTOFF) {
    inssort(a,n);
    return;
  }

  // take first, last and middle positions
  first = 0;
  middle = n-1;
  last = n/2;

  // put median-of-3 in the middle
  if (a[middle]<a[first]) { t = a[middle]; a[middle] = a[first]; a[first] = t; }
  if (a[last]<a[middle]) { t = a[last]; a[last] = a[middle]; a[middle] = t; }
  if (a[middle]<a[first]) { t = a[middle]; a[middle] = a[first]; a[first] = t; }

  // partition (first and last are already in correct half)
  p = a[middle]; // pivot
  for (i=1,j=n-2;;i++,j--) {
    while (a[i]<p) i++;
    while (p<a[j]) j--;
    if (i>=j) break;

    t = a[i]; a[i] = a[j]; a[j] = t;
  }

  // recursively sort halves
  quicksort(a,i);
  quicksort(a+i,n-i);

}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------




int main() {
    
    // condition variable, signals a put operation (receiver waits on this)
pthread_cond_t msg_in = PTHREAD_COND_INITIALIZER;
// condition variable, signals a get operation (sender waits on this)
pthread_cond_t msg_out = PTHREAD_COND_INITIALIZER;
int *a;
int i,check;
// array of structs to fill and pass to threads on creation
struct thread_params tparm[THREADS];
// table of thread IDs (handles) filled on creation, to be used later on join
pthread_t threads[THREADS];

// partial reduction table - one element per thread
int partial[THREADS];

  // initialize barrier - always on all threads
  pthread_barrier_init (&barrier, NULL, THREADS);

  // allocate vector array
  a = (int *)malloc(N*sizeof(int));
  if (a==NULL) exit(1);

  //initialize vector
  for (i=0;i<N;i++) {
    a[i]=i+1;	// 1...N
  }

  // create all threads
  check = 0;
  for (i=0;i<THREADS;i++) {
    // fill params for this thread
    tparm[i].start = a+i*BLOCKSIZE;
    tparm[i].id = i;
    tparm[i].out = &partial[i];
    if ((check+BLOCKSIZE)>=N) { // less than blocksize to do...
      tparm[i].n = N-check;
    }
    else {
      tparm[i].n = BLOCKSIZE; // there IS blocksize work to do!
    }
    check += BLOCKSIZE;

    // create thread with default attrs (attrs=NULL)
    if (pthread_create(&threads[i],NULL,thread_func,&tparm[i])!=0) {
      printf("Error in thread creation!\n");
      exit(1);
    }
  }

  // block on join of threads
  for (i=0;i<THREADS;i++) {
    pthread_join(threads[i],NULL);
  }


  // check results
  if (partial[0]!=((N*(N+1))/2)) {
    printf("computation error!\n");
  }
  // free vector
  free(a);


// mutex protecting common resources
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


  pthread_t producer,consumer;

  // create threads
  pthread_create(&producer,NULL,producer_thread,NULL);
  pthread_create(&consumer,NULL,consumer_thread,NULL);

  // then join threads
  pthread_join(producer,NULL);
  pthread_join(consumer,NULL);

  // destroy mutex - should be unlocked
  pthread_mutex_destroy(&mutex);

  // destroy cvs - no process should be waiting on these
  pthread_cond_destroy(&msg_out);
  pthread_cond_destroy(&msg_in);


// mutex protecting count variable
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;





  // thread ids (opaque handles) - used for join
  pthread_t threadids[THREADS];
thread_params tparm[THREADS];
  // array of structs to fill and pass to threads on creation


  // create threads
  for (i=0;i<THREADS;i++) {
    tparm[i].myid = i;
    pthread_create(&threadids[i],NULL,thread_func,&tparm[i]);
  }


  a = (double *)malloc(N*sizeof(double));
  if (a==NULL) {
    printf("error in malloc\n");
    exit(1);
  }

  // fill array with random numbers
  srand(time(NULL));
  for (i=0;i<N;i++) {
    a[i] = (double)rand()/RAND_MAX;
  }

  // then join threads
  for (i=0;i<THREADS;i++) {
    pthread_join(threadids[i],NULL);
  }
  return 0;
}


