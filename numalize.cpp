#include <iostream>
#include <map>
#include <cstdlib>

#include "pin.H"

const int MAXTHREADS = 128;

int num_threads = 0;

struct pageinfo {
	UINT64 accesses[MAXTHREADS];
	int firstacc;
};

map<UINT64, pageinfo> pagemap;
typedef map<UINT64, pageinfo>::iterator it_type;


VOID memaccess(BOOL is_Read, ADDRINT pc, ADDRINT addr, INT32 size, THREADID threadid)
{
	UINT64 page = addr >> 12;
	pagemap[page].accesses[threadid]++;
	// UINT64 acc = __sync_add_and_fetch(&pagemap[page], 1);

	// if (acc==1) {
	// 	// firstacc[page] = threadid;
	// 	// cout << "Page " << (page) << endl;
	// }
}


VOID trace_memory(INS ins, VOID *v)
{
	if (INS_IsMemoryRead(ins)) {
		INS_InsertCall( ins, IPOINT_BEFORE, (AFUNPTR)memaccess, IARG_BOOL, true, IARG_INST_PTR, IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, IARG_THREAD_ID, IARG_END);
	}
	if (INS_HasMemoryRead2(ins)) {
		INS_InsertCall( ins, IPOINT_BEFORE, (AFUNPTR)memaccess, IARG_BOOL, true, IARG_INST_PTR, IARG_MEMORYREAD2_EA, IARG_MEMORYREAD_SIZE, IARG_THREAD_ID, IARG_END);
	}
	if (INS_IsMemoryWrite(ins)) {
		INS_InsertCall( ins, IPOINT_BEFORE, (AFUNPTR)memaccess, IARG_BOOL, false, IARG_INST_PTR, IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, IARG_THREAD_ID, IARG_END);
	}
}


VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
	// int pid = PIN_GetTid();
	__sync_add_and_fetch(&num_threads, 1);
}


VOID Fini(INT32 code, VOID *v)
{
	UINT64 num_pages = 0;
	for(it_type it = pagemap.begin(); it != pagemap.end(); it++) {
		cout << "Page: " << it->first << endl;
		for (int i=0; i<num_threads; i++) {
			if (it->second.accesses[i] > 0)
				cout << "\tT" << i << ": " << it->second.accesses[i] << endl;
		}
		num_pages++;
	}
	cout << "total pages: "<< num_pages << " memory usage: " << num_pages*4 << " KB" << endl;
}


VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
	// brutal hack (pin 2.13, Ubuntu 13.10):
	Fini(0, NULL);
	exit(0);
}


int main(int argc, char *argv[])
{
	if (PIN_Init(argc,argv)) {return 1;}

	PIN_AddThreadStartFunction(ThreadStart, 0);
	PIN_AddThreadFiniFunction(ThreadFini, 0);

	INS_AddInstrumentFunction(trace_memory, 0);

	PIN_AddFiniFunction(Fini, 0);

	PIN_StartProgram();
}
