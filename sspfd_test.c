
#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <inttypes.h>

#define XSTR(s)                         STR(s)
#define STR(s)                          #s

#include "sspfd.h"

#define DEFAULT_NUM_OPS                 100000
#define DEFAULT_NUM_STORES              2


size_t num_ops =  DEFAULT_NUM_OPS;
size_t num_stores = DEFAULT_NUM_STORES;

int
main(int argc, char **argv)
{
  struct option long_options[] = {
    // These options don't set a flag
    {"help",                      no_argument,       NULL, 'h'},
    {"num_ops",                   required_argument, NULL, 'o'},
    {"num_stores",                required_argument, NULL, 's'},
    {NULL, 0, NULL, 0}
  };

  int i, c;

  while(1) 
    {
      i = 0;
      c = getopt_long(argc, argv, "ho:s:", long_options, &i);

      if(c == -1)
	break;

      if(c == 0 && long_options[i].flag == 0)
	c = long_options[i].val;

      switch(c) {
      case 0:
	/* Flag is automatically set */
	break;
      case 'h':
	printf("sspfd_test -- testing sspfd\n"
	       "\n"
	       "Usage:\n"
	       "  sspfd_test [options...]\n"
	       "\n"
	       "Options:\n"
	       "  -h, --help\n"
	       "        Print this message\n"
	       "  -o, --num_ops <int>\n"
	       "        Test for number of operations (0=infinite, default=" XSTR(DEFAULT_NUM_OPS) ")\n"
	       "  -s, --num_stores <int>\n"
	       "        number of sspfd stores to be used (default=" XSTR(DEFAULT_DSL_PER_CORE) ")\n"
	       );
	exit(0);
      case 'o':
	num_ops = atoi(optarg);
	break;
      case 's':
	num_stores = atoi(optarg);
	break;
      case '?':
	printf("Use -h or --help for help\n");
	exit(0);
      default:
	exit(1);
      }
    }

  assert(num_stores > 0);   
  assert(num_ops > 0);
  
  SSPFD_PRINT("* initializing %lu stores of %lu entries", num_stores, num_ops);
  sspfd_store_init(num_stores, num_ops, 0);

  volatile int* dummy = (volatile int*) malloc(sizeof(int));
  assert(dummy != NULL);
  *dummy = 13;


  while (num_stores-- > 0)
    {
      SSPFD_PRINT("* testing store %lu", num_stores);

      switch(num_stores)
	{
	case 0:
	  {
	    SSPFD_PRINT("** asm volatile(\"nop\")");

	    size_t r;
	    for (r = 0; r < num_ops; r++)
	      {
		SSPFDI(num_stores);
		asm volatile ("nop");
		SSPFDO(num_stores, r);
	      }

	    sspfd_stats_t stats;
	    sspfd_get_stats(num_stores, num_ops, &stats);
	    sspfd_print_stats(&stats);
	  }
	  break;
	case 1:
	  {
	    SSPFD_PRINT("** asm volatile(\"\")");

	    size_t r;
	    for (r = 0; r < num_ops; r++)
	      {
		SSPFDI(num_stores);
		asm volatile ("");
		SSPFDO(num_stores, r);
	      }

	    sspfd_stats_t stats;
	    sspfd_get_stats(num_stores, num_ops, &stats);
	    sspfd_print_stats(&stats);
	  }
	  break;
	case 2:
	  {
	    SSPFD_PRINT("** asm volatile(\"pause\")");

	    size_t r;
	    for (r = 0; r < num_ops; r++)
	      {
		SSPFDI(num_stores);
		asm volatile ("pause");
		SSPFDO(num_stores, r);
	      }

	    sspfd_stats_t stats;
	    sspfd_get_stats(num_stores, num_ops, &stats);
	    sspfd_print_stats(&stats);
	  }
	  break;
	default:
	  {
	    SSPFD_PRINT("** L1 access");

	    size_t r;
	    for (r = 0; r < num_ops; r++)
	      {
		int tmp = 0;
		SSPFDI(num_stores);
		tmp = *dummy;
		asm volatile ("lfence");
		SSPFDO(num_stores, r);

		if (tmp != 13)
		  {
		    SSPFD_PRINT("what?");
		  }
	      }

	    sspfd_stats_t stats;
	    sspfd_get_stats(num_stores, num_ops, &stats);
	    sspfd_print_stats(&stats);
	  }
	  break;
	}

    }

  sspfd_store_term();
  free((void*) dummy);

  return 0;
}


