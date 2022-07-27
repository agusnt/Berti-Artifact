#define _BSD_SOURCE

#include <getopt.h>
#include "ooo_cpu.h"
#include "uncore.h"
#include <fstream>
#include <string.h>
#include <sstream>

#define PHASE_SIZE_IN_CYCLES 100000

uint64_t phase_id = 0;

//#define CAPTURE_DYNAMIC_ENERGY_PROFILE
//#define IPCP_PREFETCHER
//#define BINGO_PREFETCHER


#ifdef CAPTURE_DYNAMIC_ENERGY_PROFILE

double cache_data_tag_accesses[7][2000] = {0}, cache_tag_accesses[7][2000] = {0}, dram_accesses[2000] = {0}, interconnect_request[2000] = {0}, interconnect_response[2000] = {0};

#ifdef IPCP_PREFETCHER
double ip_table_write[2000] = {0}, ip_table_read[2000] = {0}, ip_table_tag_write[2000] = {0}, ip_table_tag_read[2000] = {0};
double rstable_write[2000] = {0}, rstable_read[2000] = {0};
double cspt_write[2000] = {0}, cspt_read[2000] = {0};
double rrfilter_read[2000] = {0}, rrfilter_tag_write[2000] = {0};


extern uint64_t ip_table_write_accesses, ip_table_read_accesses, rstable_write_accesses, rstable_read_accesses, cspt_write_accesses, cspt_read_accesses, rrfilter_read_accesses;
extern uint64_t ip_table_tag_write_accesses, ip_table_tag_read_accesses, rrfilter_tag_write_accesses;

#endif

#endif


uint8_t warmup_complete[NUM_CPUS], 
        simulation_complete[NUM_CPUS], 
        all_warmup_complete = 0, 
        all_simulation_complete = 0,
        MAX_INSTR_DESTINATIONS = NUM_INSTR_DESTINATIONS,
        knob_cloudsuite = 0,
        knob_low_bandwidth = 0,
	knob_context_switch = 0;

uint64_t warmup_instructions     = 10000000,
         simulation_instructions = 10000000,
         champsim_seed, occupancy_zero_cycle;

extern int reg_instruction_pointer, reg_stack_pointer, reg_flags; //From src/ooo_cpu.cc


FILE *context_switch_file;
char context_switch_string[1024];
time_t start_time;

struct CS_file{
	int index;
	long cycle;
	uint8_t swap_cpu[2];
} ;

//Data-structure used to store the contents of context-switch file
struct CS_file cs_file[CONTEXT_SWITCH_FILE_SIZE];

// PAGE TABLE
uint32_t PAGE_TABLE_LATENCY = 0, SWAP_LATENCY = 0;
queue <uint64_t > page_queue;
map <uint64_t, uint64_t> page_table, inverse_table, recent_page, unique_cl[NUM_CPUS];
uint64_t previous_ppage, num_adjacent_page, num_cl[NUM_CPUS], allocated_pages, num_page[NUM_CPUS], minor_fault[NUM_CPUS], major_fault[NUM_CPUS];

map <uint64_t, uint64_t> temp_page_table;

void record_roi_stats(uint32_t cpu, CACHE *cache)
{
    for (uint32_t i=0; i<NUM_TYPES; i++) {
        cache->roi_access[cpu][i] = cache->sim_access[cpu][i];
        cache->roi_hit[cpu][i] = cache->sim_hit[cpu][i];
        cache->roi_miss[cpu][i] = cache->sim_miss[cpu][i];
	cache->roi_instr_miss[cpu][i] = cache->sim_instr_miss[cpu][i];
    }
}

#ifdef CAPTURE_DYNAMIC_ENERGY_PROFILE
void print_energy_profile()
{
	for(int i = 0; i < phase_id; i++)
	{
		double total = 0; 
		//caches
		for(int j = 0; j < 7; j++)
		{
			if(i > 0)
			{
				total += cache_data_tag_accesses[j][i] - cache_data_tag_accesses[j][i-1]; 
				total += cache_tag_accesses[j][i] - cache_tag_accesses[j][i-1];
			}
			else
			{
				total += cache_data_tag_accesses[j][i];
				total += cache_tag_accesses[j][i];
			}
		}
		//dram
		if(i > 0)
			total += dram_accesses[i] - dram_accesses[i-1];
		else
			total += dram_accesses[i];
		//interconnect
		if(i > 0)
		{
			total += interconnect_response[i] - interconnect_response[i-1];
		        total += interconnect_request[i] - interconnect_request[i-1];	
		}
		else
		{
			total += interconnect_response[i];
			total += interconnect_request[i];
		}
		//prefetcher
#ifdef IPCP_PREFETCHER
		if(i > 0)
		{
			total += ip_table_write[i] - ip_table_write[i-1];
			total += ip_table_tag_write[i] - ip_table_tag_write[i-1];
			total += ip_table_read[i] - ip_table_read[i-1];
			total += ip_table_tag_read[i] - ip_table_tag_read[i-1];
			total += rstable_write[i] - rstable_write[i-1];
			total += rstable_read[i] - rstable_read[i-1];
			total += cspt_write[i] - cspt_write[i-1];
			total += cspt_read[i] - cspt_read[i-1];
			total += rrfilter_tag_write[i] - rrfilter_tag_write[i-1];
			total += rrfilter_read[i] - rrfilter_read[i-1];

		}
		else
		{
			total += ip_table_write[i];
			total += ip_table_tag_write[i];
			total += ip_table_read[i];
			total += ip_table_tag_read[i];
			total += rstable_write[i];
			total += rstable_read[i];
			total += cspt_write[i];
			total += cspt_read[i];
			total += rrfilter_tag_write[i];
			total += rrfilter_read[i];
		}	
#endif
		cout << "Energy phase " << i << ": " << total << endl;

	}	
}
#endif

