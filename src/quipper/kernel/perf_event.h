/*
 * Performance events:
 *
 *    Copyright (C) 2008-2009, Thomas Gleixner <tglx@linutronix.de>
 *    Copyright (C) 2008-2011, Red Hat, Inc., Ingo Molnar
 *    Copyright (C) 2008-2011, Red Hat, Inc., Peter Zijlstra
 *
 * Data type definitions, declarations, prototypes.
 *
 *    Started by: Thomas Gleixner and Ingo Molnar
 *
 * For licencing details see kernel-base/COPYING
 */
// When including both quipper/kernel/perf_event.h and uapi/linux/perf_event.h,
// always include the former before the latter. If uapi/linux/perf_event.h were
// to be included before quipper/kernel/perf_event.h, the macros defined in
// uapi/linux/perf_event.h would expand the constants defined in
// quipper/kernel/perf_event.h and cause the compiler to complain. By including
// quipper/kernel/perf_event.h before uapi/linux/perf_event.h, the constants
// defined in the former would be replaced by macros defined in the latter.
#ifndef PERF_DATA_CONVERTER_SRC_QUIPPER_KERNEL_PERF_EVENT_H_
#define PERF_DATA_CONVERTER_SRC_QUIPPER_KERNEL_PERF_EVENT_H_

#include <linux/types.h>
#include <stdint.h>

namespace quipper {

// These typedefs are from tools/perf/util/types.h in the kernel.
typedef uint64_t u64;
typedef int64_t s64;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned short u16;  
typedef signed short s16;    
typedef unsigned char u8;
typedef signed char s8;

/*
 * User-space ABI bits:
 */

/*
 * attr.type
 */
enum perf_type_id {
  PERF_TYPE_HARDWARE = 0,
  PERF_TYPE_SOFTWARE = 1,
  PERF_TYPE_TRACEPOINT = 2,
  PERF_TYPE_HW_CACHE = 3,
  PERF_TYPE_RAW = 4,
  PERF_TYPE_BREAKPOINT = 5,

  PERF_TYPE_MAX, /* non-ABI */
};

/*
 * Generalized performance event event_id types, used by the
 * attr.event_id parameter of the sys_perf_event_open()
 * syscall:
 */
enum perf_hw_id {
  /*
   * Common hardware events, generalized by the kernel:
   */
  PERF_COUNT_HW_CPU_CYCLES = 0,
  PERF_COUNT_HW_INSTRUCTIONS = 1,
  PERF_COUNT_HW_CACHE_REFERENCES = 2,
  PERF_COUNT_HW_CACHE_MISSES = 3,
  PERF_COUNT_HW_BRANCH_INSTRUCTIONS = 4,
  PERF_COUNT_HW_BRANCH_MISSES = 5,
  PERF_COUNT_HW_BUS_CYCLES = 6,
  PERF_COUNT_HW_STALLED_CYCLES_FRONTEND = 7,
  PERF_COUNT_HW_STALLED_CYCLES_BACKEND = 8,
  PERF_COUNT_HW_REF_CPU_CYCLES = 9,

  PERF_COUNT_HW_MAX, /* non-ABI */
};

/*
 * Generalized hardware cache events:
 *
 *       { L1-D, L1-I, LLC, ITLB, DTLB, BPU, NODE } x
 *       { read, write, prefetch } x
 *       { accesses, misses }
 */
enum perf_hw_cache_id {
  PERF_COUNT_HW_CACHE_L1D = 0,
  PERF_COUNT_HW_CACHE_L1I = 1,
  PERF_COUNT_HW_CACHE_LL = 2,
  PERF_COUNT_HW_CACHE_DTLB = 3,
  PERF_COUNT_HW_CACHE_ITLB = 4,
  PERF_COUNT_HW_CACHE_BPU = 5,
  PERF_COUNT_HW_CACHE_NODE = 6,

  PERF_COUNT_HW_CACHE_MAX, /* non-ABI */
};

enum perf_hw_cache_op_id {
  PERF_COUNT_HW_CACHE_OP_READ = 0,
  PERF_COUNT_HW_CACHE_OP_WRITE = 1,
  PERF_COUNT_HW_CACHE_OP_PREFETCH = 2,

  PERF_COUNT_HW_CACHE_OP_MAX, /* non-ABI */
};

enum perf_hw_cache_op_result_id {
  PERF_COUNT_HW_CACHE_RESULT_ACCESS = 0,
  PERF_COUNT_HW_CACHE_RESULT_MISS = 1,

  PERF_COUNT_HW_CACHE_RESULT_MAX, /* non-ABI */
};

/*
 * Special "software" events provided by the kernel, even if the hardware
 * does not support performance events. These events measure various
 * physical and sw events of the kernel (and allow the profiling of them as
 * well):
 */
enum perf_sw_ids {
  PERF_COUNT_SW_CPU_CLOCK = 0,
  PERF_COUNT_SW_TASK_CLOCK = 1,
  PERF_COUNT_SW_PAGE_FAULTS = 2,
  PERF_COUNT_SW_CONTEXT_SWITCHES = 3,
  PERF_COUNT_SW_CPU_MIGRATIONS = 4,
  PERF_COUNT_SW_PAGE_FAULTS_MIN = 5,
  PERF_COUNT_SW_PAGE_FAULTS_MAJ = 6,
  PERF_COUNT_SW_ALIGNMENT_FAULTS = 7,
  PERF_COUNT_SW_EMULATION_FAULTS = 8,
  PERF_COUNT_SW_DUMMY = 9,

