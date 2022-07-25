#include "vberti.h"

#define LANZAR_INT 8

// Last edit: 27 - Sept - 2021 12:10

// FIFO queue
//#define SIZE_RR 16
//uint64_t RR[NUM_CPUS][SIZE_RR] = {0};
//uint64_t RR_cycle[NUM_CPUS][SIZE_RR] = {0};
//uint64_t RR_dx[NUM_CPUS] = {0};

void notify_prefetch(uint64_t addr, uint64_t tag, uint32_t cpu, uint64_t cycle)
{
    latency_table_add(addr, tag, cpu, 0, cycle & TIME_MASK);
}

bool compare_greater_stride_t(stride_t a, stride_t b)
{
    if (a.rpl == L1 && b.rpl != L1) return 1;
    else if (a.rpl != L1 && b.rpl == L1) return 0;
    else
    {
        if (a.rpl == L2 && b.rpl != L2) return 1;
        else if (a.rpl != L2 && b.rpl == L2) return 0;
        else
        {
            if (a.rpl == L2R && b.rpl != L2R) return 1;
            if (a.rpl != L2R && b.rpl == L2R) return 0;
            else
            {
                if (std::abs(a.stride) < std::abs(b.stride)) return 1;
                return 0;
            }
        }
    }
}

bool compare_greater_stride_t_per(stride_t a, stride_t b)
{
    if (a.per > b.per) return 1;
    else
    {
        if (std::abs(a.stride) < std::abs(b.stride)) return 1;
        return 0;
    }
}

/******************************************************************************/
/*                      Latency table functions                               */
/******************************************************************************/
void latency_table_init(uint32_t cpu)
{
    /*
     * Init pqmshr (latency) table
     *
     * Parameters:
     *      - cpu: cpu
     */
    for (uint32_t i = 0; i < LATENCY_TABLE_SIZE; i++)
    {
        latencyt[cpu][i].tag  = 0;
        latencyt[cpu][i].addr = 0;
        latencyt[cpu][i].time = 0;
        latencyt[cpu][i].pf   = 0;
    }
}

uint64_t latency_table_get_ip(uint64_t line_addr, uint32_t cpu)
{
    /*
     * Return 1 or 0 if the addr is or is not in the pqmshr (latency) table
     *
     * Parameters:
     *  - line_addr: address without cache offset
     *  - cpu: actual cpu
     *
     * Return: 1 if the line is in the latency table, otherwise 0
     */

    for (uint32_t i = 0; i < LATENCY_TABLE_SIZE; i++)
    {
        // Search if the line_addr already exists
        if (latencyt[cpu][i].addr == line_addr && latencyt[cpu][i].tag) 
            return latencyt[cpu][i].tag;
    }

    return 0;
}

uint8_t latency_table_add(uint64_t line_addr, uint64_t tag, uint32_t cpu, 
        uint8_t pf)
{
    /*
     * Save if possible the new miss into the pqmshr (latency) table
     *
     * Parameters:
     *  - line_addr: address without cache offset
     *  - cpu: actual cpu
     *  - access: is the entry accessed by a demand request
     */
    return latency_table_add(line_addr, tag, cpu, pf, current_core_cycle[cpu] & TIME_MASK);
}

uint8_t latency_table_add(uint64_t line_addr, uint64_t tag, uint32_t cpu, 
        uint8_t pf, uint64_t cycle)
{
    /*
     * Save if possible the new miss into the pqmshr (latency) table
     *
     * Parameters:
     *  - line_addr: address without cache offset
     *  - cpu: actual cpu
     *  - access: is theh entry accessed by a demand request
     *  - cycle: time to use in the latency table
     *
     * Return: 1 if the addr already exist, otherwise 0.
     */

    latency_table_t *free;
    free = nullptr;

    for (uint32_t i = 0; i < LATENCY_TABLE_SIZE; i++)
    {
        // Search if the line_addr already exists. If it exist we does not have
        // to do nothing more
        if (latencyt[cpu][i].addr == line_addr) 
        {
            latencyt[cpu][i].time = cycle;
            latencyt[cpu][i].tag  = tag;
            latencyt[cpu][i].pf   = pf;
            return latencyt[cpu][i].pf;
        }

        // We discover a free space into the latency table, save it for later
        //if (latencyt[cpu][i].addr == 0) free = &latencyt[cpu][i];
        if (latencyt[cpu][i].tag == 0) free = &latencyt[cpu][i];
    }

    // No free space!! This cannot be truth
    if (free == nullptr) return 0;

    // We save the new entry into the latency table
    free->addr = line_addr;
    free->time = cycle;
    free->tag  = tag;
    free->pf   = pf;

    return free->pf;
}