void print_roi_stats(uint32_t cpu, CACHE *cache)
{
    uint64_t TOTAL_ACCESS = 0, TOTAL_HIT = 0, TOTAL_MISS = 0, TOTAL_INSTR_MISS = 0;

    for (uint32_t i=0; i<NUM_TYPES; i++) {
        TOTAL_ACCESS += cache->roi_access[cpu][i];
        TOTAL_HIT += cache->roi_hit[cpu][i];
        TOTAL_MISS += cache->roi_miss[cpu][i];
	TOTAL_INSTR_MISS += cache->roi_instr_miss[cpu][i];
    }

    uint64_t num_instrs = ooo_cpu[cpu].finish_sim_instr;

    if(TOTAL_ACCESS != 0) 
	{
			cout << cache->NAME;
			cout << " TOTAL     ACCESS: " << setw(10) << TOTAL_ACCESS << "  HIT: " << setw(10) << TOTAL_HIT <<  "  MISS: " << setw(10) << TOTAL_MISS << "  HIT %: " << setw(10) << ((double)TOTAL_HIT*100/TOTAL_ACCESS) << "  MISS %: " << setw(10) << ((double)TOTAL_MISS*100/TOTAL_ACCESS) << "   MPKI: " <<  ((double)TOTAL_MISS*1000/num_instrs) << endl;
	}
	
	if(cache->cache_type == IS_BTB)
	{
		string type[] = { " BRANCH_DIRECT_JUMP	ACCESS: ", " BRANCH_INDIRECT	ACCESS: "," BRANCH_CONDITIONAL	ACCESS: "," BRANCH_DIRECT_CALL	ACCESS: ", " BRANCH_INDIRECT_CALL	ACCESS: ", " BRANCH_RETURN	ACCESS: ", " BRANCH_OTHER ACCESS: "};
		for(int i = 0; i < 7; i++)
		{
			cout << cache->NAME;
    		cout << type[i] << setw(10) << cache->roi_access[cpu][i] << "  HIT: " << setw(10) << cache->roi_hit[cpu][i] << "  MISS: " << setw(10) << cache->roi_miss[cpu][i] << endl;
		}
		cout << endl;
		return;
	}

	if(cache->roi_access[cpu][0])
	{
			cout << cache->NAME;
			cout << " LOAD      ACCESS: " << setw(10) << cache->roi_access[cpu][0] << "  HIT: " << setw(10) << cache->roi_hit[cpu][0] << "  MISS: " << setw(10) << cache->roi_miss[cpu][0] << "  HIT %: " << setw(10) << ((double)cache->roi_hit[cpu][0]*100/cache->roi_access[cpu][0]) << "  MISS %: " << setw(10) << ((double)cache->roi_miss[cpu][0]*100/cache->roi_access[cpu][0]) << "   MPKI: " <<  ((double)cache->roi_miss[cpu][0]*1000/num_instrs) << endl; // << " T_ACCESS: " << setw(10) << cache->ACCESS[0] << " T_HIT: " << setw(10) << cache->HIT[0] << " T_MISS: " << setw(10) << cache->MISS[0] << " T_MSHR_MERGED: " << cache->MSHR_MERGED[0] << endl; //@Vishal: MSHR merged will give correct result for 1 core, multi_core will give whole sim


		if(cache->cache_type == IS_L2C)
		{
			//Neelu: Instruction and Data loads. 
			cout << cache->NAME << " DATA LOAD MPKI: " << ((double)(cache->roi_miss[cpu][0]-cache->roi_instr_miss[cpu][0])*1000/num_instrs) << endl;
			cout << cache->NAME << " INSTRUCTION LOAD MPKI: " << ((double)(cache->roi_instr_miss[cpu][0])*1000/num_instrs) << endl;
		}
	}

	if(cache->roi_access[cpu][1])
    {
			cout << cache->NAME;
			cout << " RFO       ACCESS: " << setw(10) << cache->roi_access[cpu][1] << "  HIT: " << setw(10) << cache->roi_hit[cpu][1] << "  MISS: " << setw(10) << cache->roi_miss[cpu][1] << "  HIT %: " << setw(10) << ((double)cache->roi_hit[cpu][1]*100/cache->roi_access[cpu][1]) << "  MISS %: " << setw(10) << ((double)cache->roi_miss[cpu][1]*100/cache->roi_access[cpu][1]) << "   MPKI: " <<  ((double)cache->roi_miss[cpu][1]*1000/num_instrs) << endl; //<< " T_ACCESS: " << setw(10) << cache->ACCESS[1] << " T_HIT: " << setw(10) << cache->HIT[1] << " T_MISS: " << setw(10) << cache->MISS[1] << " T_MSHR_MERGED: " << cache->MSHR_MERGED[1] << endl;
	}

	if(cache->roi_access[cpu][2])
    {
			cout << cache->NAME;
			cout << " PREFETCH  ACCESS: " << setw(10) << cache->roi_access[cpu][2] << "  HIT: " << setw(10) << cache->roi_hit[cpu][2] << "  MISS: " << setw(10) << cache->roi_miss[cpu][2] << "  HIT %: " << setw(10) << ((double)cache->roi_hit[cpu][2]*100/cache->roi_access[cpu][2]) << "  MISS %: " << setw(10) << ((double)cache->roi_miss[cpu][2]*100/cache->roi_access[cpu][2]) << "   MPKI: " <<  ((double)cache->roi_miss[cpu][2]*1000/num_instrs) << endl; //<< " T_ACCESS: " << setw(10) << cache->ACCESS[2] << " T_HIT: " << setw(10) << cache->HIT[2] << " T_MISS: " << setw(10) << cache->MISS[2] << " T_MSHR_MERGED: " << cache->MSHR_MERGED[2] << endl;

            cout << "AGUS PREFETCH L1 MISS: " << cache->pf_miss_l1 << endl;

			if(cache->cache_type == IS_L2C)
			{
				//Neelu: Instruction and Data loads. 
				cout << cache->NAME << " DATA PREFETCH MPKI: " << ((double)(cache->roi_miss[cpu][2]-cache->roi_instr_miss[cpu][2])*1000/num_instrs) << endl;
				cout << cache->NAME << " INSTRUCTION PREFETCH MPKI: " << ((double)(cache->roi_instr_miss[cpu][2])*1000/num_instrs) << endl;
			}
    }

	if(cache->roi_access[cpu][3])
    {
			cout << cache->NAME;
			cout << " WRITEBACK ACCESS: " << setw(10) << cache->roi_access[cpu][3] << "  HIT: " << setw(10) << cache->roi_hit[cpu][3] << "  MISS: " << setw(10) << cache->roi_miss[cpu][3] << "  HIT %: " << setw(10) << ((double)cache->roi_hit[cpu][3]*100/cache->roi_access[cpu][3]) << "  MISS %: " << setw(10) << ((double)cache->roi_miss[cpu][3]*100/cache->roi_access[cpu][3]) << "   MPKI: " <<  ((double)cache->roi_miss[cpu][3]*1000/num_instrs) << endl; //<< " T_ACCESS: " << setw(10) << cache->ACCESS[3] << " T_HIT: " << setw(10) << cache->HIT[3] << " T_MISS: " << setw(10) << cache->MISS[3] <<" T_MSHR_MERGED: " << cache->MSHR_MERGED[3] << endl;
	}
    
	if(cache->roi_access[cpu][4])
    {
			cout << cache->NAME;
			cout << " LOAD TRANSLATION ACCESS: " << setw(10) << cache->roi_access[cpu][4] << "  HIT: " << setw(10) << cache->roi_hit[cpu][4] << "  MISS: " << setw(10) << cache->roi_miss[cpu][4] << "  HIT %: " << setw(10) << ((double)cache->roi_hit[cpu][4]*100/cache->roi_access[cpu][4]) << "  MISS %: " << setw(10) << ((double)cache->roi_miss[cpu][4]*100/cache->roi_access[cpu][4]) << "   MPKI: " <<  ((double)cache->roi_miss[cpu][4]*1000/num_instrs) << endl; //<< " T_ACCESS: " << setw(10) << cache->ACCESS[4] << " T_HIT: " << setw(10) << cache->HIT[4] << " T_MISS: " << setw(10) << cache->MISS[4] <<" T_MSHR_MERGED: " << cache->MSHR_MERGED[4] << endl;
	}

	if(cache->roi_access[cpu][5])
	{
			cout << cache->NAME;
			cout << " PREFETCH TRANSLATION ACCESS: " << setw(10) << cache->roi_access[cpu][5] << "  HIT: " << setw(10) << cache->roi_hit[cpu][5] << "  MISS: " << setw(10) << cache->roi_miss[cpu][5] << "  HIT %: " << setw(10) << ((double)cache->roi_hit[cpu][5]*100/cache->roi_access[cpu][5]) << "  MISS %: " << setw(10) << ((double)cache->roi_miss[cpu][5]*100/cache->roi_access[cpu][5]) << "   MPKI: " <<  ((double)cache->roi_miss[cpu][5]*1000/num_instrs) << endl; //<< " T_ACCESS: " << setw(10) << cache->ACCESS[4] << " T_HIT: " << setw(10) << cache->HIT[4] << " T_MISS: " << setw(10) << cache->MISS[4] <<" T_MSHR_MERGED: " << cache->MSHR_MERGED[4] << endl;
	}
	if(cache->roi_access[cpu][6])
    {
			cout << cache->NAME;
			cout << " TRANSLATION FROM L1D PREFETCHER ACCESS: " << setw(10) << cache->roi_access[cpu][6] << "  HIT: " << setw(10) << cache->roi_hit[cpu][6] << "  MISS: " << setw(10) << cache->roi_miss[cpu][6] << "  HIT %: " << setw(10) << ((double)cache->roi_hit[cpu][6]*100/cache->roi_access[cpu][6]) << "  MISS %: " << setw(10) << ((double)cache->roi_miss[cpu][6]*100/cache->roi_access[cpu][6]) << "   MPKI: " <<  ((double)cache->roi_miss[cpu][6]*1000/num_instrs) << endl; //<< " T_ACCESS: " << setw(10) << cache->ACCESS[4] << " T_HIT: " << setw(10) << cache->HIT[4] << " T_MISS: " << setw(10) << cache->MISS[4] <<" T_MSHR_MERGED: " << cache->MSHR_MERGED[4] << endl;
	}
	//if(cache->pf_requested)
	//{
			cout << cache->NAME;
			cout << " PREFETCH  REQUESTED: " << setw(10) << cache->pf_requested << "  ISSUED: " << setw(10) << cache->pf_issued;
			cout << "  USEFUL: " << setw(10) << cache->pf_useful << "  USELESS: " << setw(10) << cache->pf_useless << endl;

			//cout << cache->NAME << " PF LOWER LEVEL TEST: " << cache->pf_lower_level_test << endl;	
			 cout << cache->NAME;
 		         cout << " USEFUL LOAD PREFETCHES: " << setw(10) << cache->pf_useful << " PREFETCH ISSUED TO LOWER LEVEL: " << setw(10) << cache->pf_lower_level << "  ACCURACY: " <<      ((double)cache->pf_useful*100/cache->pf_lower_level) << endl;
  		         cout << cache->NAME << " TIMELY PREFETCHES: " << setw(10) << cache->pf_useful << " LATE PREFETCHES: " << cache->pf_late << " DROPPED PREFETCHES: " << cache->pf_dropped <<  endl;
			 //cout << cache->NAME << " PREFETCHES SAME FILL-ORIGIN LEVEL: " << cache->pf_same_fill_level << " DIFFERENT FILL-ORIGIN LEVEL: " << cache->pf_lower_fill_level << endl;

	//}
			cout << cache->NAME << " PREFETCHES SAME FILL-ORIGIN LEVEL: " << cache->pf_same_fill_level << " DIFFERENT FILL-ORIGIN LEVEL: " << cache->pf_lower_fill_level << endl;

    if(cache->cache_type == IS_PSCL5 || cache->cache_type == IS_PSCL4 || cache->cache_type == IS_PSCL3 || cache->cache_type == IS_PSCL2)
	{
	}
	else	
	{
			cout << cache->NAME;
			cout << " AVERAGE MISS LATENCY: " << (1.0*(cache->total_miss_latency))/TOTAL_MISS << " cycles" << endl;
	}

    //@Vishal: Will work only for 1 core, for multi-core this will give sim_result not roi_result
	if(cache->RQ.ACCESS)
    	cout << cache->NAME << " RQ	ACCESS: "<< setw(10) << cache->RQ.ACCESS << "	FORWARD: " << setw(10) << cache->RQ.FORWARD << "	MERGED: " << setw(10) << cache->RQ.MERGED << "	TO_CACHE: " << setw(10) << cache->RQ.TO_CACHE << endl;
	
	if(cache->WQ.ACCESS)
    	cout << cache->NAME << " WQ	ACCESS: "<< setw(10) << cache->WQ.ACCESS << "	FORWARD: " << setw(10) << cache->WQ.FORWARD << "	MERGED: " << setw(10) << cache->WQ.MERGED << "	TO_CACHE: " << setw(10) << cache->WQ.TO_CACHE << endl;
	
	if(cache->PQ.ACCESS)
    	cout << cache->NAME << " PQ	ACCESS: "<< setw(10) << cache->PQ.ACCESS << "	FORWARD: " << setw(10) << cache->PQ.FORWARD << "	MERGED: " << setw(10) << cache->PQ.MERGED << "	TO_CACHE: " << setw(10) << cache->PQ.TO_CACHE << endl;
	
	cout << endl;



	//Neelu: addition ideal spatial region stats
	if(cache->cache_type == IS_L1D)
	{
		cout << cache->NAME<<" UNIQUE REGIONS ACCESSED: "<<cache->unique_region_count<<endl;
		cout<< cache->NAME<<" REGIONS CONFLICTS: "<<cache->region_conflicts<<endl;
		cout<< cache->NAME<<" Cross Page Prefetch Requests: "<<cache->cross_page_prefetch_requests<<endl;
		cout<< cache->NAME<<" Same Page Prefetch Requests: "<<cache->same_page_prefetch_requests<<endl;

		cout<< cache->NAME<<" ROI Sum of L1D PQ occupancy: " << cache->sum_pq_occupancy << endl;
		cout<< cache->NAME<<" PREFETCHES PUSHED FROM L2C: " << cache->pf_pushed_from_L2C << endl;
	}

	if(cache->cache_type == IS_STLB)
	{
		cout << cache->NAME << " Hit, L1D data hit: " << cache->l1d_data_hit << endl;
		cout << cache->NAME << " Hit, L2C data hit: " << cache->l2c_data_hit << endl;
		cout << cache->NAME << " Hit, LLC data hit: " << cache->llc_data_hit << endl;
		cout << cache->NAME << " Hit, LLC data miss: " << cache->llc_data_miss << endl;
		cout << cache->NAME << " STLB hints to L2: " << cache->stlb_hints_to_l2 << endl;
	}

	if(cache->cache_type == IS_L2C)
	{
		cout << cache->NAME << " Instructions Evicting Data " << cache->instr_evicting_data << endl; 
		cout << cache->NAME << " Translations Evicting Data " << cache->transl_evicting_data << endl;
		cout << cache->NAME << " Data Evicting Data " << cache->data_evicting_data << endl;
		cout << cache->NAME << " Instructions Evicting Instructions " << cache->instr_evicting_instr << endl;
		cout << cache->NAME << " Translations Evicting Instructions " << cache->transl_evicting_instr << endl;
		cout << cache->NAME << " Data Evicting Instructions " << cache->data_evicting_instr << endl;
		cout << cache->NAME << " Instructions Evicting Translations " << cache->instr_evicting_transl << endl;
		cout << cache->NAME << " Translations Evicting Translations " << cache->transl_evicting_transl << endl;
		cout << cache->NAME << " Data Evicting Translations " << cache->data_evicting_transl << endl;

	}

	if(cache->cache_type == IS_L2C)
		cout << cache->NAME << " Dense regions hint from L2: " << cache->getting_hint_from_l2 << endl;
	if(cache->cache_type == IS_LLC)
		cout << cache->NAME << " Dense regions hint to LLC: " << cache->sending_hint_to_llc << endl;

	static int flag = 0;
	#ifdef PUSH_DTLB_PB
        if(cache->cache_type == IS_DTLB_PB && flag == 0)
        {
                flag = 1;
                cout <<"DTLB_PB PREFETCH USEFUL - " << cache->pf_useful << " USELESS PREFETCHES: " << cache->pf_useless << endl;
        }
        #endif

#ifdef CAPTURE_DYNAMIC_ENERGY_PROFILE
	//print_energy_profile();
#endif

}


void print_sim_stats(uint32_t cpu, CACHE *cache)
{
    uint64_t TOTAL_ACCESS = 0, TOTAL_HIT = 0, TOTAL_MISS = 0;

    for (uint32_t i=0; i<NUM_TYPES; i++) {
        TOTAL_ACCESS += cache->sim_access[cpu][i];
        TOTAL_HIT += cache->sim_hit[cpu][i];
        TOTAL_MISS += cache->sim_miss[cpu][i];
    }

    uint64_t num_instrs = ooo_cpu[cpu].num_retired - ooo_cpu[cpu].begin_sim_instr;

    cout << cache->NAME;
    cout << " TOTAL     ACCESS: " << setw(10) << TOTAL_ACCESS << "  HIT: " << setw(10) << TOTAL_HIT <<  "  MISS: " << setw(10) << TOTAL_MISS << "  HIT %: " << setw(10) << ((double)TOTAL_HIT*100/TOTAL_ACCESS) << "  MISS %: " << setw(10) << ((double)TOTAL_MISS*100/TOTAL_ACCESS) << "   MPKI: " <<  ((double)TOTAL_MISS*1000/num_instrs) << endl;

    cout << cache->NAME;
    cout << " LOAD      ACCESS: " << setw(10) << cache->sim_access[cpu][0] << "  HIT: " << setw(10) << cache->sim_hit[cpu][0] << "  MISS: " << setw(10) << cache->sim_miss[cpu][0] << "  HIT %: " << setw(10) << ((double)cache->sim_hit[cpu][0]*100/cache->sim_access[cpu][0]) << "  MISS %: " << setw(10) << ((double)cache->sim_miss[cpu][0]*100/cache->sim_access[cpu][0]) << "   MPKI: " <<  ((double)cache->sim_miss[cpu][0]*1000/num_instrs) << endl;

    cout << cache->NAME;
    cout << " RFO       ACCESS: " << setw(10) << cache->sim_access[cpu][1] << "  HIT: " << setw(10) << cache->sim_hit[cpu][1] << "  MISS: " << setw(10) << cache->sim_miss[cpu][1] << "  HIT %: " << setw(10) << ((double)cache->sim_hit[cpu][1]*100/cache->sim_access[cpu][1]) << "  MISS %: " << setw(10) << ((double)cache->sim_miss[cpu][1]*100/cache->sim_access[cpu][1]) << "   MPKI: " <<  ((double)cache->sim_miss[cpu][1]*1000/num_instrs) << endl;

    cout << cache->NAME;
    cout << " PREFETCH  ACCESS: " << setw(10) << cache->sim_access[cpu][2] << "  HIT: " << setw(10) << cache->sim_hit[cpu][2] << "  MISS: " << setw(10) << cache->sim_miss[cpu][2] << "  HIT %: " << setw(10) << ((double)cache->sim_hit[cpu][2]*100/cache->sim_access[cpu][2]) << "  MISS %: " << setw(10) << ((double)cache->sim_miss[cpu][2]*100/cache->sim_access[cpu][2]) << "   MPKI: " <<  ((double)cache->sim_miss[cpu][2]*1000/num_instrs) << endl;

    cout << cache->NAME;
    cout << " WRITEBACK ACCESS: " << setw(10) << cache->sim_access[cpu][3] << "  HIT: " << setw(10) << cache->sim_hit[cpu][3] << "  MISS: " << setw(10) << cache->sim_miss[cpu][3] << "  HIT %: " << setw(10) << ((double)cache->sim_hit[cpu][3]*100/cache->sim_access[cpu][3]) << "  MISS %: " << setw(10) << ((double)cache->sim_miss[cpu][3]*100/cache->sim_access[cpu][3]) << "   MPKI: " <<  ((double)cache->sim_miss[cpu][3]*1000/num_instrs) << endl;
}

