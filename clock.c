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

int *referenced_list; 
int pointer;
/* The page to evict is chosen using the CLOCK algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {
	//loop until we find referenced_list[pointer] == 0
	while(1) {
		//The frame has recieved its second chance so change its referenced to 0
		if (referenced_list[pointer]) {
			referenced_list[pointer] = 0;
		} else {
			//Page is going to be evicted so set reference to 1 in order to avoid infinite replacement
			referenced_list[pointer] = 1;
			return pointer;
		}
		//Increase pointer by 1 to check next frame number for next evict 
		pointer++;
		if (pointer == memsize) {
			pointer = 0;
		}
	}
	//This will never happen
	return 0;
}

/* This function is called on every access to a page to update any information
 * needed by the CLOCK algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pt_entry_t *pte) {
	//Get frame number that pte is in and update that it has been referenced
	int frame_number = pte->frame >> PAGE_SHIFT;
	referenced_list[frame_number] = 1;
}

/* Initialize any data structures needed for this replacement
 * algorithm.
 */
void clock_init() {
	referenced_list = (int *) malloc(memsize * sizeof(int));
       	if (referenced_list == NULL) {
		exit(1);
	}
	for (int i = 0; i < memsize; i++) {
		referenced_list[i] = 0;
	}
	pointer = 0;
}

/* Cleanup any data structures created in clock_init(). */
void clock_cleanup() {
	free(referenced_list);
}
