#include "CodeGenerator.hpp"

#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
	#include <conio.h>
#else
	inline int getch() {return fgetc(stdin);}
#endif

#if defined(__cplusplus) && !defined(for) && defined(_MSC_VER) && (_MSC_VER <= 1200)
	#define for if(0);else for
#endif

void testIntrinsics()
{
	printf("Testing run-time intrinsics.\n\n");
	printf("Press any key to start assembling.\n\n");
	_getch();

	SoftWire::Assembler x86(false);

	static char string[] = "All working!";

	x86.push((unsigned int)string);
	x86.call((unsigned int )printf);
	x86.add(x86.esp, 4);
	x86.ret();

	void (*emulator)() = (void(*)())x86.callable();

	printf("%s\n\n", x86.getListing());
	printf("Execute code (y/n)?\n\n");

	int c;
	do
	{
		c = _getch();
	}
	while(c != 'y' && c != 'n');

	if(c == 'y')
	{
		printf("output: ");
		emulator();
		printf("\n\n");
	}
}

class TestRegisterAllocator : public SoftWire::CodeGenerator
{
public:
	TestRegisterAllocator() : CodeGenerator(false)
	{
		x1 = 1;
		x2 = 2;
		x3 = 3;
		x4 = 4;
		x5 = 5;
		x6 = 6;
		x7 = 7;
		x8 = 8;
		x9 = 9;

		prologue(0);

		Int t1;
		Int t2;
		Int t3;
		Int t4;
		Int t5;
		Int t6;
		Int t7;
		Int t8;
		Int t9;

		mov(t1, r32(&x1));
		mov(t2, r32(&x2));
		mov(t3, r32(&x3));
		mov(t4, r32(&x4));
		mov(t5, r32(&x5));
		mov(t6, r32(&x6));
		mov(t7, r32(&x7));
		mov(t8, r32(&x8));
		mov(t9, r32(&x9));

		mov(dword_ptr [&x1], t9);
		mov(dword_ptr [&x2], t8);
		mov(dword_ptr [&x3], t7);
		mov(dword_ptr [&x4], t6);
		mov(dword_ptr [&x5], t5);
		mov(dword_ptr [&x6], t4);
		mov(dword_ptr [&x7], t3);
		mov(dword_ptr [&x8], t2);
		mov(dword_ptr [&x9], t1);

		epilogue();
	}

	int x1;
	int x2;
	int x3;
	int x4;
	int x5;
	int x6;
	int x7;
	int x8;
	int x9;
};

void testRegisterAllocator()
{
	printf("Testing register allocator. SoftWire will swap nine numbers using nine virtual general-purpose registers.\n\n");
	printf("Press any key to start assembling.\n\n");
	_getch();

	TestRegisterAllocator x86;

	void (*script)() = (void(*)())x86.callable();

	printf("%s\n\n", x86.getListing());
	printf("Execute code (y/n)?\n\n");

	int c;
	do
	{
		c = _getch();
	}
	while(c != 'y' && c != 'n');

	if(c == 'y')
	{
		printf("Input:  %d %d %d %d %d %d %d %d %d\n", x86.x1, x86.x2, x86.x3, x86.x4, x86.x5, x86.x6, x86.x7, x86.x8, x86.x9);
		script();
		printf("output: %d %d %d %d %d %d %d %d %d\n", x86.x1, x86.x2, x86.x3, x86.x4, x86.x5, x86.x6, x86.x7, x86.x8, x86.x9);
		printf("\n");
	}
}

class StressTest : public SoftWire::CodeGenerator
{
public:
	StressTest(int seed, int tests, int level, bool copyProp, bool loadElim, bool spillElim) : CodeGenerator(false)
	{
		if(copyProp) enableCopyPropagation(); else disableCopyPropagation();
		if(loadElim) enableLoadElimination(); else disableLoadElimination();
		if(spillElim) enableSpillElimination(); else disableSpillElimination();
	#if 0
		Int a;
		Int b;
		Int c;

		pushad();
		prologue(1024);
		freeAll();

		for(int i = 0; i < 3; i++)
		{
			add(r32(&x[2 * i + 0]), r32(&x[2 * i + 1]));
		}

		nop();

		a = (a + b) * c;

		nop();

		for(int i = 2; i >= 1; i--)
		{
			add(r32(&x[2 * i + 0]), r32(&x[2 * i + 1]));
		}

		nop();
		spillAll();
		epilogue();
		popad();
		ret();

		return;
	#else
		// -------------------------------------------------

		srand(seed);

		for(int i = 0; i < 16; i++)
		{
			w[i] = rand();
		}

		for(int i = 0; i < 16; i++)
		{
			x[i] = w[i];
		}

		if(level & 0x01) for(int i = 0; i < tests; i++)
		{
			x[q()] = x[q()];

			if(i % 3 == 0 && rand() < RAND_MAX / 2)
			{
				int a = q(); int b = q();
				x[b] += x[a];
			}
		}

		if(level & 0x02) for(int i = 0; i < tests; i++)
		{
			int a = q(); int b = q();
			x[b] += x[a];

			if(i % 3 == 0 && rand() < RAND_MAX / 2)
			{
				x[q()] = x[q()];
			}
		}

		if(level & 0x04) for(int i = 0; i < tests; i++)
		{
			int a = q(); int b = q();
			x[b] += x[a];

			if(i % 3 == 0 && rand() < RAND_MAX / 2)
			{
				x[q()];   // spill(reg)
			}
		}

		if(level & 0x08) for(int i = 0; i < tests; i++)
		{
			if(q() < q() / 2) {int a = q(); int b = q(); x[b] += x[a];}   // add(r(), r());
			if(q() < q() / 2) {x[q()] = x[q()];}   // mov(r(), r());
			if(q() < q() / 2) q();   // spill(r());
		}

		for(int i = 0; i < 16; i++)
		{
			y[i] = x[i];
		}

		// -------------------------------------------------

		srand(seed);

		for(int i = 0; i < 16; i++)
		{
			w[i] = rand();
		}

		pushad();

		for(int i = 0; i < 16; i++)
		{
			mov(eax, dword_ptr [&w[i]]);
			mov(dword_ptr [&x[i]], eax);
		}

		freeAll();

		nop();

		if(level & 0x01) for(int i = 0; i < tests; i++)
		{
			mov(r(), r());

			if(i % 3 == 0 && rand() < RAND_MAX / 2)
			{
				add(r(), r());
			}
		}

		if(level & 0x02) for(int i = 0; i < tests; i++)
		{
			add(r(), r());

			if(i % 3 == 0 && rand() < RAND_MAX / 2)
			{	
				mov(r(), r());
			}
		}

		if(level & 0x04) for(int i = 0; i < tests; i++)
		{
			add(r(), r());

			if(i % 3 == 0 && rand() < RAND_MAX / 2)
			{	
				spill(r());
			}
		}

		if(level & 0x08) for(int i = 0; i < tests; i++)
		{
			if(q() < q() / 2) add(r(), r());
			if(q() < q() / 2) mov(r(), r());
			if(q() < q() / 2) spill(r());
		}

		nop();

		spillAll();

		nop();

		for(int i = 0; i < 16; i++)
		{
			mov(eax, dword_ptr [&x[i]]);
			mov(dword_ptr [&z[i]], eax);
		}

		popad();
		ret();
	#endif
	}

