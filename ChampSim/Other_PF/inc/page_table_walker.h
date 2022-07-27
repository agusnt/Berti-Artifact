#include "cache.h"

#define PSCL5_SET 1
#define PSCL5_WAY 2

#define PSCL4_SET 1
#define PSCL4_WAY 4

#define PSCL3_SET 2
#define PSCL3_WAY 4

#define PSCL2_SET 4
#define PSCL2_WAY 8

#define PTW_RQ_SIZE 16 //Should be greater than or equal to STLB MSHR Size
#define PTW_WQ_SIZE 4
#define PTW_MSHR_SIZE 1
#define PTW_PQ_SIZE 16

#define MMU_CACHE_LATENCY 1

#define NUM_ENTRIES_PER_PAGE 512

#define IS_PTL1 1
#define IS_PTL2 2
#define IS_PTL3 3
#define IS_PTL4 4
#define IS_PTL5 5

//Virtual Address: 57 bit (9+9+9+9+9+12), rest MSB bits will be used to generate a unique VA per CPU.
//PTL5->PTL4->PTL3->PTL2->PTL1->PFN

class PAGE_TABLE_PAGE
{
public:
    PAGE_TABLE_PAGE* entry[NUM_ENTRIES_PER_PAGE];
    uint64_t next_level_base_addr[NUM_ENTRIES_PER_PAGE]; //This address will not have page offset bits

    PAGE_TABLE_PAGE()
    {
        for (int i = 0; i < NUM_ENTRIES_PER_PAGE; ++i)
        {
            entry[i] = NULL;
            next_level_base_addr[i] = UINT64_MAX;
        }
    };

    /*~PAGE_TABLE_PAGE()
    {
        for (int i = 0; i < NUM_ENTRIES_PER_PAGE; ++i)
            if(next_level_base_addr[i] != UINT64_MAX)
                delete(entry[i]);
    }*/
};

class PAGE_TABLE_WALKER : public MEMORY {
  public:
    const string NAME;
    uint32_t cpu;
    uint8_t cache_type;
    uint32_t LATENCY; //Latency for accessing MMU cache

    uint64_t rq_full;
    uint64_t total_miss_latency;

    uint64_t next_translation_virtual_address = 0xf000000f00000000; //Stores the virtual address from which translation pages will reside

	map <uint64_t, uint64_t> page_table;

    PACKET_QUEUE WQ{NAME + "_WQ", PTW_WQ_SIZE}, // write queue
                 RQ{NAME + "_RQ", PTW_RQ_SIZE}, // read queue
                 MSHR{NAME + "_MSHR", PTW_MSHR_SIZE}, // MSHR
		 PQ{NAME+ "_PQ", PTW_PQ_SIZE};	      // PQ

    CACHE PSCL5{"PSCL5", PSCL5_SET, PSCL5_WAY, PSCL5_SET*PSCL5_WAY, 0, 0, 0, 1}, //Translation from L5->L4
          PSCL4{"PSCL4", PSCL4_SET, PSCL4_WAY, PSCL4_SET*PSCL4_WAY, 0, 0, 0, 1}, //Translation from L5->L3
          PSCL3{"PSCL3", PSCL3_SET, PSCL3_WAY, PSCL3_SET*PSCL3_WAY, 0, 0, 0, 1}, //Translation from L5->L2
          PSCL2{"PSCL2", PSCL2_SET, PSCL2_WAY, PSCL2_SET*PSCL2_WAY, 0, 0, 0, 1}; //Translation from L5->L1

    PAGE_TABLE_PAGE *L5; //CR3 register points to the base of this page.
    uint64_t CR3_addr; //This address will not have page offset bits.
    bool CR3_set;
          
    // constructor
    PAGE_TABLE_WALKER(string v1) : NAME (v1) {

   	assert(LOG2_PAGE_SIZE == 12); //@Vishal: Translation pages are also using this variable, dont change it, or keep it 12 if changing.

        CR3_addr = UINT64_MAX;
        CR3_set = false;
        L5 = NULL;
        rq_full = 0;

        PSCL5.fill_level = 0;
        PSCL4.fill_level = 0;
        PSCL3.fill_level = 0;
        PSCL2.fill_level = 0;
    };

    // destructor
    /*~PAGE_TABLE_WALKER() {
        if(L5 != NULL)
            delete L5;
    };*/

    // functions
    int  add_rq(PACKET *packet),
         add_wq(PACKET *packet),
         add_pq(PACKET *packet);
    int  check_mshr(PACKET *packet);

    void return_data(PACKET *packet),
         operate(),
         increment_WQ_FULL(uint64_t address),
         fill_mmu_cache(CACHE &cache, uint64_t next_level_base_addr, PACKET *packet, uint8_t cache_type),
         add_mshr(PACKET *packet);

    uint32_t get_occupancy(uint8_t queue_type, uint64_t address),
             get_size(uint8_t queue_type, uint64_t address);

    uint64_t get_index(uint64_t address, uint8_t cache_type),
             check_hit(CACHE &cache, uint64_t address, uint8_t type),
             get_offset(uint64_t address, uint8_t pt_level),
             handle_page_fault(PAGE_TABLE_PAGE* page, PACKET *packet, uint8_t pt_level);

	uint64_t map_translation_page(bool *page_swap),
		 map_data_page(uint64_t instr_id, uint64_t full_virtual_address, bool *page_swap);

	uint64_t va_to_pa_ptw(uint8_t cpu, uint64_t instr_id, bool translation_page, uint64_t va, uint64_t unique_vpage, bool *page_swap);

	void write_translation_page(uint64_t next_level_base_addr, PACKET *packet, uint8_t pt_level);

};