uint64_t latency_table_del(uint64_t line_addr, uint32_t cpu)
{
    /*
     * Remove the address from the latency table
     *
     * Parameters:
     *  - line_addr: address without cache offset
     *  - cpu: actual cpu
     *
     *  Return: the latency of the address
     */
    for (uint32_t i = 0; i < LATENCY_TABLE_SIZE; i++)
    {
        // Line already in the table
        if (latencyt[cpu][i].addr == line_addr)
        {
            uint64_t latency = (current_core_cycle[cpu] & TIME_MASK)
                - latencyt[cpu][i].time; // Calculate latency

            //latencyt[cpu][i].addr = 0; // Free the entry
            latencyt[cpu][i].tag  = 0; // Free the entry
            latencyt[cpu][i].time = 0; // Free the entry
            latencyt[cpu][i].pf   = 0; // Free the entry

            // Return the latency
            return latency;
        }
    }

    // We should always track the misses
    //assert(0);
    return 0;
}

uint64_t latency_table_get(uint64_t line_addr, uint32_t cpu)
{
    /*
     * Return 1 or 0 if the addr is or is not in the pqmshr (latency) table
     *
     * Parameters:
     *  - line_addr: address without cache offset
     *  - cpu: actual cpu
     *
     * Return: 1 if the line is in the latency table, otherwise 0
     */

    for (uint32_t i = 0; i < LATENCY_TABLE_SIZE; i++)
    {
        // Search if the line_addr already exists
        if (latencyt[cpu][i].addr == line_addr) return latencyt[cpu][i].time;
    }

    return 0;
}

/******************************************************************************/
/*                       Shadow Cache functions                               */
/******************************************************************************/
void shadow_cache_init(uint32_t cpu)
{
    /*
     * Init shadow cache
     *
     * Parameters:
     *      - cpu: cpu
     */
    for (uint8_t i = 0; i < L1D_SET; i++)
    {
        for (uint8_t ii = 0; ii < L1D_WAY; ii++)
        {
            scache[cpu][i][ii].addr = 0;
            scache[cpu][i][ii].lat  = 0;
            scache[cpu][i][ii].pf   = 0;
        }
    }
}

uint8_t shadow_cache_add(uint32_t cpu, uint32_t set, uint32_t way, 
        uint64_t line_addr, uint8_t pf, uint64_t latency)
{
    /*
     * Add block to shadow cache
     *
     * Parameters:
     *      - cpu: cpu
     *      - set: cache set
     *      - way: cache way
     *      - addr: cache block v_addr
     *      - access: the cache is access by a demand
     */
    scache[cpu][set][way].addr = line_addr;
    scache[cpu][set][way].pf   = pf;
    scache[cpu][set][way].lat  = latency;
    return scache[cpu][set][way].pf;
}

uint8_t shadow_cache_get(uint32_t cpu, uint64_t line_addr)
{
    /*
     * Init shadow cache
     *
     * Parameters:
     *      - cpu: cpu
     *      - addr: cache block v_addr
     *
     * Return: 1 if the addr is in the l1d cache, 0 otherwise
     */

    for (uint32_t i = 0; i < L1D_SET; i++)
    {
        for (uint32_t ii = 0; ii < L1D_WAY; ii++)
        {
            if (scache[cpu][i][ii].addr == line_addr) return 1;
        }
    }

    return 0;
}

uint8_t shadow_cache_pf(uint32_t cpu, uint64_t line_addr)
{
    /*
     * Init shadow cache
     *
     * Parameters:
     *      - cpu: cpu
     *      - addr: cache block v_addr
     *
     * Return: 1 if the addr is in the l1d cache, 0 otherwise
     */

    for (uint32_t i = 0; i < L1D_SET; i++)
    {
        for (uint32_t ii = 0; ii < L1D_WAY; ii++)
        {
            if (scache[cpu][i][ii].addr == line_addr) 
            {
                scache[cpu][i][ii].pf = 0;
                return 1;
            }
        }
    }

    return 0;
}