  PERF_COUNT_SW_MAX, /* non-ABI */
};

/*
 * Bits that can be set in attr.sample_type to request information
 * in the overflow packets.
 */
enum perf_event_sample_format {
  PERF_SAMPLE_IP = 1U << 0,
  PERF_SAMPLE_TID = 1U << 1,
  PERF_SAMPLE_TIME = 1U << 2,
  PERF_SAMPLE_ADDR = 1U << 3,
  PERF_SAMPLE_READ = 1U << 4,
  PERF_SAMPLE_CALLCHAIN = 1U << 5,
  PERF_SAMPLE_ID = 1U << 6,
  PERF_SAMPLE_CPU = 1U << 7,
  PERF_SAMPLE_PERIOD = 1U << 8,
  PERF_SAMPLE_STREAM_ID = 1U << 9,
  PERF_SAMPLE_RAW = 1U << 10,
  PERF_SAMPLE_BRANCH_STACK = 1U << 11,
  PERF_SAMPLE_REGS_USER = 1U << 12,
  PERF_SAMPLE_STACK_USER = 1U << 13,
  PERF_SAMPLE_WEIGHT = 1U << 14,
  PERF_SAMPLE_DATA_SRC = 1U << 15,
  PERF_SAMPLE_IDENTIFIER = 1U << 16,
  PERF_SAMPLE_TRANSACTION = 1U << 17,
  PERF_SAMPLE_REGS_INTR = 1U << 18,
  PERF_SAMPLE_PHYS_ADDR = 1U << 19,
  PERF_SAMPLE_AUX = 1U << 20,
  PERF_SAMPLE_CGROUP = 1U << 21,
  PERF_SAMPLE_DATA_PAGE_SIZE = 1U << 22,
  PERF_SAMPLE_CODE_PAGE_SIZE = 1U << 23,
  PERF_SAMPLE_WEIGHT_STRUCT = 1U << 24,

  PERF_SAMPLE_MAX = 1U << 25, /* non-ABI */
};

/*
 * values to program into branch_sample_type when PERF_SAMPLE_BRANCH is set
 *
 * If the user does not pass priv level information via branch_sample_type,
 * the kernel uses the event's priv level. Branch and event priv levels do
 * not have to match. Branch priv level is checked for permissions.
 *
 * The branch types can be combined, however BRANCH_ANY covers all types
 * of branches and therefore it supersedes all the other types.
 */
enum perf_branch_sample_type {
  PERF_SAMPLE_BRANCH_USER = 1U << 0,   /* user branches */
  PERF_SAMPLE_BRANCH_KERNEL = 1U << 1, /* kernel branches */
  PERF_SAMPLE_BRANCH_HV = 1U << 2,     /* hypervisor branches */

  PERF_SAMPLE_BRANCH_ANY = 1U << 3,        /* any branch types */
  PERF_SAMPLE_BRANCH_ANY_CALL = 1U << 4,   /* any call branch */
  PERF_SAMPLE_BRANCH_ANY_RETURN = 1U << 5, /* any return branch */
  PERF_SAMPLE_BRANCH_IND_CALL = 1U << 6,   /* indirect calls */
  PERF_SAMPLE_BRANCH_ABORT_TX = 1U << 7,   /* transaction aborts */
  PERF_SAMPLE_BRANCH_IN_TX = 1U << 8,      /* in transaction */
  PERF_SAMPLE_BRANCH_NO_TX = 1U << 9,      /* not in transaction */
  PERF_SAMPLE_BRANCH_COND = 1U << 10,      /* conditional branches */
  PERF_SAMPLE_BRANCH_HW_INDEX = 1U << 17,  /* sample contains hw_idx field */

  PERF_SAMPLE_BRANCH_MAX = 1U << 18, /* non-ABI */
};

/*
 * Common flow change classification
 */
enum {
  PERF_BR_UNKNOWN = 0,     /* unknown */
  PERF_BR_COND = 1,        /* conditional */
  PERF_BR_UNCOND = 2,      /* unconditional  */
  PERF_BR_IND = 3,         /* indirect */
  PERF_BR_CALL = 4,        /* function call */
  PERF_BR_IND_CALL = 5,    /* indirect function call */
  PERF_BR_RET = 6,         /* function return */
  PERF_BR_SYSCALL = 7,     /* syscall */
  PERF_BR_SYSRET = 8,      /* syscall return */
  PERF_BR_COND_CALL = 9,   /* conditional function call */
  PERF_BR_COND_RET = 10,   /* conditional function return */
  PERF_BR_ERET = 11,       /* exception return */
  PERF_BR_IRQ = 12,        /* irq */
  PERF_BR_SERROR = 13,     /* system error */
  PERF_BR_NO_TX = 14,      /* not in transaction */
  PERF_BR_EXTEND_ABI = 15, /* extend ABI */
  PERF_BR_MAX,
};

/*
 * Common branch speculation outcome classification
 */
enum {
  PERF_BR_SPEC_NA = 0,               /* Not available */
  PERF_BR_SPEC_WRONG_PATH = 1,       /* Speculative but on wrong path */
  PERF_BR_NON_SPEC_CORRECT_PATH = 2, /* Non-speculative but on correct path */
  PERF_BR_SPEC_CORRECT_PATH = 3,     /* Speculative and on correct path */
  PERF_BR_SPEC_MAX,
};

const u16 PERF_SAMPLE_BRANCH_PLM_ALL =
    (PERF_SAMPLE_BRANCH_USER | PERF_SAMPLE_BRANCH_KERNEL |
     PERF_SAMPLE_BRANCH_HV);

/*
 * Values to determine ABI of the registers dump.
 */
enum perf_sample_regs_abi {
  PERF_SAMPLE_REGS_ABI_NONE = 0,
  PERF_SAMPLE_REGS_ABI_32 = 1,
  PERF_SAMPLE_REGS_ABI_64 = 2,
};

/*
 * Values for the memory transaction event qualifier, mostly for
 * abort events. Multiple bits can be set.
 */
enum {
  PERF_TXN_ELISION = (1 << 0),        /* From elision */
  PERF_TXN_TRANSACTION = (1 << 1),    /* From transaction */
  PERF_TXN_SYNC = (1 << 2),           /* Instruction is related */
  PERF_TXN_ASYNC = (1 << 3),          /* Instruction not related */
  PERF_TXN_RETRY = (1 << 4),          /* Retry possible */
  PERF_TXN_CONFLICT = (1 << 5),       /* Conflict abort */
  PERF_TXN_CAPACITY_WRITE = (1 << 6), /* Capacity write abort */
  PERF_TXN_CAPACITY_READ = (1 << 7),  /* Capacity read abort */

