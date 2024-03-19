/*
 * kernel_code.c - Project 2, CMPSC 473
 * Copyright 2023 Ruslan Nikolaev <rnikola@psu.edu>
 * Distribution, modification, or usage without explicit author's permission
 * is not allowed.
 */

#include <kernel.h>
#include <types.h>
#include <printf.h>
#include <malloc.h>
#include <string.h>

void *page_table = NULL; /* TODO: Must be initialized to the page table address */
void *user_stack = NULL; /* TODO: Must be initialized to a user stack virtual address */
void *user_program = NULL; /* TODO: Must be initialized to a user program virtual address */




void kernel_init(void *ustack, void *uprogram, void *memory, size_t memorySize)
{
	// 'memory' points to the place where memory can be used to create
	// page tables (assume 1:1 initial virtual-to-physical mappings here)
	// 'memorySize' is the maximum allowed size, do not exceed that (given just in case)

	// TODO: Create a page table here

	
	// printf("%p\n", ustack);
	// printf("%ld\n", *((size_t *)(ustack)));
	// printf("%p\n", uprogram);
	// printf("%ld\n", *((size_t *)(uprogram)));
	// printf("%p\n", memory);
	// printf("%ld\n", *((size_t *)(memory)));
	// printf("%ld\n", (memorySize));

	uint64_t* PTE = 0; // pointer used to initialise page table entries
	uint64_t* PDE = 0;  // pointer used to initialise page directory entries
	uint64_t* PDPE = 0;  // pointer used to initialise page directory pointer pagetable entries
	uint64_t* PMLE4E = 0;	// pointer used to initialise page map level 4 SINGLE entry (4GB mapping) 

	PTE = (uint64_t *)memory; // memory is starting addr of all valid memory regions for page table hierarchy, so PTE set to it.
	// PTE = 1,048,576 page table entries 			  [LEVEL 1]  8 BYTES EACH
	// PDE = 2048 pages -> each with 512 PTE's  	  [LEVEL 2]  4kIB EACH??		 
	// PDPE = 4 pages/tables -> each with 512 PDE's   [LEVEL 3]  1 GB EACH
	// PMLE4E = 1 pointer/table -> 4 PDPE's   		  [LEVEL 4]  4GB EACH


	// Counter i is size_t which is 8bytes so incrementing by this size across memory (first 8MB or so)
	// 1-to-1 mapping so only need to add shifted i (by 2^12) and also seeting write/present to 1.
	// Want to index 2^20 entries of 8 bytes
	for (size_t i = 0 ; i < 1048576 ; i++)
	{
		PTE[i] = (uint64_t)(i << 12) | 3; // shift i to erase 12 bits but also preserve write/present hence the BITWISE OR at index i
	}


	// Then for PDE, we move past memory region housing PTE's and map 2048 pages with 512 PTE each. No shifting required as PTE is being used for PDE calculation. Only need to increment by page size which is 4Kib and index 2048 pages that PDPE can map to.
	PDE = PTE + 1048576;
	for (size_t i = 0 ; i < 2048 ; i++)
	{
		uint64_t* startPTE = PTE + (512 * i);
		PDE[i] = (uint64_t)(startPTE) | 3;
	}


	PDPE = (PDE + (4 * 512));
	for (size_t i = 0 ; i < 4; i++)
	{
		uint64_t* startPDPE = PDE + (512 * i);
		PDPE[i] = (uint64_t)(startPDPE) | 3;
	}

	for (size_t i = 4 ; i < 512; i++)
	{
		PDPE[i] = 0;
	}

	

	// PMLE4E = SOMETHING FROM LEVEL 4 PAGE TABLE IN INDEX ZERO, KERNEL POINTER
	PMLE4E = PDPE + 512;
	*PMLE4E = ((uint64_t)(PDPE)) | 3;

	for (size_t i = 1 ; i < 512; i++)
	{
		PMLE4E[i] = 0;
	}



	// Set PMLE4E[0] to page table pointer on level 4, now have established 4-level page table hierarchy
	page_table = PMLE4E;


///////////////////////////////////////////// Start of User Space Implementation


	// TODO: It is OK for Q1-Q3, but should changed
	// for the final part's Q4 when creating your own user page table
	
	
	//user_stack = ustack;

	// TODO: It is OK for Q1-Q3, but should changed
	// for the final part's Q4 when creating your own user page table
	//user_program = uprogram;
	uint64_t* user_level_one = 0; // pointer used to initialise page table entries
	uint64_t* user_level_two = 0;  // pointer used to initialise page directory entries
	uint64_t* user_level_three = 0;  // pointer used to initialise page directory pointer pagetable entries


	user_level_one = PMLE4E + 512;
	printf("%p\n", user_level_one);
	for (size_t i = 0 ; i < 510; i++)
	{
		user_level_one[i] = 0;
	}
	user_level_one[510] = ((uint64_t)ustack - 4096)| 7;
	user_level_one[511] = ((uint64_t)uprogram)| 7;
	printf("%lld\n", user_level_one[510]);
	printf("%lld\n", user_level_one[511]);
	
	user_level_two = user_level_one + 512;
	printf("%p\n", user_level_two);
	for (size_t i = 0 ; i < 511; i++)
	{
		user_level_two[i] = 0;
	}
	user_level_two[511] = (uint64_t)user_level_one | 7;
	printf("%lld\n", user_level_two[511]);
	
	user_level_three = user_level_two + 512;
	printf("%p\n", user_level_three);
	for (size_t i = 0 ; i < 511; i++)
	{
		user_level_three[i] = 0;
	}
	user_level_three[511] = (uint64_t)user_level_two | 7;
	printf("%lld\n", user_level_three[511]);

	PMLE4E[511] = 	(uint64_t)user_level_three | 7;
	printf("%lld\n", PMLE4E[511]);
	user_stack = (uint64_t)-4096L;
	printf("%p\n", user_stack);
	user_program = (uint64_t)-4096L;



///////////////////////////////////////////// End of User Space Implementation

	// The remaining portion just loads the page table,
	// load 'page_table' into the CR3 register
	const char *err = load_page_table(page_table);
	if (err != NULL) {
		printf("ERROR: %s\n", err);
	}

	printf("\nHello from sgroup115, bta5090\n\n");

	// The extra credit assignment
	mem_extra_test();
}






/* The entry point for all system calls */
long syscall_entry(long n, long a1, long a2, long a3, long a4, long a5)
{
	// TODO: the system call handler to print a message (n = 1)
	// the system call number is in 'n', make sure it is valid!

	// Arguments are passed in a1,.., a5 and can be of any type
	// (including pointers, which are casted to 'long')
	// For the project, we only use a1 which will contain the address
	// of a string, cast it to a pointer appropriately 

	// For simplicity, assume that the address supplied by the
	// user program is correct
	//
	// Hint: see how 'printf' is used above, you want something very
	// similar here
	const char *aOne = (const char *) a1;
	if (n == 1)
	{
		printf("System Call Message: %s\n", aOne);
		return 0;
	}

	return -1; /* Success: 0, Failure: -1 */
}
