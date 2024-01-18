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

int *p;
int c;
int *case_4;
pt_entry_t **T1;
pt_entry_t **T2;
pt_entry_t **B1;
pt_entry_t **B2;
pt_entry_t *evict_page;
//These are indices
int *T1_size;
int *T2_size;
int *B1_size;
int *B2_size;
int *LRU_T1;
int *MRU_T1;
int *LRU_T2;
int *MRU_T2;
int *LRU_B1;
int *MRU_B1;
int *LRU_B2;
int *MRU_B2;

/*
 replace routine
*/
void replace() {
	//Find access hit in B2
	int access_hit_B2 = 0; //0 if false, 1 if true
	for (int i = 0; i < *B2_size; i++) {
                if (B2[i]->frame >> PAGE_SHIFT == page->frame >> PAGE_SHIFT) {
                        access_hit_B2 = 1;
                        break;
                }
        }

        //replace condition
        if (*T1_size != 0 && (*T1_size > *p || (access_hit_B2 == 1 && *T1_size == *p))) {
		//Evict LRU(T1)
		evict_page = T1[*LRU_T1];
                T1[*LRU_T1] = NULL;

		//Shrink T1
                for (int i = *LRU_T1+1; i < *T1_size; i++) {
                        T1[i-1] = T1[i];
                }
                if (*MRU_T1 != 0) {
                        T1[*MRU_T1] = NULL;
                        (*MRU_T1)--;
                }
                (*T1_size)--;

                //Grow B1
                if (*B1_size != 0 && *B1_size != c) {
                        (*MRU_B1)++;
                }
                if (*B1_size != c) {
                        (*B1_size)++;
                } 
		//if B1_size = c
		else {
                        //Make room in B1, shift down and delete lru_b1
                        for (int i = *LRU_B1 + 1; i <= *MRU_B1; i++) {
                                B1[i-1] = B1[i];
                        }
                }
		//Move reference to MRU(B1)
                B1[*MRU_B1] = evict_page;
        } 

	//otherwise
	else {  
		//Evict LRU(T2)
                evict_page = T2[*LRU_T2];
                T2[*LRU_T2] = NULL;
              
		//Shrink T2
		if (*LRU_T2 != 0) {
                        (*LRU_T2)--;
                }
                (*T2_size)--;
                if (*T2_size < 0) {
                        exit(1);
                }

                //Grow B2
                if (*B2_size != 0 && *B2_size != c) {
                        (*LRU_B2)++;
                }
                if (*B2_size != c) {
                        (*B2_size)++;
                }
                for(int i = *LRU_B2-1; i >= *MRU_B2; i--){
                        B2[i+1] = B2[i];
                }

		//Move reference to MRU(B2)
                B2[*MRU_B2] = evict_page;
        }
}

