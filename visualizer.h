#ifndef VISUALIZER_HEADER_GUARD
#define VISUALIZER_HEADER_GUARD

#include <stdlib.h>
#include <stdio.h>
#include <string.h> // for memset
#include "memlib.h"
#include "alloc_types.h"

// Heap Visualizer - visually display the state of the heap, as encoded
// in the heap metadata. This cannot help you if the metadata are wrong,
// and this should be used in conjunction with the heap checker.

// ** State enumerations ** //

// States of a cell in a small run
// We will represent filled with 'O', unfilled with '.', and never with '-'
#define FILLED 0
#define UNFILLED 1
#define NEVER 2

// ** Prototypes ** //
void visualize_large_run(arena_chunk_hdr* this_chunk, size_t this_page);
void visualize_small_run(arena_chunk_hdr* this_chunk, size_t this_page);
void visualize_chunk(arena_chunk_hdr* this_chunk);
inline size_t small_address_to_cell(small_run_hdr* this_hdr, size_t* this_cell, size_t object_size); 

/*************
 * Functions *
 *************/

void visualize_arena(arena_hdr* this_arena) {
  // Visualizes only the first chunk, because I said so.
  // TODO: Visualize more chunks.
  //visualize_chunk((arena_chunk_hdr*)(this_arena->deepest));
  visualize_chunk((arena_chunk_hdr*)((byte*)this_arena + ARENA_HDR_SIZE));
}

void visualize_chunk(arena_chunk_hdr* this_chunk) {
  printf("\n\n** Visualizing chunk at heap point %p **\n", this_chunk);
  // To compute position in heap, we get our delta to the true heap bottom, and divide
  // by the size of chunks. 
  printf("This is %zu chunk(s) up from the bottom of the heap.\n",  
	 (long)((byte*)this_chunk - (byte*)mem_heap_lo())/ FINAL_CHUNK_SIZE);
  printf("It also has %zu free pages.\n", this_chunk->num_pages_available);
  printf("Page map:\n");

  // Iterate over the page map, doing work
  size_t ii;
  for (ii=0 ; ii < this_chunk->num_pages_allocated ; ii++) {
    // Print page number
    printf(" % 4zu : ", ii);
    switch (this_chunk->page_map[ii]) {
      // Print trivial cases
    case HEADER:
      printf("Arena chunk metadata.\n");
      break;
    case FREE:
      printf("Unallocated page.\n");
      break;
    case SMALL_RUN_FRAGMENT:
      printf("... small run continues ...\n");
      break;
    case LARGE_RUN_FRAGMENT:
      printf("... large run continues ...\n");
      break;
      
      // Nontrivial case - header of a large run.
    case LARGE_RUN_HEADER:
      visualize_large_run(this_chunk, ii);
      break;
      
      // Very nontrivial case - header of a small run
    case SMALL_RUN_HEADER:
      visualize_small_run(this_chunk, ii);
      break;
    default:
      printf("This value does not make sense: %d.\n", this_chunk->page_map[ii]);
      break;
    }
  }
  printf("\n");
}

void visualize_large_run(arena_chunk_hdr* this_chunk, size_t this_page) {
  // Extract run location, convert to run header, analyze
  large_run_hdr* this_hdr = (large_run_hdr *) (this_chunk->get_page_location((size_t)this_page));
  printf("Large run begins, spanning %zu pages.\n",
	 (this_hdr->num_pages));
}

void visualize_small_run(arena_chunk_hdr* this_chunk, size_t this_page) {
  // Extract location etc.
  small_run_hdr* this_hdr = (small_run_hdr *) (this_chunk->get_page_location((size_t)this_page));
  size_t num_cells = this_hdr->parent->available_registrations;
  size_t object_size = this_hdr->parent->object_size;

  printf("Small run begins, containing %zu slots of size %zu bytes.\n",
	 (num_cells),
	 (object_size));
  printf("        In total, this run spans %zu bytes.\n",
	 (this_hdr->parent->run_length));
  printf("        Cells are mapped as follows:");

  // Now to visualize the run. Start by assigning every cell FILLED.
  uint8_t cells[num_cells];
  int ii;
  memset(&cells, FILLED, num_cells * sizeof(uint8_t));

  // Then mark everything at or after the NEXT pointer NEVER
  if (this_hdr->next != NULL) {
    size_t trailing_free_cell_start = small_address_to_cell(this_hdr, this_hdr->next, object_size);
    for (ii = trailing_free_cell_start ; ii < num_cells ; ii++)
      cells[ii] = NEVER;
  }

  // Then follow the free list, marking those cells UNFILLED
  size_t* follow_free = this_hdr->free_list;
  size_t follow_free_cell;
  while (follow_free != NULL) {
    follow_free_cell = small_address_to_cell(this_hdr, follow_free, object_size);
    cells[follow_free_cell] = UNFILLED;
    // Since we're following the free list, we know the contents of a cell is 
    // a pointer to the next free cell or to NULL, and can follow it down
    follow_free = (size_t*)(*follow_free);
  }

  // Print the visualization, up to 50 cells per line, using a single
  // symbol to represent the state of each cell
  for (ii = 0 ; ii < num_cells ; ii++) {
    // Linebreak every 64 chars
    if (ii%64 == 0) {
      printf("\n        ");
    }

    switch (cells[ii]) {
    case FILLED:
      printf("O");
      break;
    case UNFILLED:
      printf(".");
      break;
    case NEVER:
      printf("-");
      break;
    }

  }
  printf("\n");

}

inline size_t small_address_to_cell(small_run_hdr* this_hdr, size_t* this_cell, size_t object_size) {
  // Given an address in a header, determine what cell number that is in the cell map
  // Direct address subtraction is fun!
  size_t delta = (size_t)((byte*)this_cell - (byte*)this_hdr) - SMALL_RUN_HDR_SIZE;
  return (size_t)(delta / object_size);
} 

#endif /* VISUALIZER_HEADER_GUARD */