uint8_t shadow_cache_is_pf(uint32_t cpu, uint64_t line_addr)
{
    /*
     * Init shadow cache
     *
     * Parameters:
     *      - cpu: cpu
     *      - addr: cache block v_addr
     *
     * Return: 1 if the addr is in the l1d cache, 0 otherwise
     */

    for (uint32_t i = 0; i < L1D_SET; i++)
    {
        for (uint32_t ii = 0; ii < L1D_WAY; ii++)
        {
            if (scache[cpu][i][ii].addr == line_addr) return scache[cpu][i][ii].pf;
        }
    }

    return 0;
}

uint8_t shadow_cache_latency(uint32_t cpu, uint64_t line_addr)
{
    /*
     * Init shadow cache
     *
     * Parameters:
     *      - cpu: cpu
     *      - addr: cache block v_addr
     *
     * Return: 1 if the addr is in the l1d cache, 0 otherwise
     */

    for (uint32_t i = 0; i < L1D_SET; i++)
    {
        for (uint32_t ii = 0; ii < L1D_WAY; ii++)
        {
            if (scache[cpu][i][ii].addr == line_addr) return scache[cpu][i][ii].lat;
        }
    }
    assert(0);
    return 0;
}


/******************************************************************************/
/*                       History Table functions                               */
/******************************************************************************/
// Auxiliar history table functions
void history_table_init(uint32_t cpu)
{
    /*
     * Initialize history table pointers
     *
     * Parameters:
     *      - cpu: cpu
     */
    for (uint32_t i = 0; i < HISTORY_TABLE_SET; i++) 
    {
        // Pointer to the first element
        history_pointers[cpu][i] = historyt[cpu][i];

        for (uint32_t ii = 0; ii < HISTORY_TABLE_WAY; ii++) 
        {
            historyt[cpu][i][ii].tag = 0;
            historyt[cpu][i][ii].time = 0;
            historyt[cpu][i][ii].addr = 0;
        }
    }
}

void history_table_add(uint64_t tag, uint32_t cpu, uint64_t addr)
{
    /*
     * Save the new information into the history table
     *
     * Parameters:
     *  - tag: PC tag
     *  - cpu: actual cpu
     *  - addr: ip addr access
     */
    uint16_t set = tag & TABLE_SET_MASK;
    addr &= ADDR_MASK;

    uint64_t cycle = current_core_cycle[cpu] & TIME_MASK;
    // Save new element into the history table
    history_pointers[cpu][set]->tag       = tag;
    history_pointers[cpu][set]->time      = cycle;
    history_pointers[cpu][set]->addr      = addr;

    if (history_pointers[cpu][set] == &historyt[cpu][set][HISTORY_TABLE_WAY - 1])
    {
        history_pointers[cpu][set] = &historyt[cpu][set][0]; // End the cycle
    } else history_pointers[cpu][set]++; // Pointer to the next (oldest) entry
}

uint16_t history_table_get_aux(uint32_t cpu, uint32_t latency, 
        uint64_t tag, uint64_t act_addr, uint64_t ip[HISTORY_TABLE_WAY],
        uint64_t addr[HISTORY_TABLE_WAY], uint64_t cycle)
{
    uint16_t num_on_time = 0;
    uint16_t set = tag & TABLE_SET_MASK;

    // The IPs that is launch in this cycle will be able to launch this prefetch
    if (cycle < latency) return num_on_time;
    cycle -= latency; 

    // Pointer to guide
    history_table_t *pointer = history_pointers[cpu][set];

    do
    {
        // Look for the IPs that can launch this prefetch
        if (pointer->tag == tag && pointer->time <= cycle)
        {
            // Test that addr is not duplicated
            if (pointer->addr == act_addr) return num_on_time;

            int found = 0;
            for (int i = 0; i < num_on_time; i++)
            {
                if (pointer->addr == addr[i]) return num_on_time;
            }

            // This IP can launch the prefetch
            ip[num_on_time]   = pointer->tag;
            addr[num_on_time] = pointer->addr;
            num_on_time++;
        }

        if (pointer == historyt[cpu][set])
        {
            pointer = &historyt[cpu][set][HISTORY_TABLE_WAY - 1];
        } else pointer--;
    } while (pointer != history_pointers[cpu][set]);

    return num_on_time;
}