void print_branch_stats()
{
    for (uint32_t i=0; i<NUM_CPUS; i++) {
        cout << endl << "CPU " << i << " Branch Prediction Accuracy: ";
        cout << (100.0*(ooo_cpu[i].num_branch - ooo_cpu[i].branch_mispredictions)) / ooo_cpu[i].num_branch;
        cout << "% MPKI: " << (1000.0*ooo_cpu[i].branch_mispredictions)/(ooo_cpu[i].num_retired - ooo_cpu[i].warmup_instructions);
	cout << " Average ROB Occupancy at Mispredict: " << (1.0*ooo_cpu[i].total_rob_occupancy_at_branch_mispredict)/ooo_cpu[i].branch_mispredictions << endl;

	cout << "Branch types" << endl;
	cout << "NOT_BRANCH: " << ooo_cpu[i].total_branch_types[0] << " " << (100.0*ooo_cpu[i].total_branch_types[0])/(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) << "%" << endl;
	cout << "BRANCH_DIRECT_JUMP: " << ooo_cpu[i].total_branch_types[1] << " " << (100.0*ooo_cpu[i].total_branch_types[1])/(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) << "%" << endl;
	cout << "BRANCH_INDIRECT: " << ooo_cpu[i].total_branch_types[2] << " " << (100.0*ooo_cpu[i].total_branch_types[2])/(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) << "%" << endl;
	cout << "BRANCH_CONDITIONAL: " << ooo_cpu[i].total_branch_types[3] << " " << (100.0*ooo_cpu[i].total_branch_types[3])/(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) << "%" << endl;
	cout << "BRANCH_DIRECT_CALL: " << ooo_cpu[i].total_branch_types[4] << " " << (100.0*ooo_cpu[i].total_branch_types[4])/(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) << "%" << endl;
	cout << "BRANCH_INDIRECT_CALL: " << ooo_cpu[i].total_branch_types[5] << " " << (100.0*ooo_cpu[i].total_branch_types[5])/(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) << "%" << endl;
	cout << "BRANCH_RETURN: " << ooo_cpu[i].total_branch_types[6] << " " << (100.0*ooo_cpu[i].total_branch_types[6])/(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) << "%" << endl;
	cout << "BRANCH_OTHER: " << ooo_cpu[i].total_branch_types[7] << " " << (100.0*ooo_cpu[i].total_branch_types[7])/(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) << "%" << endl << endl;

    }
}

void print_dram_stats()
{
    cout << endl;
    cout << "DRAM Statistics" << endl;
    for (uint32_t i=0; i<DRAM_CHANNELS; i++) {
        cout << " CHANNEL " << i << endl;
        cout << " RQ ROW_BUFFER_HIT: " << setw(10) << uncore.DRAM.RQ[i].ROW_BUFFER_HIT << "  ROW_BUFFER_MISS: " << setw(10) << uncore.DRAM.RQ[i].ROW_BUFFER_MISS << endl;
        cout << " DBUS_CONGESTED: " << setw(10) << uncore.DRAM.dbus_congested[NUM_TYPES][NUM_TYPES] << endl; 
        cout << " WQ ROW_BUFFER_HIT: " << setw(10) << uncore.DRAM.WQ[i].ROW_BUFFER_HIT << "  ROW_BUFFER_MISS: " << setw(10) << uncore.DRAM.WQ[i].ROW_BUFFER_MISS;
        cout << "  FULL: " << setw(10) << uncore.DRAM.WQ[i].FULL << endl; 
        cout << endl;
    }

    uint64_t total_congested_cycle = 0;
    for (uint32_t i=0; i<DRAM_CHANNELS; i++)
        total_congested_cycle += uncore.DRAM.dbus_cycle_congested[i];
    if (uncore.DRAM.dbus_congested[NUM_TYPES][NUM_TYPES])
        cout << " AVG_CONGESTED_CYCLE: " << (total_congested_cycle / uncore.DRAM.dbus_congested[NUM_TYPES][NUM_TYPES]) << endl;
    else
        cout << " AVG_CONGESTED_CYCLE: -" << endl;

    uncore.DRAM.print_DRAM_busy_stats();
}

void reset_cache_stats(uint32_t cpu, CACHE *cache)
{
    for (uint32_t i=0; i<NUM_TYPES; i++) {
        cache->ACCESS[i] = 0;
        cache->HIT[i] = 0;
        cache->MISS[i] = 0;
        cache->MSHR_MERGED[i] = 0;
        cache->STALL[i] = 0;

        cache->sim_access[cpu][i] = 0;
        cache->sim_hit[cpu][i] = 0;
        cache->sim_miss[cpu][i] = 0;
	cache->sim_instr_miss[cpu][i] = 0;
    }

    cache->total_miss_latency = 0;

    cache->RQ.ACCESS = 0;
    cache->RQ.MERGED = 0;
    cache->RQ.TO_CACHE = 0;
    cache->RQ.FORWARD = 0;
    cache->RQ.FULL = 0;
    
    cache->WQ.ACCESS = 0;
    cache->WQ.MERGED = 0;
    cache->WQ.TO_CACHE = 0;
    cache->WQ.FORWARD = 0;
    cache->WQ.FULL = 0;
    
    //reset_prefetch_stats
    cache->pf_requested = 0;
    cache->pf_issued = 0;
    cache->pf_useful = 0;
    cache->pf_useless = 0;
    cache->pf_fill = 0;
    cache->pf_late = 0;
    cache->pf_lower_level = 0;
    cache->pf_lower_level_test = 0;
    cache->pf_same_fill_level = 0;
    cache->pf_lower_fill_level = 0;
    cache->pf_dropped = 0;
    cache->sum_pq_occupancy = 0;
    cache->pf_pushed_from_L2C = 0;
    cache->l1d_data_hit = 0;
    cache->l2c_data_hit = 0;
    cache->llc_data_hit = 0;
    cache->llc_data_miss = 0;
    cache->getting_hint_from_l2 = 0;
    cache->sending_hint_to_llc = 0;
    cache->stlb_hints_to_l2 = 0;
    cache->instr_evicting_data = 0;
    cache->data_evicting_instr = 0;
    cache->data_evicting_transl = 0;
    cache->transl_evicting_data = 0;
    cache->instr_evicting_transl = 0;
    cache->transl_evicting_instr = 0;
    cache->data_evicting_data = 0;                      
    cache->instr_evicting_instr = 0;                    
    cache->transl_evicting_transl = 0;


    cache->PQ.ACCESS = 0;
    cache->PQ.MERGED = 0;
    cache->PQ.TO_CACHE = 0;
    cache->PQ.FORWARD = 0;
    cache->PQ.FULL = 0;
}

void finish_warmup()
{
    uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time),
             elapsed_minute = elapsed_second / 60,
             elapsed_hour = elapsed_minute / 60;
    elapsed_minute -= elapsed_hour*60;
    elapsed_second -= (elapsed_hour*3600 + elapsed_minute*60);

    // reset core latency
    SCHEDULING_LATENCY = 6;
    EXEC_LATENCY = 1;
    DECODE_LATENCY = 2;
    PAGE_TABLE_LATENCY = 100;
    SWAP_LATENCY = 100000;

    cout << endl;
    for (uint32_t i=0; i<NUM_CPUS; i++) {
        cout << "Warmup complete CPU " << i << " instructions: " << ooo_cpu[i].num_retired << " cycles: " << current_core_cycle[i];
        cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << endl;

        ooo_cpu[i].begin_sim_cycle = current_core_cycle[i]; 
        ooo_cpu[i].begin_sim_instr = ooo_cpu[i].num_retired;

	//Neelu: Setting current_epoch_all_ip_prefetch.
	//ooo_cpu[i].current_epoch_all_ip_prefetch = 1;

        // reset branch stats
        ooo_cpu[i].num_branch = 0;
        ooo_cpu[i].branch_mispredictions = 0;
	ooo_cpu[i].total_rob_occupancy_at_branch_mispredict = 0;

	for(uint32_t j=0; j<8; j++)
	{
		ooo_cpu[i].total_branch_types[j] = 0;
	}


	//@Vishal: reset cpu stats
	ooo_cpu[i].sim_RAW_hits = 0;
	ooo_cpu[i].sim_load_gen = 0;
	ooo_cpu[i].sim_load_sent = 0;
	ooo_cpu[i].sim_store_gen = 0;
	ooo_cpu[i].sim_store_sent = 0;


	reset_cache_stats(i, &ooo_cpu[i].ITLB);
        reset_cache_stats(i, &ooo_cpu[i].DTLB);
        reset_cache_stats(i, &ooo_cpu[i].STLB);
        reset_cache_stats(i, &ooo_cpu[i].L1I);
        reset_cache_stats(i, &ooo_cpu[i].L1D);
        reset_cache_stats(i, &ooo_cpu[i].L2C);
        reset_cache_stats(i, &uncore.LLC);
		reset_cache_stats(i, &ooo_cpu[i].BTB);

	//@Vishal: Reset MMU cache stats
	reset_cache_stats(i, &ooo_cpu[i].PTW.PSCL5);
	reset_cache_stats(i, &ooo_cpu[i].PTW.PSCL4);
	reset_cache_stats(i, &ooo_cpu[i].PTW.PSCL3);
	reset_cache_stats(i, &ooo_cpu[i].PTW.PSCL2);
	#ifdef PUSH_DTLB_PB
        reset_cache_stats(i, &ooo_cpu[i].DTLB_PB);
        #endif
    }
    cout << endl;

    // reset DRAM stats
    for (uint32_t i=0; i<DRAM_CHANNELS; i++) {
        uncore.DRAM.RQ[i].ROW_BUFFER_HIT = 0;
        uncore.DRAM.RQ[i].ROW_BUFFER_MISS = 0;
        uncore.DRAM.WQ[i].ROW_BUFFER_HIT = 0;
        uncore.DRAM.WQ[i].ROW_BUFFER_MISS = 0;
    }

    // set actual cache latency
    for (uint32_t i=0; i<NUM_CPUS; i++) {
        ooo_cpu[i].ITLB.LATENCY = ITLB_LATENCY;
        ooo_cpu[i].DTLB.LATENCY = DTLB_LATENCY;
        ooo_cpu[i].STLB.LATENCY = STLB_LATENCY;
        ooo_cpu[i].L1I.LATENCY  = L1I_LATENCY;
        ooo_cpu[i].L1D.LATENCY  = L1D_LATENCY;
        ooo_cpu[i].L2C.LATENCY  = L2C_LATENCY;
    }
    uncore.LLC.LATENCY = LLC_LATENCY;
}