  PERF_TXN_MAX = (1 << 8), /* non-ABI */

  /* bits 32..63 are reserved for the abort code */

  PERF_TXN_ABORT_MASK = (0xffffffffULL << 32),
  PERF_TXN_ABORT_SHIFT = 32,
};

/*
 * The format of the data returned by read() on a perf event fd,
 * as specified by attr.read_format:
 *
 * struct read_format {
 *	{ u64		value;
 *	  { u64		time_enabled; } && PERF_FORMAT_TOTAL_TIME_ENABLED
 *	  { u64		time_running; } && PERF_FORMAT_TOTAL_TIME_RUNNING
 *	  { u64		id;           } && PERF_FORMAT_ID
 *	  { u64		lost;         } && PERF_FORMAT_LOST
 *	} && !PERF_FORMAT_GROUP
 *
 *	{ u64		nr;
 *	  { u64		time_enabled; } && PERF_FORMAT_TOTAL_TIME_ENABLED
 *	  { u64		time_running; } && PERF_FORMAT_TOTAL_TIME_RUNNING
 *	  { u64		value;
 *	    { u64	id;           } && PERF_FORMAT_ID
 *	    { u64	lost;         } && PERF_FORMAT_LOST
 *	  }		cntr[nr];
 *	} && PERF_FORMAT_GROUP
 * };
 */
enum perf_event_read_format {
  PERF_FORMAT_TOTAL_TIME_ENABLED = 1U << 0,
  PERF_FORMAT_TOTAL_TIME_RUNNING = 1U << 1,
  PERF_FORMAT_ID = 1U << 2,
  PERF_FORMAT_GROUP = 1U << 3,
  PERF_FORMAT_LOST = 1U << 4,

  PERF_FORMAT_MAX = 1U << 5, /* non-ABI */
};

const u16 PERF_ATTR_SIZE_VER0 = 64; /* sizeof first published struct */
const u16 PERF_ATTR_SIZE_VER1 = 72; /* add: config2 */
const u16 PERF_ATTR_SIZE_VER2 = 80; /* add: branch_sample_type */
const u16 PERF_ATTR_SIZE_VER3 = 96; /* add: sample_regs_user */
                                    /* add: sample_stack_user */

/*
 * Hardware event_id to monitor via a performance monitoring event:
 */
struct perf_event_attr {
  /*
   * Major type: hardware/software/tracepoint/etc.
   */
  __u32 type;

  /*
   * Size of the attr structure, for fwd/bwd compat.
   */
  __u32 size;

  /*
   * Type specific configuration information.
   */
  __u64 config;

  union {
    __u64 sample_period;
    __u64 sample_freq;
  };

  __u64 sample_type;
  __u64 read_format;

  __u64 disabled : 1,     /* off by default        */
      inherit : 1,        /* children inherit it   */
      pinned : 1,         /* must always be on PMU */
      exclusive : 1,      /* only group on PMU     */
      exclude_user : 1,   /* don't count user      */
      exclude_kernel : 1, /* ditto kernel          */
      exclude_hv : 1,     /* ditto hypervisor      */
      exclude_idle : 1,   /* don't count when idle */
      mmap : 1,           /* include mmap data     */
      comm : 1,           /* include comm data     */
      freq : 1,           /* use freq, not period  */
      inherit_stat : 1,   /* per task counts       */
      enable_on_exec : 1, /* next exec enables     */
      task : 1,           /* trace fork/exit       */
      watermark : 1,      /* wakeup_watermark      */
      /*
       * precise_ip:
       *
       *  0 - SAMPLE_IP can have arbitrary skid
       *  1 - SAMPLE_IP must have constant skid
       *  2 - SAMPLE_IP requested to have 0 skid
       *  3 - SAMPLE_IP must have 0 skid
       *
       *  See also PERF_RECORD_MISC_EXACT_IP
       */
      precise_ip : 2,    /* skid constraint       */
      mmap_data : 1,     /* non-exec mmap data    */
      sample_id_all : 1, /* sample_type all events */

      exclude_host : 1,  /* don't count in host   */
      exclude_guest : 1, /* don't count in guest  */

