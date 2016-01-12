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
#define DEFAULT_NB_ACCOUNTS             1024
#define DEFAULT_NB_THREADS              1
#define DEFAULT_READ_ALL                0
#define DEFAULT_CHECK                   0
#define DEFAULT_WRITE_ALL               0
#define DEFAULT_READ_THREADS            0
#define DEFAULT_WRITE_THREADS           0
#define DEFAULT_DISJOINT                0
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

/* ################################################################### *
 * BANK ACCOUNTS
 * ################################################################### */

typedef struct account 
{
  uint64_t number;
  int64_t balance;
} account_t;

typedef struct bank 
{
  account_t* accounts;
  size_t size;
} bank_t;

static bank_t* bank;

int 
transfer(account_t* src, account_t* dst, int amount) 
{
  /* printf("in transfer"); */

  /* Allow overdrafts */
  TX_START();
  int64_t i, j;
  i = TX_LOAD(&src->balance);
  j = TX_LOAD(&dst->balance);
  i -= amount;
  j += amount;
  TX_STORE(&src->balance, i); 
  TX_STORE(&dst->balance, j);
  TX_COMMIT();

  return amount;
}

void
check_accs(account_t* acc1, account_t* acc2) 
{
  /* printf("in check_accs"); */

  volatile int i, j;

  TX_START();
  i = TX_LOAD(&acc1->balance);
  j = TX_LOAD(&acc2->balance);
  TX_COMMIT();

  if (i == j<<20)
    {
      //printf("dummy print");
    }
}

int
total(bank_t* bank, int transactional) 
{
  int i, total;

  if (!transactional)
    {
      total = 0;
      for (i = 0; i < bank->size; i++)
	{
	  total += bank->accounts[i].balance;
	}
    }
  else
    {
      TX_START();
      total = 0;
      for (i = 0; i < bank->size; i++)
	{
	  total += TX_LOAD(&bank->accounts[i].balance);
	}
      TX_COMMIT();
    }

  if (total != 0)
    {
      printf("Got a bank total of: %d\n", total);
    }

  return total;
}

void
reset(bank_t *bank) 
{
  int i;

  TX_START();
  for (i = 0; i < bank->size; i++)
    {
      TX_STORE(&bank->accounts[i].balance, 0);
    }
  TX_COMMIT();
}


/* ################################################################### *
 * STRESS TEST
 * ################################################################### */

typedef struct thread_data 
{
  uint64_t nb_transfer;
  uint64_t nb_checks;
  uint64_t nb_read_all;
  uint64_t nb_write_all;
  int32_t id;
  int32_t read_cores;
  int32_t write_cores;
  int32_t read_all;
  int32_t write_all;
  int32_t check;
  size_t duration;
  uint32_t nb_accounts;
} thread_data_t;


volatile int work = 1;

