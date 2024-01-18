#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pagetable.h"

extern int memsize;
extern bool debug;
extern struct frame *coremap;

int first_in;

/* Page to evict is chosen using the FIFO algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int fifo_evict() {
	//The first frame page out is the first frame page in
	int first_out = first_in;
	//Increase first frame page in by 1
	first_in++;
	if (first_in == memsize) {
		first_in = 0;
	}
	//Return first frame page out
	return first_out;
}

/* This function is called on each access to a page to update any information
 * needed by the FIFO algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void fifo_ref(pt_entry_t *pte) {

	// This is keep the compiler happy, until you implement this.
	(void)pte;
	return;
}

/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void fifo_init() {
	first_in = 0;
}

/* Cleanup any data structures created in fifo_init(). */
void fifo_cleanup() {
	//No data structures to clean up
	return;
}
