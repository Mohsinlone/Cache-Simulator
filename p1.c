#include "p1.h"

cache *l1 = NULL, *l2 = NULL;
u_int address;
u_int mem_traffic;
double swap_req, swap;

// initialize the cache, allocate memory for cache. 
void init_cache (cache *cache, u_int blocksize, u_int size, u_int assoc)
{
  int i, j;

  cache->set = size/(blocksize * assoc);
  cache->assoc = assoc;
  cache->blocksize = blocksize;
 
  cache->entry = (u_int **)calloc (cache->set, sizeof(u_int *));
  for (i = 0; i < cache->set; ++i)
  {
    cache->entry[i] = (u_int *)calloc (assoc, sizeof(u_int));
  } 

  cache->address = (u_int **)calloc (cache->set, sizeof(u_int *));
  for (i = 0; i < cache->set; ++i)
  {
    cache->address[i] = (u_int *)calloc (assoc, sizeof(u_int));
  } 

  cache->lru = (u_int **)calloc (cache->set, sizeof(u_int *));  
  for (i = 0; i < cache->set; ++i)
  {
    cache->lru[i] = (u_int *)calloc (assoc, sizeof(u_int));
    for (j = 0; j < assoc; ++j)
    {
      cache->lru[i][j] = j;
    }
  } 

  cache->valid = (char **)calloc (cache->set, sizeof(char *));  
  for (i = 0; i < cache->set; ++i)
  {
    cache->valid[i] = (char *)calloc (assoc, sizeof(char));
  } 

  cache->dirty = (char **)calloc (cache->set, sizeof(char *));
  for (i = 0; i < cache->set; ++i)
  {
    cache->dirty[i] = (char *)calloc (assoc, sizeof(char));
  } 

}

// free the cache memory
void free_cache (cache *cache)
{
  int i;
 
  for (i = 0; i < cache->set; ++i)
  {
    free (cache->entry[i]);
  } 
  free (cache->entry);


  for (i = 0; i < cache->set; ++i)
  {
    free (cache->address[i]);
  } 
  free (cache->address);


  for (i = 0; i < cache->set; ++i)
  {
     free (cache->lru[i]);
  }
  free (cache->lru);


  for (i = 0; i < cache->set; ++i)
  {
    free(cache->valid[i]);
  } 
  free (cache->valid);


  for (i = 0; i < cache->set; ++i)
  {
    free (cache->dirty[i]);
  } 
  free (cache->dirty);

  free (cache);
}

// print function to print cache contents from MRU to LRU
void print_contents (cache *cache)
{
  u_int count;
  int i, j;
  
  for (j = 0; j < cache->set; ++j)
  {
    printf ("  set%4d:  ", j);
    count = 0;
    while (count < cache->assoc)
    {
      for (i = 0; i < cache->assoc; ++i)
      {
        if (cache->valid[j][i] && (cache->lru[j][i] == count))
        { 
          if (cache->entry[j][i])
            printf ("%4x ", cache->entry[j][i]);
          if (cache->dirty[j][i])
            printf ("D ");
          else
            printf ("  ");

          break;
        }  
      }
      count++;
    }
 
    printf ("\n");

  }
}

// returns tag of an address for the cache
u_int gettag (cache *cache, u_int add)
{
  u_int tag, tagbits, mask;
  tagbits = WORD_SIZE - log2 (cache->blocksize) - log2 (cache->set);  
  mask = pow (2, tagbits) - 1;
  tag = ((add >> (WORD_SIZE - tagbits)) & mask);

  return tag;
}

//returns index of address for the given cache
u_int getindex (cache *cache, u_int add)
{
  u_int index, indexbits, mask;
  indexbits = log2 (cache->set);  
  mask = pow (2, indexbits) - 1;
  index = ((add >> (int)log2 (cache->blocksize)) & mask);

  return index;
}

// check if address is present in victim. 
// if found, return the index of address in the set, else return -1. 
int find_in_victim (cache *cache, u_int add)
{
  int off, i;
  off = (int) log2 (cache->blocksize);
  u_int tag;
  tag = add >> off;

  swap_req++;

  for (i = 0; i < cache->assoc; ++i)
  {
    if (cache->entry[0][i] == tag)
      return i;
  }
  
  return -1;
}