void print_deadlock(uint32_t i)
{
    cout << "DEADLOCK! CPU " << i << " instr_id: " << ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].instr_id;
    cout << " translated: " << +ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].translated;
    cout << " fetched: " << +ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].fetched;
    cout << " scheduled: " << +ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].scheduled;
    cout << " executed: " << +ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].executed;
    cout << " is_memory: " << +ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].is_memory;
    cout << " event: " << ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].event_cycle;
    cout << " current: " << current_core_cycle[i] << endl;

    // print LQ entry
    cout << endl << "Load Queue Entry" << endl;
    for (uint32_t j=0; j<LQ_SIZE; j++) {
        cout << "[LQ] entry: " << j << " instr_id: " << ooo_cpu[i].LQ.entry[j].instr_id << " address: " << hex << ooo_cpu[i].LQ.entry[j].physical_address << dec << " translated: " << +ooo_cpu[i].LQ.entry[j].translated << " fetched: " << +ooo_cpu[i].LQ.entry[i].fetched << endl;
    }

    // print SQ entry
    cout << endl << "Store Queue Entry" << endl;
    for (uint32_t j=0; j<SQ_SIZE; j++) {
        cout << "[SQ] entry: " << j << " instr_id: " << ooo_cpu[i].SQ.entry[j].instr_id << " address: " << hex << ooo_cpu[i].SQ.entry[j].physical_address << dec << " translated: " << +ooo_cpu[i].SQ.entry[j].translated << " fetched: " << +ooo_cpu[i].SQ.entry[i].fetched << endl;
    }

    // print L1D MSHR entry
    PACKET_QUEUE *queue;
    queue = &ooo_cpu[i].L1D.MSHR;
    cout << endl << queue->NAME << " Entry" << endl;
    for (uint32_t j=0; j<queue->SIZE; j++) {
        cout << "[" << queue->NAME << "] entry: " << j << " instr_id: " << queue->entry[j].instr_id << " rob_index: " << queue->entry[j].rob_index;
        cout << " address: " << hex << queue->entry[j].address << " full_addr: " << queue->entry[j].full_addr << dec << " type: " << +queue->entry[j].type;
        cout << " fill_level: " << queue->entry[j].fill_level << " lq_index: " << queue->entry[j].lq_index << " sq_index: " << queue->entry[j].sq_index << endl; 
    }

    queue = &ooo_cpu[i].ITLB.RQ;
    cout << endl << queue->NAME << " Entry " << endl;
    for (uint32_t j=0; j<queue->SIZE; j++) {
        cout << "[" << queue->NAME << "] entry: " << j << " instr_id: " << queue->entry[j].instr_id << " prefetch_id: " << queue->entry[j].prefetch_id << " rob_index: " << queue->entry[j].rob_index;
        cout << " address: " << hex << queue->entry[j].address << " full_addr: " << queue->entry[j].full_addr << dec << " type: " << +queue->entry[j].type;
        cout << " fill_level: " << queue->entry[j].fill_level << " lq_index: " << queue->entry[j].lq_index << " sq_index: " << queue->entry[j].sq_index << endl; 
    }
    queue = &ooo_cpu[i].ITLB.PQ;
    cout << endl << queue->NAME << " Entry " << endl;
    for (uint32_t j=0; j<queue->SIZE; j++) {
        cout << "[" << queue->NAME << "] entry: " << j << " instr_id: " << queue->entry[j].instr_id << " prefetch_id: " << queue->entry[j].prefetch_id << " rob_index: " << queue->entry[j].rob_index;
        cout << " address: " << hex << queue->entry[j].address << " full_addr: " << queue->entry[j].full_addr << dec << " type: " << +queue->entry[j].type;
        cout << " fill_level: " << queue->entry[j].fill_level << " lq_index: " << queue->entry[j].lq_index << " sq_index: " << queue->entry[j].sq_index << endl; 
    }
    queue = &ooo_cpu[i].ITLB.MSHR;
    cout << endl << queue->NAME << " Entry " << endl;
    for (uint32_t j=0; j<queue->SIZE; j++) {
        cout << "[" << queue->NAME << "] entry: " << j << " instr_id: " << queue->entry[j].instr_id << " prefetch_id: " << queue->entry[j].prefetch_id << " rob_index: " << queue->entry[j].rob_index;
        cout << " address: " << hex << queue->entry[j].address << " full_addr: " << queue->entry[j].full_addr << dec << " type: " << +queue->entry[j].type;
        cout << " fill_level: " << queue->entry[j].fill_level << " lq_index: " << queue->entry[j].lq_index << " sq_index: " << queue->entry[j].sq_index << endl; 
    }
    queue = &ooo_cpu[i].DTLB.RQ;
    cout << endl << queue->NAME << " Entry " << endl;
    for (uint32_t j=0; j<queue->SIZE; j++) {
        cout << "[" << queue->NAME << "] entry: " << j << " instr_id: " << queue->entry[j].instr_id << " prefetch_id: " << queue->entry[j].prefetch_id << " rob_index: " << queue->entry[j].rob_index;
        cout << " address: " << hex << queue->entry[j].address << " full_addr: " << queue->entry[j].full_addr << dec << " type: " << +queue->entry[j].type;
        cout << " fill_level: " << queue->entry[j].fill_level << " lq_index: " << queue->entry[j].lq_index << " sq_index: " << queue->entry[j].sq_index << endl; 
    }
    queue = &ooo_cpu[i].DTLB.PQ;
    cout << endl << queue->NAME << " Entry " << endl;
    for (uint32_t j=0; j<queue->SIZE; j++) {
        cout << "[" << queue->NAME << "] entry: " << j << " instr_id: " << queue->entry[j].instr_id << " prefetch_id: " << queue->entry[j].prefetch_id << " rob_index: " << queue->entry[j].rob_index;
        cout << " address: " << hex << queue->entry[j].address << " full_addr: " << queue->entry[j].full_addr << dec << " type: " << +queue->entry[j].type;
        cout << " fill_level: " << queue->entry[j].fill_level << " lq_index: " << queue->entry[j].lq_index << " sq_index: " << queue->entry[j].sq_index << endl; 
    }
    queue = &ooo_cpu[i].DTLB.MSHR;
    cout << endl << queue->NAME << " Entry " << endl;
    for (uint32_t j=0; j<queue->SIZE; j++) {
        cout << "[" << queue->NAME << "] entry: " << j << " instr_id: " << queue->entry[j].instr_id << " rob_index: " << queue->entry[j].rob_index;
        cout << " address: " << hex << queue->entry[j].address << " full_addr: " << queue->entry[j].full_addr << dec << " type: " << +queue->entry[j].type;
        cout << " fill_level: " << queue->entry[j].fill_level << " lq_index: " << queue->entry[j].lq_index << " sq_index: " << queue->entry[j].sq_index << endl; 
    }
    queue = &ooo_cpu[i].L1I.MSHR;
    cout << endl << queue->NAME << " Entry" << endl;
    for (uint32_t j=0; j<queue->SIZE; j++) {
        cout << "[" << queue->NAME << "] entry: " << j << " instr_id: " << queue->entry[j].instr_id << " rob_index: " << queue->entry[j].rob_index;
        cout << " address: " << hex << queue->entry[j].address << " full_addr: " << queue->entry[j].full_addr << dec << " type: " << +queue->entry[j].type;
        cout << " fill_level: " << queue->entry[j].fill_level << " lq_index: " << queue->entry[j].lq_index << " sq_index: " << queue->entry[j].sq_index << endl; 
    }

    queue = &ooo_cpu[i].STLB.MSHR;
    cout << endl << queue->NAME << " Entry " << endl;
    for (uint32_t j=0; j<queue->SIZE; j++) {
        cout << "[" << queue->NAME << "] entry: " << j << " instr_id: " << queue->entry[j].instr_id << " rob_index: " << queue->entry[j].rob_index;
        cout << " address: " << hex << queue->entry[j].address << " full_addr: " << queue->entry[j].full_addr << dec << " type: " << +queue->entry[j].type;
        cout << " fill_level: " << queue->entry[j].fill_level << " lq_index: " << queue->entry[j].lq_index << " sq_index: " << queue->entry[j].sq_index << endl; 
    }
    queue = &ooo_cpu[i].PTW.MSHR;
    cout << endl << queue->NAME << " Entry " << endl;
    for (uint32_t j=0; j<queue->SIZE; j++) {
        cout << "[" << queue->NAME << "] entry: " << j << " instr_id: " << queue->entry[j].instr_id << " rob_index: " << queue->entry[j].rob_index;
        cout << " address: " << hex << queue->entry[j].address << " full_addr: " << queue->entry[j].full_addr << dec << " type: " << +queue->entry[j].type;
        cout << " fill_level: " << queue->entry[j].fill_level << " lq_index: " << queue->entry[j].lq_index << " sq_index: " << queue->entry[j].sq_index << endl; 
    }
    queue = &ooo_cpu[i].PTW.RQ;
    cout << endl << queue->NAME << " Entry " << endl;
    for (uint32_t j=0; j<queue->SIZE; j++) {
        cout << "[" << queue->NAME << "] entry: " << j << " instr_id: " << queue->entry[j].instr_id << " rob_index: " << queue->entry[j].rob_index;
        cout << " address: " << hex << queue->entry[j].address << " full_addr: " << queue->entry[j].full_addr << dec << " type: " << +queue->entry[j].type;
        cout << " fill_level: " << queue->entry[j].fill_level << " lq_index: " << queue->entry[j].lq_index << " sq_index: " << queue->entry[j].sq_index << endl; 
    }
    queue = &ooo_cpu[i].PTW.PQ;
    cout << endl << queue->NAME << " Entry " << endl;
    for (uint32_t j=0; j<queue->SIZE; j++) {
        cout << "[" << queue->NAME << "] entry: " << j << " instr_id: " << queue->entry[j].instr_id << " rob_index: " << queue->entry[j].rob_index;
        cout << " address: " << hex << queue->entry[j].address << " full_addr: " << queue->entry[j].full_addr << dec << " type: " << +queue->entry[j].type;
        cout << " fill_level: " << queue->entry[j].fill_level << " lq_index: " << queue->entry[j].lq_index << " sq_index: " << queue->entry[j].sq_index << endl; 
    }
    assert(0);
}

void signal_handler(int signal) 
{
	cout << "Caught signal: " << signal << endl;
	exit(1);
}

// log base 2 function from efectiu
int lg2(int n)
{
    int i, m = n, c = -1;
    for (i=0; m; i++) {
        m /= 2;
        c++;
    }
    return c;
}

uint64_t rotl64 (uint64_t n, unsigned int c)
{
    const unsigned int mask = (CHAR_BIT*sizeof(n)-1);

    assert ( (c<=mask) &&"rotate by type width or more");
    c &= mask;  // avoid undef behaviour with NDEBUG.  0 overhead for most types / compilers
    return (n<<c) | (n>>( (-c)&mask ));
}

uint64_t rotr64 (uint64_t n, unsigned int c)
{
    const unsigned int mask = (CHAR_BIT*sizeof(n)-1);

    assert ( (c<=mask) &&"rotate by type width or more");
    c &= mask;  // avoid undef behaviour with NDEBUG.  0 overhead for most types / compilers
    return (n>>c) | (n<<( (-c)&mask ));
}

RANDOM champsim_rand(champsim_seed);

