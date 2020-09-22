/* Copyright (C) 2017, Jonathan Lewis and Ward Jaradat
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

int atomic_compare_exchange(int *mem, int cmp, int new) 
{

	/*
	This function atomically compares the content of mem to the value cmp,
	and if they match sets the content of mem to the value new. The function 
	returns the value inn mem at the time of the comparison.
	
	The assembly code uses the Intel LOCK signal prefix to lock the memory 
	bus for the duration of the next instruction to ensure atomicity of the CAS 
	function. The machine instruction CMPXCHG then compares the content of the 
	memory address mem held in register B to the cmp value held in A and if they 
	match replaces the content of the memory address in B with the new value held 
	on the stack at %[new].
	
	A minor alteration to the assembly code:
		- commenting out the  line 'movl %%eax ...'
		- uncommenting  the 'setz ...' line and alters the function to return 1 if
		the content of mem was swapped and 0 otherwise.
	*/

	int res = 0;
	asm(	"movl %[cmp], %%eax\n\t"                   			// store double word cmp in A
			"movq %[mem], %%rbx\n\t"                   			// store quad word mem in B
			"lock cmpxchg %[new], (%%rbx)\n\t"         			// lock mem BUS & do CAS
			"movl %%eax, %[res]\n\t"                   			// res = *mem at cmp-time
			//"setz %[res]\n\t"                        			// or if success set res = 1
			: [res] "=m"(res)                                	// outputs
			: [mem] "m"(mem), [cmp] "m"(cmp), [new] "r"(new) 	// inputs
			: "eax", "rbx"                                   	// clobbers
	);
	return res;
}

unsigned long atomic_compare_exchange_x86_64(unsigned long *mem, unsigned long cmp, unsigned long new) 
{

	/* 
	This function atomically swaps the content of mem with a value 1 less (using the exchange add 
	and -1 as the arg). 
	
	The assembly code uses the Intel LOCK signal prefix to lock the memory bus 
	for the duration of the next instruction to ensure atomicity of the function. 
	*/

	unsigned long res = 0;
	asm(	"movq %[cmp], %%rax\n\t"                   			// store quad word cmp in A
			"movq %[mem], %%rbx\n\t"                   			// store quad word mem in B
			"lock cmpxchg %[new], (%%rbx)\n\t"         			// lock mem BUS & do CAS
			"movq %%rax, %[res]\n\t"                   			// res = *mem at cmp-time
			//"setz %[res]\n\t"                        			// or if success set res = 1
			: [res] "=m"(res)                                	// outputs
			: [mem] "m"(mem), [cmp] "m"(cmp), [new] "r"(new) 	// inputs
			: "rax", "rbx"                                   	// clobbers
	);
	return res;
}

int atomic_decrement(int *mem) 
{
	int res = 0;
	asm(	"movl $-1, %%eax\n\t"                       		// store double word -1 in A
			"movq %[mem], %%rbx\n\t"                   			// store quad word mem in B
			"lock xadd %%eax, (%%rbx)\n\t"             			// lock mem BUS & do swap-add using xadd to perform *mem = *mem + -1 to decrement value at mem
			"decl %%eax\n\t"                                    // establish new value after atomic decrement
			"movl %%eax, %[res]\n\t"                   			// set return value to be = *mem after decrement
			: [res] "=m"(res)                              	    // outputs
			: [mem] "m"(mem)                                 	// inputs
			: "eax", "rbx"                                   	// clobbers
	);
	return res;
}

int atomic_exchange_add(int volatile *mem, int value) 
{

	/* 
	This function atomically swaps the content of mem to the new value 
	and adds the specified amount. 
	
	The assembly code uses the Intel LOCK signal prefix to lock the memory 
	bus for the duration of the next instruction to ensure atomicity of the
	function. 
	*/

	int res = 0;
	asm(	"movl %[value], %%eax\n\t"                 			// store double word value in A
			"movq %[mem], %%rbx\n\t"                   			// store quad word mem in B
			"lock xadd %%eax, (%%rbx)\n\t"             			// lock mem BUS & do swap-add value
			"movl %%eax, %[res]\n\t"                   			// res = *mem at swap-time
			: [res] "=m"(res)                                	// outputs
			: [mem] "m"(mem), [value] "r"(value)             	// inputs
			: "eax", "rbx"                                   	// clobbers
	);
	return res;
}

unsigned long atomic_exchange_add_x86_64(unsigned long volatile *mem, unsigned long value) 
{
	unsigned long res = 0;
	asm(	"movq %[value], %%rax\n\t"                 			// store quad word value in A
			"movq %[mem], %%rbx\n\t"                   			// store quad word mem in B
			"lock xadd %%rax, (%%rbx)\n\t"             			// lock mem BUS & do swap-add value
			"movq %%rax, %[res]\n\t"                   			// res = *mem at swap-time
			: [res] "=m"(res)                                	// outputs
			: [mem] "m"(mem), [value] "r"(value)             	// inputs
			: "rax", "rbx"                                   	// clobbers
	);
	return res;
}

int atomic_exchange(int *mem, int new) 
{

	/*
	This function atomically swaps the content of mem to the new value.
	
	The assembly code uses the Intel LOCK signal prefix to lock the memory bus
	for the duration of the next instruction to ensure atomicity of the
	function. The machine instruction XCHG then swaps the content of
	the memory address mem held in register B to the value held in A.
	*/

	int res = 0;
	asm(	"movl %[new], %%eax\n\t"                   			// store double word new in A
			"movq %[mem], %%rbx\n\t"                   			// store quad word mem in B
			"lock xchg %%eax, (%%rbx)\n\t"             			// lock mem BUS & do swap
			"movl %%eax, %[res]\n\t"                   			// res = *mem at swap-time
			: [res] "=m"(res)                                	// outputs
			: [mem] "m"(mem), [new] "r"(new)                 	// inputs
			: "eax", "rbx"                                   	// clobbers
	);
	return res;
}

unsigned long atomic_exchange_x86_64(unsigned long *mem, unsigned long new) 
{

	/*
 	This function atomically swaps the content of mem with a value 1 more (using 
	the exchange add and +1 as the arg)
 
 	The assembly code uses the Intel LOCK signal prefix to lock the memory bus
	for the duration of the next instruction to ensure atomicity of the function.
 	*/

	unsigned long res = 0;
	asm(	"movq %[new], %%rax\n\t"                   			// store quad word new in A
			"movq %[mem], %%rbx\n\t"                   			// store quad word mem in B
			"lock xchg %%rax, (%%rbx)\n\t"             			// lock mem BUS & do swap
			"movq %%rax, %[res]\n\t"                   			// res = *mem at swap-time
			: [res] "=m"(res)                                	// outputs
			: [mem] "m"(mem), [new] "r"(new)                 	// inputs
			: "rax", "rbx"                                   	// clobbers
	);
	return res;
}

int atomic_increment(int *mem) 
{
	int res = 0;
	asm(	"movl $1, %%eax\n\t"                        		// store double word +1 in A
			"movq %[mem], %%rbx\n\t"                   			// store quad word mem in B
			"lock xadd %%eax, (%%rbx)\n\t"             			// lock mem BUS & do swap-add using xadd to perform *mem = *mem + 1 to increment value at mem
			"incl %%eax\n\t"                                    // establish new value after atomic increment
			"movl %%eax, %[res]\n\t"                   			// set return value to be = *mem after increment
			: [res] "=m"(res)                                	// outputs
			: [mem] "m"(mem)                                 	// inputs
			: "eax", "rbx"                                   	// clobbers
	);
	return res;
}