// check if given set in the cache has any free blocks. 
// if avaialble, return index of free block in cache set, else return -1. 
int is_free (cache *cache, u_int set)
{
  int i = 0;
 
  for (i = 0; i < cache->assoc; ++i)
  {
    if (cache->valid[set][i] == 0)
      return i;
  }
  
  return -1;  
}

// return the index of lru block in the give cache set
u_int getlru (cache *cache, u_int set)
{
  int i;
  
  for (i = 0; i < cache->assoc; ++i)
  {
    if (cache->lru[set][i] == (cache->assoc - 1))
      return i;
  }

  return cache->assoc;
}

// set the block in the cache as MRU
void set_mru (cache *cache, u_int set, u_int index)
{
  int i;

  for (i = 0; i < cache->assoc; ++i)
  {
    if (cache->lru[set][i] < cache->lru[set][index])
      cache->lru[set][i]++;
  }
  
  cache->lru[set][index] = 0;
}

// add the given address to cache at specificed position in cache.
void add_to_cache (cache *cache, u_int address, u_int set, u_int index)
{
  u_int tag;
  tag = gettag (cache, address);
  cache->entry[set][index] = tag;
  cache->address[set][index] = address;
  cache->valid[set][index] = 1;
}

// cache write function
void cwrite (cache *cache, u_int add)
{
  u_int set, tag;
  int i, loc;
  cache->write++;

  set = getindex (cache, add);
  tag = gettag (cache, add);

  // search the cache
  for (i = 0; i < cache->assoc; ++i)
  {
    if (cache->valid[set][i] && (cache->entry[set][i] == tag))
    {
      set_mru (cache, set, i);
      cache->whit++;
      cache->dirty[set][i] = 1;
      return;
    }
  }

  // not found in this cache, update the miss counter
  cache->wmiss++;
  // search for free block if any in the current cache. 
  // is_free returns the free block in current set.
  if ((loc = is_free (cache, set)) >= 0)
  {
    // if free block is found, read from next level, skip the VC
    if (cache->next_level)
    {
      cread (cache->next_level, add);
      // after the read, update the current cache. 
      cache->entry[set][loc] = tag;
      cache->valid[set][loc] = 1;
      // store the current address 
      cache->address[set][loc] = add;
      cache->dirty[set][loc] = 1;
      set_mru (cache, set, loc);

      return;
    }
    else
    {
      // If cache has no next_level, this is last level of cache, access memory
      mem_traffic++;
      // update the current cache, set valid bit, save address in current cache block
      cache->entry[set][loc] = tag;
      cache->valid[set][loc] = 1;
      cache->address[set][loc] = address;
      cache->dirty[set][loc] = 1;
      set_mru (cache, set, loc);

      return;
    }
  }  // if (is_free) end
  // free block not found, need to find lru and evict it. 
  else
  {
    // this is the block ([set][loc]) that will be evicted from current cache. 
    loc = getlru (cache, set);
    
    // evict to victim cache if present
    if (cache->victim)
    {
      int vloc;
      char tmp_dirty;
      u_int tmp_address;

      // check for the address in the victim cache
      vloc = find_in_victim (cache->victim, add);
      
      if (vloc >= 0)
      {
        // address found in victim, need to swap

        // temp store current cache LRU block address and dirty bit
        tmp_dirty = cache->dirty[set][loc];
        tmp_address = cache->address[set][loc];
        
        // swap dirty bits of cache and VC
        cache->dirty[set][loc] = cache->victim->dirty[0][vloc];
        cache->victim->dirty[0][vloc] = tmp_dirty;

        // add the current address to cache, since it was found in VC, brought into cache from VC.
        add_to_cache (cache, add, set, loc);
        // set the [set][loc] as MRU
        set_mru (cache, set, loc);
        cache->dirty[set][loc] = 1;
 
        // add the evicted block of cache to VC
        add_to_cache (cache->victim, tmp_address, 0, vloc);
        // set [0][vloc] block of VC as MRU
        set_mru (cache->victim, 0, vloc);

        swap++;
        return;
      }
      else
      // not found in VC
      {
        // check if vc has a free block
        vloc = is_free (cache->victim, 0);

        // if VC does not have a free block, evict the lru block of VC
        if (vloc < 0)
        {
          vloc = getlru (cache->victim, 0);

          // check for dirty bit, if 1 then write to next level
          if (cache->victim->dirty[0][vloc])
          {
            u_int vcadd;
            vcadd = cache->victim->address[0][vloc];
            if (cache->next_level)
            {
              cwrite (cache->next_level, vcadd);
            }
            else
              mem_traffic++;
          
            // increment the writeback counter and set dirty bit to 0 and valid bit to 0 to indicate eviction of the block.
            cache->wb++;
            cache->victim->dirty[0][vloc] = 0;
            cache->victim->valid[0][vloc] = 0;
          }
        }
        // free block found in VC
        // add the lru block of current cache to VC        
        add_to_cache (cache->victim, cache->address[set][loc], 0, vloc);
        cache->victim->dirty[0][vloc] = cache->dirty[set][loc];
        set_mru (cache->victim, 0, vloc);

        // current cache issues read to next level for address
        if (cache->next_level) 
          cread (cache->next_level, add);
        else
          mem_traffic++;

        // add the address to current cache
        cache->entry[set][loc] = tag;
        cache->address[set][loc] = address;
        cache->valid[set][loc] = 1;
        cache->dirty[set][loc] = 1;
        set_mru (cache, set, loc);

        return;
      } // end of else
        
    } // end of cache->victim
    else
    // victim cache is not present and current cache is full
    { 
      // check if cache has next level 
      if (cache->next_level)
      {
        // if dirty bit is set, then issue write to next level
        if (cache->dirty[set][loc])
        {
          cwrite (cache->next_level, cache->address[set][loc]);
          cache->dirty[set][loc] = 0;
          cache->wb++;
        }
        
        // read the block from next level and add it to the cache. 
        cread (cache->next_level, add);
        add_to_cache (cache, add, set, loc);
        set_mru (cache, set, loc);
        // preform write on the newly bought in block 
        cache->dirty[set][loc] = 1;

        return;
      }
      else
      { // cache does not have next level, access the memory
        if (cache->dirty[set][loc])
        {
          // write lru block to memory
          mem_traffic++;
          cache->wb++;
          cache->dirty[set][loc] = 0;
        }
  
        // fetch new address from memory
        mem_traffic++;        
        add_to_cache (cache, add, set, loc);
        set_mru (cache, set, loc);
        cache->dirty[set][loc] = 1;

        return;
      }
    }
  }
}

