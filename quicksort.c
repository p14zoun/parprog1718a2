#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define THREADS 4
#define CUTOFF 1000
#define N 1000



struct params
{
	double *start;		//deixnei sto prwto keli toy pinaka pou tha ginei sort
	int end;			    //deixnei sto teleutaio keli toy pinaka
	int shutdown;		  //otan ginei sort o pinakas ginetai 1
};

struct params circlr_buf[CIRCLR_BUF_SIZE]; //	pinakas me arithmo kelion [CIRCLR_BUF_SIZE]

int q_insert = 0;	//arithmos kelion buffer
int q_output = 0;

pthread_cond_t msg_in = PTHREAD_COND_INITIALIZER;
pthread_cond_t msg_out = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;



void inssort(double *a,int n) { //inssort apo kwdika stef
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


void quicksort(double *a,int n) { //quicksort apo kwdika stef
int first,last,middle;
double t,p;
int i,j;

//an einai mikrotero toy cutoff pou oristike stin arxi
  if (n<=CUTOFF) {
    inssort(a,n);
    return;
  }

  first = 0;
  middle = n-1;
  last = n/2;

  if (a[middle]<a[first]) { t = a[middle]; a[middle] = a[first]; a[first] = t; }
  if (a[last]<a[middle]) { t = a[last]; a[last] = a[middle]; a[middle] = t; }
  if (a[middle]<a[first]) { t = a[middle]; a[middle] = a[first]; a[first] = t; }


//xorisma toy pinaka sti mesi (middle)
  p = a[middle]; // pivot
  for (i=1,j=n-2;;i++,j--) {
    while (a[i]<p) i++;
    while (p<a[j]) j--;
    if (i>=j) break;

    t = a[i]; a[i] = a[j]; a[j] = t;
  }

  quicksort(a,i);
  quicksort(a+i,n-i);

}





void *work(void *tp) {
  double *a;
  int n;
  int i;
  struct thread_params *tparm;
  tparm = (struct thread_params *)tp;
  printf("Hello\n");
  a = tparm->a;
  n = tparm->n;


  if(n < LIMIT) {
    quicksort(a, n);
  }
  else {
    i = partition(a, n);
    pthread_t child_0, child_1;
    struct thread_params ch_0, ch_1;
    ch_0.a = a;
    ch_0.n = i;
    pthread_create(&child_0, NULL, work, &ch_0);

    ch_1.a = a + i;
    ch_1.n = n - i;
    pthread_create(&child_1, NULL, work, &ch_1);

    pthread_join(child_0, NULL);
    pthread_join(child_1, NULL);
  }

  pthread_exit(NULL);
}


int main()
{
	printf("Hello World!\n");

double *a;
int i;
struct thread_params tp;

// gemizei ton pinaka me random times
srand(time(NULL));
for (i=0;i<N;i++) {
	a[i] = (double)rand()/RAND_MAX;
}

tp.a = a;
tp.n = N;



//thread_pool
	int thread_number;

	pthread_t thread_pool[THREADS];


	for(thread_number=0; thread_number<THREADS; thread_number++)
	{

		if (pthread_create(&thread_pool[thread_number], NULL, work, NULL)!=0)

		{
     		printf("Error in thread creation!\n");
      		exit(1);
		}
		printf("Thread %d created!\n", thread_number);
	}


	for(thread_number=0; thread_number<THREADS; thread_number++)
	{
		pthread_join(thread_pool[thread_number], NULL);
		printf("Thread %d joined!\n", thread_number);
	}

//ftiaxnei enan pinaka me malloc kai ton gemizei me random arithmous
a = (double *)malloc(N*sizeof(double));
  if (a==NULL) {
    printf("error in malloc\n");
    exit(1);
  }





// checkarei an egine sorting ston pinaka
 for (i=0;i<(N-1);i++) {
	 if (a[i]>a[i+1]) {
		 printf("Sort failed!\n");
		 break;}
		 else {
		 shutdown=1; //epistrefei shutdown=1 an exei ginei sortarisma ston pinaka
	 }

	return 0;
}