uint16_t history_table_get(uint32_t cpu, uint32_t latency, 
        uint64_t tag, uint64_t act_addr,
        uint64_t ip[HISTORY_TABLE_WAY],
        uint64_t addr[HISTORY_TABLE_WAY], 
        uint64_t cycle)
{
    /*
     * Return an array (by parameter) with all the possible PC that can launch
     * an on-time and late prefetch
     *
     * Parameters:
     *  - tag: PC tag
     *  - cpu: actual cpu
     *  - latency: latency of the processor
     *  - on_time_ip (out): ips that can launch an on-time prefetch
     *  - on_time_addr (out): addr that can launch an on-time prefetch
     *  - num_on_time (out): number of ips that can launch an on-time prefetch
     */

    act_addr &= ADDR_MASK;

    uint16_t num_on_time = history_table_get_aux(cpu, latency, tag, act_addr, 
            ip, addr, cycle);

    // We found on-time prefetchs
    return num_on_time;
}

/******************************************************************************/
/*                      Latency table functions                               */
/******************************************************************************/
// Auxiliar history table functions
void vberti_increase_conf_ip(uint64_t tag, uint32_t cpu)
{
    if (vbertit[cpu].find(tag) == vbertit[cpu].end()) return;

    vberti_t *tmp = vbertit[cpu][tag];
    stride_t *aux = tmp->stride;

    tmp->conf += CONFIDENCE_INC;

    if (tmp->conf == CONFIDENCE_MAX) 
    {

        // Max confidence achieve
        for(int i = 0; i < BERTI_TABLE_STRIDE_SIZE; i++)
        {
            float temp = (float) aux[i].conf / (float) tmp->conf;
            uint64_t aux_conf   = (uint64_t) (temp * 100);

            // Set bits
            if (aux_conf > CONFIDENCE_L1) aux[i].rpl = L1;
            else if (aux_conf > CONFIDENCE_L2) aux[i].rpl = L2;
            else if (aux_conf > CONFIDENCE_L2R) aux[i].rpl = L2R;
            else aux[i].rpl = R;
            
            aux[i].conf = 0;
        }

        tmp->conf = 0;
    }
}

void vberti_table_add(uint64_t tag, uint32_t cpu, int64_t stride)
{
    /*
     * Save the new information into the history table
     *
     * Parameters:
     *  - tag: PC tag
     *  - cpu: actual cpu
     *  - stride: actual cpu
     */
    if (vbertit[cpu].find(tag) == vbertit[cpu].end())
    {
        // FIFO MAP
        if (vbertit_queue[cpu].size() > BERTI_TABLE_SIZE)
        {
            uint64_t key = vbertit_queue[cpu].front();
            vberti_t *tmp = vbertit[cpu][key];
            delete tmp->stride;
            delete tmp;
            vbertit[cpu].erase(vbertit_queue[cpu].front());
            vbertit_queue[cpu].pop();
        }
        vbertit_queue[cpu].push(tag);

        assert(vbertit[cpu].size() <= BERTI_TABLE_SIZE);

        vberti_t *tmp = new vberti_t;
        tmp->stride = new stride_t[BERTI_TABLE_STRIDE_SIZE]();
        
        // Confidence IP
        tmp->conf = CONFIDENCE_INC;

        // Create new stride
        tmp->stride[0].stride = stride;
        tmp->stride[0].conf = CONFIDENCE_INIT;
        tmp->stride[0].rpl = R;

        // Save value
        vbertit[cpu].insert(make_pair(tag, tmp));
        return;
    }

    vberti_t *tmp = vbertit[cpu][tag];
    stride_t *aux = tmp->stride;

    // Increase IP confidence
    uint8_t max = 0;

    for (int i = 0; i < BERTI_TABLE_STRIDE_SIZE; i++)
    {
        if (aux[i].stride == stride)
        {
            aux[i].conf += CONFIDENCE_INC;
            if (aux[i].conf > CONFIDENCE_MAX) aux[i].conf = CONFIDENCE_MAX;
            return;
        }
    }

    uint8_t dx_conf = 100;
    int dx_remove = -1;
    for (int i = 0; i < BERTI_TABLE_STRIDE_SIZE; i++)
    {
        if (aux[i].rpl == R && aux[i].conf < dx_conf)
        {
            dx_conf = aux[i].conf;
            dx_remove = i;
        }
    }

    if (dx_remove > -1)
    {
        tmp->stride[dx_remove].stride = stride;
        tmp->stride[dx_remove].conf   = CONFIDENCE_INIT;
        tmp->stride[dx_remove].rpl    = R;
        return;
    } else
    {
        for (int i = 0; i < BERTI_TABLE_STRIDE_SIZE; i++)
        {
            if (aux[i].rpl == L2R && aux[i].conf < dx_conf)
            {
                dx_conf = aux[i].conf;
                dx_remove = i;
            }
            //if (aux[i].rpl == L2R)
            //{
            //    tmp->stride[i].stride = stride;
            //    tmp->stride[i].conf   = CONFIDENCE_INIT;
            //    tmp->stride[i].rpl    = R;
            //    return;
            //}
        }
        if (dx_remove > -1)
        {
            tmp->stride[dx_remove].stride = stride;
            tmp->stride[dx_remove].conf   = CONFIDENCE_INIT;
            tmp->stride[dx_remove].rpl    = R;
            return;
        }
    }
}

