/* 046267 Computer Architecture - Spring 2021 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <vector>
#include <stdexcept>


class Thread
{
	std::vector<int32_t> regs;
	int tid;
	uint32_t line;
public:
	Thread(int tid) : regs(REGS_COUNT, 0), tid(tid), line(0) {};

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
};

void CORE_BlockedMT() {
}

void CORE_FinegrainedMT() {
}

double CORE_BlockedMT_CPI(){
	return 0;
}

double CORE_FinegrainedMT_CPI(){
	return 0;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
}
