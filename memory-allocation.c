#define NULL 0
#define BLKSIZ 65536

extern long grow(int blocks);
extern unsigned int memoryBytesLength();

void *allocateMemory (unsigned bytes_required);
void freeMemory(void *startOfMemoryToFree);

typedef long align;

// header of an allocation block
typedef union header {
  struct {
	  union header *next; // Pointer to circular successor
  	unsigned int size; // Size of the block
  } value;
  long align; // Forces block alignment
} header_t;

static header_t *free_list = (header_t*)NULL;
static unsigned int initial_offset = 8; // preserve address 0 for null and pad by 4 bytes.
static int is_initialised = 0;

static header_t* getMoreMemory(unsigned bytes_required)
{
  // We need to add the header to the bytes required.
  bytes_required += sizeof(header_t);

  // The memory gets delivered in blocks. Ensure we get enough.
  unsigned int blocks = bytes_required / BLKSIZ;
  if (blocks * BLKSIZ < bytes_required)
    blocks += 1;
  unsigned int start_of_new_memory = memoryBytesLength();
  long end_of_new_memory = grow(blocks);

  if (end_of_new_memory == 0) // grow returns 0 in the event of an error
  	return NULL;

  // Create the block to insert.
  header_t* block_to_insert = (header_t *) start_of_new_memory;
  block_to_insert->value.size = end_of_new_memory - start_of_new_memory - sizeof(header_t);
  block_to_insert->value.next = NULL;

  // add to the free list
  freeMemory((void *) (block_to_insert + 1));

  return free_list;
}

static void ensureInitialised()
{
  if (is_initialised == 0)
  {
    is_initialised = 1;

    // initialise the memory allocator.
    unsigned int bytes_length = memoryBytesLength();

    // Start at 1 to save 0 for NULL.
    header_t* unallocated = (header_t*) initial_offset;
    unallocated->value.size = bytes_length - (sizeof(header_t) + initial_offset);
    unallocated->value.next = NULL;

    free_list = unallocated;
  }
}

__attribute__((used)) void *allocateMemory(unsigned bytes_required)
{
  ensureInitialised();

  // Pad to 8 bytes until I find a better solution.
  bytes_required += (8 - bytes_required % 8) % 8;
  unsigned int bytes_required_plus_header = bytes_required + sizeof(header_t);

  if (free_list == NULL)
  {
    free_list = getMoreMemory(bytes_required_plus_header);
    if (free_list == NULL)
      return NULL;
  }

  header_t* current = free_list;
  header_t* previous = current;
  while (current != NULL)
  {
    if (current->value.size == bytes_required)
    {
      // exact match
      if (current == free_list)
      {
        // allocate all of the free list
        free_list = NULL;
        current->value.next = NULL;
        return current + 1;
      }
      else
      {
        // remove the block
        previous->value.next = current->value.next;
        current->value.next = NULL;
        return current + 1;
      }     
    }
    else if (current->value.size > bytes_required)
    {
      // split the bigger block

      // create the unallocated portion
      header_t* unallocated = (header_t*)((char*)current + bytes_required_plus_header);
      unallocated->value.size = current->value.size - bytes_required_plus_header;
      unallocated->value.next = current->value.next;

      if (current == free_list)
      {
        // We are at the start of the list so make the free listthe unallocated block.
        free_list = unallocated;
      }
      else
      {
        // We past the start of the list so remove the current block.
        previous->value.next = unallocated;
      }

      // prepare the allocated portion.
      current->value.size = bytes_required;
      current->value.next = NULL;

      return current + 1;
    }

    previous = current;
    current = current->value.next;
  }

  // No block was big enough. Grow the memory and try again
  if (getMoreMemory(bytes_required) == NULL)
    return NULL;

  return allocateMemory(bytes_required_plus_header);
}

__attribute__((used)) void freeMemory(void *ptr)
{
  ensureInitialised();

  if (ptr == NULL)
    return;
    
  header_t* unallocated = ((header_t *) ptr) - 1;

  if (free_list == NULL)
  {
    // If the free list is null the unallocated block becomes the free list.
    free_list = unallocated;
    return;
  }

  // Find the place in the free list where the unallocated block should be inserted.
  header_t* current = free_list;
  header_t* previous = current;
  while (current != NULL)
  {
    if (unallocated > previous  && unallocated < current)
      break; // The unallocated block is between the previous and current.
    else if (current == previous && unallocated < current)
    {
      // There is only one block in the list and it is after the unallocated.
      previous = NULL;
      break;
    }

    // Step forward.
    previous = current;
    current = current->value.next;
  }

  // Attach the unallocated block to the current block.
  if (current != NULL)
  {
    // Are the blocks adjacent?
    if (current == (header_t*)((char*)(unallocated + 1) + unallocated->value.size))
    {
      // Merge the unallocated with the current block.
      unallocated->value.size += current->value.size + sizeof(header_t);
      unallocated->value.next = current->value.next;
    }
    else
    {
      // Chain the unallocated block to the current.
      unallocated->value.next = current;
    }
  }

  if (previous == NULL)
  {
    // The unallocated block now starts the free list.
    free_list = unallocated;
  }
  else
  {
    // Are the blocks adjacent?
    if (unallocated == (header_t*)((char*)(previous + 1) + previous->value.size))
    {
      // Merge the previous block with the unallocated.
      previous->value.size += unallocated->value.size + sizeof(header_t);
      previous->value.next = unallocated->value.next;
    }
    else
    {
      // Chain the previous block to the unallocated.
      previous->value.next = unallocated;
    }
  }
}

__attribute__((used)) double reportFreeMemory()
{
  ensureInitialised();

  if (free_list == NULL)
    return 0;

  header_t* current = free_list;
  unsigned int total = 0;
  while (current != NULL)
  {
    total += current->value.size;
    current = current->value.next;
  }

  return (double)total;
}