      exclude_callchain_kernel : 1, /* exclude kernel callchains */
      exclude_callchain_user : 1,   /* exclude user callchains */
      mmap2 : 1,                    /* include mmap with inode data     */
      comm_exec : 1,      /* flag comm events that are due to an exec */
      use_clockid : 1,    /* use @clockid for time fields */
      context_switch : 1, /* context switch data */
      write_backward : 1, /* Write ring buffer from end to beginning */
      namespaces : 1,     /* include namespaces data */
      ksymbol : 1,        /* include ksymbol events */
      bpf_event : 1,      /* include bpf events */
      aux_output : 1,     /* generate AUX records instead of events */
      cgroup : 1,         /* include cgroup events */
      task_poke : 1,      /* include task_poke events */
      __reserved_1 : 30;

  union {
    __u32 wakeup_events;    /* wakeup every n events */
    __u32 wakeup_watermark; /* bytes before wakeup   */
  };

  __u32 bp_type;
  union {
    __u64 bp_addr;
    __u64 config1; /* extension of config */
  };
  union {
    __u64 bp_len;
    __u64 config2; /* extension of config1 */
  };
  __u64 branch_sample_type; /* enum perf_branch_sample_type */

  /*
   * Defines set of user regs to dump on samples.
   * See asm/perf_regs.h for details.
   */
  __u64 sample_regs_user;

  /*
   * Defines size of the user stack to dump on samples.
   */
  __u32 sample_stack_user;

  /* Align to u64. */
  __u32 __reserved_2;
};

/*
 * Structure of the page that can be mapped via mmap
 */
struct perf_event_mmap_page {
  __u32 version;        /* version number of this structure */
  __u32 compat_version; /* lowest version this is compat with */

  /*
   * Bits needed to read the hw events in user-space.
   *
   *   u32 seq, time_mult, time_shift, idx, width;
   *   u64 count, enabled, running;
   *   u64 cyc, time_offset;
   *   s64 pmc = 0;
   *
   *   do {
   *     seq = pc->lock;
   *     barrier()
   *
   *     enabled = pc->time_enabled;
   *     running = pc->time_running;
   *
   *     if (pc->cap_usr_time && enabled != running) {
   *       cyc = rdtsc();
   *       time_offset = pc->time_offset;
   *       time_mult   = pc->time_mult;
   *       time_shift  = pc->time_shift;
   *     }
   *
   *     idx = pc->index;
   *     count = pc->offset;
   *     if (pc->cap_usr_rdpmc && idx) {
   *       width = pc->pmc_width;
   *       pmc = rdpmc(idx - 1);
   *     }
   *
   *     barrier();
   *   } while (pc->lock != seq);
   *
   * NOTE: for obvious reason this only works on self-monitoring
   *       processes.
   */
  __u32 lock;         /* seqlock for synchronization */
  __u32 index;        /* hardware event identifier */
  __s64 offset;       /* add to hardware event value */
  __u64 time_enabled; /* time event active */
  __u64 time_running; /* time event on cpu */
  union {
    __u64 capabilities;
    struct {
      __u64 cap_bit0 : 1, /* Always 0, deprecated, see commit 860f085b74e9 */
          cap_bit0_is_deprecated : 1, /* Always 1, signals that bit 0 is zero */

          cap_user_rdpmc : 1,     // The RDPMC instruction can be used to read
                                  // counts
          cap_user_time : 1,      /* The time_* fields are used */
          cap_user_time_zero : 1, /* The time_zero field is used */
          cap_____res : 59;
    };
  };

  /*
   * If cap_usr_rdpmc this field provides the bit-width of the value
   * read using the rdpmc() or equivalent instruction. This can be used
   * to sign extend the result like:
   *
   *   pmc <<= 64 - width;
   *   pmc >>= 64 - width; // signed shift right
   *   count += pmc;
   */
  __u16 pmc_width;

  /*
   * If cap_usr_time the below fields can be used to compute the time
   * delta since time_enabled (in ns) using rdtsc or similar.
   *
   *   u64 quot, rem;
   *   u64 delta;
   *
   *   quot = (cyc >> time_shift);
   *   rem = cyc & ((1 << time_shift) - 1);
   *   delta = time_offset + quot * time_mult +
   *              ((rem * time_mult) >> time_shift);
   *
   * Where time_offset,time_mult,time_shift and cyc are read in the
   * seqcount loop described above. This delta can then be added to
   * enabled and possible running (if idx), improving the scaling:
   *
   *   enabled += delta;
   *   if (idx)
   *     running += delta;
   *
   *   quot = count / running;
   *   rem  = count % running;
   *   count = quot * enabled + (rem * enabled) / running;
   */
  __u16 time_shift;
  __u32 time_mult;
  __u64 time_offset;
  /*
   * If cap_usr_time_zero, the hardware clock (e.g. TSC) can be calculated
   * from sample timestamps.
   *
   *   time = timestamp - time_zero;
   *   quot = time / time_mult;
   *   rem  = time % time_mult;
   *   cyc = (quot << time_shift) + (rem << time_shift) / time_mult;
   *
   * And vice versa:
   *
   *   quot = cyc >> time_shift;
   *   rem  = cyc & ((1 << time_shift) - 1);
   *   timestamp = time_zero + quot * time_mult +
   *               ((rem * time_mult) >> time_shift);
   */
  __u64 time_zero;
  __u32 size; /* Header size up to __reserved[] fields. */

  /*
   * Hole for extension of the self monitor capabilities
   */

  __u8 __reserved[118 * 8 + 4]; /* align to 1k. */