#ifndef INS_PAGE_TABLE_WALKER
uint64_t va_to_pa(uint32_t cpu, uint64_t instr_id, uint64_t va, uint64_t unique_vpage)
{
#ifdef SANITY_CHECK
    if (va == 0) 
        assert(0);
#endif
	//DP ( if (warmup_complete[cpu]) {
          //      cout << "va = " << hex << va << "unique_vpage = " << unique_vpage << endl; });
    uint8_t  swap = 0;
    uint64_t high_bit_mask = rotr64(cpu, lg2(NUM_CPUS)),
             unique_va = va | high_bit_mask;
    //DP ( if (warmup_complete[cpu]) {
      //      cout <<  hex << "unique_va = " << unique_va << "highBitMask = " << high_bit_mask << endl; });
    //uint64_t vpage = unique_va >> LOG2_PAGE_SIZE,
    uint64_t vpage = unique_vpage | high_bit_mask,
             voffset = unique_va & ((1<<LOG2_PAGE_SIZE) - 1);
    //DP ( if (warmup_complete[cpu]) {
      //          cout << "voffset = " << hex << voffset << "vpage = " << vpage << endl; });
    // smart random number generator
    uint64_t random_ppage;

    map <uint64_t, uint64_t>::iterator pr = page_table.begin();
    map <uint64_t, uint64_t>::iterator ppage_check = inverse_table.begin();

    // check unique cache line footprint
    map <uint64_t, uint64_t>::iterator cl_check = unique_cl[cpu].find(unique_va >> LOG2_BLOCK_SIZE);
    if (cl_check == unique_cl[cpu].end()) { // we've never seen this cache line before
        unique_cl[cpu].insert(make_pair(unique_va >> LOG2_BLOCK_SIZE, 0));
        num_cl[cpu]++;
    }
    else
        cl_check->second++;

    pr = page_table.find(vpage);
    if (pr == page_table.end()) { // no VA => PA translation found 

        if (allocated_pages >= DRAM_PAGES) { // not enough memory

            // TODO: elaborate page replacement algorithm
            // here, ChampSim randomly selects a page that is not recently used and we only track 32K recently accessed pages
            uint8_t  found_NRU = 0;
            uint64_t NRU_vpage = 0; // implement it
            map <uint64_t, uint64_t>::iterator pr2 = recent_page.begin();
            for (pr = page_table.begin(); pr != page_table.end(); pr++) {

                NRU_vpage = pr->first;
                if (recent_page.find(NRU_vpage) == recent_page.end()) {
                    found_NRU = 1;
                    break;
                }
            }
#ifdef SANITY_CHECK
            if (found_NRU == 0)
                assert(0);

            if (pr == page_table.end())
                assert(0);
#endif
            DP ( if (warmup_complete[cpu]) {
            cout << "[SWAP] update page table NRU_vpage: " << hex << pr->first << " new_vpage: " << vpage << " ppage: " << pr->second << dec << endl; });

            // update page table with new VA => PA mapping
            // since we cannot change the key value already inserted in a map structure, we need to erase the old node and add a new node
            uint64_t mapped_ppage = pr->second;
            page_table.erase(pr);
            page_table.insert(make_pair(vpage, mapped_ppage));
	    cout << " Inserted in page table " << endl;

            // update inverse table with new PA => VA mapping
            ppage_check = inverse_table.find(mapped_ppage);
#ifdef SANITY_CHECK
            if (ppage_check == inverse_table.end())
                assert(0);
#endif
            ppage_check->second = vpage;

            DP ( if (warmup_complete[cpu]) {
            cout << "[SWAP] update inverse table NRU_vpage: " << hex << NRU_vpage << " new_vpage: ";
            cout << ppage_check->second << " ppage: " << ppage_check->first << dec << endl; });

            // update page_queue
            page_queue.pop();
            page_queue.push(vpage);

            // invalidate corresponding vpage and ppage from the cache hierarchy
            ooo_cpu[cpu].ITLB.invalidate_entry(NRU_vpage);
            ooo_cpu[cpu].DTLB.invalidate_entry(NRU_vpage);
            ooo_cpu[cpu].STLB.invalidate_entry(NRU_vpage);
            for (uint32_t i=0; i<BLOCK_SIZE; i++) {
                uint64_t cl_addr = (mapped_ppage << 6) | i;
                ooo_cpu[cpu].L1I.invalidate_entry(cl_addr);
                ooo_cpu[cpu].L1D.invalidate_entry(cl_addr);
                ooo_cpu[cpu].L2C.invalidate_entry(cl_addr);
                uncore.LLC.invalidate_entry(cl_addr);
            }

            // swap complete
            swap = 1;
        } else {
            uint8_t fragmented = 0;
            if (num_adjacent_page > 0)
                random_ppage = ++previous_ppage;
            else {
                random_ppage = champsim_rand.draw_rand();
                fragmented = 1;
            }

            // encoding cpu number 
            // this allows ChampSim to run homogeneous multi-programmed workloads without VA => PA aliasing
            // (e.g., cpu0: astar  cpu1: astar  cpu2: astar  cpu3: astar...)
            //random_ppage &= (~((NUM_CPUS-1)<< (32-LOG2_PAGE_SIZE)));
            //random_ppage |= (cpu<<(32-LOG2_PAGE_SIZE)); 

            while (1) { // try to find an empty physical page number
                ppage_check = inverse_table.find(random_ppage); // check if this page can be allocated 
                if (ppage_check != inverse_table.end()) { // random_ppage is not available
                    DP ( if (warmup_complete[cpu]) {
                    cout << "vpage: " << hex << ppage_check->first << " is already mapped to ppage: " << random_ppage << dec << endl; }); 
                    
                    if (num_adjacent_page > 0)
                        fragmented = 1;

                    // try one more time
                    random_ppage = champsim_rand.draw_rand();
                    
                    // encoding cpu number 
                    //random_ppage &= (~((NUM_CPUS-1)<<(32-LOG2_PAGE_SIZE)));
                    //random_ppage |= (cpu<<(32-LOG2_PAGE_SIZE)); 
                }
                else
                    break;
            }

            // insert translation to page tables
            //printf("Insert  num_adjacent_page: %u  vpage: %lx  ppage: %lx\n", num_adjacent_page, vpage, random_ppage);
            page_table.insert(make_pair(vpage, random_ppage));
	    cout << " Inserted in page table " << endl;
            inverse_table.insert(make_pair(random_ppage, vpage));
	    cout << " Inserted in inverse table " << endl;
            page_queue.push(vpage);
            previous_ppage = random_ppage;
            num_adjacent_page--;
            num_page[cpu]++;
            allocated_pages++;

            // try to allocate pages contiguously
            if (fragmented) {
                num_adjacent_page = 1 << (rand() % 10);
                DP ( if (warmup_complete[cpu]) {
                cout << "Recalculate num_adjacent_page: " << num_adjacent_page << endl; });
            }
        }

        if (swap)
            major_fault[cpu]++;
        else
            minor_fault[cpu]++;
    }
    else {
        //printf("Found  vpage: %lx  random_ppage: %lx\n", vpage, pr->second);
    }

    pr = page_table.find(vpage);
#ifdef SANITY_CHECK
    if (pr == page_table.end())
        assert(0);
#endif
    uint64_t ppage = pr->second;

    uint64_t pa = ppage << LOG2_PAGE_SIZE;
    pa |= voffset;

    DP ( if (warmup_complete[cpu]) {
    cout << "[PAGE_TABLE] instr_id: " << instr_id << " vpage: " << hex << vpage;
    cout << " => ppage: " << (pa >> LOG2_PAGE_SIZE) << " vadress: " << unique_va << " paddress: " << pa << dec << endl; });

    /*
    if (swap)
        stall_cycle[cpu] = current_core_cycle[cpu] + SWAP_LATENCY;
    else
        stall_cycle[cpu] = current_core_cycle[cpu] + PAGE_TABLE_LATENCY;
*/
    //cout << "cpu: " << cpu << " allocated unique_vpage: " << hex << unique_vpage << " to ppage: " << ppage << dec << endl;

    return pa;
}
#endif

void swap_context(uint8_t swap_cpu_0, uint8_t swap_cpu_1)
{
	//swap file descriptor
	FILE *swap_file;
	swap_file = ooo_cpu[swap_cpu_0].trace_file;
	ooo_cpu[swap_cpu_0].trace_file = ooo_cpu[swap_cpu_1].trace_file;
	ooo_cpu[swap_cpu_1].trace_file = swap_file;

	//fetch_stall should be zero after context-switch
	//Since context-switch penalty is not included, this needs to be done, once it is added, fetch_stall will be 0 as branch mispredict penalty is only 20, while context-switch penalty is in miliseconds 
	ooo_cpu[swap_cpu_0].fetch_stall = 0;
	ooo_cpu[swap_cpu_1].fetch_stall = 0;

	cout << "FLUSHING TLB \n\n\n" << endl;
	//TLB flush
	
	ooo_cpu[swap_cpu_0].ITLB.flush_TLB();
	ooo_cpu[swap_cpu_1].ITLB.flush_TLB();
	ooo_cpu[swap_cpu_0].DTLB.flush_TLB();
	ooo_cpu[swap_cpu_1].DTLB.flush_TLB();
	ooo_cpu[swap_cpu_0].STLB.flush_TLB();
	ooo_cpu[swap_cpu_1].STLB.flush_TLB();

}

void fill_page_table(string trace_name)
{
	cout << trace_name << endl;
	string final_trace;
	final_trace = trace_name.substr( 69, trace_name.size() - 69);	//spec(63), server(65), cloudsuite(69)
	cout << "Final trace: " << final_trace << endl;
	string file_name = "results_50M/cloudsuite_PT/" + final_trace + "-hashed_perceptron-no-no-no-no-no-no-srrip-drrip-1core-cloudsuite.txt";
	ifstream test_file(file_name);
	if(!test_file.good())
	{
		printf("BASELINE FILE DOESN'T EXIST\n");
		assert(0);
	}

	string line;
	while(getline(test_file, line, '\n'))
	{
		if(line.size() > 2 && line[0]=='P' && line[1]=='T') 
		{
			istringstream temp(line);
			string t, va, pa;
			getline(temp, t, ' ');
			getline(temp, va, ' ');
			getline(temp, pa, ' ');

			temp_page_table[stol(va)] = stol(pa);
		}
	}
}
void cpu_l1i_prefetcher_cache_operate(uint32_t cpu_num, uint64_t v_addr, uint8_t cache_hit, uint8_t prefetch_hit)
{
	  ooo_cpu[cpu_num].l1i_prefetcher_cache_operate(v_addr, cache_hit, prefetch_hit);
}

void cpu_l1i_prefetcher_cache_fill(uint32_t cpu_num, uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr)
{
	  ooo_cpu[cpu_num].l1i_prefetcher_cache_fill(addr, set, way, prefetch, evicted_addr);
}


int main(int argc, char** argv)
{
	
	// interrupt signal hanlder
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = signal_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);

    cout << endl << "*** ChampSim Multicore Out-of-Order Simulator ***" << endl << endl;

    // initialize knobs
    uint8_t show_heartbeat = 1;

    uint32_t seed_number = 0;

    // check to see if knobs changed using getopt_long()
    int c, index;
    while (1) {
        static struct option long_options[] =
        {
            {"warmup_instructions", required_argument, 0, 'w'},
            {"simulation_instructions", required_argument, 0, 'i'},
            {"hide_heartbeat", no_argument, 0, 'h'},
            {"cloudsuite", no_argument, 0, 'c'},
			{"cvp_trace", no_argument, 0, 'v'},
            {"low_bandwidth",  no_argument, 0, 'b'},
            {"traces",  no_argument, 0, 't'},
      	    {"context_switch", required_argument, 0, 's'},
            {0,0,0,0}	    
        };

        int option_index = 0;

        c = getopt_long_only(argc, argv, "wihsb", long_options, &option_index);

        // no more option characters
        if (c == -1)
            break;

        int traces_encountered = 0;

        switch(c) {
            case 'w':
                warmup_instructions = atol(optarg);
                break;
            case 'i':
                simulation_instructions = atol(optarg);
                break;
            case 'h':
                show_heartbeat = 0;
                break;
            case 'c':
                knob_cloudsuite = 1;
                MAX_INSTR_DESTINATIONS = NUM_INSTR_DESTINATIONS_SPARC;
                break;
            case 'b':
                std::cout << "Low Bandwidth" << std::endl;
                knob_low_bandwidth = 1;
                break;
            case 't':
                traces_encountered = 1;
                break;
			case 's':
				knob_context_switch = 1;
				break;
			case 'v': //CVP TRACE
				reg_instruction_pointer = 103;
				reg_stack_pointer = 102;
				reg_flags = 64;
				break;
            default:
                abort();
        }

        if (traces_encountered == 1)
            break;
    }

    // consequences of knobs
    cout << "Warmup Instructions: " << warmup_instructions << endl;
    cout << "Simulation Instructions: " << simulation_instructions << endl;
    //cout << "Scramble Loads: " << (knob_scramble_loads ? "ture" : "false") << endl;
    cout << "Number of CPUs: " << NUM_CPUS << endl;
    cout << "LLC sets: " << LLC_SET << endl;
    cout << "LLC ways: " << LLC_WAY << endl;
#ifdef CAPTURE_DYNAMIC_ENERGY_PROFILE
    cout << "PHASE_SIZE_IN_CYCLES: " << PHASE_SIZE_IN_CYCLES << endl;
