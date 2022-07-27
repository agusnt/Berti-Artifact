#include "cache.h"

#define maxRRPV 3
uint32_t l2_rrpv[L2C_SET][L2C_WAY];

// initialize replacement state
void CACHE::l2c_initialize_replacement()
{
    cout << "Initialize SRRIP state for L2C" << endl;

    for (int i=0; i<L2C_SET; i++) {
        for (int j=0; j<L2C_WAY; j++) {
            l2_rrpv[i][j] = maxRRPV;
        }
    }
}

// find replacement victim
uint32_t CACHE::l2c_find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
    // look for the maxRRPV line
    while (1)
    {
        for (int i=0; i<L2C_WAY; i++)
            if (l2_rrpv[set][i] == maxRRPV)
                return i;

        for (int i=0; i<L2C_WAY; i++)
            l2_rrpv[set][i]++;
    }

    // WE SHOULD NOT REACH HERE
    assert(0);
    return 0;
}

// called on every cache hit and cache fill
void CACHE::l2c_update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    string TYPE_NAME;
    if (type == LOAD)
        TYPE_NAME = "LOAD";
    else if (type == RFO)
        TYPE_NAME = "RFO";
    else if (type == PREFETCH)
        TYPE_NAME = "PF";
    else if (type == WRITEBACK)
        TYPE_NAME = "WB";
    else if (type == LOAD_TRANSLATION)
	TYPE_NAME = "L_TN";
    else if (type == PREFETCH_TRANSLATION)
	TYPE_NAME = "P_TN";
    else if (type == TRANSLATION_FROM_L1D)
	TYPE_NAME = "TN_L1D";
    else
        assert(0);

    if (hit)
        TYPE_NAME += "_HIT";
    else
        TYPE_NAME += "_MISS";

    if ((type == WRITEBACK) && ip)
        assert(0);

    // uncomment this line to see the L2C accesses
    // cout << "CPU: " << cpu << "  L2C " << setw(9) << TYPE_NAME << " set: " << setw(5) << set << " way: " << setw(2) << way;
    // cout << hex << " paddr: " << setw(12) << paddr << " ip: " << setw(8) << ip << " victim_addr: " << victim_addr << dec << endl;
    
    if (hit)
        l2_rrpv[set][way] = 0;
    else
        l2_rrpv[set][way] = maxRRPV-1;
}

// use this function to print out your own stats at the end of simulation
void CACHE::l2c_replacement_final_stats()
{

}