  /*
   * Control data for the mmap() data buffer.
   *
   * User-space reading the @data_head value should issue an smp_rmb(),
   * after reading this value.
   *
   * When the mapping is PROT_WRITE the @data_tail value should be
   * written by userspace to reflect the last read data, after issueing
   * an smp_mb() to separate the data read from the ->data_tail store.
   * In this case the kernel will not over-write unread data.
   *
   * See perf_output_put_handle() for the data ordering.
   */
  __u64 data_head; /* head in the data section */
  __u64 data_tail; /* user-space written tail */
};

const u16 PERF_RECORD_MISC_CPUMODE_MASK = 7 << 0;
const u16 PERF_RECORD_MISC_CPUMODE_UNKNOWN = 0 << 0;
const u16 PERF_RECORD_MISC_KERNEL = 1 << 0;
const u16 PERF_RECORD_MISC_USER = 2 << 0;
const u16 PERF_RECORD_MISC_HYPERVISOR = 3 << 0;
const u16 PERF_RECORD_MISC_GUEST_KERNEL = 4 << 0;
const u16 PERF_RECORD_MISC_GUEST_USER = 5 << 0;

/*
 * Indicates that /proc/PID/maps parsing are truncated by time out.
 */
const u16 PERF_RECORD_MISC_PROC_MAP_PARSE_TIMEOUT = 1 << 12;

/*
 * PERF_RECORD_MISC_MMAP_DATA and PERF_RECORD_MISC_COMM_EXEC are used on
 * different events so can reuse the same bit position.
 * Ditto PERF_RECORD_MISC_SWITCH_OUT.
 */
const u16 PERF_RECORD_MISC_MMAP_DATA = 1 << 13;
const u16 PERF_RECORD_MISC_COMM_EXEC = 1 << 13;
const u16 PERF_RECORD_MISC_SWITCH_OUT = 1 << 13;

/*
 * These PERF_RECORD_MISC_* flags below are safely reused
 * for the following events:
 *
 *   PERF_RECORD_MISC_EXACT_IP           - PERF_RECORD_SAMPLE of precise events
 *   PERF_RECORD_MISC_SWITCH_OUT_PREEMPT - PERF_RECORD_SWITCH* events
 *   PERF_RECORD_MISC_MMAP_BUILD_ID      - PERF_RECORD_MMAP2 event
 *
 *
 * PERF_RECORD_MISC_EXACT_IP:
 *   Indicates that the content of PERF_SAMPLE_IP points to
 *   the actual instruction that triggered the event. See also
 *   perf_event_attr::precise_ip.
 *
 * PERF_RECORD_MISC_SWITCH_OUT_PREEMPT:
 *   Indicates that thread was preempted in TASK_RUNNING state.
 *
 * PERF_RECORD_MISC_MMAP_BUILD_ID:
 *   Indicates that mmap2 event carries build id data.
 */
const u16 PERF_RECORD_MISC_EXACT_IP = 1 << 14;
const u16 PERF_RECORD_MISC_SWITCH_OUT_PREEMPT = 1 << 14;
const u16 PERF_RECORD_MISC_MMAP_BUILD_ID = 1 << 14;
/*
 * Reserve the last bit to indicate some extended misc field
 */
const u16 PERF_RECORD_MISC_EXT_RESERVED = 1 << 15;

struct perf_event_header {
  __u32 type;
  __u16 misc;
  __u16 size;
};

struct perf_ns_link_info {
  __u64 dev;
  __u64 ino;
};

enum {
  NET_NS_INDEX = 0,
  UTS_NS_INDEX = 1,
  IPC_NS_INDEX = 2,
  PID_NS_INDEX = 3,
  USER_NS_INDEX = 4,
  MNT_NS_INDEX = 5,
  CGROUP_NS_INDEX = 6,

  NR_NAMESPACES, /* number of available namespaces */
};

enum perf_event_type {
  /*
   * If perf_event_attr.sample_id_all is set then all event types will
   * have the sample_type selected fields related to where/when
   * (identity) an event took place (TID, TIME, ID, STREAM_ID, CPU,
   * IDENTIFIER) described in PERF_RECORD_SAMPLE below, it will be stashed
   * just after the perf_event_header and the fields already present for
   * the existing fields, i.e. at the end of the payload. That way a newer
   * perf.data file will be supported by older perf tools, with these new
   * optional fields being ignored.
   *
   * struct sample_id {
   * 	{ u32			pid, tid; } && PERF_SAMPLE_TID
   * 	{ u64			time;     } && PERF_SAMPLE_TIME
   * 	{ u64			id;       } && PERF_SAMPLE_ID
   * 	{ u64			stream_id;} && PERF_SAMPLE_STREAM_ID
   * 	{ u32			cpu, res; } && PERF_SAMPLE_CPU
   *	{ u64			id;	  } && PERF_SAMPLE_IDENTIFIER
   * } && perf_event_attr::sample_id_all
   *
   * Note that PERF_SAMPLE_IDENTIFIER duplicates PERF_SAMPLE_ID.  The
   * advantage of PERF_SAMPLE_IDENTIFIER is that its position is fixed
   * relative to header.size.
   */

  /*
   * The MMAP events record the PROT_EXEC mappings so that we can
   * correlate userspace IPs to code. They have the following structure:
   *
   * struct {
   *	struct perf_event_header	header;
   *
   *	u32				pid, tid;
   *	u64				addr;
   *	u64				len;
   *	u64				pgoff;
   *	char				filename[];
   * 	struct sample_id		sample_id;
   * };
   */
  PERF_RECORD_MMAP = 1,

  /*
   * struct {
   *	struct perf_event_header	header;
   *	u64				id;
   *	u64				lost;
   * 	struct sample_id		sample_id;
   * };
   */
  PERF_RECORD_LOST = 2,

