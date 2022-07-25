#ifndef VBERTI_H_
#define VBERTI_H_
#include "cache.h"
#include <cmath>
#include <map>
// Last edit: 27 - Sept - 2021 12:10

// Debug flags
//# define DEBUG_VBERTI // All debug flags
//# define DEBUG_VBERTI_LATENCY // Latency Table
//# define DEBUG_VBERTI_ADDR // Operate address
//# define DEBUG_VBERTI_HISTORY // HISTORY Table
//# define DEBUG_VBERTI_BERTI // BERTI Table
//# define DEBUG_VBERTI_PREF // Prefetches launched
//# define DEBUG_VBERTI_STRIDE // Stride launched
//# define DEBUG_VBERTI_CONFIDENCE // Stride confidence
//# define DEBUG_VBERTI_CACHE // Shadow Cache
//# define DEBUG_STREAM

// Only best time, previous prefetch or late prefetch
//# define ONLY_BEST_TIME_PREFETCH
# define ALLOW_LATE_PREFETCH

// Enable decreasing confidence
# define ENABLE_CONFIDENCE

// Enable stream prefetch
//# define ENABLE_STREAM

// Enable increment confidence to launch multiple prefetch
//# define ENABLE_INCREMENT_CONF

// vBerti defines
# define LATENCY_TABLE_PF             (L1D_PQ_SIZE)
# define LATENCY_TABLE_SIZE           (L1D_MSHR_SIZE)
# define HISTORY_TABLE_SIZE           (512)
# define BERTI_TABLE_SIZE             (128)
# define BERTI_TABLE_STRIDE_SIZE      (16)

// Mask and Limits
# define STRIDE_MASK            (12)
# define IP_MASK                (0x3FF)
# define LIMIT_TIME             (1000)

// Confidence
# define CONFIDENCE_MAX         (64) // 6 bits
# define CONFIDENCE_INIT        (1) // 6 bits
# define CONFIDENCE_ENOUGH      (4) // 6 bits
# define CONFIDENCE_INC         (4) // 6 bits
# define CONFIDENCE_DEC         (0) // 6 bits
# define CONFIDENCE_DEC_LATE    (1) // 6 bits
# define CONFIDENCE_DEC_WRONG   (4) // 6 bits
# define CONFIDENCE_DEC_MAX     (CONFIDENCE_MAX / 2) // 6 bits
# define CONFIDENCE_RAND        (1) // 6 bits

// Stream
# define MAX_PREFETCH_DEGREE    (6)
# define GS_SIZE                (8)
# define GS_OFFSET_MASK         (0x1F)
# define GS_MAX_POS_NEG_COUNT   (64)
# define GS_NUM_REGIONS         (32)
# define GS_WHEN_DENSE          (24)
# define GS_ACC_INC_PF_DEGREE   (75)
# define GS_ACC_DEC_PF_DEGREE   (40)
# define GS_ACC_MED_PF_DEGREE   (60)

// Structs define
typedef struct latency_table {
    uint64_t addr; // Addr
    uint64_t time; // Time where the line is accessed or time between PQ and 
                   // MSHR in case of prefetch
    uint8_t  accessed; // Is the entry accessed by a demand miss
} latency_table_t; // This struct is the latency table

typedef struct history_table {
    uint64_t tag; // IP Tag
    uint64_t time; // Time where the line is accessed
    uint64_t addr; // IP @ accessed
} history_table_t; // This struct is the history table

typedef struct vberti_table {
    uint64_t tag; // IP Tag
    int64_t stride[BERTI_TABLE_STRIDE_SIZE]; // Stride 
    uint64_t conf[BERTI_TABLE_STRIDE_SIZE]; // Confiance
    uint8_t dense_region;
} vberti_table_t; // This struct is the vberti table

typedef struct shadow_cache {
    uint64_t addr; // IP Tag
    uint8_t accessed; // Is this accesed
} shadow_cache_t; // This struct is the vberti table

