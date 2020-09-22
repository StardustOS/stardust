# Copyright (C) 2017, Ward Jaradat
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
# 
# Notes:
#
# Code obtained and ported from Xen/MiniOS

#include <os/kernel.h>
#include <public/features.h>

.section __xen_guest
	.ascii	"GUEST_OS=Stardust"
	.ascii	",XEN_VER=xen-3.0"
	.ascii	",VIRT_BASE=0x0"
	.ascii	",ELF_PADDR_OFFSET=0x0"
	.ascii	",HYPERCALL_PAGE=0x2"
	.ascii	",LOADER=generic"
	.byte	0

.text
#define ENTRY(X) .globl X ; X :
.globl shared_info, hypercall_page, xenstore_page
        .org 0x1000

shared_info:
        .org 0x2000

hypercall_page:
        .org 0x3000

xenstore_page:
        .org 0x4000

#define STACK_SIZE  16384
.globl _start
_start:
        cld
        movq stack_start(%rip),%rsp
        movq %rsi,%rdi
        call start_kernel

stack_start:
        .quad stack+STACK_SIZE

#define evtchn_upcall_pending	
#define evtchn_upcall_mask		1

NMI_MASK = 0x80000000

#define TRAP_FRAME_SLOTS       14
#define RDI 112
#define ORIG_RAX 120       
#define EFLAGS 144

#define sizeof_vcpu_shift       6
#define pda_cpunumber           32

#define XEN_GET_VCPU_INFO(reg)                    \
				movq %gs:pda_cpunumber,reg		; \
				shl  $32, reg					; \
				shr  $32-sizeof_vcpu_shift,reg	; \
				addq HYPERVISOR_shared_info,reg
#define XEN_PUT_VCPU_INFO(reg)
#define XEN_LOCKED_BLOCK_EVENTS(reg)	movb $1,evtchn_upcall_mask(reg)
#define XEN_LOCKED_UNBLOCK_EVENTS(reg)	movb $0,evtchn_upcall_mask(reg)
#define XEN_TEST_PENDING(reg)	testb $0xFF,evtchn_upcall_pending(reg)

#define XEN_BLOCK_EVENTS(reg)	XEN_GET_VCPU_INFO(reg)			; \
                    			XEN_LOCKED_BLOCK_EVENTS(reg)	; \
    				            XEN_PUT_VCPU_INFO(reg)

#define XEN_UNBLOCK_EVENTS(reg)	XEN_GET_VCPU_INFO(reg)			; \
                				XEN_LOCKED_UNBLOCK_EVENTS(reg)	; \
    			            	XEN_PUT_VCPU_INFO(reg)

#define GET_THREAD_INFO(reg)    movq %gs:24,reg;


#define REST_SKIP 6*8
.macro SAVE_REST
	subq $REST_SKIP,%rsp
	movq %rbx,5*8(%rsp)
	movq %rbp,4*8(%rsp)
	movq %r12,3*8(%rsp)
	movq %r13,2*8(%rsp)
	movq %r14,1*8(%rsp)
	movq %r15,(%rsp)
.endm


.macro RESTORE_REST
	movq (%rsp),%r15
	movq 1*8(%rsp),%r14
	movq 2*8(%rsp),%r13
	movq 3*8(%rsp),%r12
	movq 4*8(%rsp),%rbp
	movq 5*8(%rsp),%rbx
	addq $REST_SKIP,%rsp
.endm


#define ARG_SKIP 9*8
.macro RESTORE_ARGS skiprax=0,addskip=0,skiprcx=0,skipr11=0,skipr8910=0,skiprdx=0
	.if \skipr11
	.else
	movq (%rsp),%r11
	.endif
	.if \skipr8910
	.else
	movq 1*8(%rsp),%r10
	movq 2*8(%rsp),%r9
	movq 3*8(%rsp),%r8
	.endif
	.if \skiprax
	.else
	movq 4*8(%rsp),%rax
	.endif
	.if \skiprcx
	.else
	movq 5*8(%rsp),%rcx
	.endif
	.if \skiprdx
	.else
	movq 6*8(%rsp),%rdx
	.endif
	movq 7*8(%rsp),%rsi
	movq 8*8(%rsp),%rdi
	.if ARG_SKIP+\addskip > 0
	addq $ARG_SKIP+\addskip,%rsp
	.endif
.endm


.macro HYPERVISOR_IRET flag
	testl $NMI_MASK,2*8(%rsp)
	jnz   2f

	testb $1,(xen_features+XENFEAT_supervisor_mode_kernel)
	jnz   1f

	orb   $3,1*8(%rsp)
	orb   $3,4*8(%rsp)
1:	iretq

2:
	andl  $~NMI_MASK, 16(%rsp)
	pushq $\flag
	jmp  hypercall_page + (__HYPERVISOR_iret * 32)
.endm

ENTRY(error_entry)
	cld
	subq  $TRAP_FRAME_SLOTS*8,%rsp
	movq %rsi,13*8(%rsp)
	movq 14*8(%rsp),%rsi
	movq %rdx,12*8(%rsp)
	movq %rcx,11*8(%rsp)
	movq %rsi,10*8(%rsp)
	movq %r8, 9*8(%rsp)
	movq %r9, 8*8(%rsp)
	movq %r10,7*8(%rsp)
	movq %r11,6*8(%rsp)
	movq %rbx,5*8(%rsp)
	movq %rbp,4*8(%rsp)
	movq %r12,3*8(%rsp)
	movq %r13,2*8(%rsp)
	movq %r14,1*8(%rsp)
	movq %r15,(%rsp)