	const SoftWire::OperandREG32 r()
	{
		return r32(&x[q()]);
	}

	int q()
	{
		return 16 * rand() / (RAND_MAX + 1);
	}

	static int x[16];
	static int y[16];
	int z[16];
	int w[16];
};

int StressTest::x[16];
int StressTest::y[16];

void testOptimizations()
{
	printf("Optimization stress test.\n\n");
	printf("Press any key to start assembling.\n\n");
	_getch();

	StressTest a(0, 1024, 0xFF, false, false, false);
	StressTest b(0, 1024, 0xFF, true, true, true);

	void (*funca)() = a.callable();
	void (*funcb)() = b.callable();

	funca();
	funcb();

	int x = a.instructionCount();
	int y = b.instructionCount();

	float optimization = 100.0f * (x - y) / y;

	int i;

	for(i = 0; i < 16; i++)
	{
		if(b.z[i] != b.y[i])
		{
			break;
		}

		if(b.z[i] != a.z[i])
		{
			break;
		}

		if(a.z[i] != a.y[i])
		{
			break;
		}
	}

	if(i == 16)
	{
		printf("Optimization stress test succesful. %f%% optimized.\n\n", optimization);
	}
	else
	{
		printf("Optimization test failed.\n\n");
	}
}

class BackEnd : public SoftWire::CodeGenerator
{
public:
	BackEnd() : CodeGenerator(false)
	{
		prologue(0);

		Int a = 11;
		Int b = 22;
		Int c = 33;
		Int d = 44;
		Int e = 55;
		Int f = 66;
		Int g = 77;
		Int h = 88;
		Int i = 99;

		a = ((g & h) - (b + c - e) * (a - b)) | i * f;

		mov(eax, a);

		epilogue();
	}
};

void testBackEnd()
{
	printf("Compiler back-end test.\n\n");
	printf("Press any key to start assembling.\n\n");
	_getch();

	BackEnd backEnd;
	int x = ((int(*)())backEnd.callable())();

	int a = 11;
	int b = 22;
	int c = 33;
	int d = 44;
	int e = 55;
	int f = 66;
	int g = 77;
	int h = 88;
	int i = 99;

	int y = ((g & h) - (b + c - e) * (a - b)) | i * f;

	if(x == y)
	{
		printf("Compiler back-end stress test succesful.\n\n");
	}
	else
	{
		printf("Compiler back-end test failed.\n\n");
	}
}

class X64 : public SoftWire::CodeGenerator
{
public:
	X64() : CodeGenerator(true)
	{
		prologue(0);

		static __int64 x;

		// Simple operations
		mov(r10, r11);
		mov(r0, r1);
		mov(r0d, r1d);
		mov(r0b, r1b);
		xor(rax, rax);
		xor(rbx, rbx);
		mov(qword_ptr [&x+rax+4*rbx], 1);
		mov(qword_ptr [&x], 2);   // RIP-relative addressing
		push(r14);
		pop(r14);

		const char *string = "Good luck with your 64-bit processor!";

		mov(rcx, (unsigned int)string);
		call((unsigned int)printf);

		epilogue();
	}
};

void testX64()
{
	printf("X64 test.\n\n");
	printf("Press any key to start assembling.\n\n");
	_getch();

	X64 x64;

	void (*function)() = (void(*)())x64.callable();

	printf("%s\n\n", x64.getListing());
	printf("Execute code (y/n)?\n\n");

	int c;
	do
	{
		c = _getch();
	}
	while(c != 'y' && c != 'n');

	if(c == 'y')
	{
		printf("output: ");
		function();
		printf("\n\n");
	}
}

int main()
{
#if 0
	testIntrinsics();
	testRegisterAllocator();
	testOptimizations();
	testBackEnd();
#else   // 64-bit platform
	testX64();
#endif

	printf("Press any key to continue\n");
	_getch();
	
	return 0;
}