#ifdef ENABLE_STREAM
typedef struct global_stream  {
    uint64_t region_id;
    uint64_t offset;
    uint8_t vector[GS_NUM_REGIONS];
    uint64_t pos_neg_count;
    uint64_t dense;
    uint64_t trained;
    uint64_t tentative;
    uint64_t dir;
    uint64_t lru;
    uint8_t padding[32];
} global_stream_t;
#endif

// Structs
latency_table_t latencyt[NUM_CPUS][LATENCY_TABLE_SIZE];
history_table_t historyt[NUM_CPUS][HISTORY_TABLE_SIZE];
vberti_table_t vbertit[NUM_CPUS][BERTI_TABLE_SIZE];
shadow_cache_t scache[NUM_CPUS][L1D_SET][L1D_WAY];
#ifdef ENABLE_STREAM
global_stream_t __attribute__((aligned (64))) gstream[NUM_CPUS][GS_SIZE];
#endif

// Hardware Counters
uint64_t good_pf   = 0;
uint64_t late_pf   = 0;
uint64_t wrong_pf  = 0;
uint64_t stream_pf = 0;

#ifdef ENABLE_STREAM
int acc[NUM_CPUS];
int acc_useful[NUM_CPUS];
int acc_filled[NUM_CPUS];
int prefetch_d[NUM_CPUS];
#endif

// Temporal save to know strides used
std::map<uint64_t, uint64_t*> map_addr_conf;
// Auxiliar history global vars
history_table_t *history_pointers[NUM_CPUS];
vberti_table_t *vberti_pointers[NUM_CPUS];

void notify_prefetch(uint64_t addr, uint64_t cycle);

// Auxiliary latency table functions
void latency_table_init(uint32_t cpu);
uint8_t latency_table_add(uint64_t line_addr, uint32_t cpu, uint8_t pf);
uint8_t latency_table_add(uint64_t line_addr, uint32_t cpu, uint8_t pf, 
        uint64_t cycle);
uint64_t latency_table_del(uint64_t line_addr, uint32_t cpu);
uint64_t latency_table_get(uint64_t line_addr, uint32_t cpu);
uint8_t latency_table_is_accessed(uint64_t line_addr, uint32_t cpu);

// Shadow cache
void shadow_cache_init(uint32_t cpu);
uint8_t shadow_cache_add(uint32_t cpu, uint32_t set, uint32_t way, 
        uint64_t line_addr, uint8_t access);
uint8_t shadow_cache_get(uint32_t cpu, uint64_t line_addr);
uint8_t shadow_cache_accessed(uint32_t cpu, uint64_t line_addr);
uint8_t shadow_cache_is_accessed(uint32_t cpu, uint64_t line_addr);

// Auxiliar history table functions
void history_table_init(uint32_t cpu);
void history_table_add(uint64_t tag, uint32_t cpu, uint64_t addr);
uint64_t history_table_get_time(uint32_t cpu, uint64_t addr);
void history_table_get(uint32_t cpu, uint32_t latency, 
        uint64_t on_time_ip[HISTORY_TABLE_SIZE],
        uint64_t on_time_addr[HISTORY_TABLE_SIZE], uint32_t &num_on_time,
        uint64_t cycle);

// Auxiliar history table functions
void vberti_table_init(uint32_t cpu);
void vberti_table_add(uint64_t tag, uint32_t cpu, int64_t stride);
int64_t vberti_table_get(uint64_t tag, uint32_t cpu, int64_t *strides,
        uint64_t **stride_dx);
void vberti_table_confidence(uint64_t tag, uint32_t cpu, int32_t stride, 
        uint8_t increase);
void vberti_table_minus_confidence(uint64_t tag, uint32_t cpu);
uint8_t vberti_table_dense_region(uint64_t tag, uint32_t cpu);
void vberti_table_set_dense_region(uint64_t tag, uint32_t cpu, uint8_t set);
int vberti_table_get_dx(uint64_t tag, uint32_t cpu);
void vberti_table_set_dense_region_dx(int dx, uint32_t cpu, uint8_t set);
uint8_t vberti_table_dense_region_dx(int dx, uint32_t cpu);
void vberti_table_minus_confidence(uint32_t cpu);

#endif
