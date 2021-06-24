/* 046267 Computer Architecture - Spring 2021 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <vector>
#include <stdexcept>
// #include <iostream>

using std::vector;
// using std::cout;
// using std::endl;

class Thread
{
	vector<int32_t> regs;
	int tid;
	uint32_t line;
public:
	int release_time;
	Thread(int tid) : regs(REGS_COUNT, 0), tid(tid), line(0), release_time(0) {}

	// run a single instruction, and return the latency (0 for arithmetic instructions)
	// -1 denotes HALT
	int RunInstruction() {
		// Read the instruction
		Instruction inst;
		SIM_MemInstRead(line, &inst, tid);

		// Advance instruction pointer
		++line;

		// Do the instruction
		switch(inst.opcode) {
			case CMD_NOP:
				// cout << "NOP";
				return 0;
			case CMD_ADD:
				// cout << "ADD";
				regs[inst.dst_index] = regs[inst.src1_index] + regs[inst.src2_index_imm];
				return 0;
			case CMD_SUB:
				// cout << "SUB";
				regs[inst.dst_index] = regs[inst.src1_index] - regs[inst.src2_index_imm];
				return 0;
			case CMD_ADDI:
				// cout << "ADDI";
				regs[inst.dst_index] = regs[inst.src1_index] + inst.src2_index_imm;
				return 0;
			case CMD_SUBI:
				// cout << "SUBI";
				regs[inst.dst_index] = regs[inst.src1_index] - inst.src2_index_imm;
				return 0;
			case CMD_LOAD: {
				// cout << "LOAD";
				int32_t data;
				uint32_t addr = static_cast<uint32_t>(regs[inst.src1_index]);
				addr += inst.isSrc2Imm ? inst.src2_index_imm : regs[inst.src2_index_imm];
				SIM_MemDataRead(addr, &data);
				regs[inst.dst_index] = data;
				return SIM_GetLoadLat();
			}
			case CMD_STORE: {
				// cout << "STORE";
				uint32_t addr = static_cast<uint32_t>(inst.dst_index);
				addr += inst.isSrc2Imm ? inst.src2_index_imm : regs[inst.src2_index_imm];
				SIM_MemDataWrite(addr, regs[inst.src1_index]);
				return SIM_GetStoreLat();
			}
			case CMD_HALT:
				// cout << "HALT";
				return -1;
		}
		throw std::domain_error("Unrecognized opcode: " + std::to_string(inst.opcode));
	}

	void GetContext(tcontext & context) {
		for (int i = 0; i < REGS_COUNT; ++i)
		{
			context.reg[i] = regs[i];
		}
	}
};



class Blocked
{
	int cycle, instructions;
	vector<Thread> threads;
public:
	Blocked() : cycle(0), instructions(0), threads() {
		for (int tid = 0; tid < SIM_GetThreadsNum(); ++tid)
		{
			threads.emplace_back(tid);
		}
	}

	void Run() {
		int running_threads = threads.size();
		int active_thread = -1;

		// cout << "RUNNING BLOCKED SIMULATOR" << endl;
		while (running_threads > 0) {
			for (int tid = 0; tid < static_cast<int>(threads.size()); ++tid) {
				if (active_thread == tid) {
					// no threads can progress, idle cycle
					// cout << cycle << "\tidle" << endl;
					++cycle;
				}
				while (threads[tid].release_time >= 0 && threads[tid].release_time <= cycle) {
					// Thread is not halted (>= 0) and not waiting (<= cycle)
					// Perform context switch (if needed)
					if (active_thread != tid && active_thread != -1) {
						// for (int c = 0; c < SIM_GetSwitchCycles(); ++c)
						// {
						// 	cout << cycle + c << "\tswitch " << active_thread << " > " << tid << endl;
						// }
						cycle += SIM_GetSwitchCycles();
					}
					active_thread = tid;

					// Run an instruction
					// cout << cycle << "\t" << "thread " << tid << "\t";
					int delay = threads[tid].RunInstruction();
					// cout << endl;
					++cycle; ++instructions;

					// update thread's release time
					if (delay < 0) {
						// halted
						threads[tid].release_time = -1;
						--running_threads;
					} else {
						// thread is locked until delay has passed
						// (note that cycle has already been incremented)
						// for arithmetic operations, delay is 0 so it doesn't matter
						threads[tid].release_time = cycle + delay;
					}
				}
			}
		}
	}

	void GetContext(tcontext context[], int threadid) {
		threads[threadid].GetContext(context[threadid]);
	}

	double GetCPI() {
		return static_cast<double>(cycle) / static_cast<double>(instructions);
	}
};


class Finegrained
{
	int cycle, instructions;
	vector<Thread> threads;
public:
	Finegrained() : cycle(0), instructions(0), threads() {
		for (int tid = 0; tid < SIM_GetThreadsNum(); ++tid)
		{
			threads.emplace_back(tid);
		}
	}

	void Run() {
		int running_threads = threads.size();
		int last_run_thread = -1;

		// cout << "RUNNING FINEGRAINED SIMULATOR" << endl;
		while (running_threads > 0) {
			for (int tid = 0; tid < static_cast<int>(threads.size()); ++tid) {
				if (threads[tid].release_time >= 0 && threads[tid].release_time <= cycle) {
					// Thread is not halted (>= 0) and not waiting (<= cycle)
					// cout << cycle << "\t" << "thread " << tid << "\t";
					int delay = threads[tid].RunInstruction();
					// cout << endl;
					++cycle; ++instructions;
					last_run_thread = tid;

					if (delay < 0) {
						// halted
						threads[tid].release_time = -1;
						--running_threads;
					} else {
						// thread is locked until delay has passed
						// (note that cycle has already been incremented)
						// for arithmetic operations, delay is 0 so it doesn't matter
						threads[tid].release_time = cycle + delay;
					}
				} else if (last_run_thread == tid) {
					// no threads can progress, idle cycle
					// cout << cycle << "\tidle" << endl;
					++cycle;
				}
			}
		}
	}

	void GetContext(tcontext context[], int threadid) {
		threads[threadid].GetContext(context[threadid]);
	}

	double GetCPI() {
		return static_cast<double>(cycle) / static_cast<double>(instructions);
	}
};

Blocked * blc;

void CORE_BlockedMT() {
	// cout << "CORE_BlockedMT" << endl;
	blc = new Blocked();
	// cout << "Running..." << endl;
	blc->Run();
	// cout << "Done" << endl;
}

double CORE_BlockedMT_CPI(){
	double cpi = blc->GetCPI();
	delete blc;
	return cpi;
}

void CORE_BlockedMT_CTX(tcontext context[], int threadid) {
	blc->GetContext(context, threadid);
}


Finegrained * fg;

void CORE_FinegrainedMT() {
	// cout << "CORE_BlockedMT" << endl;
	fg = new Finegrained();
	// cout << "Running..." << endl;
	fg->Run();
	// cout << "Done" << endl;
}

double CORE_FinegrainedMT_CPI(){
	double cpi = fg->GetCPI();
	delete fg;
	return cpi;
}

void CORE_FinegrainedMT_CTX(tcontext context[], int threadid) {
	fg->GetContext(context, threadid);
}