  /*
   * struct {
   *	struct perf_event_header	header;
   *
   *	u32				pid, tid;
   *	char				comm[];
   * 	struct sample_id		sample_id;
   * };
   */
  PERF_RECORD_COMM = 3,

  /*
   * struct {
   *	struct perf_event_header	header;
   *	u32				pid, ppid;
   *	u32				tid, ptid;
   *	u64				time;
   * 	struct sample_id		sample_id;
   * };
   */
  PERF_RECORD_EXIT = 4,

  /*
   * struct {
   *	struct perf_event_header	header;
   *	u64				time;
   *	u64				id;
   *	u64				stream_id;
   * 	struct sample_id		sample_id;
   * };
   */
  PERF_RECORD_THROTTLE = 5,
  PERF_RECORD_UNTHROTTLE = 6,

  /*
   * struct {
   *	struct perf_event_header	header;
   *	u32				pid, ppid;
   *	u32				tid, ptid;
   *	u64				time;
   * 	struct sample_id		sample_id;
   * };
   */
  PERF_RECORD_FORK = 7,

  /*
   * struct {
   *	struct perf_event_header	header;
   *	u32				pid, tid;
   *
   *	struct read_format		values;
   * 	struct sample_id		sample_id;
   * };
   */
  PERF_RECORD_READ = 8,

  /*
   * struct {
   *	struct perf_event_header	header;
   *
   *	#
   *	# Note that PERF_SAMPLE_IDENTIFIER duplicates PERF_SAMPLE_ID.
   *	# The advantage of PERF_SAMPLE_IDENTIFIER is that its position
   *	# is fixed relative to header.
   *	#
   *
   *	{ u64			id;	  } && PERF_SAMPLE_IDENTIFIER
   *	{ u64			ip;	  } && PERF_SAMPLE_IP
   *	{ u32			pid, tid; } && PERF_SAMPLE_TID
   *	{ u64			time;     } && PERF_SAMPLE_TIME
   *	{ u64			addr;     } && PERF_SAMPLE_ADDR
   *	{ u64			id;	  } && PERF_SAMPLE_ID
   *	{ u64			stream_id;} && PERF_SAMPLE_STREAM_ID
   *	{ u32			cpu, res; } && PERF_SAMPLE_CPU
   *	{ u64			period;   } && PERF_SAMPLE_PERIOD
   *
   *	{ struct read_format	values;	  } && PERF_SAMPLE_READ
   *
   *	{ u64			nr,
   *	  u64			ips[nr];  } && PERF_SAMPLE_CALLCHAIN
   *
   *	#
   *	# The RAW record below is opaque data wrt the ABI
   *	#
   *	# That is, the ABI doesn't make any promises wrt to
   *	# the stability of its content, it may vary depending
   *	# on event, hardware, kernel version and phase of
   *	# the moon.
   *	#
   *	# In other words, PERF_SAMPLE_RAW contents are not an ABI.
   *	#
   *
   *	{ u32			size;
   *	  char                  data[size];}&& PERF_SAMPLE_RAW
   *
   *	{ u64                   nr;
   *	  { u64	hw_idx; } && PERF_SAMPLE_BRANCH_HW_INDEX
   *      { u64 from, to, flags } lbr[nr];
   *    } && PERF_SAMPLE_BRANCH_STACK
   *
   * 	{ u64			abi; # enum perf_sample_regs_abi
   * 	  u64			regs[weight(mask)]; } && PERF_SAMPLE_REGS_USER
   *
   * 	{ u64			size;
   * 	  char			data[size];
   * 	  u64			dyn_size; } && PERF_SAMPLE_STACK_USER
   *
   *	{ union perf_sample_weight
   *	 {
   *		u64		full; && PERF_SAMPLE_WEIGHT
   *	#if defined(__LITTLE_ENDIAN_BITFIELD)
   *		struct {
   *			u32	var1_dw;
   *			u16	var2_w;
   *			u16	var3_w;
   *		} && PERF_SAMPLE_WEIGHT_STRUCT
   *	#elif defined(__BIG_ENDIAN_BITFIELD)
   *		struct {
   *			u16	var3_w;
   *			u16	var2_w;
   *			u32	var1_dw;
   *		} && PERF_SAMPLE_WEIGHT_STRUCT
   *	#endif
   *	 }
   *	}
   *	{ u64			data_src; } && PERF_SAMPLE_DATA_SRC
   *	{ u64			transaction; } && PERF_SAMPLE_TRANSACTION

   * 	{ u64			abi; # enum perf_sample_regs_abi
   * 	  u64			regs[weight(mask)]; } && PERF_SAMPLE_REGS_INTR

   *	{ u64			physical_addr; } && PERF_SAMPLE_PHYS_ADDR
   *	{ u64			cgroup; } && PERF_SAMPLE_CGROUP
   *	{ u64			data_page_size; } && PERF_SAMPLE_DATA_PAGE_SIZE
   *	{ u64			code_page_size; } && PERF_SAMPLE_CODE_PAGE_SIZE
   * };
   */
  PERF_RECORD_SAMPLE = 9,

