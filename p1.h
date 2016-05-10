#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define WORD_SIZE 32

typedef unsigned int u_int;

typedef struct cache_s 
{
  u_int blocksize;
  u_int **entry;
  u_int set;
  u_int assoc;
  u_int read;
  u_int write;
  u_int wb;
  u_int rmiss;
  u_int wmiss;
  u_int rhit;
  u_int whit;
  char **valid;
  u_int **lru;
  u_int **address;
  char **dirty;
  struct cache_s *next_level;
  struct cache_s *victim;
} cache;

void cread (cache *cache, u_int add);
void cwrite (cache *cache, u_int add);

