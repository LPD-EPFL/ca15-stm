#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <malloc.h>
#include <stdlib.h>

#include "sstm.h"
#include "random.h"
__thread unsigned long* seeds; 

/*
 * Useful macros to work with transactions. Note that, to use nested
 * transactions, one should check the environment returned by
 * stm_get_env() and only call sigsetjmp() if it is not null.
 */

#define DEFAULT_DURATION                1
#define DEFAULT_DELAY                   0
#define DEFAULT_SIZE                    1024
#define DEFAULT_NB_THREADS              1
#define DEFAULT_PERC_UPDATES            20
#define DEFAULT_VERBOSE                 0

int delay = DEFAULT_DELAY;
int test_verbose = DEFAULT_VERBOSE;
int argc;
char **argv;

#define XSTR(s)                         STR(s)
#define STR(s)                          #s

/* ################################################################### *
 * GLOBALS
 * ################################################################### */

typedef struct node
{
  size_t key;
  struct node* next;
} node_t;

typedef struct ll
{
  node_t* head;
} ll_t;

static ll_t* list;

int 
ll_insert(ll_t* list, size_t key) 
{
  int ret = 0;

  TX_START();
  node_t* cur = (node_t*) TX_LOAD(&list->head);
  node_t* pred = NULL;

  while (cur != NULL && cur->key < key)
    {
      pred = cur;
      cur = (node_t*) TX_LOAD(&cur->next);//
    }

  if (cur == NULL || cur->key != key)
    {
      node_t* new = TX_MALLOC(sizeof(node_t));
      assert(new != NULL);
      new->key = key;
      new->next = cur;
      if (pred == NULL)
	{
	  TX_STORE(&list->head, new);
	}
      else
	{
	  TX_STORE(&pred->next, new);
	}
      ret = 1;
    }
  else
    {
      ret = 0;
    }

  TX_COMMIT();
  return ret;
}

int 
ll_delete(ll_t* list, size_t key) 
{
  int ret = 0;

  TX_START();
  node_t* cur = (node_t*) TX_LOAD(&list->head);
  node_t* pred = NULL;

  while (cur != NULL && cur->key < key)
    {
      pred = cur;
      cur = (node_t*) TX_LOAD(&cur->next);
    }

  if (cur == NULL || cur->key != key)
    {
      ret = 0;
    }
  else
    {
      node_t* nxt = (node_t*) TX_LOAD(&cur->next);
      if (pred != NULL)
	{
	  TX_STORE(&pred->next, nxt);
	}
      else
      	{
	  TX_STORE(&list->head, nxt);
      	}
      TX_FREE(cur);
      ret = 1;
    }

  TX_COMMIT();
  return ret;
}

int 
ll_search(ll_t* list, size_t key) 
{
  int ret = 0;

  TX_START();
  node_t* cur = (node_t*) TX_LOAD(&list->head);

  while (cur != NULL && cur->key < key)
    {
      cur = (node_t*) TX_LOAD(&cur->next);
    }

  if (cur == NULL || cur->key != key)
    {
      ret = 0;
    }
  else
    {
      ret = 1;
    }

  TX_COMMIT();
  return ret;
}

size_t 
ll_size() 
{
  size_t size = 0;
  TX_START();
  size = 0;
  node_t* cur = (node_t*) TX_LOAD(&list->head);
  while (cur != NULL)
    {
      size++;
      cur = (node_t*) TX_LOAD(&cur->next);
    }
  TX_COMMIT();

  return size;
}



/* ################################################################### *
 * STRESS TEST
 * ################################################################### */

typedef struct thread_data 
{
  uint64_t nb_inserts;
  uint64_t nb_inserts_succ;
  uint64_t nb_deletes;
  uint64_t nb_deletes_succ;
  uint64_t nb_searchs;
  uint64_t nb_searchs_succ;
  int32_t id;
  int32_t perc_search;
  size_t duration;
  uint32_t size;
} thread_data_t;


volatile int work = 1;

