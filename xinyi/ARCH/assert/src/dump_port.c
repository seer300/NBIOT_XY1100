#include "dump.h"


void Dump_Memory(void)
{
	extern void Start_Dump_No_Recursion(void);
	Start_Dump_No_Recursion();
}
