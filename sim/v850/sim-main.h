#define WITH_MODULO_MEMORY 1
#define WITH_WATCHPOINTS 1
#define WITH_TARGET_WORD_MSB 31

#include "sim-basics.h"

typedef address_word sim_cia;

/* This simulator doesn't cache state */
#define SIM_ENGINE_HALT_HOOK(sd,last_cpu,cia) while (0)
#define SIM_ENGINE_RESTART_HOOK(sd,last_cpu,cia) while (0)

#include "sim-base.h"

typedef signed8 int8;
typedef unsigned8 uint8;
typedef signed16 int16;
typedef unsigned16 uint16;
typedef signed32 int32;
typedef unsigned32 uint32;
typedef unsigned32 reg_t;


/* The current state of the processor; registers, memory, etc.  */

typedef struct _v850_regs {
  reg_t regs[32];		/* general-purpose registers */
  reg_t sregs[32];		/* system registers, including psw */
  reg_t pc;
  int dummy_mem;		/* where invalid accesses go */
  int pending_nmi;
} v850_regs;

struct _sim_cpu
{
  /* ... simulator specific members ... */
  v850_regs reg;
  /* ... base type ... */
  sim_cpu_base base;
};

#define CPU_CIA(CPU) ((CPU)->reg.pc)

struct sim_state {
  sim_cpu cpu[MAX_NR_PROCESSORS];
#if (WITH_SMP)
#define STATE_CPU(sd,n) (&(sd)->cpu[n])
#else
#define STATE_CPU(sd,n) (&(sd)->cpu[0])
#endif
#if 0
  SIM_ADDR rom_size;
  SIM_ADDR low_end;
  SIM_ADDR high_start;
  SIM_ADDR high_base;
  void *mem;
#endif
  sim_state_base base;
};

/* For compatibility, until all functions converted to passing
   SIM_DESC as an argument */
extern SIM_DESC simulator;


#define V850_ROM_SIZE 0x8000
#define V850_LOW_END 0x200000
#define V850_HIGH_START 0xffe000


#define SIG_V850_EXIT	-1	/* indication of a normal exit */

extern uint32 OP[4];

/* Because we are still using the old semantic table, provide compat
   macro's that store the instruction where the old simops expects
   it. */

#if 0
OP[0] = inst & 0x1f;
OP[1] = (inst >> 11) & 0x1f;
OP[2] = (inst >> 16) & 0xffff;
OP[3] = inst;
#endif

#define COMPAT_1(CALL) \
PC = cia; \
OP[0] = instruction_0 & 0x1f; \
OP[1] = (instruction_0 >> 11) & 0x1f; \
OP[2] = 0; \
OP[3] = instruction_0 ; \
PC += (CALL); \
nia = PC

#define COMPAT_2(CALL) \
PC = cia; \
OP[0] = instruction_0 & 0x1f; \
OP[1] = (instruction_0 >> 11) & 0x1f; \
OP[2] = instruction_1; \
OP[3] = (instruction_1 << 16) | instruction_0; \
PC += (CALL); \
nia = PC


#if 0
extern struct simops Simops[];
#endif

#define State    (STATE_CPU (simulator, 0)->reg)
#define PC	(State.pc)
#define SP	(State.regs[3])
#define EP	(State.regs[30])

#define EIPC  (State.sregs[0])
#define EIPSW (State.sregs[1])
#define FEPC  (State.sregs[2])
#define FEPSW (State.sregs[3])
#define ECR   (State.sregs[4])
#define PSW   (State.sregs[5])
/* start-sanitize-v850e */
#define CTPC  (State.sregs[16])
#define CTPSW (State.sregs[17])
/* end-sanitize-v850e */
#define DBPC  (State.sregs[18])
#define DBPSW (State.sregs[19])
/* start-sanitize-v850e */
#define CTBP  (State.sregs[20])
/* end-sanitize-v850e */

#define PSW_NP 0x80
#define PSW_EP 0x40
#define PSW_ID 0x20
#define PSW_SAT 0x10
#define PSW_CY 0x8
#define PSW_OV 0x4
#define PSW_S 0x2
#define PSW_Z 0x1

#define SEXT3(x)	((((x)&0x7)^(~0x3))+0x4)	

/* sign-extend a 4-bit number */
#define SEXT4(x)	((((x)&0xf)^(~0x7))+0x8)	

/* sign-extend a 5-bit number */
#define SEXT5(x)	((((x)&0x1f)^(~0xf))+0x10)	

/* sign-extend a 9-bit number */
#define SEXT9(x)	((((x)&0x1ff)^(~0xff))+0x100)

/* sign-extend a 22-bit number */
#define SEXT22(x)	((((x)&0x3fffff)^(~0x1fffff))+0x200000)

/* sign extend a 40 bit number */
#define SEXT40(x)	((((x) & UNSIGNED64 (0xffffffffff)) \
			  ^ (~UNSIGNED64 (0x7fffffffff))) \
			 + UNSIGNED64 (0x8000000000))

/* sign extend a 44 bit number */
#define SEXT44(x)	((((x) & UNSIGNED64 (0xfffffffffff)) \
			  ^ (~ UNSIGNED64 (0x7ffffffffff))) \
			 + UNSIGNED64 (0x80000000000))

/* sign extend a 60 bit number */
#define SEXT60(x)	((((x) & UNSIGNED64 (0xfffffffffffffff)) \
			  ^ (~ UNSIGNED64 (0x7ffffffffffffff))) \
			 + UNSIGNED64 (0x800000000000000))

/* No sign extension */
#define NOP(x)		(x)

#define INC_ADDR(x,i)	x = ((State.MD && x == MOD_E) ? MOD_S : (x)+(i))

#define RLW(x) load_mem (x, 4)

#ifdef _WIN32
#ifndef SIGTRAP
#define SIGTRAP 5
#endif
#ifndef SIGQUIT
#define SIGQUIT 3
#endif
#endif

/* Function declarations.  */

#define IMEM(EA) \
sim_core_read_aligned_2 (STATE_CPU (sd, 0), \
			 PC, sim_core_execute_map, (EA))

#define IMEM_IMMED(EA,N) \
sim_core_read_aligned_2 (STATE_CPU (sd, 0), \
			 PC, sim_core_execute_map, (EA) + (N) * 2)

#define load_mem(ADDR,LEN) \
sim_core_read_unaligned_##LEN (STATE_CPU (simulator, 0), \
			       PC, sim_core_read_map, (ADDR))

#define store_mem(ADDR,LEN,DATA) \
sim_core_write_unaligned_##LEN (STATE_CPU (simulator, 0), \
				PC, sim_core_write_map, (ADDR), (DATA))

#include "simops.h"