#endif

    if (knob_low_bandwidth)
        DRAM_MTPS = DRAM_IO_FREQ/8;
    else
        DRAM_MTPS = DRAM_IO_FREQ;

    // DRAM access latency
    tRP  = (uint32_t)((1.0 * tRP_DRAM_NANOSECONDS  * CPU_FREQ) / 1000); 
    tRCD = (uint32_t)((1.0 * tRCD_DRAM_NANOSECONDS * CPU_FREQ) / 1000); 
    tCAS = (uint32_t)((1.0 * tCAS_DRAM_NANOSECONDS * CPU_FREQ) / 1000); 
    
    

    // default: 16 = (64 / 8) * (3200 / 1600)
    // it takes 16 CPU cycles to tranfser 64B cache block on a 8B (64-bit) bus 
    // note that dram burst length = BLOCK_SIZE/DRAM_CHANNEL_WIDTH
    DRAM_DBUS_RETURN_TIME = (BLOCK_SIZE / DRAM_CHANNEL_WIDTH) * (CPU_FREQ / DRAM_MTPS);

    printf("Off-chip DRAM Size: %u MB Channels: %u Width: %u-bit Data Rate: %u MT/s\n",
            DRAM_SIZE, DRAM_CHANNELS, 8*DRAM_CHANNEL_WIDTH, DRAM_MTPS);

    // end consequence of knobs

    // search through the argv for "-traces"
    int found_traces = 0;
    int count_traces = 0;
    
    for (int i=0; i<argc; i++) {	    
        if (found_traces) {
        	
            printf("CPU %d runs %s\n", count_traces, argv[i]);

	    //@Vasudha: Perfect DTLB Prefetcher - read dumped page table
	    //fill_page_table(argv[i]);

            sprintf(ooo_cpu[count_traces].trace_string, "%s", argv[i]);

            char *full_name = ooo_cpu[count_traces].trace_string,
                 *last_dot = strrchr(ooo_cpu[count_traces].trace_string, '.');
		cout << last_dot << endl;
			ifstream test_file(full_name);
			if(!test_file.good()){
				printf("TRACE FILE DOES NOT EXIST\n");
				assert(false);
			}
				

            if (full_name[last_dot - full_name + 1] == 'g') // gzip format
                sprintf(ooo_cpu[count_traces].gunzip_command, "gunzip -c %s", argv[i]);
            else if (full_name[last_dot - full_name + 1] == 'x') // xz
                sprintf(ooo_cpu[count_traces].gunzip_command, "xz -dc %s", argv[i]);
            else {
                cout << "ChampSim does not support traces other than gz or xz compression!" << endl; 
                assert(0);
            }

            char *pch[100];
            int count_str = 0;
            pch[0] = strtok (argv[i], " /,.-");
            while (pch[count_str] != NULL) {
                //printf ("%s %d\n", pch[count_str], count_str);
                count_str++;
                pch[count_str] = strtok (NULL, " /,.-");
            }

            //printf("max count_str: %d\n", count_str);
            //printf("application: %s\n", pch[count_str-3]);

            int j = 0;
            while (pch[count_str-3][j] != '\0') {
                seed_number += pch[count_str-3][j];
                //printf("%c %d %d\n", pch[count_str-3][j], j, seed_number);
                j++;
            }

            ooo_cpu[count_traces].trace_file = popen(ooo_cpu[count_traces].gunzip_command, "r");
            if (ooo_cpu[count_traces].trace_file == NULL) {
                printf("\n*** Trace file not found: %s ***\n\n", argv[i]);
                assert(0);
            }

           count_traces++;
            if (count_traces > NUM_CPUS) {
                printf("\n*** Too many traces for the configured number of cores ***\n\n");
                assert(0);
            }
        }
        else if(strcmp(argv[i],"-traces") == 0) {
            found_traces = 1;
        }
	/* Below code is required so that, further arguements are not treated as tracefile */
 	if(i+1<argc && (strcmp(argv[i+1], "-context_switch") == 0 || strcmp(argv[i+1], "-s") == 0))
	{
		found_traces = 0;
		knob_context_switch = 1;
		cout << "knob on" << endl;
	}
    }
    
    if(knob_context_switch == 1)
    {
	     
//	for(int i=0; i<count_traces; i++)
//	{
	sprintf(context_switch_string, "%s", argv[argc-1]);
	
	ifstream test_file(context_switch_string);
	if(!test_file.good()){
		printf("CONTEXT SWITCH FILE DOES NOT EXIST\n");
		assert(false);
	}
	else
		cout << "CONTEXT SWITCH FILE EXIST\n";
	        //ooo_cpu[i].context_switch_file = popen(ooo_cpu[i].context_switch_string, "r");
	context_switch_file = fopen(context_switch_string, "r");

	if (context_switch_file == NULL) {
        	printf("\n*** Context switch file not found: %s ***\n\n", argv[argc-1]);
                assert(0);
        }
	else
		cout << "Context_switch file found\n\n" ;
        index=0;
	while((fscanf(context_switch_file, "%ld %d %d", &cs_file[index].cycle, &cs_file[index].swap_cpu[0], &cs_file[index].swap_cpu[1]))!=EOF)
	{
		cs_file[index].index = index;
		cout << "print file:" << cs_file[index].index << " " << cs_file[index].cycle <<"- "<<  cs_file[index].swap_cpu[0] << cs_file[index].swap_cpu[1] <<  endl;
		index++;
	}
	
//	}
    }

    if (count_traces != NUM_CPUS) {
        printf("\n*** Not enough traces for the configured number of cores ***\n\n");
        assert(0);
    }
    // end trace file setup
    // TODO: can we initialize these variables from the class constructor?
    srand(seed_number);
    champsim_seed = seed_number;
    for (int i=0; i<NUM_CPUS; i++) {

	
        ooo_cpu[i].cpu = i; 
        ooo_cpu[i].warmup_instructions = warmup_instructions;
        ooo_cpu[i].simulation_instructions = simulation_instructions;
        ooo_cpu[i].begin_sim_cycle = 0; 
        ooo_cpu[i].begin_sim_instr = warmup_instructions;

        // ROB
        ooo_cpu[i].ROB.cpu = i;

	//Neelu: Adding call to initialize core.
	ooo_cpu[i].initialize_core();

        // BRANCH PREDICTOR
        ooo_cpu[i].initialize_branch_predictor();

		ooo_cpu[i].BTB.cpu = i;
		ooo_cpu[i].BTB.cache_type = IS_BTB;

		ooo_cpu[i].BTB.initialize_replacement = &CACHE::btb_initialize_replacement;
        ooo_cpu[i].BTB.update_replacement_state = &CACHE::btb_update_replacement_state;
        ooo_cpu[i].BTB.find_victim = &CACHE::btb_find_victim;
        ooo_cpu[i].BTB.replacement_final_stats = &CACHE::btb_replacement_final_stats;
        (ooo_cpu[i].BTB.*(ooo_cpu[i].BTB.initialize_replacement))();

        // TLBs
        ooo_cpu[i].ITLB.cpu = i;
        ooo_cpu[i].ITLB.cache_type = IS_ITLB;
	ooo_cpu[i].ITLB.MAX_READ = 2;
        ooo_cpu[i].ITLB.fill_level = FILL_L1;
        ooo_cpu[i].ITLB.extra_interface = &ooo_cpu[i].L1I;
        ooo_cpu[i].ITLB.lower_level = &ooo_cpu[i].STLB; 
        ooo_cpu[i].ITLB.itlb_prefetcher_initialize();
	
	ooo_cpu[i].ITLB.initialize_replacement = &CACHE::itlb_initialize_replacement;
	ooo_cpu[i].ITLB.update_replacement_state = &CACHE::itlb_update_replacement_state;
	ooo_cpu[i].ITLB.find_victim = &CACHE::itlb_find_victim;
	ooo_cpu[i].ITLB.replacement_final_stats = &CACHE::itlb_replacement_final_stats;
	(ooo_cpu[i].ITLB.*(ooo_cpu[i].ITLB.initialize_replacement))();

        ooo_cpu[i].DTLB.cpu = i;
        ooo_cpu[i].DTLB.cache_type = IS_DTLB;
        ooo_cpu[i].DTLB.MAX_READ = (2 > MAX_READ_PER_CYCLE) ? MAX_READ_PER_CYCLE : 2;
        ooo_cpu[i].DTLB.fill_level = FILL_L1;
        ooo_cpu[i].DTLB.extra_interface = &ooo_cpu[i].L1D;
        ooo_cpu[i].DTLB.lower_level = &ooo_cpu[i].STLB;
        ooo_cpu[i].DTLB.dtlb_prefetcher_initialize();

	#ifdef PUSH_DTLB_PB
	ooo_cpu[i].DTLB_PB.cpu = i;
	ooo_cpu[i].DTLB_PB.cache_type = IS_DTLB_PB;
	ooo_cpu[i].DTLB_PB.fill_level = FILL_L1;
	#endif
	ooo_cpu[i].DTLB.initialize_replacement = &CACHE::dtlb_initialize_replacement;
	ooo_cpu[i].DTLB.update_replacement_state = &CACHE::dtlb_update_replacement_state;
	ooo_cpu[i].DTLB.find_victim = &CACHE::dtlb_find_victim;
	ooo_cpu[i].DTLB.replacement_final_stats = &CACHE::dtlb_replacement_final_stats;
	(ooo_cpu[i].DTLB.*(ooo_cpu[i].DTLB.initialize_replacement))();

        ooo_cpu[i].STLB.cpu = i;
        ooo_cpu[i].STLB.cache_type = IS_STLB;
        ooo_cpu[i].STLB.fill_level = FILL_L2;
        ooo_cpu[i].STLB.upper_level_icache[i] = &ooo_cpu[i].ITLB;
        ooo_cpu[i].STLB.upper_level_dcache[i] = &ooo_cpu[i].DTLB;
        ooo_cpu[i].STLB.stlb_prefetcher_initialize();
#ifdef INS_PAGE_TABLE_WALKER
	ooo_cpu[i].STLB.lower_level = &ooo_cpu[i].PTW;
#endif

	ooo_cpu[i].STLB.initialize_replacement = &CACHE::stlb_initialize_replacement;
	ooo_cpu[i].STLB.update_replacement_state = &CACHE::stlb_update_replacement_state;
	ooo_cpu[i].STLB.find_victim = &CACHE::stlb_find_victim;
	ooo_cpu[i].STLB.replacement_final_stats = &CACHE::stlb_replacement_final_stats;
	(ooo_cpu[i].STLB.*(ooo_cpu[i].STLB.initialize_replacement))();

		ooo_cpu[i].PTW.cpu = i;
        ooo_cpu[i].PTW.cache_type = IS_PTW;
        ooo_cpu[i].PTW.upper_level_icache[i] = &ooo_cpu[i].STLB;
        ooo_cpu[i].PTW.upper_level_dcache[i] = &ooo_cpu[i].STLB;

	ooo_cpu[i].PTW.PSCL5.cache_type = IS_PSCL5;
	ooo_cpu[i].PTW.PSCL4.cache_type = IS_PSCL4;
	ooo_cpu[i].PTW.PSCL3.cache_type = IS_PSCL3;
	ooo_cpu[i].PTW.PSCL2.cache_type = IS_PSCL2;

        // PRIVATE CACHE
        ooo_cpu[i].L1I.cpu = i;
        ooo_cpu[i].L1I.cache_type = IS_L1I;
        ooo_cpu[i].L1I.MAX_READ = (FETCH_WIDTH > MAX_READ_PER_CYCLE) ? MAX_READ_PER_CYCLE : FETCH_WIDTH;
        ooo_cpu[i].L1I.fill_level = FILL_L1;
        ooo_cpu[i].L1I.lower_level = &ooo_cpu[i].L2C; 
	ooo_cpu[i].l1i_prefetcher_initialize();
        ooo_cpu[i].L1I.l1i_prefetcher_cache_operate = cpu_l1i_prefetcher_cache_operate;
        ooo_cpu[i].L1I.l1i_prefetcher_cache_fill = cpu_l1i_prefetcher_cache_fill;

	ooo_cpu[i].L1I.initialize_replacement = &CACHE::l1i_initialize_replacement;
	ooo_cpu[i].L1I.update_replacement_state = &CACHE::l1i_update_replacement_state;
	ooo_cpu[i].L1I.find_victim = &CACHE::l1i_find_victim;
	ooo_cpu[i].L1I.replacement_final_stats = &CACHE::l1i_replacement_final_stats;
	(ooo_cpu[i].L1I.*(ooo_cpu[i].L1I.initialize_replacement))();


        ooo_cpu[i].L1D.cpu = i;
        ooo_cpu[i].L1D.cache_type = IS_L1D;
        ooo_cpu[i].L1D.MAX_READ = (2 > MAX_READ_PER_CYCLE) ? MAX_READ_PER_CYCLE : 2;
        ooo_cpu[i].L1D.fill_level = FILL_L1;
        ooo_cpu[i].L1D.lower_level = &ooo_cpu[i].L2C; 
        ooo_cpu[i].L1D.l1d_prefetcher_initialize();

	ooo_cpu[i].L1D.initialize_replacement = &CACHE::l1d_initialize_replacement;
	ooo_cpu[i].L1D.update_replacement_state = &CACHE::l1d_update_replacement_state;
	ooo_cpu[i].L1D.find_victim = &CACHE::l1d_find_victim;
	ooo_cpu[i].L1D.replacement_final_stats = &CACHE::l1d_replacement_final_stats;
	(ooo_cpu[i].L1D.*(ooo_cpu[i].L1D.initialize_replacement))();


        ooo_cpu[i].L2C.cpu = i;
        ooo_cpu[i].L2C.cache_type = IS_L2C;
        ooo_cpu[i].L2C.fill_level = FILL_L2;
        ooo_cpu[i].L2C.upper_level_icache[i] = &ooo_cpu[i].L1I;
        ooo_cpu[i].L2C.upper_level_dcache[i] = &ooo_cpu[i].L1D;
        ooo_cpu[i].L2C.lower_level = &uncore.LLC;
	ooo_cpu[i].L2C.extra_interface = &ooo_cpu[i].PTW;
        ooo_cpu[i].L2C.l2c_prefetcher_initialize();

	ooo_cpu[i].L2C.initialize_replacement = &CACHE::l2c_initialize_replacement;
	ooo_cpu[i].L2C.update_replacement_state = &CACHE::l2c_update_replacement_state;
	ooo_cpu[i].L2C.find_victim = &CACHE::l2c_find_victim;
	ooo_cpu[i].L2C.replacement_final_stats = &CACHE::l2c_replacement_final_stats;
	(ooo_cpu[i].L2C.*(ooo_cpu[i].L2C.initialize_replacement))();


        // SHARED CACHE
        uncore.LLC.cache_type = IS_LLC;
        uncore.LLC.fill_level = FILL_LLC;
        uncore.LLC.MAX_READ = NUM_CPUS;
        uncore.LLC.upper_level_icache[i] = &ooo_cpu[i].L2C;
        uncore.LLC.upper_level_dcache[i] = &ooo_cpu[i].L2C;
        uncore.LLC.lower_level = &uncore.DRAM;

	uncore.LLC.initialize_replacement = &CACHE::llc_initialize_replacement;
	uncore.LLC.update_replacement_state = &CACHE::llc_update_replacement_state;
	uncore.LLC.find_victim = &CACHE::llc_find_victim;
	uncore.LLC.replacement_final_stats = &CACHE::llc_replacement_final_stats;
	(uncore.LLC.*(uncore.LLC.initialize_replacement))();



        // OFF-CHIP DRAM
        uncore.DRAM.fill_level = FILL_DRAM;
        uncore.DRAM.upper_level_icache[i] = &uncore.LLC;
        uncore.DRAM.upper_level_dcache[i] = &uncore.LLC;
        for (uint32_t i=0; i<DRAM_CHANNELS; i++) {
            uncore.DRAM.RQ[i].is_RQ = 1;
            uncore.DRAM.WQ[i].is_WQ = 1;
        }

        warmup_complete[i] = 0;
        //all_warmup_complete = NUM_CPUS;
        simulation_complete[i] = 0;
        current_core_cycle[i] = 0;
        stall_cycle[i] = 0;
        
        previous_ppage = 0;
        num_adjacent_page = 0;
        num_cl[i] = 0;
        allocated_pages = 0;
        num_page[i] = 0;
        minor_fault[i] = 0;
        major_fault[i] = 0;
    }

    uncore.LLC.llc_prefetcher_initialize();

    // simulation entry point
    start_time = time(NULL);
    uint8_t run_simulation = 1;
    int cs_index = 0;
    while (run_simulation) {

        uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time),
                 elapsed_minute = elapsed_second / 60,
                 elapsed_hour = elapsed_minute / 60;
        elapsed_minute -= elapsed_hour*60;
        elapsed_second -= (elapsed_hour*3600 + elapsed_minute*60);
	
	
	if(cs_index >= index)
		cs_index = -1;
	
        for (int i=0; i<NUM_CPUS; i++) {
            // proceed one cycle
            current_core_cycle[i]++;

	
	    //Neelu: Capturing the phase-wise stats for dynamic energy. 
#ifdef CAPTURE_DYNAMIC_ENERGY_PROFILE
	    if(warmup_complete[i] && current_core_cycle[i] % PHASE_SIZE_IN_CYCLES == 0)
	    {
		//Listing the variables I need: 
		//L1I: rq_to_cache, rq_merged, prefetch_miss, load_miss, pq_to_cache. 
		cache_data_tag_accesses[0][phase_id] = 0.2907293 * (ooo_cpu[i].L1I.RQ.TO_CACHE + ooo_cpu[i].L1I.RQ.MERGED + ooo_cpu[i].L1I.sim_miss[i][2]);  
		cache_tag_accesses[0][phase_id] = 0.00244234 * (ooo_cpu[i].L1I.PQ.TO_CACHE + ooo_cpu[i].L1I.sim_miss[i][0]); 

		//L1D: rq_to_cache, rq_merged, prefetch_miss, wq_to_cache, load_miss, pq_to_cache, rfo_cache
		cache_data_tag_accesses[1][phase_id] = 0.4359786 * (ooo_cpu[i].L1D.RQ.TO_CACHE + ooo_cpu[i].L1D.RQ.MERGED + ooo_cpu[i].L1D.sim_miss[i][2] + ooo_cpu[i].L1D.WQ.TO_CACHE);
		cache_tag_accesses[1][phase_id] = 0.00556264 * (ooo_cpu[i].L1D.PQ.TO_CACHE + ooo_cpu[i].L1D.sim_miss[i][0] + ooo_cpu[i].L1D.sim_miss[i][1]);

		//L2C: rq_to_cache, prefetch_miss, wq_to_cache, total_miss, pq_to_cache
		cache_data_tag_accesses[2][phase_id] = 0.4349426 * (ooo_cpu[i].L2C.RQ.TO_CACHE + ooo_cpu[i].L2C.sim_miss[i][2] + ooo_cpu[i].L2C.WQ.TO_CACHE + ooo_cpu[i].L2C.PQ.TO_CACHE);

		uint64_t l2c_total_miss = 0;
		for (uint32_t j=0; j<NUM_TYPES; j++) {
        		l2c_total_miss += ooo_cpu[i].L2C.sim_miss[i][j];
	        }


		cache_tag_accesses[2][phase_id] = 0.00909169 * l2c_total_miss;

		
		//LLC: rq_to_cache, prefetch_miss, wq_to_cache, total_miss, pq_to_cache
		
		cache_data_tag_accesses[3][phase_id] = 0.8522517 * (uncore.LLC.RQ.TO_CACHE + uncore.LLC.sim_miss[i][2] + uncore.LLC.WQ.TO_CACHE + uncore.LLC.PQ.TO_CACHE);

		uint64_t total_miss = 0;
		for (uint32_t j=0; j<NUM_TYPES; j++) {
			total_miss += uncore.LLC.sim_miss[i][j];
		}

		cache_tag_accesses[3][phase_id] = 0.0229417 * total_miss;
		
		//DRAM: RQ/WQ row_buffer_hits/misses	NOTE: CAPTURING ONLY FOR CHANNEL 0, BASED ON CURRENT CONFIGURATION. 
		dram_accesses[phase_id] = 10.24 * (uncore.DRAM.RQ[0].ROW_BUFFER_HIT + uncore.DRAM.RQ[0].ROW_BUFFER_MISS + uncore.DRAM.WQ[0].ROW_BUFFER_HIT + uncore.DRAM.WQ[0].ROW_BUFFER_MISS);  
		//Prefetcher: 

		//Bingo: 

		//IPCP: 
		#ifdef IPCP_PREFETCHER 
		ip_table_read[phase_id] = ip_table_read_accesses * 0.00084576;
		ip_table_tag_read[phase_id] = ip_table_tag_read_accesses * 0.000423324;
		ip_table_write[phase_id] = ip_table_write_accesses * 0.000659022;
		ip_table_tag_write[phase_id] = ip_table_tag_write_accesses * 0.000352568; 
		cspt_write[phase_id] = cspt_write_accesses * 0.000294389;
		cspt_read[phase_id] = cspt_read_accesses * 0.000275815; 
		rstable_read[phase_id] = rstable_read_accesses * 0.00236372;
		rstable_write[phase_id] = rstable_write_accesses * 0.00245553;
		rrfilter_read[phase_id] = rrfilter_read_accesses * 0.00188962;
		rrfilter_tag_write[phase_id] = rrfilter_tag_write_accesses * 0.00184435;
		
		#endif	

		//ITLB: rq_to_cache, rq_merged, total_miss

		cache_data_tag_accesses[4][phase_id] = 0.001027806 * (ooo_cpu[i].ITLB.RQ.TO_CACHE + ooo_cpu[i].ITLB.RQ.MERGED);

		total_miss = 0;
		for (uint32_t j=0; j<NUM_TYPES; j++) {
			total_miss += ooo_cpu[i].ITLB.sim_miss[i][j];
		}
		cache_tag_accesses[4][phase_id] = 0.000564828 * total_miss;

		//DTLB: rq_to_cache, rq_merged, load_miss

		cache_data_tag_accesses[5][phase_id] = 0.001027806 * (ooo_cpu[i].DTLB.RQ.TO_CACHE + ooo_cpu[i].DTLB.RQ.MERGED); 

		cache_tag_accesses[5][phase_id] = 0.000564828 * ooo_cpu[i].DTLB.sim_miss[i][0];

		//STLB: rq_to_cache, rq_merged, total_miss
		cache_data_tag_accesses[6][phase_id] = 0.01775155 * (ooo_cpu[i].STLB.RQ.TO_CACHE + ooo_cpu[i].STLB.RQ.MERGED);

                total_miss = 0;
                for (uint32_t j=0; j<NUM_TYPES; j++) {
                        total_miss += ooo_cpu[i].STLB.sim_miss[i][j];
                }
                cache_tag_accesses[6][phase_id] = 0.00809473 * total_miss;	

		//Interconnect: llc_accesses, llc_writebacks, l2c_misses
		uint64_t llc_total_access = 0; 
		for (uint32_t j=0; j<NUM_TYPES; j++) {
			llc_total_access += uncore.LLC.sim_access[i][j];
		}

		interconnect_request[phase_id] = 1 * 0.8 * llc_total_access;

		interconnect_response[phase_id] = 8 * 0.8 * (uncore.LLC.sim_access[i][3] + l2c_total_miss);
		
		phase_id++;
	    }
#endif


	  
	   /* context-switch code
	    //cs_index - points the details of next context switch
	    //operating index - If the core is involved in context-switch, it stores the index of file which has details about the same 
	    */
	    if( cs_index!=-1 && ( current_core_cycle[i]==cs_file[cs_index].cycle) && (i==cs_file[cs_index].swap_cpu[0] || i==cs_file[cs_index].swap_cpu[1]))
	    {
		    //ooo_cpu[i].operating_index = cs_file[cs_index].index;
		    //ooo_cpu[i].context_switch = 1;
		    ooo_cpu[cs_file[cs_index].swap_cpu[0]].operating_index = cs_file[cs_index].index;
		    ooo_cpu[cs_file[cs_index].swap_cpu[1]].operating_index = cs_file[cs_index].index;
		    ooo_cpu[cs_file[cs_index].swap_cpu[0]].context_switch = 1;
		    ooo_cpu[cs_file[cs_index].swap_cpu[1]].context_switch = 1;
		    cout << "context_switch at cycle" << cs_file[cs_index].cycle << " ,i="<< i << cs_file[cs_index].swap_cpu[0] << " " << cs_file[cs_index].swap_cpu[1] << endl;
		    //read_context_switch_file = 1;	//@v - ready to read next cycle for context-switching
	    	    cs_index++;
	    	    if(cs_index >= index)
			cs_index = -1;
	
	    } 
            //cout << "Trying to process instr_id: " << ooo_cpu[i].instr_unique_id << " fetch_stall: " << +ooo_cpu[i].fetch_stall;
            //cout << " stall_cycle: " << stall_cycle[i] << " current: " << current_core_cycle[i] << endl;

	    

            // core might be stalled due to page fault or branch misprediction
            if (stall_cycle[i] <= current_core_cycle[i]) {
		
                // fetch unit
            /*    if (ooo_cpu[i].ROB.occupancy < ooo_cpu[i].ROB.SIZE) {
                    // handle branch
                    if (ooo_cpu[i].fetch_stall == 0) 
                    {
                        ooo_cpu[i].handle_branch();
			@Vasudha - STOP simulation once trace file ends
			//Neelu: Commenting this, to continue rereading trace file (for multi-core, one core might get free before another.
			if(TRACE_ENDS_STOP == 1)
			{
				run_simulation = 0;
				simulation_complete[i] = 1;
			}
                    }
		 
                }*/



		// retire
		// Neelu: Commented first condition. 
                if (/*(ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].executed == COMPLETED) && */ (ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].event_cycle <= current_core_cycle[i]))
			ooo_cpu[i].retire_rob();

		// complete 
                ooo_cpu[i].update_rob();

                // schedule (including decode latency)
                uint32_t schedule_index = ooo_cpu[i].ROB.next_schedule;
                if ((ooo_cpu[i].ROB.entry[schedule_index].scheduled == 0) && (ooo_cpu[i].ROB.entry[schedule_index].event_cycle <= current_core_cycle[i]))
                    ooo_cpu[i].schedule_instruction();

                // execute
                ooo_cpu[i].execute_instruction();

				ooo_cpu[i].update_rob();

                // memory operation
                ooo_cpu[i].schedule_memory_instruction();
                ooo_cpu[i].execute_memory_instruction();

				ooo_cpu[i].update_rob();

		//decode
		if(ooo_cpu[i].DECODE_BUFFER.occupancy > 0)
		{
			ooo_cpu[i].decode_and_dispatch();
		}

		//fetch
		ooo_cpu[i].fetch_instruction();

		//Neelu: Checking IFETCH Buffer occupancy as now instructions won't be added directly to ROB. 
        if ((ooo_cpu[i].IFETCH_BUFFER.occupancy < ooo_cpu[i].IFETCH_BUFFER.SIZE) && (ooo_cpu[i].fetch_stall == 0))
        {
            ooo_cpu[i].read_from_trace();
        }



	/*	
	 	Context-switch code
		//If a core is involved in context-switch, check if ROB occupancy of this core as well as another core is 0. Also, if they have reached same current_core_cycle, swap the contexts of those two cores
	 	*/
		
		if(ooo_cpu[i].operating_index!=-1)
		{
			cout <<ooo_cpu[cs_file[ooo_cpu[i].operating_index].swap_cpu[0]].ROB.occupancy << " " <<  ooo_cpu[cs_file[ooo_cpu[i].operating_index].swap_cpu[1]].ROB.occupancy << " ";
			if(ooo_cpu[cs_file[ooo_cpu[i].operating_index].swap_cpu[0]].ROB.occupancy==0 && ooo_cpu[cs_file[ooo_cpu[i].operating_index].swap_cpu[1]].ROB.occupancy==0 && current_core_cycle[cs_file[ooo_cpu[i].operating_index].swap_cpu[0]] == current_core_cycle[cs_file[ooo_cpu[i].operating_index].swap_cpu[1]])
			{
				cout<<"entered with index "<< ooo_cpu[i].operating_index<< " with cpu = "<< i << endl;
				swap_context(cs_file[ooo_cpu[i].operating_index].swap_cpu[0], cs_file[ooo_cpu[i].operating_index].swap_cpu[1]);
				cout <<"IMPLEMENT CONTEXT SWITCH on cpu " << i <<" cycle of cpu0" << current_core_cycle[cs_file[ooo_cpu[i].operating_index].swap_cpu[0]] << " CPU1 " << current_core_cycle[cs_file[ooo_cpu[i].operating_index].swap_cpu[1]] << endl;
				ooo_cpu[cs_file[ooo_cpu[i].operating_index].swap_cpu[0]].context_switch=0;
				ooo_cpu[cs_file[ooo_cpu[i].operating_index].swap_cpu[1]].context_switch=0;
				cout << "Done! "<< cs_file[ooo_cpu[i].operating_index].swap_cpu[0] << cs_file[ooo_cpu[i].operating_index].swap_cpu[1] << endl;
				int index = ooo_cpu[i].operating_index;	//operating index will be modified
				ooo_cpu[cs_file[index].swap_cpu[0]].operating_index = -1;
				ooo_cpu[cs_file[index].swap_cpu[1]].operating_index = -1;

				cout << "operatingINDEX"<<ooo_cpu[cs_file[index].swap_cpu[0]].operating_index << " "<< ooo_cpu[cs_file[index].swap_cpu[1]].operating_index <<  endl;
			}
		}
            }

            // heartbeat information
	    
            if (show_heartbeat && (ooo_cpu[i].num_retired >= ooo_cpu[i].next_print_instruction)) {
                float cumulative_ipc;
                if (warmup_complete[i])
                    cumulative_ipc = (1.0*(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr)) / (current_core_cycle[i] - ooo_cpu[i].begin_sim_cycle);
                else
                    cumulative_ipc = (1.0*ooo_cpu[i].num_retired) / current_core_cycle[i];
                float heartbeat_ipc = (1.0*ooo_cpu[i].num_retired - ooo_cpu[i].last_sim_instr) / (current_core_cycle[i] - ooo_cpu[i].last_sim_cycle);

                cout << "Heartbeat CPU " << i << " instructions: " << ooo_cpu[i].num_retired << " cycles: " << current_core_cycle[i];
                cout << " heartbeat IPC: " << heartbeat_ipc << " cumulative IPC: " << cumulative_ipc; 
                cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << endl;
                ooo_cpu[i].next_print_instruction += STAT_PRINTING_PERIOD;

                ooo_cpu[i].last_sim_instr = ooo_cpu[i].num_retired;
                ooo_cpu[i].last_sim_cycle = current_core_cycle[i];
            }

            // check for deadlock
            if (ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].ip && (ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].event_cycle + DEADLOCK_CYCLE) <= current_core_cycle[i])
                print_deadlock(i);

			if(ooo_cpu[i].ROB.occupancy == 0)
			{
				if(occupancy_zero_cycle == 0)
					occupancy_zero_cycle = current_core_cycle[i];

				if(occupancy_zero_cycle + DEADLOCK_CYCLE <= current_core_cycle[i])
				{
					print_deadlock(i);
				}
			}
			if(ooo_cpu[i].ROB.occupancy > 0)
				occupancy_zero_cycle = 0;


            // check for warmup
            // warmup complete
            if ((warmup_complete[i] == 0) && (ooo_cpu[i].num_retired > warmup_instructions)) {
                warmup_complete[i] = 1;
                all_warmup_complete++;
            }
            if (all_warmup_complete == NUM_CPUS) { // this part is called only once when all cores are warmed up
                all_warmup_complete++;
                finish_warmup();
            }

            /*
            if (all_warmup_complete == 0) { 
                all_warmup_complete = 1;
                finish_warmup();
            }
            if (ooo_cpu[1].num_retired > 0)
                warmup_complete[1] = 1;
            */
            
            // simulation complete
          
	    if(((all_warmup_complete > NUM_CPUS) && (simulation_complete[i] == 0) && (ooo_cpu[i].num_retired >= (ooo_cpu[i].begin_sim_instr + ooo_cpu[i].simulation_instructions)))) {
	    	simulation_complete[i] = 1;
                if(all_warmup_complete > NUM_CPUS)
			ooo_cpu[i].finish_sim_instr = ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr;
		else
			ooo_cpu[i].finish_sim_instr = ooo_cpu[i].num_retired;
                ooo_cpu[i].finish_sim_cycle = current_core_cycle[i] - ooo_cpu[i].begin_sim_cycle;

                cout << "Finished CPU " << i << " instructions: " << ooo_cpu[i].finish_sim_instr << " cycles: " << ooo_cpu[i].finish_sim_cycle;
                cout << " cumulative IPC: " << ((float) ooo_cpu[i].finish_sim_instr / ooo_cpu[i].finish_sim_cycle);
                cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << endl;

		//@Vishal: record cpu stats
		ooo_cpu[i].roi_RAW_hits = ooo_cpu[i].sim_RAW_hits;
		ooo_cpu[i].roi_load_gen = ooo_cpu[i].sim_load_gen;
		ooo_cpu[i].roi_load_sent = ooo_cpu[i].sim_load_sent;
		ooo_cpu[i].roi_store_gen = ooo_cpu[i].sim_store_gen;
                ooo_cpu[i].roi_store_sent = ooo_cpu[i].sim_store_sent;

                record_roi_stats(i, &ooo_cpu[i].ITLB);
                record_roi_stats(i, &ooo_cpu[i].DTLB);
                record_roi_stats(i, &ooo_cpu[i].STLB);
		record_roi_stats(i, &ooo_cpu[i].L1D);
                record_roi_stats(i, &ooo_cpu[i].L1I);
                record_roi_stats(i, &ooo_cpu[i].L2C);
                record_roi_stats(i, &uncore.LLC);
		#ifdef PUSH_DTLB_PB
                record_roi_stats(i, &ooo_cpu[i].DTLB_PB);
                #endif
		record_roi_stats(i, &ooo_cpu[i].BTB);				

		//MMU Caches
        	record_roi_stats(i, &ooo_cpu[i].PTW.PSCL5);
		record_roi_stats(i, &ooo_cpu[i].PTW.PSCL4);
		record_roi_stats(i, &ooo_cpu[i].PTW.PSCL3);
		record_roi_stats(i, &ooo_cpu[i].PTW.PSCL2);

                all_simulation_complete++;
            }

            if (all_simulation_complete == NUM_CPUS)
                run_simulation = 0;
       
       	}

        // TODO: should it be backward?
        uncore.LLC.operate();
        uncore.DRAM.operate();
    }

    uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time),
             elapsed_minute = elapsed_second / 60,
             elapsed_hour = elapsed_minute / 60;
    elapsed_minute -= elapsed_hour*60;
    elapsed_second -= (elapsed_hour*3600 + elapsed_minute*60);
    
    cout << endl << "ChampSim completed all CPUs" << endl;
    if (NUM_CPUS > 1) {
        cout << endl << "Total Simulation Statistics (not including warmup)" << endl;
        for (uint32_t i=0; i<NUM_CPUS; i++) {
            cout << endl << "CPU " << i << " cumulative IPC: " << (float) (ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) / (current_core_cycle[i] - ooo_cpu[i].begin_sim_cycle); 
            cout << " instructions: " << ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr << " cycles: " << current_core_cycle[i] - ooo_cpu[i].begin_sim_cycle << endl;
#ifndef CRC2_COMPILE

		ooo_cpu[i].core_final_stats();
	    print_sim_stats(i, &ooo_cpu[i].ITLB);
            print_sim_stats(i, &ooo_cpu[i].DTLB);
            print_sim_stats(i, &ooo_cpu[i].STLB);
            print_sim_stats(i, &ooo_cpu[i].L1D);
            print_sim_stats(i, &ooo_cpu[i].L1I);
#ifdef PERFECT_BTB
			print_sim_stats(i, &ooo_cpu[i].BTB);
#endif
            print_sim_stats(i, &ooo_cpu[i].L2C);
	    #ifdef PUSH_DTLB_PB
            print_sim_stats(i, &ooo_cpu[i].DTLB_PB);
            #endif

	    //MMU Caches
            print_sim_stats(i, &ooo_cpu[i].PTW.PSCL5);
            print_sim_stats(i, &ooo_cpu[i].PTW.PSCL4);
            print_sim_stats(i, &ooo_cpu[i].PTW.PSCL3);
            print_sim_stats(i, &ooo_cpu[i].PTW.PSCL2);

	    ooo_cpu[i].l1i_prefetcher_final_stats();
            ooo_cpu[i].L1D.l1d_prefetcher_final_stats();
            ooo_cpu[i].L2C.l2c_prefetcher_final_stats();
	    ooo_cpu[i].L2C.l2c_replacement_final_stats();
	    ooo_cpu[i].STLB.stlb_prefetcher_final_stats();
#endif
            print_sim_stats(i, &uncore.LLC);
	    
	    //@Vishal: print stats
	    cout<<endl;
	    cout<<"RAW hits: "<<ooo_cpu[i].sim_RAW_hits<<endl;
	    cout<<"Loads Generated: "<<ooo_cpu[i].sim_load_gen<<endl;
	    cout<<"Loads sent to L1D: "<<ooo_cpu[i].sim_load_sent<<endl;
	    cout<<"Stores Generated: "<<ooo_cpu[i].sim_store_gen<<endl;
            cout<<"Stores sent to L1D: "<<ooo_cpu[i].sim_store_sent<<endl;
        }
        uncore.LLC.llc_prefetcher_final_stats();
    }

    cout << endl << "Region of Interest Statistics" << endl;
    for (uint32_t i=0; i<NUM_CPUS; i++) {
        cout << endl << "CPU " << i << " cumulative IPC: " << ((float) ooo_cpu[i].finish_sim_instr / ooo_cpu[i].finish_sim_cycle); 
        cout << " instructions: " << ooo_cpu[i].finish_sim_instr << " cycles: " << ooo_cpu[i].finish_sim_cycle << endl;

#ifdef CAPTURE_DYNAMIC_ENERGY_PROFILE
	        print_energy_profile();
#endif

#ifndef CRC2_COMPILE
	ooo_cpu[i].core_final_stats();
	print_roi_stats(i, &ooo_cpu[i].ITLB);
        print_roi_stats(i, &ooo_cpu[i].DTLB);
        print_roi_stats(i, &ooo_cpu[i].STLB);
        print_roi_stats(i, &ooo_cpu[i].L1D);
        print_roi_stats(i, &ooo_cpu[i].L1I);
#ifndef PERFECT_BTB
		print_roi_stats(i, &ooo_cpu[i].BTB);
#endif
        print_roi_stats(i, &ooo_cpu[i].L2C);
	 #ifdef PUSH_DTLB_PB
        print_roi_stats(i, &ooo_cpu[i].DTLB_PB);
        #endif
	//MMU Caches
	print_roi_stats(i, &ooo_cpu[i].PTW.PSCL5);
	print_roi_stats(i, &ooo_cpu[i].PTW.PSCL4);
	print_roi_stats(i, &ooo_cpu[i].PTW.PSCL3);
	print_roi_stats(i, &ooo_cpu[i].PTW.PSCL2);
#endif
        print_roi_stats(i, &uncore.LLC);

	//@Vishal: print stats
        cout<<endl;
	cout<<"RAW hits: "<<ooo_cpu[i].roi_RAW_hits<<endl;
	cout<<"Loads Generated: "<<ooo_cpu[i].roi_load_gen<<endl;
	cout<<"Loads sent to L1D: "<<ooo_cpu[i].roi_load_sent<<endl;
	cout<<"Stores Generated: "<<ooo_cpu[i].roi_store_gen<<endl;
        cout<<"Stores sent to L1D: "<<ooo_cpu[i].roi_store_sent<<endl;
	
	cout << "Major fault: " << major_fault[i] << " Minor fault: " << minor_fault[i] << endl;
	cout<<"Allocated PAGES: "<< allocated_pages<<endl;
    }

    for (uint32_t i=0; i<NUM_CPUS; i++) {
        ooo_cpu[i].l1i_prefetcher_final_stats();
	ooo_cpu[i].L1D.l1d_prefetcher_final_stats();
        ooo_cpu[i].L2C.l2c_prefetcher_final_stats();
	ooo_cpu[i].DTLB.dtlb_prefetcher_final_stats();
	ooo_cpu[i].STLB.stlb_prefetcher_final_stats();
    }

    uncore.LLC.llc_prefetcher_final_stats();

#ifdef SANITY_CHECK
	for(uint32_t i = 0; i < NUM_CPUS; i++)
	{
		for(auto it: ooo_cpu[i].PTW.page_table)
		{
			for(uint32_t j = 0 ; j < NUM_CPUS; j++)
			{
				if(i == j)
				continue;
			
				for(auto jt: ooo_cpu[j].PTW.page_table)
					if(it.second == jt.second)
					{
						cout << "Same page allocated to virtual page of two different CPUs" << endl;
						cout << "CPU " << i << " and CPU " << j << " Physical PageL " << jt.second << endl;
						assert(0); 
					}
			}
		}
	}
#endif

#ifndef CRC2_COMPILE
    uncore.LLC.llc_replacement_final_stats();
    print_dram_stats();
    print_branch_stats();
#endif

    cout<<"DRAM PAGES: "<<DRAM_PAGES<<endl;
    cout<<"Allocated PAGES: "<<allocated_pages<<endl;


    //@Vasudha: Dumping page table
/*     map <uint64_t, uint64_t>::iterator pr = page_table.begin();
            for (pr = page_table.begin(); pr != page_table.end(); pr++) {
		cout << "PT: " << pr->first << " " << pr->second << endl;
            }*/
    return 0;
}
