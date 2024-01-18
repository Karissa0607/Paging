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

int latest_time;
int *time_reference_list;

/* The page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
	//Find page with oldest time
	int oldest_time = 0;
	for (int i = 0; i < memsize; i++) {
		if(time_reference_list[oldest_time] > time_reference_list[i]){
			oldest_time = i;
		}
	}

	//Evict page with oldest time
	return oldest_time;
}

/* This function is called on each access to a page to update any information
 * needed by the LRU algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pt_entry_t *pte) {
	//increase time stamp
	latest_time++;
	
	//Physical frame number of pte
	int frame_number = pte->frame >> PAGE_SHIFT;
	
	//Store the latest time stamp of the page
	time_reference_list[frame_number] = latest_time;
	
	// This is keep the compiler happy, until you implement this.
	(void)pte;
}


/* Initialize any data structures needed for this
 * replacement algorithm
 */
void lru_init() {
	//Initialize time_reference_list 
	time_reference_list = (int *) malloc(memsize * sizeof(int));
	if (time_reference_list == NULL) {
                exit(1);
        }

	//Initialize time_reference_list  to 0
	for (int i = 0; i < memsize; i++) {
                time_reference_list[i] = 0;
        }

	//Initialize the latest time to 0
	latest_time = 0;
}

/* Cleanup any data structures created in lru_init(). */
void lru_cleanup() {
	free(time_reference_list);
}

