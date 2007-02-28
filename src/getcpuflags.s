// getcpucpuflags.s: get CPUcpuflags

// copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
// see COPYING and AUTHORS files in distribution or http://mpg123.de
// initially written by KIMURA Takuhiro
// I have to learn assembler to turn these three into one function again

// extern int getcpuid(void) 
// -> 0x00000000 (CPUID instruction not supported)
// or CPUcpuflags

.text
	.align 4
.globl getextcpuflags
	.type	 getextcpuflags,@function
getextcpuflags:
	pushl %ebp
	movl %esp,%ebp
	subl $4,%esp
	pushl %edx
	pushl %ecx
	pushl %ebx
	movl $0x80000000,%eax 
	
	pushfl
	pushfl
	popl %eax
	movl %eax,%ebx
	xorl $0x00200000,%eax
	pushl %eax
	popfl
	pushfl
	popl %eax
	popfl
	cmpl %ebx,%eax
	je .L0
	/ for detect 3DNow! support (bit 31)
	movl $0x80000001,%eax
	cpuid
	movl %edx,%eax
	jmp .L1
	.align 4
.L0:	
	movl $0,%eax
	.align 4
.L1:
	movl %eax,-4(%esp)
	popl %ebx
	popl %ecx
	popl %edx
	movl %ebp,%esp
	popl %ebp
	ret

/ the basic model/family bits
.globl getcpuid
	.type	 getcpuid,@function
getcpuid:
	pushl %ebp
	movl %esp,%ebp
	subl $4,%esp
	pushl %edx
	pushl %ecx
	pushl %ebx
	movl $0x80000000,%eax 
	
	pushfl
	pushfl
	popl %eax
	movl %eax,%ebx
	xorl $0x00200000,%eax
	pushl %eax
	popfl
	pushfl
	popl %eax
	popfl
	cmpl %ebx,%eax
	je .L0i
	/ standard level
	movl $0x00000001,%eax
	cpuid
	jmp .L1i
	.align 4
.L0i:	
	movl $0,%eax
	.align 4
.L1i:
	movl %eax,-4(%esp)
	popl %ebx
	popl %ecx
	popl %edx
	movl %ebp,%esp
	popl %ebp
	ret

.globl getstdcpuflags
	.type	 getstdcpuflags,@function
getstdcpuflags:
	pushl %ebp
	movl %esp,%ebp
	subl $4,%esp
	pushl %edx
	pushl %ecx
	pushl %ebx
	movl $0x80000000,%eax 
	
	pushfl
	pushfl
	popl %eax
	movl %eax,%ebx
	xorl $0x00200000,%eax
	pushl %eax
	popfl
	pushfl
	popl %eax
	popfl
	cmpl %ebx,%eax
	je .L0s
	/ standard level
	movl $0x00000001,%eax
	cpuid
	movl %ecx,%eax
	jmp .L1s
	.align 4
.L0s:	
	movl $0,%eax
	.align 4
.L1s:
	movl %eax,-4(%esp)
	popl %ebx
	popl %ecx
	popl %edx
	movl %ebp,%esp
	popl %ebp
	ret

.globl getstd2cpuflags
	.type	 getstd2cpuflags,@function
getstd2cpuflags:
	pushl %ebp
	movl %esp,%ebp
	subl $4,%esp
	pushl %edx
	pushl %ecx
	pushl %ebx
	movl $0x80000000,%eax 
	
	pushfl
	pushfl
	popl %eax
	movl %eax,%ebx
	xorl $0x00200000,%eax
	pushl %eax
	popfl
	pushfl
	popl %eax
	popfl
	cmpl %ebx,%eax
	je .L0t
	/ standard level
	movl $0x00000001,%eax
	cpuid
	movl %edx,%eax
	jmp .L1t
	.align 4
.L0t:	
	movl $0,%eax
	.align 4
.L1t:
	movl %eax,-4(%esp)
	popl %ebx
	popl %ecx
	popl %edx
	movl %ebp,%esp
	popl %ebp
	ret