void*
test(void *data) 
{
  int rand_max;

  seed_rand();

  uint8_t is_read_core = 0;
  thread_data_t *d = (thread_data_t *) data;
  bank_t* bank_local = bank;

  if (d->id < d->read_cores)
    {
      is_read_core = 1;
    }

  rand_max = d->nb_accounts;

  TM_THREAD_START();

  while(work)
    {
      uint8_t nb = fast_rand() & 127;
      if (is_read_core || nb < d->read_all)
	{
	  /* Read all */
	  total(bank_local, 1);
	  d->nb_read_all++;
	}
      else
	{
	  /* Choose fast_random accounts */

	  uint32_t src = fast_rand() % rand_max;
	  uint32_t dst = fast_rand() % rand_max;
	  if (dst == src)
	    {
	      dst = ((src + 1) % rand_max);
	    }
	  if (nb < d->check)
	    {
	      check_accs(bank_local->accounts + src, bank_local->accounts + dst);
	      d->nb_checks++;
	    }
	  else
	    {
	      transfer(bank_local->accounts + src, bank_local->accounts + dst, 1);
	      d->nb_transfer++;
	    }
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
      {"duration", required_argument, NULL, 'd'},
      {"delay", required_argument, NULL, 'D'},
      {"read-all-rate", required_argument, NULL, 'r'},
      {"check", required_argument, NULL, 'c'},
      {"read-threads", required_argument, NULL, 'R'},
      {"verbose", no_argument, NULL, 'v'},
      {NULL, 0, NULL, 0}
    };


  static int duration;
  static int nb_accounts;
  static int read_all;
  static int read_cores;
  static int write_all;
  static int check;
  static int write_cores;
  static int num_threads;

  duration = DEFAULT_DURATION;
  nb_accounts = DEFAULT_NB_ACCOUNTS;
  read_all = DEFAULT_READ_ALL;
  read_cores = DEFAULT_READ_THREADS;
  write_all = DEFAULT_READ_ALL + DEFAULT_WRITE_ALL;
  check = write_all + DEFAULT_CHECK;
  write_cores = DEFAULT_WRITE_THREADS;
  num_threads = DEFAULT_NB_THREADS;

  int i, c;
  while (1)
    {
      i = 0;
      c = getopt_long(argc, argv, "hn:a:d:r:c:R:v", long_options, &i);

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
	  printf("bank -- STM stress test\n"
		 "\n"
		 "Usage:\n"
		 "  bank [options...]\n"
		 "\n"
		 "Options:\n"
		 "  -h, --help\n"
		 "        Print this message\n"
		 "  -n, --num-threads <int>\n"
		 "        Number of threads (default=" XSTR(DEFAULT_NB_THREADS) ")\n"
		 "  -a, --accounts <int>\n"
		 "        Number of accounts in the bank (default=" XSTR(DEFAULT_NB_ACCOUNTS) ")\n"
		 "  -d, --duration <double>\n"
		 "        Test duration in seconds (default=" XSTR(DEFAULT_DURATION) ")\n"
		 "  -d, --delay <int>\n"
		 "        Delay ns after succesfull request. Used to understress the system, default=" XSTR(DEFAULT_DELAY) ")\n"
		 "  -c, --check <int>\n"
		 "        Percentage of check transactions transactions (default=" XSTR(DEFAULT_CHECK) ")\n"
		 "  -r, --read-all-rate <int>\n"
		 "        Percentage of read-all transactions (default=" XSTR(DEFAULT_READ_ALL) ")\n"
		 "  -R, --read-threads <int>\n"
		 "        Number of threads issuing only read-all transactions (default=" XSTR(DEFAULT_READ_THREADS) ")\n"
		 );
	  exit(0);
	case 'a':
	  nb_accounts = atoi(optarg);
	  break;
	case 'n':
	  num_threads = atoi(optarg);
	  break;
	case 'd':
	  duration = atoi(optarg);
	  break;
	case 'D':
	  delay = atoi(optarg);
	  break;
	case 'c':
	  check = atoi(optarg);
	  break;
	case 'r':
	  read_all = atoi(optarg);
	  break;
	case 'R':
	  read_cores = atoi(optarg);
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


  write_all = 0;
  write_cores = 0;

  write_all += read_all;
  check += write_all;

  assert(duration >= 0);
  assert(nb_accounts >= 2);
  assert(read_all >= 0 && write_all >= 0 && check >= 0 && check <= 100);
  
  if (test_verbose)
    {
      printf("Nb accounts    : %d\n", nb_accounts);
      printf("Duration       : %ds\n", duration);
      printf("Check acc rate : %d\n", check - write_all);
      printf("Transfer rate  : %d\n", 100 - check);
      printf("# Read cores   : %d\n", read_cores);
    }
  /* normalize percentages to 128 */

  double normalize = (double) 128/100;
  check *= normalize;
  write_all *= normalize;
  read_all *= normalize;

  TM_START();

  bank = (bank_t*) malloc(sizeof (bank_t));
  if (bank == NULL)
    {
      printf("malloc bank");
      exit(1);
    }

  bank->accounts = (account_t *) malloc(nb_accounts * sizeof (account_t));
  if (bank->accounts == NULL)
    {
      printf("malloc bank->accounts");
      exit(1);
    }

  bank->size = nb_accounts;

	
  {
    int i;
    for (i = 0; i < bank->size; i++)
      {
	bank->accounts[i].number = i;
	bank->accounts[i].balance = 0;
      }
  }

  uint32_t tot = total(bank, 0);
  if (test_verbose)
    {
      printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\tBank total (before): %d\n",
	     tot);
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
      data[t].check = check;
      data[t].read_all = read_all;
      data[t].write_all = write_all;
      data[t].read_cores = read_cores;
      data[t].write_cores = write_cores;
      data[t].nb_transfer = 0;
      data[t].nb_checks = 0;
      data[t].nb_read_all = 0;
      data[t].nb_write_all = 0;
      data[t].nb_accounts = bank->size;
      data[t].duration = duration;
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

  TM_STOP();

  if (test_verbose)
    {
      for(t = 0; t < num_threads; t++)
	{
	  printf("---Core %ld\n  #transfer   : %zu\n  #checks     : %zu\n  #read-all   : %zu\n  #write-all  : %zu\n",
		 t, data[t].nb_transfer, data[t].nb_checks, data[t].nb_read_all, data[t].nb_write_all);
	}
    }



  tot = total(bank, 0);
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\tBank total  (after): %u\n", tot);
  if (tot != 0)
    {
      printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\tBank total must always be 0!\n");
    }
  assert(tot == 0);

  TM_STATS(duration);


  /* Delete bank and accounts */
  free(bank->accounts);
  free(bank);
}