void cread (cache *cache, u_int add)
{
  u_int set, tag;
  int i, loc;
  cache->read++;
  set = getindex (cache, add);
  tag = gettag (cache, add);

  // search the cache
  for (i = 0; i < cache->assoc; ++i)
  {
    if (cache->valid[set][i] && (cache->entry[set][i] == tag))
    {
      set_mru (cache, set, i);
      cache->rhit++;
      return;
    }
  }

  // not found in this cache, update the miss
  cache->rmiss++;
  // not found in cache, then search for free block
  // is_free returns the free block in current set
  if ((loc = is_free (cache, set)) >= 0)
  {
    // if free block is found, read from next level, skip the VC
    if (cache->next_level)
    {
      cread (cache->next_level, add);
      // after the read, update the current cache. 
      cache->entry[set][loc] = tag;
      cache->valid[set][loc] = 1;
      // store the current address 
      cache->address[set][loc] = add;
      cache->dirty[set][loc] = 0;
      set_mru (cache, set, loc);

      return;
    }
    else
    {
      // this is last level of cache, access memory
      mem_traffic++;
      // update the current cache, set valid bit, save address
      cache->entry[set][loc] = tag;
      cache->valid[set][loc] = 1;
      cache->address[set][loc] = address;
      cache->dirty[set][loc] = 0;
      set_mru (cache, set, loc);

      return;
    }
  }  // if (is_free) end
  // free block not found, need to find lru and evict it. 
  else
  {
    // this is the block ([set][loc]) that will be evicted from current cache. 
    loc = getlru (cache, set);
    
    // evict to victim cache if present
    if (cache->victim)
    {
      int vloc;
      char tmp_dirty;
      u_int tmp_address;

      // check for current address in the victim cache
      vloc = find_in_victim (cache->victim, add);
      
      if (vloc >= 0)
      {
        // address found in victim, need to swap

        // temp store current cache LRU block address and dirty bit
        tmp_dirty = cache->dirty[set][loc];
        tmp_address = cache->address[set][loc];
        
        // swap dirty bits of cache and VC
        cache->dirty[set][loc] = cache->victim->dirty[0][vloc];
        cache->victim->dirty[0][vloc] = tmp_dirty;

        // add the current address to cache, since it was found in VC, brought into cache from VC.
        add_to_cache (cache, add, set, loc);
        // set the [set][loc] as MRU
        set_mru (cache, set, loc);
 
        // add the evicted block of cache to VC
        add_to_cache (cache->victim, tmp_address, 0, vloc);
        // set [0][vloc] block of VC as MRU
        set_mru (cache->victim, 0, vloc);

        swap++;
        return;
      }
      else
      // not found in VC
      {
        // check if vc has a free block
        vloc = is_free (cache->victim, 0);

        // if VC does not have a free block, evict the lru block of VC
        if (vloc < 0)
        {
          vloc = getlru (cache->victim, 0);

          // check for dirty bit, if 1 then write to next level
          if (cache->victim->dirty[0][vloc])
          {
            u_int vcadd;
            vcadd = cache->victim->address[0][vloc];
            if (cache->next_level)
            {
              cwrite (cache->next_level, vcadd);
            }
            else
              mem_traffic++;
            
            cache->wb++;
            cache->victim->dirty[0][vloc] = 0;
            cache->victim->valid[0][vloc] = 0;
          }
        }
        // free block found in VC
        // add the lru block of current cache to VC        
        add_to_cache (cache->victim, cache->address[set][loc], 0, vloc);
        cache->victim->dirty[0][vloc] = cache->dirty[set][loc];
        set_mru (cache->victim, 0, vloc);

        // current cache issues read to next level for address 
        if (cache->next_level) 
          cread (cache->next_level, add);
        else
          mem_traffic++;
        // add the address to current cache
        cache->entry[set][loc] = tag;
        cache->address[set][loc] = address;
        cache->valid[set][loc] = 1;
        cache->dirty[set][loc] = 0;
        set_mru (cache, set, loc);

        return;
      } // end of else
        
    } // end of cache->victim
    else
    // victim cache is not present and current cache is full
    {
      loc = getlru (cache, set);
      
      if (cache->next_level)
      { // if dirty bit is set, write to next level before evicting from current cache
        if (cache->dirty[set][loc])
        {
          cwrite (cache->next_level, cache->address[set][loc]);
          cache->dirty[set][loc] = 0;
          cache->wb++;
        }
        
        // read new block from next level 
        cread (cache->next_level, add);
        add_to_cache (cache, add, set, loc);
        set_mru (cache, set, loc);
        return;
      }
      else
      { // current cache does not have a next level, access the memory
        if (cache->dirty[set][loc])
        {
          // write lru block to memory
          cache->wb++;
          mem_traffic++;
          cache->dirty[set][loc] = 0;
        }
     
        // read the new address block from memory
        mem_traffic++;        
        add_to_cache (cache, add, set, loc);
        set_mru (cache, set, loc); 
        return;
      }
    }

 }
}


