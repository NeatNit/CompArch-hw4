/* 046267 Computer Architecture - Spring 2021 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <vector>
#include <stdexcept>

using std::vector;

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
				return 0;
			case CMD_ADD:
				regs[inst.dst_index] = regs[inst.src1_index] + regs[inst.src2_index_imm];
				return 0;
			case CMD_SUB:
				regs[inst.dst_index] = regs[inst.src1_index] - regs[inst.src2_index_imm];
				return 0;
			case CMD_ADDI:
				regs[inst.dst_index] = regs[inst.src1_index] + inst.src2_index_imm;
				return 0;
			case CMD_SUBI:
				regs[inst.dst_index] = regs[inst.src1_index] - inst.src2_index_imm;
				return 0;
			case CMD_LOAD:
				int32_t data;
				uint32_t addr = reinterpret_cast<uint32_t>(regs[inst.src1_index]);
				addr += inst.isSrc2Imm ? inst.src2_index_imm : regs[inst.src2_index_imm];
				SIM_MemDataRead(addr, &data);
				regs[inst.dst_index] = data;
				return SIM_GetLoadLat();
			case CMD_STORE:
				uint32_t addr = reinterpret_cast<uint32_t>(inst.dst_index);
				addr += inst.isSrc2Imm ? inst.src2_index_imm : regs[inst.src2_index_imm];
				SIM_MemDataWrite(addr, regs[inst.src1_index]);
				return SIM_GetStoreLat();
			case CMD_HALT:
				return -1;
		}
		throw domain_error("Unrecognized opcode: " + std::to_string(inst.opcode));
	}

	void GetContext(tcontext context[]) {
		for (int i = 0; i < REGS_COUNT; ++i)
		{
			context[0].regs[i] = regs[i];
		}
	}
};

class Finegrained
{
	int cycle, instructions;
	vector<Threads> threads;
public:
	Finegrained() : cycle(0), instructions(0), threads() {
		for (int tid = 0; tid < SIM_GetThreadNum(); ++tid)
		{
			threads.emplace_back(tid);
		}
	}

	void Run() {
		int running_threads = threads.size();
		int last_run_thread = -1;
		while (running_threads > 0) {
			for (int tid = 0; tid < threads.size(); ++tid)
			{
				if (threads[tid].release_time >= 0 && threads[tid].release_time <= cycle) {
					// Thread is not halted (>= 0) and not waiting (<= cycle)
					int delay = threads[tid].RunInstruction();
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
					++cycle;
				}
			}
		}
	}

	void GetContext(tcontext context[], int threadid) {
		threads[threadid].GetContext(context);
	}

	double GetCPI() {
		return static_cast<double>(cycle) / static_cast<double>(instructions);
	}
};

void CORE_BlockedMT() {
}

double CORE_BlockedMT_CPI(){
	return 0;
}

void CORE_BlockedMT_CTX(tcontext context[], int threadid) {
}


Finegrained * fg;

void CORE_FinegrainedMT() {
	fg = new Finegrained();
	fg->Run();
}

double CORE_FinegrainedMT_CPI(){
	double cpi = fg->GetCPI();
	delete fg;
	return cpi;
}

void CORE_FinegrainedMT_CTX(tcontext context[], int threadid) {
	fg->GetContext(context, threadid);
}