uint8_t vberti_table_get(uint64_t tag, uint32_t cpu, stride_t res[MAX_PF])
{
    /*
     * Save the new information into the history table
     *
     * Parameters:
     *  - tag: PC tag
     *  - cpu: actual cpu
     *
     * Return: the stride to prefetch
     */
    if (!vbertit[cpu].count(tag)) return 0;

    vberti_t *tmp = vbertit[cpu][tag];
    stride_t *aux = tmp->stride;
    uint64_t max_conf = 0;
    uint16_t dx = 0;
    
    for (int i = 0; i < BERTI_TABLE_STRIDE_SIZE; i++)
    {
        if (aux[i].stride != 0 && aux[i].rpl)
        {
            // Substitue min confidence for the next one
            res[dx].stride = aux[i].stride;
            res[dx].rpl = aux[i].rpl;
            dx++;
        }
    }

    if (dx == 0 && tmp->conf >= LANZAR_INT)
    {
        for (int i = 0; i < BERTI_TABLE_STRIDE_SIZE; i++)
        {
            if (aux[i].stride != 0)
            {
                // Substitue min confidence for the next one
                res[dx].stride = aux[i].stride;
                float temp = (float) aux[i].conf / (float) tmp->conf;
                uint64_t aux_conf   = (uint64_t) (temp * 100);
                res[dx].per = aux_conf;
                dx++;
            }
        }
        sort(res, res + MAX_PF, compare_greater_stride_t_per);

        for (int i = 0; i < MAX_PF; i++)
        {
            if (res[i].per > 80) res[i].rpl = L1;
            else if (res[i].per > 35) res[i].rpl = L2;
            //if (res[i].per > 80) res[i].rpl = L2;
            else res[i].rpl = R;
        }
        sort(res, res + MAX_PF, compare_greater_stride_t);
        return 1;
    }

    sort(res, res + MAX_PF, compare_greater_stride_t);

    return 1;
}

void find_and_update(uint32_t cpu, uint64_t latency, uint64_t tag, 
        uint64_t cycle, uint64_t line_addr)
{ 
    // We were tracking this miss
    uint64_t ip[HISTORY_TABLE_WAY];
    uint64_t addr[HISTORY_TABLE_WAY];
    uint16_t num_on_time = 0;

    // Get the IPs that can launch a prefetch
    num_on_time = history_table_get(cpu, latency, tag, line_addr, ip, addr, cycle);

    //vberti_increase_conf_ip(tag, cpu);
    
    for (uint32_t i = 0; i < num_on_time; i++)
    {
        // Increase conf ip
        if (i == 0) vberti_increase_conf_ip(tag, cpu);
        
        // Max number of strides that we can find
        if (i >= MAX_HISTORY_IP) break;

        // Add information into berti table
        int64_t stride;
        line_addr &= ADDR_MASK;

        // Usually applications go from lower to higher memory position.
        // The operation order is important (mainly because we allow
        // negative strides)
        stride = (int64_t) (line_addr - addr[i]);

        if ((std::abs(stride) < (1 << STRIDE_MASK)))
        {
            // Only useful strides
            vberti_table_add(ip[i], cpu, stride);
        }
    }
}

void CACHE::l1d_prefetcher_initialize() 
{
    shadow_cache_init(cpu);
    latency_table_init(cpu);
    history_table_init(cpu);

    std::cout << "History Sets: " << HISTORY_TABLE_SET << std::endl;
    std::cout << "History Ways: " << HISTORY_TABLE_WAY << std::endl;
    std::cout << "BERTI Size: " << BERTI_TABLE_SIZE << std::endl;
    std::cout << "BERTI Stride Size: " << BERTI_TABLE_STRIDE_SIZE << std::endl;
}