  /*
   * The MMAP2 records are an augmented version of MMAP, they add
   * maj, min, ino numbers to be used to uniquely identify each mapping
   *
   * struct {
   *	struct perf_event_header	header;
   *
   *	u32				pid, tid;
   *	u64				addr;
   *	u64				len;
   *	u64				pgoff;
   *	union {
   *		struct {
   *			u32		maj;
   *			u32		min;
   *			u64		ino;
   *			u64		ino_generation;
   *		};
   *		struct {
   *			u8		build_id_size;
   *			u8		__reserved_1;
   *			u16		__reserved_2;
   *			u8		build_id[20];
   *		};
   *	};
   *	u32				prot, flags;
   *	char				filename[];
   * 	struct sample_id		sample_id;
   * };
   */
  PERF_RECORD_MMAP2 = 10,

  /*
   * Records that new data landed in the AUX buffer part.
   *
   * struct {
   * 	struct perf_event_header	header;
   *
   * 	u64				aux_offset;
   * 	u64				aux_size;
   *	u64				flags;
   * 	struct sample_id		sample_id;
   * };
   */
  PERF_RECORD_AUX = 11,

  /*
   * Indicates that instruction trace has started
   *
   * struct {
   *	struct perf_event_header	header;
   *	u32				pid;
   *	u32				tid;
   * 	struct sample_id		sample_id;
   * };
   */
  PERF_RECORD_ITRACE_START = 12,

  /*
   * Records the dropped/lost sample number.
   *
   * struct {
   *	struct perf_event_header	header;
   *
   *	u64				lost;
   *	struct sample_id		sample_id;
   * };
   */
  PERF_RECORD_LOST_SAMPLES = 13,

  /*
   * Records a context switch in or out (flagged by
   * PERF_RECORD_MISC_SWITCH_OUT). See also
   * PERF_RECORD_SWITCH_CPU_WIDE.
   *
   * struct {
   *	struct perf_event_header	header;
   *	struct sample_id		sample_id;
   * };
   */
  PERF_RECORD_SWITCH = 14,

  /*
   * CPU-wide version of PERF_RECORD_SWITCH with next_prev_pid and
   * next_prev_tid that are the next (switching out) or previous
   * (switching in) pid/tid.
   *
   * struct {
   *	struct perf_event_header	header;
   *	u32				next_prev_pid;
   *	u32				next_prev_tid;
   *	struct sample_id		sample_id;
   * };
   */
  PERF_RECORD_SWITCH_CPU_WIDE = 15,

  /*
   * struct {
   *	struct perf_event_header	header;
   *	u32				pid;
   *	u32				tid;
   *	u64				nr_namespaces;
   *	{ u64				dev, inode; } [nr_namespaces];
   *	struct sample_id		sample_id;
   * };
   */
  PERF_RECORD_NAMESPACES = 16,

  /*
   * Record ksymbol register/unregister events:
   *
   * struct {
   *	struct perf_event_header	header;
   *	u64				addr;
   *	u32				len;
   *	u16				ksym_type;
   *	u16				flags;
   *	char				name[];
   *	struct sample_id		sample_id;
   * };
   */
  PERF_RECORD_KSYMBOL = 17,

  /*
   * Record bpf events:
   *  enum perf_bpf_event_type {
   *	PERF_BPF_EVENT_UNKNOWN		= 0,
   *	PERF_BPF_EVENT_PROG_LOAD	= 1,
   *	PERF_BPF_EVENT_PROG_UNLOAD	= 2,
   *  };
   *
   * struct {
   *	struct perf_event_header	header;
   *	u16				type;
   *	u16				flags;
   *	u32				id;
   *	u8				tag[BPF_TAG_SIZE];
   *	struct sample_id		sample_id;
   * };
   */
  PERF_RECORD_BPF_EVENT = 18,

  /*
   * struct {
   *	struct perf_event_header	header;
   *	u64				id;
   *	char				path[];
   *	struct sample_id		sample_id;
   * };
   */
  PERF_RECORD_CGROUP = 19,

  /*
   * Records changes to kernel text i.e. self-modified code. 'old_len' is
   * the number of old bytes, 'new_len' is the number of new bytes. Either
   * 'old_len' or 'new_len' may be zero to indicate, for example, the
   * addition or removal of a trampoline. 'bytes' contains the old bytes
   * followed immediately by the new bytes.
   *
   * struct {
   *	struct perf_event_header	header;
   *	u64				addr;
   *	u16				old_len;
   *	u16				new_len;
   *	u8				bytes[];
   *	struct sample_id		sample_id;
   * };
   */
  PERF_RECORD_TEXT_POKE = 20,

  PERF_RECORD_MAX, /* non-ABI */
};

enum perf_record_ksymbol_type {
  PERF_RECORD_KSYMBOL_TYPE_UNKNOWN = 0,
  PERF_RECORD_KSYMBOL_TYPE_BPF = 1,
  /*
   * Out of line code such as kprobe-replaced instructions or optimized
   * kprobes or ftrace trampolines.
   */
  PERF_RECORD_KSYMBOL_TYPE_OOL = 2,
  PERF_RECORD_KSYMBOL_TYPE_MAX /* non-ABI */
};

const u64 PERF_RECORD_KSYMBOL_FLAGS_UNREGISTER = 1UL << 0;

enum perf_bpf_event_type {
  PERF_BPF_EVENT_UNKNOWN = 0,
  PERF_BPF_EVENT_PROG_LOAD = 1,
  PERF_BPF_EVENT_PROG_UNLOAD = 2,
  PERF_BPF_EVENT_MAX, /* non-ABI */
};

#define PERF_MAX_STACK_DEPTH 127

enum perf_callchain_context {
  PERF_CONTEXT_HV = (__u64)-32,
  PERF_CONTEXT_KERNEL = (__u64)-128,
  PERF_CONTEXT_USER = (__u64)-512,

