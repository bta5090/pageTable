/*
 * kernel_extra.c - Project 2 Extra Credit, CMPSC 473
 * Copyright 2023 Ruslan Nikolaev <rnikola@psu.edu>
 * Distribution, modification, or usage without explicit author's permission
 * is not allowed.
 */

#include <malloc.h>
#include <types.h>
#include <string.h>
#include <printf.h>


// global heap pointer pointing to start address (fake/special 8 byte padding initially) 
static size_t* heapPoint = 0;
// Pointer to top of doubly  linked list, thrown into temp variables but never changed outside of free()
static size_t* listTop = 0;

static size_t PACK(size_t size, size_t alloc) { return (size | alloc);}

static size_t PUT(size_t* p, size_t val) {return (*p = val);}

static size_t* HDRP(void* p) { return (size_t*)(p - 1);}


static size_t headSize() {return (16 / 2);}
static size_t footSize() {return (16 / 2);}
static size_t minBlock() {return (headSize() + footSize() + 16 + 16);}

static void place(size_t *bp, size_t asize);
static size_t* find_fit(size_t size);
static size_t* extend_heap(size_t size);

static size_t align(size_t x)
{
    return 16 * ((x+16-1)/16);
}

// Your mm_init(), malloc(), free() code from mm.c here
// You can only use mem_sbrk(), mem_heap_lo(), mem_heap_hi() and
// Project 2's kernel headers provided in 'include' such
// as memset and memcpy.
// No other files from Project 1 are allowed!


bool mm_init()
{
	// Empty heap initialized with min size 48 bytes = head, prev, next, footer, min 16 free space
    heapPoint = mem_sbrk(minBlock());

    // Checks for error message from mem_sbrk()
    if (heapPoint == ((void *)-1)) 
    {
        return false;   
    }

    // Actually create the initial two words = fake footer, epilogue header
    // Created one  block of 48 bytes.

    memset(heapPoint, (((size_t) 0) | 1), 8); //fake footer

    memset(heapPoint + 1, (((size_t) 32) | 0), 8); // header of block
    memset(heapPoint + 2, ((size_t)0 ), 16);
    memset(heapPoint + 4, (((size_t) 32) | 0), 8); // footer of block

    memset(heapPoint + 5, (((size_t) 0) | 1), 8);// epi header

    // set head node tracker to header of free block
    listTop = heapPoint + 1;
    

    return true;
}

void *malloc(size_t size)
{
	size_t asize;
    size_t* blockp;

    if (size == 0){
        return NULL;
    }
    
   // align the size to 16 bytes
    asize = align(size + 16);
       
    // blockp = header of find_fit block
    blockp = find_fit(asize);
    if((blockp)!= NULL)
    {
    
        place(blockp, asize);
        return blockp + 2;
    
    }
    
    // find_fit failed so need to extend
    blockp = extend_heap(asize);

    if ((blockp) == NULL){
    	return NULL;
    }
    
    place(blockp, asize);
    return blockp;
}



//helper functions


// puts the size and allocated bit into the header of block
static void place(size_t *bp, size_t asize){
    
    	PUT(bp, PACK(asize,1));
    
}


// finding the first fit block to alloc
static size_t* find_fit(size_t size){
        

    size_t* temp_next_block = heapPoint;
    size_t temp_next_val = *((size_t*)heapPoint + 2); // & of next pointer
    size_t temp_size = *((size_t*)heapPoint); // size of the free block
    
    while (temp_next_val != 0){
        size_t* new_return = temp_next_block;
        if (size <= temp_size){
            
            return new_return; //points to the header of the next free block

        }
        // update block and val for the loop
        temp_next_block = temp_next_block + temp_size;
        temp_next_val = *((size_t*)temp_next_block + 2);

    }    

    return NULL;

}


// extend the heap x bytes to make a perfect fit
static size_t* extend_heap(size_t size){

    size_t* blockp;    
    char* temp; 
    temp = mem_heap_hi()-7;

    blockp = mem_sbrk(size);
    
    // mem sbrk fails check
    if (blockp == (void*) -1){
    	return NULL;
    
    }
   

    *((size_t*)temp) = (size | 0); // update header size
    char* new_end = mem_heap_hi();
    new_end = new_end - 7; // new epi
   
    
    *((size_t*)new_end) = 1;
    *((size_t*)(new_end -1)) = size; //set size in footer
    
    return blockp; // pointer to head of newly created block

}

void free(void *ptr)
{
    size_t *findAddr = listTop;
    size_t* storeValue = listTop;
    size_t ptrSize = *(HDRP(ptr));


    // if ptr address provided is NULL/invalid
    if (ptr == NULL)
    {
        return;
    }

    // ptr address is valid but block being pointed to is already free
    else if ((*((size_t *)ptr - 1) & 0x1) == 0)
    {
        return;
    }
    
    // provided block address is freed, size readjusted, payload rest to zero
    else
    {
         

        memset(ptr - 1, (ptrSize & ~1), 8); // header changed to deallocated
  
        memset(ptr, 0 , 8); // prev points to null since top of list
        
        memset(ptr + 1,  *storeValue, 8); // next should point to old head node

        memset((ptr + ptrSize - 16), (ptrSize & ~1), 8);  // footer of new head

        memset(findAddr + 1, (size_t)(ptr - 1), 8); // making prev of old head equal to newTop header address

        listTop = (size_t *)(ptr - 1);     // new head of list confirmed and updated

    }
}

void* realloc(void* oldptr, size_t size)
{

    size_t sizeOld = *(HDRP(oldptr));
    size_t* new;
    
    if (oldptr == NULL)
    {    
        new = malloc(size);
    }
    else if (size == 0)
    {    
        free(oldptr);
    }
    else if (sizeOld == size)
    {
        return oldptr;
    }
    else
    {

        /* 
        malloc(size)
        memcpy(new, old, *pointer to size of old)
            or memcpy(new, old, size)
        free(old) and add to free list
        */
       new = malloc(size);
       memcpy(new, oldptr, size);
       free(oldptr);

       return new;

    }
    return NULL;
}