void*
test(void *data) 
{
  srand(time(NULL));
  seed_rand();

  int rand_max;
  thread_data_t *d = (thread_data_t *) data;
  ll_t* list_local = list;

  rand_max = 2 * d->size;
  int lim_search = d->perc_search;
  int lim_update_one = (INT_MAX - lim_search) / 2;
  int lim_insert = lim_search + lim_update_one;

  TM_THREAD_START();

  /* BARRIER; */
  while(work)
    {
      int op = (int) fast_rand();
      uint32_t key = fast_rand() % rand_max;

      if (op < lim_search)
	{
	  d->nb_searchs_succ += ll_search(list_local, key);
	  d->nb_searchs++;
	}
      else if (op < lim_insert)
	{
	  d->nb_inserts_succ += ll_insert(list_local, key);
	  d->nb_inserts++;
	}
      else
	{
	  d->nb_deletes_succ += ll_delete(list_local, key);
	  d->nb_deletes++;
	}
    }

  TM_THREAD_STOP();

  return NULL;
}

int
main(int argc, char **argv)
{
  struct option long_options[] =
    {
      // These options don't set a flag
      {"help", no_argument, NULL, 'h'},
      {"num-threads", required_argument, NULL, 'n'},
      {"accounts", required_argument, NULL, 'a'},
      {"contention-manager", required_argument, NULL, 'c'},
      {"duration", required_argument, NULL, 'd'},
      {"delay", required_argument, NULL, 'D'},
      {"read-all-rate", required_argument, NULL, 'r'},
      {"check", required_argument, NULL, 'c'},
      {"read-threads", required_argument, NULL, 'R'},
      {"write-all-rate", required_argument, NULL, 'w'},
      {"write-threads", required_argument, NULL, 'W'},
      {"verbose", no_argument, NULL, 'v'},
      {NULL, 0, NULL, 0}
    };


  static uint32_t duration, perc_updates, size, num_threads;

  duration = DEFAULT_DURATION;
  perc_updates = DEFAULT_PERC_UPDATES;
  size = DEFAULT_SIZE;
  num_threads = DEFAULT_NB_THREADS;

  int i, c;
  while (1)
    {
      i = 0;
      c = getopt_long(argc, argv, "hn:i:d:r:u::v", long_options, &i);

      if (c == -1)
	break;

      if (c == 0 && long_options[i].flag == 0)
	c = long_options[i].val;

      switch (c)
	{
	case 0:
	  /* Flag is automatically set */
	  break;
	case 'h':
	  printf("list -- STM stress test\n"
		 "\n"
		 "Usage:\n"
		 "  ll [options...]\n"
		 "\n"
		 "Options:\n"
		 "  -h, --help\n"
		 "        Print this message\n"
		 "  -i, --initial <int>\n"
		 "        Initial list lise (default=" XSTR(DEFAULT_SIZE) ")\n"
		 "  -d, --duration <double>\n"
		 "        Test duration in seconds (default=" XSTR(DEFAULT_DURATION) ")\n"
		 "  -u, --update <int>\n"
		 "        Percentage of update transactions (default=" XSTR(DEFAULT_PERC_UPDATES) ")\n"
		 );
	  exit(0);
	case 'i':
	  size = atoi(optarg);
	  break;
	case 'n':
	  num_threads = atoi(optarg);
	  break;
	case 'd':
	  duration = atoi(optarg);
	  break;
	case 'u':
	  perc_updates = atoi(optarg);
	  break;
	case 'v':
	  test_verbose = 1;
	  break;
	case '?':
	  printf("Use -h or --help for help\n");
	  exit(0);
	default:
	  exit(1);
	}
    }


  assert(duration >= 0);
  assert(size >= 2);
  assert(perc_updates <= 100);

  if (test_verbose)
    {
      printf("Initial size   : %d\n", size);
      printf("Duration       : %d s\n", duration);
      printf("Updates        : %d%%\n", perc_updates);
    }
  /* normalize percentages to 128 */

  perc_updates *= (INT_MAX / 100.0);

  TM_START();
  TM_THREAD_START();

  list = (ll_t*) malloc(sizeof(ll_t));
  if (list == NULL)
    {
      printf("malloc list");
      exit(1);
    }
  list->head = NULL;

  for (i = 0; i < size; i++)
    {
      ll_insert(list, i);
    }


  size_t lsize = ll_size(list);
  if (test_verbose)
    {
      printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~List size (before): %zu\n", lsize);
    }


  thread_data_t data[num_threads];
  pthread_t threads[num_threads];
  pthread_attr_t attr;
  int rc;
  void *status;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  long t;
  for(t = 0; t < num_threads; t++)
    {
      data[t].id = t;
      data[t].nb_inserts = 0;
      data[t].nb_deletes = 0;
      data[t].nb_searchs = 0;
      data[t].nb_inserts_succ = 0;
      data[t].nb_deletes_succ = 0;
      data[t].nb_searchs_succ = 0;
      data[t].size = size; 
      data[t].duration = duration;
      data[t].perc_search = INT_MAX - perc_updates;
      rc = pthread_create(&threads[t], &attr, test, &data[t]);
      if (rc)
	{
	  printf("ERROR; return code from pthread_create() is %d\n", rc);
	  exit(-1);
	}
        
    }
    
  /* Free attribute and wait for the other threads */
  pthread_attr_destroy(&attr);

  printf(" ZZZzzz %d seconds\n", duration);
  sleep(duration);
  printf(" Woken up\n");
  asm volatile ("mfence");
  work = 0;
  asm volatile ("mfence");


  for(t = 0; t < num_threads; t++) 
    {
      rc = pthread_join(threads[t], &status);
      if (rc) 
	{
	  printf("ERROR; return code from pthread_join() is %d\n", rc);
	  exit(-1);
	}
    }

  size_t search_suc = 0, insert_suc = 0, delete_suc = 0,
    search_all = 0, insert_all = 0, delete_all = 0;
  for(t = 0; t < num_threads; t++)
    {
      search_suc += data[t].nb_searchs_succ;
      delete_suc += data[t].nb_deletes_succ;
      insert_suc += data[t].nb_inserts_succ;
      search_all += data[t].nb_searchs;
      delete_all += data[t].nb_deletes;
      insert_all += data[t].nb_inserts;
      if (test_verbose)
	{
	  double insert_suc_rate = 100 * data[t].nb_inserts_succ / (double) data[t].nb_inserts;
	  double delete_suc_rate = 100 * data[t].nb_deletes_succ / (double) data[t].nb_deletes;
	  double search_suc_rate = 100 * data[t].nb_searchs_succ / (double) data[t].nb_searchs;
	  printf("---Core %ld\n  #inserts   : %-10zu ( %-3.2f%% succ)\n"
		 "  #deletes   : %-10zu ( %-3.2f%% succ)\n"
		 "  #searches  : %-10zu ( %-3.2f%% succ)\n",
		 t, data[t].nb_inserts, insert_suc_rate,
		 data[t].nb_deletes, delete_suc_rate,
		 data[t].nb_searchs,search_suc_rate);
	  /* printf("  Successful\n  #inserts   : %zu\n  #deletes   : %zu\n  #searches  : %zu\n", */
	  /* 	 data[t].nb_inserts_succ, data[t].nb_deletes_succ, data[t].nb_searchs_succ); */
	}
    }


  int32_t correct_size = size + insert_suc - delete_suc;
  lsize = ll_size(list);
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~List size (after)  : %zu\n", lsize);
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~List size (correct): %d\n", correct_size);
  if (correct_size != lsize)
    {
      printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~List size is wrong\n");
    }
  assert(correct_size == lsize);

  double insert_suc_rate = 100 * insert_suc / (double) insert_all;
  double delete_suc_rate = 100 * delete_suc / (double) delete_all;
  double search_suc_rate = 100 * search_suc / (double) search_all;

  printf("-- Total:\n");
  printf("  #inserts   : %-10zu ( %-3.2f%% succ)\n"
	 "  #deletes   : %-10zu ( %-3.2f%% succ)\n"
	 "  #searches  : %-10zu ( %-3.2f%% succ)\n",
	 insert_all, insert_suc_rate,
	 delete_all, delete_suc_rate,
	 search_all, search_suc_rate);
	 


  TM_STATS(duration);
  TM_THREAD_STOP();
  TM_STOP();

  free(list);
}