  PERF_CONTEXT_GUEST = (__u64)-2048,
  PERF_CONTEXT_GUEST_KERNEL = (__u64)-2176,
  PERF_CONTEXT_GUEST_USER = (__u64)-2560,

  PERF_CONTEXT_MAX = (__u64)-4095,
};

/**
 * PERF_RECORD_AUX::flags bits
 */
const u64 PERF_AUX_FLAG_TRUNCATED = 0x01; /* record was truncated to fit */
const u64 PERF_AUX_FLAG_OVERWRITE = 0x02; /* snapshot from overwrite mode */
const u64 PERF_AUX_FLAG_PARTIAL = 0x04;   /* record contains gaps */

const u64 PERF_FLAG_FD_NO_GROUP = 1UL << 0;
const u64 PERF_FLAG_FD_OUTPUT = 1UL << 1;
const u64 PERF_FLAG_PID_CGROUP = 1UL
                                 << 2; /* pid=cgroup id, per-cpu mode only */
const u64 PERF_FLAG_FD_CLOEXEC = 1UL << 3; /* O_CLOEXEC */

union perf_mem_data_src {
  __u64 val;
  struct {
    __u64 mem_op : 5,  /* type of opcode */
        mem_lvl : 14,  /* memory hierarchy level */
        mem_snoop : 5, /* snoop mode */
        mem_lock : 2,  /* lock instr */
        mem_dtlb : 7,  /* tlb access */
        mem_rsvd : 31;
  };
};

// type of opcode (load/store/prefetch,code)
const u64 PERF_MEM_OP_NA = 0x01;      // not available
const u64 PERF_MEM_OP_LOAD = 0x02;    // load instruction
const u64 PERF_MEM_OP_STORE = 0x04;   // store instruction
const u64 PERF_MEM_OP_PFETCH = 0x08;  // prefetch
const u64 PERF_MEM_OP_EXEC = 0x10;    // code (execution)

// memory hierarchy (memory level, hit or miss)
const u64 PERF_MEM_LVL_NA = 0x01;         // not available
const u64 PERF_MEM_LVL_HIT = 0x02;        // hit level
const u64 PERF_MEM_LVL_MISS = 0x04;       // miss level
const u64 PERF_MEM_LVL_L1 = 0x08;         // L1
const u64 PERF_MEM_LVL_LFB = 0x10;        // Line Fill Buffer
const u64 PERF_MEM_LVL_L2 = 0x20;         // L2
const u64 PERF_MEM_LVL_L3 = 0x40;         // L3
const u64 PERF_MEM_LVL_LOC_RAM = 0x80;    // Local DRAM
const u64 PERF_MEM_LVL_REM_RAM1 = 0x100;  // Remote DRAM (1 hop)
const u64 PERF_MEM_LVL_REM_RAM2 = 0x200;  // Remote DRAM (2 hops)
const u64 PERF_MEM_LVL_REM_CCE1 = 0x400;  // Remote Cache (1 hop)
const u64 PERF_MEM_LVL_REM_CCE2 = 0x800;  // Remote Cache (2 hops)
const u64 PERF_MEM_LVL_IO = 0x1000;       // I/O memory
const u64 PERF_MEM_LVL_UNC = 0x2000;      // Uncached memory

// snoop mode
const u64 PERF_MEM_SNOOP_NA = 0x01;    // not available
const u64 PERF_MEM_SNOOP_NONE = 0x02;  // no snoop
const u64 PERF_MEM_SNOOP_HIT = 0x04;   // snoop hit
const u64 PERF_MEM_SNOOP_MISS = 0x08;  // snoop miss
const u64 PERF_MEM_SNOOP_HITM = 0x10;  // snoop hit modified

// locked instruction
const u64 PERF_MEM_LOCK_NA = 0x01;      // not available
const u64 PERF_MEM_LOCK_LOCKED = 0x02;  // locked transaction

// TLB access
const u64 PERF_MEM_TLB_NA = 0x01;    // not available
const u64 PERF_MEM_TLB_HIT = 0x02;   // hit level
const u64 PERF_MEM_TLB_MISS = 0x04;  // miss level
const u64 PERF_MEM_TLB_L1 = 0x08;    // L1
const u64 PERF_MEM_TLB_L2 = 0x10;    // L2
const u64 PERF_MEM_TLB_WK = 0x20;    // Hardware Walker
const u64 PERF_MEM_TLB_OS = 0x40;    // OS fault handler

/*
 * single taken branch record layout:
 *
 *      from: source instruction (may not always be a branch insn)
 *        to: branch target
 *   mispred: branch target was mispredicted
 * predicted: branch target was predicted
 *
 * support for mispred, predicted is optional. In case it
 * is not supported mispred = predicted = 0.
 *
 *     in_tx: running in a hardware transaction
 *     abort: aborting a hardware transaction
 *    cycles: cycles from last branch (or 0 if not supported)
 *      type: branch type
 *      spec: branch speculation info (or 0 if not supported)
 */
struct perf_branch_entry {
  __u64 from;
  __u64 to;
  __u64 mispred : 1, /* target mispredicted */
      predicted : 1, /* target predicted */
      in_tx : 1,     /* in transaction */
      abort : 1,     /* transaction abort */
      cycles : 16,   /* cycle count to last branch */
      type : 4,      /* branch type */
      spec : 2,      /* branch speculation info */
      reserved : 38;
};

}  // namespace quipper

#endif /* PERF_DATA_CONVERTER_SRC_QUIPPER_KERNEL_PERF_EVENT_H_ */