void CACHE::l1d_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit,
        uint8_t type, uint8_t critical_ip_flag)
{
    assert(type == LOAD || type == RFO);
    
    uint64_t line_addr = (addr >> LOG2_BLOCK_SIZE); // Line addr
    
    ip = ((ip >> 1) ^ (ip >> 4));
    //ip = (ip >> 1) ^ (ip >> 4) ^ (ip >> 8);
    ip = ip & IP_MASK;

    if (!cache_hit)
    {
        // This is a miss

        // Add @ to latency table
        latency_table_add(line_addr, ip, cpu, 1);

        // Add to history table
        history_table_add(ip, cpu, line_addr);

    } else if (cache_hit && shadow_cache_is_pf(cpu, line_addr))
    {
        // Cache line access
        shadow_cache_pf(cpu, line_addr);

        // Buscar strides Y actualizar
        uint64_t latency = shadow_cache_latency(cpu, line_addr);
        find_and_update(cpu, latency, ip, current_core_cycle[cpu] & TIME_MASK, 
                line_addr);

        history_table_add(ip, cpu, line_addr); 
    } else
    {
        // Cache line access
        shadow_cache_pf(cpu, line_addr);
        // No pf in hit
        //return;
    }

    // Get stride to prefetch
    stride_t stride[MAX_PF];
    for (int i = 0; i < MAX_PF; i++) 
    {
        stride[i].conf = 0;
        stride[i].stride = 0;
        stride[i].rpl = R;
    }

    if (!vberti_table_get(ip, cpu, stride)) return;

    int launched = 0;
    for (int i = 0; i < MAX_PF_LAUNCH; i++)
    {
        uint64_t p_addr = (line_addr + stride[i].stride) << LOG2_BLOCK_SIZE;
        uint64_t p_b_addr = (p_addr >> LOG2_BLOCK_SIZE);

        //if (!shadow_cache_get(cpu, p_b_addr)
        if (!latency_table_get(p_addr, cpu))
        {
            // Is in the RR
            //bool find_rr = false;
            //for (int ii = 0; ii < SIZE_RR && !find_rr; ii++) 
            //    find_rr = ((p_addr == RR[cpu][ii]));
            //if (find_rr) continue;


            int fill_level = FILL_L1;
            float mshr_load = ((float) MSHR.occupancy / (float) MSHR_SIZE) * 100;

            //if ((p_addr >> LOG2_PAGE_SIZE) != (addr >> LOG2_PAGE_SIZE))
            //    break;

            // Level of prefetching depends son CONFIDENCE
            if (stride[i].rpl == L1 && mshr_load < MSHR_LIMIT)
            {
                fill_level = FILL_L1;
            } else if (stride[i].rpl == L1 || stride[i].rpl == L2 
                    || stride[i].rpl == L2R ){
                fill_level = FILL_L2;
            } else
            {
                return;
            }

            if (prefetch_line(ip, addr, p_addr, fill_level, 1))
            {
                //if (!find_rr)
                //{
                //    RR[cpu][RR_dx[cpu]] = p_addr;
                //    RR_cycle[cpu][RR_dx[cpu]] = current_core_cycle[cpu];
                //    RR_dx[cpu]++;
                //    if (RR_dx[cpu] == SIZE_RR) RR_dx[cpu] = 0;
                //}
                launched++;
            }
        }
    }
}

void CACHE::l1d_prefetcher_notify_about_dtlb_eviction(uint64_t addr, 
        uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, 
        uint32_t metadata_in)
{

}

void CACHE::l1d_prefetcher_cache_fill(uint64_t v_addr, uint64_t addr, 
        uint32_t set, uint32_t way, uint8_t prefetch, uint64_t v_evicted_addr, 
        uint64_t evicted_addr, uint32_t metadata_in)
{
    uint64_t line_addr = (v_addr >> LOG2_BLOCK_SIZE); // Line addr
    uint64_t line_evicted = (v_evicted_addr >> LOG2_BLOCK_SIZE); // Line addr

    // Remove @ from latency table
    uint64_t tag     = latency_table_get_ip(line_addr, cpu);
    uint64_t cycle   = latency_table_get(line_addr, cpu);
    uint64_t latency = latency_table_del(line_addr, cpu);

    if (latency > LAT_MASK) latency = 0;

    // Add to the shadow cache
    shadow_cache_add(cpu, set, way, line_addr, prefetch, latency);

    if (latency != 0 && !prefetch)
    {
        find_and_update(cpu, latency, tag, cycle, line_addr);
    }

    //for (int ii = 0; ii < SIZE_RR; ii++) 
    //{
    //    if (RR[cpu][ii] == v_evicted_addr)
    //    {
    //        RR[cpu][ii] = 0;
    //    }
    //}
}

void CACHE::l1d_prefetcher_final_stats()
{
}