error_call_handler:
	movq %rdi, RDI(%rsp)
	movq %rsp,%rdi
	movq ORIG_RAX(%rsp),%rsi
	movq $-1,ORIG_RAX(%rsp)
	call *%rax
error_exit:
    	RESTORE_REST
	XEN_BLOCK_EVENTS(%rsi)
	jmp retint_kernel


.macro zeroentry sym
    movq (%rsp),%rcx
    movq 8(%rsp),%r11
    addq $0x10,%rsp 
	pushq $0	
	pushq %rax
	leaq  \sym(%rip),%rax
	jmp error_entry
.endm

.macro zeroentry_save_regs sym
    movq (%rsp),%rcx
    movq 8(%rsp),%r11
    addq $0x10,%rsp 
	pushq $0	
	pushq %rax
    GET_THREAD_INFO(%rax)
    movq %rsp,8(%rax)
    subq $TRAP_FRAME_SLOTS*8,8(%rax)
	leaq  \sym(%rip),%rax
	jmp error_entry
.endm


.macro errorentry sym
        movq (%rsp),%rcx
        movq 8(%rsp),%r11
        addq $0x10,%rsp
	pushq %rax
    GET_THREAD_INFO(%rax)
    movq %rsp,8(%rax)
    subq $TRAP_FRAME_SLOTS*8,8(%rax)
	leaq  \sym(%rip),%rax
	jmp error_entry
.endm
ENTRY(hypervisor_callback)
    zeroentry hypervisor_callback2

ENTRY(hypervisor_callback2)
        movq %rdi, %rsp
11:     movq %gs:8,%rax
        incl %gs:0
        cmovzq %rax,%rsp
        pushq %rdi
        call do_hypervisor_callback
        popq %rsp
        decl %gs:0
        jmp error_exit

restore_all_enable_events:
	XEN_UNBLOCK_EVENTS(%rsi)      

scrit:
	XEN_TEST_PENDING(%rsi)
	jnz  14f			
	XEN_PUT_VCPU_INFO(%rsi)
        RESTORE_ARGS 0,8,0
        HYPERVISOR_IRET 0

14:	XEN_LOCKED_BLOCK_EVENTS(%rsi)
	XEN_PUT_VCPU_INFO(%rsi)
	SAVE_REST
        movq %rsp,%rdi
	jmp  11b
ecrit:

retint_restore_args:
	movl EFLAGS-REST_SKIP(%rsp), %eax
	shr $9, %eax
	XEN_GET_VCPU_INFO(%rsi)
	andb evtchn_upcall_mask(%rsi),%al
	andb $1,%al
	jnz restore_all_enable_events
	XEN_PUT_VCPU_INFO(%rsi)

	RESTORE_ARGS 0,8,0
	HYPERVISOR_IRET 0

retint_kernel:
    GET_THREAD_INFO(%rcx)
    cmpl $0,0(%rcx)
    jnz retint_restore_args
    bt $2,4(%rcx)
    jnc retint_restore_args
    bt $9,EFLAGS-REST_SKIP(%rsp)
    jnc  retint_restore_args
    SAVE_REST
    call preempt_schedule_irq
    RESTORE_REST
    jmp retint_kernel

ENTRY(failsafe_callback)
        popq  %rcx
        popq  %r11
        iretq

ENTRY(coprocessor_error)
        zeroentry do_coprocessor_error

ENTRY(simd_coprocessor_error)
        zeroentry do_simd_coprocessor_error


ENTRY(device_not_available)
        zeroentry do_device_not_available

ENTRY(debug)
       zeroentry_save_regs do_debug

ENTRY(int3)
        zeroentry_save_regs do_int3

ENTRY(overflow)
        zeroentry do_overflow

ENTRY(bounds)
        zeroentry do_bounds


ENTRY(invalid_op)
        zeroentry do_invalid_op


ENTRY(coprocessor_segment_overrun)
        zeroentry do_coprocessor_segment_overrun


ENTRY(invalid_TSS)
        errorentry do_invalid_TSS


ENTRY(segment_not_present)
        errorentry do_segment_not_present

ENTRY(stack_segment)
        errorentry do_stack_segment

ENTRY(general_protection)
        errorentry do_general_protection


ENTRY(alignment_check)
        errorentry do_alignment_check


ENTRY(divide_error)
        zeroentry do_divide_error


ENTRY(spurious_interrupt_bug)
        zeroentry do_spurious_interrupt_bug


ENTRY(page_fault)
        errorentry do_page_fault

ENTRY(idle_thread_starter)
        popq %rdi
        popq %rbx
		popq %r14
        call *%rbx
        call exit_current_thread

ENTRY(thread_starter)
        movq %rax,%rdi
        call sched_switch_thread_in
        popq %rdi
        popq %rbx
		popq %r14
        call *%rbx
        call exit_current_thread

ENTRY(set_db_back_handler)
	movq (%rsp), %rax
	movq %rax, (%rdi)
	movq %rsp, %rax
	addq $8, %rax
	movq %rax, 8(%rdi)
	movq %rbp, 16(%rdi)
	xor %rax, %rax
	ret

ENTRY(jmp_db_back_handler)
	movq $1, %rax
	movq 8(%rdi), %rsp
	movq 16(%rdi), %rbp
	mov (%rdi), %rdi
	jmp *%rdi

ENTRY(getspreg)
	movq %rsp, %rax
	ret

ENTRY(getpcreg)
	movq (%rsp), %rax
	ret

ENTRY(getbpreg)
	movq %rbp, %rax
	ret

ENTRY(getr14reg)
	movq %r14, %rax
	ret