int main (int argc, char *argv[])
{
  
  u_int blocksize = atoi (argv[1]);
  u_int l1size = atoi (argv[2]);
  u_int l1assoc = atoi (argv[3]);
  u_int vc = atoi (argv[4]);
  u_int l2size = atoi (argv[5]);
  u_int l2assoc = atoi (argv[6]);
  char *file = argv[7];
 
  printf ("===== Simulator configuration =====\n");
  printf ("  BLOCKSIZE:           %10u\n", blocksize);
  printf ("  L1_SIZE:             %10u\n", l1size);
  printf ("  L1_ASSOC:            %10u\n", l1assoc);
  printf ("  VC_NUM_BLOCKS:       %10u\n", vc);
  printf ("  L2_SIZE:             %10u\n", l2size);
  printf ("  L2_ASSOC:            %10u\n", l2assoc);
  printf ("  trace_file:     %15s\n", file);
  printf ("\n");

  // create l1 cache
  l1 = (struct cache_s *)calloc (1, sizeof(cache));
  init_cache (l1, blocksize, l1size, l1assoc);

  // if l2 is specified, create l2 cache
  if (l2size)
  {
    l2 = (struct cache_s *)calloc (1, sizeof(cache));
    init_cache (l2, blocksize, l2size, l2assoc);
    l2->victim = NULL;
    l1->next_level = l2;
    l2->next_level = NULL;
  }
  else
  {
    l1->next_level = NULL;
  }

  // if vc_num_blocks is specified, create VC
  if (vc)
  {
    l1->victim = (struct cache_s *)calloc (1, sizeof(cache));
    init_cache (l1->victim, blocksize, vc*blocksize, vc);
  }
  else 
   l1->victim = NULL;
 
  FILE *fp;
  char op;

  // open the trace file and read line by line
  fp = fopen (file, "r+");
  while (fscanf (fp,"%c %x\n", &op, &address) == 2)
  {
    if (op == 'r')
    {
      cread (l1, address);
    }
    else if (op == 'w')
    {
      cwrite (l1, address);
    }
  }

  if (l1)
  {
    printf ("===== L1 contents =====\n");
    print_contents (l1);
    printf ("\n");
  }
  
  if (l1 && l1->victim)
  {
    printf ("===== VC contents =====\n");
    print_contents (l1->victim);
    printf ("\n");
  }

  if (l2)
  {
    printf ("===== L2 contents =====\n");
    print_contents (l2);
    printf ("\n");
  }
 
  printf ("===== Simulation results =====\n");

  int l1read = 0, l1rmiss = 0, l1write = 0, l1wmiss = 0, l1vcwb = 0;    
  int l2read = 0, l2rmiss = 0, l2write = 0, l2wmiss = 0, l2wb = 0;   
  double srr = 0, l1_vc_missrate = 0, l2_missrate = 0;
 
  if (l1)
  {
    l1read = l1->read;
    l1rmiss = l1->rmiss;
    l1write = l1->write;
    l1wmiss = l1->wmiss;

    if (l1read || l1write)
      srr = swap_req/(l1read + l1write);
  
    if (l1read || l1write)
      l1_vc_missrate = (l1rmiss + l1wmiss - swap)/(l1read + l1write);

    if (l1->victim)
      l1vcwb = l1->wb + l1->victim->wb;
    else
      l1vcwb = l1->wb;
  }
  
  if (l2)
  {
    l2read = l2->read;
    l2rmiss = l2->rmiss;
    l2write = l2->write;
    l2wmiss = l2->wmiss;
    l2wb = l2->wb;
    if (l2read)
      l2_missrate = (double)l2rmiss/l2read;
  }

  
    printf ("a. number of L1 reads:       	   	%5d\n", l1read);
    printf ("b. number of L1 read misses:  	   	%5d\n", l1rmiss);
    printf ("c. number of L1 writes:        	   	%5d\n", l1write);
    printf ("d. number of L1 write misses:  	   	%5d\n", l1wmiss);
    printf ("e. number of swap requests:    	   	%5d\n", (int)swap_req);
    printf ("f. swap request rate:                     %.4f\n", srr);
    printf ("g. number of swaps:                   	%5d\n", (int)swap);
    printf ("h. combined L1+VC miss rate:              %.4f\n", l1_vc_missrate);
    printf ("i. number writebacks from L1/VC:   	%5d\n", l1vcwb);
    printf ("j. number of L2 reads:       	   	%5d\n", l2read);
    printf ("k. number of L2 read misses:  	   	%5d\n", l2rmiss);
    printf ("l. number of L2 writes:        	   	%5d\n", l2write);
    printf ("m. number of L2 write misses:  	   	%5d\n", l2wmiss);
    printf ("n. L2 miss rate:                          %.4f\n", l2_missrate);
    printf ("o. number of writebacks from L2:      	%5d\n", l2wb);
    printf ("p. total memory traffic:              	%5d\n", mem_traffic);
  
  
  if (l2)
    free_cache (l2);
  if (l1 && l1->victim)
    free_cache (l1->victim);
  if (l1)
    free_cache (l1);
  
  return 0;
}