/* The page to evict is chosen using the ARC algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int arc_evict() {
        //calling evict means that the entry is invalid and is either the first time being referenced or is on swap. 
	//So a physical page frame needs to be allocated for the address space page
        //Either case means that the frame is not on T1 or T2, thus a frame needs to be evicted. 
	//We need pte to find the frame number of the physical page that pte is on, this is to test if it is on B1 or B2 or for case 4
        
	//Check for access hit in B1 and find its index
        int access_hit_B1 = 0;
        int access_hit_B1_index = 0;
        for (int i = 0; i < *B1_size; i++) {
                if (B1[i] == page) {
                        access_hit_B1 = 1;
                        access_hit_B1_index = i;
                        break;
                }
        }

	//Check for access hit in B1 and find its index
        int access_hit_B2 = 0;
        int access_hit_B2_index = 0;
        for (int i = 0; i < *B2_size; i++) {
                if (B2[i] == page) {
                        access_hit_B2 = 1;
                        access_hit_B2_index = i;
                        break;
                }
        }
	
	//Case 2: access is a miss, but referenced in B1
        if (access_hit_B1 == 1) {
		//delta = max(B2_size/B1_size, 1)
		int delta = (*B2_size)/(*B1_size);
                if (delta < 1){
                        delta = 1;
                }

		//p = min(p+delta, c)
                delta = delta + (*p);
                if (delta > c) {
                        *p = c;
                } else{
                        *p = delta;
                }

		//apply replace
                replace();

		//Grow T2
                if (*T2_size != 0 && *T2_size != c) {
                        (*LRU_T2)++;
                }
                if (*T2_size != c) {
                        (*T2_size)++;
                }
		for (int i = *LRU_T2-1; i >= *MRU_T2; i--){
                        T2[i+1] = T2[i];
                }

		//Move B1 reference into MRU(T2)
                T2[*MRU_T2] = B1[access_hit_B1_index];

		//Shrink B1
		B1[access_hit_B1_index] = NULL;
                for(int i = access_hit_B1_index+1; i<= *MRU_B1; i++){
                        B1[i-1] = B1[i];
                }
                B1[*MRU_B1] = NULL;
                if (*MRU_B1 != 0) {
                        (*MRU_B1)--;
                }
                (*B1_size)--;
        }

        //Case 3: access is a miss, but referenced in B2
        else if (access_hit_B2 == 1) {
		//delta = max(B1_size/B2_size, 1)
                int delta = (*B1_size)/(*B2_size);
                if (delta < 1){
                        delta = 1;
                }

		//p = max(p-delta, 0)
                delta = (*p) - delta;
                if (delta < 0){
                        *p = 0;
                }
                else {
			*p = delta;
                }

		//apply replace
                replace();

		//Grow T2
                if (*T2_size != 0 && *T2_size != c) {
                        (*LRU_T2)++;
                }
                if (*T2_size != c) {
                        (*T2_size)++;
                }
                for (int i = *LRU_T2 - 1; i >= *MRU_T2; i--){
                        T2[i+1] = T2[i];
                }

		//Move B2 reference into MRU(T2)
                T2[*MRU_T2] = B2[access_hit_B2_index];
                
		//Shrink B2
		B2[access_hit_B2_index] = NULL;
                for(int i = access_hit_B2_index + 1; i<= *LRU_B2; i++){
                        B2[i-1] = B2[i];
                }
                B2[*LRU_B2] = NULL;
                if(*LRU_B2 != 0) {
                        (*LRU_B2)--;
                }
                (*B2_size)--;
        }

        //Case 4: access is a complete miss
        else{
                *case_4 = 1; //Case 4 final implementation not completed 
	
		//Case A:
	       	if (*T1_size + *B1_size == c){
                        if (*T1_size < c){
				//Delete reference in the LRU(B1)
                                B1[*LRU_B1] = NULL;
				//Shrink B1
                                for(int i = *LRU_B1 + 1; i <= *MRU_B1; i++) {
                                        B1[i-1] = B1[i];
                                }
                                if (*MRU_B1 != 0) {
                                        B1[*MRU_B1] = NULL;
                                        (*MRU_B1)--;
                                }
                                (*B1_size)--;
				
				//apply replace
                                replace();

                        }

			//B1_size = 0
			else{
			       //Evict LRU(T1)
                                evict_page = T1[*LRU_T1];
				
				//Shrink T1
                                T1[*LRU_T1] = NULL;
                                for (int i = *LRU_T1 + 1; i < *T1_size; i++) {
                                        T1[i-1] = T1[i];
                                }
                                if (*MRU_T1 != 0) {
                                        T1[*MRU_T1] = NULL;
                                        (*MRU_T1)--;
                                }
                                (*T1_size)--;
                        }
                }
		
		//Case B:
                if(*T1_size + *B1_size < c){
                        if((*T1_size + *T2_size + *B1_size + *B2_size) >= c){
                                if((*T1_size + *T2_size + *B1_size + *B2_size) == 2*c){
                                        //Delete LRU(B2)
					B2[*LRU_B2] = NULL;
					//Shrink B2
                                        if (*LRU_B2 != 0) {
                                                (*LRU_B2)--;
                                        }
                                        (*B2_size)--;
                                }

				//apply replace
                                replace();
                        }
                    
                }
        }
	return evict_page->frame >> PAGE_SHIFT;
}

/* This function is called on each access to a page to update any information
 * needed by the ARC algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void arc_ref(pt_entry_t *pte) {
        //arc_evict is called first so we know that either case 2,3, or 4 has been completed
        
	//Case 1: access hit in T1 or T2, regardless if it's in T1 or T2, move the accessed page to MRU spot of T2
	
	//Check for access hit in T1 and find its index
	int access_hit_T1 = 0;
        int access_hit_T1_index = 0;
        for (int i = 0; i <= *MRU_T1 && *T1_size !=0; i++) {
                if (T1[i] == pte) {
                        access_hit_T1 = 1;
                        access_hit_T1_index = i;
                        break;
                }
        }

	//Check for access hit in T2 and find its index
        int access_hit_T2 = 0;
        int access_hit_T2_index = 0;
        for (int i = *MRU_T2; i <= *LRU_T2 && *T2_size != 0; i++) {
                if (T2[i] == pte) {
                        access_hit_T2 = 1;
                        access_hit_T2_index = i;
                        break;
                }
        }

	//Check for access hit in B1
        int access_hit_B1 = 0;
        for (int i = 0; i < *B1_size; i++) {
                if (B1[i] == pte) {
                        access_hit_B1 = 1;
                        break;
                }
        }

	////Check for access hit in B2
        int access_hit_B2 = 0;
        for (int i = 0; i < *B2_size; i++) {
                if (B2[i] == pte) {
                        access_hit_B2 = 1;
                        break;
                }
        }

	//In the case when arc_evict has not been called yet, but still need to add page to a frame number
        if ((access_hit_T1 == 0 && access_hit_T2 == 0 && access_hit_B1 == 0 && access_hit_B2 == 0) || *case_4 == 1) {
                //Grow T1
		if (*T1_size != 0 && *T1_size != c) {
                        (*MRU_T1)++;
                }
                if (*T1_size != c) {
                        (*T1_size)++;
                } 
		//T1_size = c
		else {
                        //Make room in T1, shift down and delete lru_b1
                        for (int i = *LRU_T1 + 1; i <= *MRU_T1; i++) {
                                T1[i-1] = T1[i];
                        }
                }
		//Add pte to T1
                T1[*MRU_T1] = pte;

                *case_4 = 0; //Case 4 final implementation completed so reset to 0
                return;

        }

	//Hit in T1
        if (access_hit_T1 == 1) {
		//page to be moved
                pt_entry_t *moved_page = T1[access_hit_T1_index];
                
		//Shrink T1
		T1[access_hit_T1_index] = NULL;
                for (int i = access_hit_T1_index + 1; i <= *MRU_T1; i++) {
                        T1[i-1] = T1[i];
                }
                T1[*MRU_T1] = NULL;
                (*T1_size)--;
                if (*MRU_T1 != 0) {
                        (*MRU_T1)--;
                }

		//Grow T2
                if (*T2_size != 0 && *T2_size != c) {
                        (*LRU_T2)++;
                }
                if (*T2_size != c) {
                        (*T2_size)++;
                }
                for (int i = *LRU_T2 - 1; i >= *MRU_T2; i--){
                        T2[i+1] = T2[i];
                }

		//Move page to MRU(T2)
                T2[*MRU_T2] = moved_page;
        }

	//Hit in T2
        if (access_hit_T2 == 1) {
		//page to be moved
                pt_entry_t *moved_page = T2[access_hit_T2_index];
                
		//Shift T2 up
		for (int i = access_hit_T2_index-1; i >= *MRU_T2; i--) {
                        T2[i+1] = T2[i];
                }

		//Add page to MRU(T2)
                T2[*MRU_T2] = moved_page;
        }
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void arc_init() {
        c = memsize;
	p = (int *)  malloc(sizeof(int));
        case_4 = (int *)  malloc(sizeof(int));
        T1_size = (int *)  malloc(sizeof(int));
        T2_size = (int *)  malloc(sizeof(int));
        B1_size = (int *)  malloc(sizeof(int));
        B2_size = (int *)  malloc(sizeof(int));
        MRU_T1 = (int *)  malloc(sizeof(int));
        LRU_T1 = (int *)  malloc(sizeof(int));
        MRU_T2 = (int *)  malloc(sizeof(int));
        LRU_T2 = (int *)  malloc(sizeof(int));
        MRU_B1 = (int *)  malloc(sizeof(int));
        LRU_B1 = (int *)  malloc(sizeof(int));
        MRU_B2 = (int *)  malloc(sizeof(int));
        LRU_B2 = (int *)  malloc(sizeof(int));
        B1 = (pt_entry_t **) malloc(c*sizeof(pt_entry_t *));
        B2 = (pt_entry_t **) malloc(c*sizeof(pt_entry_t *));
        T1 = (pt_entry_t **) malloc(c*sizeof(pt_entry_t *));
        T2 = (pt_entry_t **) malloc(c*sizeof(pt_entry_t *));
	
	//page to be evicted
        evict_page = (pt_entry_t *) malloc(sizeof(pt_entry_t));

	//initialize lists to NULL
	for (int i = 0; i < c; i++) {
                T1[i] = NULL;
                T2[i] = NULL;
                B1[i] = NULL;
                B2[i] = NULL;
	}

        //Error checking
        if ((p==NULL) || (case_4 == NULL) || (B1 == NULL) || (B2 == NULL) || (T2 == NULL) || (T1 == NULL) || (evict_page == NULL)) {
                exit(1);
        }
        evict_page = NULL;

        *p=0;
        *case_4 = 0; //To check if case 4 was completed in arc_evict

	//initialize sizes
        *T1_size = 0;
        *T2_size = 0;
        *B1_size = 0;
        *B2_size = 0;

	//initialize indices
        *LRU_T1 = 0;
        *MRU_T1 = 0;
        *LRU_T2 = 0;
        *MRU_T2 = 0;
        *LRU_B1 = 0;
        *MRU_B1 = 0;
        *LRU_B2 = 0;
        *MRU_B2 = 0;
}

/* Cleanup any data structures created in arc_init(). */
void arc_cleanup() {
        free(B1);
        free(B2);
        free(T1);
        free(T2);
        free(evict_page);
        free(p);
        free(case_4);
	free(B1_size);
        free(B2_size);
        free(T1_size);
        free(T2_size);
	free(MRU_T1);
	free(LRU_T1);
	free(MRU_T2);
        free(LRU_T2);
	free(MRU_B1);
        free(LRU_B1);
        free(MRU_B2);
        free(LRU_B2);
}

