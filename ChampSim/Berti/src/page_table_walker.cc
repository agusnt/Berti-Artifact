#include "ooo_cpu.h"
#include "uncore.h"

void PAGE_TABLE_WALKER::operate()
{

#ifndef INS_PAGE_TABLE_WALKER
    assert(0);
#endif

    if(MSHR.occupancy > 0) //Handle pending request, only one request is serviced at a time.
    {
        if((MSHR.entry[MSHR.head].returned == COMPLETED) && (MSHR.entry[MSHR.head].event_cycle <= current_core_cycle[cpu])) //Check if current level translation complete
        {
            int index = MSHR.head;

            assert(CR3_addr != UINT64_MAX);
            PAGE_TABLE_PAGE* curr_page = L5; //Start wth the L5 page
            uint64_t next_level_base_addr = UINT64_MAX;
            bool page_fault = false;
            bool dropped_prefetch_request = false;

            for (int i = 5; i > MSHR.entry[index].translation_level; i--)
            {
                uint64_t offset = get_offset(MSHR.entry[index].full_virtual_address, i); //Get offset according to page table level
                assert(curr_page != NULL);
                next_level_base_addr = curr_page->next_level_base_addr[offset];
                if(next_level_base_addr == UINT64_MAX)
                {
                    if(MSHR.entry[index].type == PREFETCH_TRANSLATION || MSHR.entry[index].type == TRANSLATION_FROM_L1D)
                    {
                        //if(warmup_complete[cpu])
                        //	cout << "DROP" << endl;
                        //Prefetch request must be droppped in case of page fault
                        dropped_prefetch_request = true;
                        MSHR.entry[index].data = UINT64_MAX;
                        if (knob_cloudsuite)
                        {
                            if(MSHR.entry[index].instruction)
                                MSHR.entry[index].address = (( MSHR.entry[index].full_virtual_address >> LOG2_PAGE_SIZE) << 9) | ( 256 + MSHR.entry[index].asid[0]);
                            else
                            {
                                MSHR.entry[index].address = (( MSHR.entry[index].full_virtual_address >> LOG2_PAGE_SIZE) << 9) |  MSHR.entry[index].asid[1];
                            }
                        }
                        else
                        {
                            MSHR.entry[index].address = MSHR.entry[index].full_virtual_address >> LOG2_PAGE_SIZE;
                        }

                        MSHR.entry[index].full_addr = MSHR.entry[index].full_virtual_address;

                        if (MSHR.entry[index].instruction)
                            upper_level_icache[cpu]->return_data(&MSHR.entry[index]);
                        else // data
                            upper_level_dcache[cpu]->return_data(&MSHR.entry[index]);

                        MSHR.remove_queue(&MSHR.entry[index]);
                        return;
                    }
                    handle_page_fault(curr_page, &MSHR.entry[index], i); //i means next level does not exist.
                    page_fault = true;
                    MSHR.entry[index].translation_level = 0; //In page fault, All levels are translated.
                    break;
                }
                curr_page = curr_page->entry[offset];
            }

            if(MSHR.entry[index].translation_level == 0 && !dropped_prefetch_request) //If translation complete
            {
                curr_page = L5;
                next_level_base_addr = UINT64_MAX;
                for (int i = 5; i > 1; i--) //Walk the page table and fill MMU caches
                {
                    uint64_t offset = get_offset(MSHR.entry[index].full_virtual_address, i);
                    assert(curr_page != NULL);
                    next_level_base_addr = curr_page->next_level_base_addr[offset];
                    assert(next_level_base_addr != UINT64_MAX);
                    curr_page = curr_page->entry[offset];

                    if(MSHR.entry[index].init_translation_level - i >= 0) //Check which translation levels needs to filled
                    {
                        switch(i)
                        {
                            case 5: fill_mmu_cache(PSCL5, next_level_base_addr, &MSHR.entry[index], IS_PSCL5);
                                    break;
                            case 4: fill_mmu_cache(PSCL4, next_level_base_addr, &MSHR.entry[index], IS_PSCL4);
                                    break;
                            case 3: fill_mmu_cache(PSCL3, next_level_base_addr, &MSHR.entry[index], IS_PSCL3);
                                    break;
                            case 2: fill_mmu_cache(PSCL2, next_level_base_addr, &MSHR.entry[index], IS_PSCL2);
                                    break;
                        }
                    }
                }

                uint64_t offset = get_offset(MSHR.entry[index].full_virtual_address, IS_PTL1);
                next_level_base_addr = curr_page->next_level_base_addr[offset];

                if(page_fault)
                    MSHR.entry[index].event_cycle = stall_cycle[cpu]; //It is updated according to whether is required page swap or not.
                else
                    MSHR.entry[index].event_cycle = current_core_cycle[cpu];	


                MSHR.entry[index].data = next_level_base_addr << LOG2_PAGE_SIZE | (MSHR.entry[index].full_virtual_address && ((1<<LOG2_PAGE_SIZE) - 1)); //Return the translated physical address to STLB

                if (knob_cloudsuite)
                {
                    if(MSHR.entry[index].instruction)
                        MSHR.entry[index].address = (( MSHR.entry[index].full_virtual_address >> LOG2_PAGE_SIZE) << 9) | ( 256 + MSHR.entry[index].asid[0]);		
                    else
                    {
                        //cout << "PTW.MSHR.address: "<<hex<<MSHR.entry[index].address<< "full virtual address:"<<MSHR.entry[index].full_virtual_address << " asid[1]: " << MSHR.entry[index].asid[1] ;
                        MSHR.entry[index].address = (( MSHR.entry[index].full_virtual_address >> LOG2_PAGE_SIZE) << 9) |  MSHR.entry[index].asid[1];
                        //cout << " final address: " << hex << MSHR.entry[index].address << endl;
                    }
                }
                else
                {
                    MSHR.entry[index].address = MSHR.entry[index].full_virtual_address >> LOG2_PAGE_SIZE;
                }

                MSHR.entry[index].full_addr = MSHR.entry[index].full_virtual_address;

                if (MSHR.entry[index].instruction)
                    upper_level_icache[cpu]->return_data(&MSHR.entry[index]);
                else // data
                    upper_level_dcache[cpu]->return_data(&MSHR.entry[index]);

                if(warmup_complete[cpu])
                {
                    uint64_t current_miss_latency = (current_core_cycle[cpu] - MSHR.entry[index].cycle_enqueued);	
                    total_miss_latency += current_miss_latency;
                }

                MSHR.remove_queue(&MSHR.entry[index]);
            }
            else if(!dropped_prefetch_request)
            {
                assert(!page_fault); //If page fault was there, then all levels of translation should have be done.

                if((ooo_cpu[cpu].L2C.RQ.occupancy < ooo_cpu[cpu].L2C.RQ.SIZE)) //If L2 RQ has space then send the next level of translation.
                {
                    MSHR.entry[index].event_cycle = current_core_cycle[cpu];
                    MSHR.entry[index].full_addr = next_level_base_addr << LOG2_PAGE_SIZE | (get_offset(MSHR.entry[index].full_virtual_address, MSHR.entry[index].translation_level) << 3);
                    MSHR.entry[index].address = MSHR.entry[index].full_addr >> LOG2_BLOCK_SIZE;
                    MSHR.entry[index].returned = INFLIGHT;

                    int rq_index = ooo_cpu[cpu].L2C.add_rq(&MSHR.entry[index]);
                    if(rq_index != -1)
                    {
                        assert(0);
                    }
                    //assert(rq_index == -1);
                }
                else
                    rq_full++;
            }
        }
    }
    else if(RQ.occupancy > 0) //If there is no pending request which is undergoing translation, then process new request.
    {
        if((RQ.entry[RQ.head].event_cycle <= current_core_cycle[cpu]) && (ooo_cpu[cpu].L2C.RQ.occupancy < ooo_cpu[cpu].L2C.RQ.SIZE))
        {
            int index = RQ.head;

            //cout << hex << RQ.entry[index].full_addr << dec << endl;
            assert((RQ.entry[index].full_addr >> 32) != 0xf000000f); //Page table is stored at this address
            assert(RQ.entry[index].full_virtual_address != 0);

            uint64_t address_pscl5 = check_hit(PSCL5,get_index(RQ.entry[index].full_addr,IS_PSCL5),RQ.entry[index].type);
            uint64_t address_pscl4 = check_hit(PSCL4,get_index(RQ.entry[index].full_addr,IS_PSCL4),RQ.entry[index].type);
            uint64_t address_pscl3 = check_hit(PSCL3,get_index(RQ.entry[index].full_addr,IS_PSCL3),RQ.entry[index].type);
            uint64_t address_pscl2 = check_hit(PSCL2,get_index(RQ.entry[index].full_addr,IS_PSCL2),RQ.entry[index].type);


            PACKET packet = RQ.entry[index];

            packet.fill_level = FILL_L1; //@Vishal: This packet will be sent from L2 to PTW, TODO: check if this is done or not
            packet.cpu = cpu;
            packet.instr_id = RQ.entry[index].instr_id;
            packet.ip = RQ.entry[index].ip; // translation does not have ip
            packet.event_cycle = current_core_cycle[cpu];
            packet.full_virtual_address = RQ.entry[index].full_addr;

            uint64_t next_address = UINT64_MAX;

            if(address_pscl2 != UINT64_MAX)
            {
                next_address = address_pscl2 << LOG2_PAGE_SIZE | (get_offset(RQ.entry[index].full_addr,IS_PTL1) << 3);				
                packet.translation_level = 1;
            }
            else if(address_pscl3 != UINT64_MAX)
            {
                next_address = address_pscl3 << LOG2_PAGE_SIZE | (get_offset(RQ.entry[index].full_addr,IS_PTL2) << 3);				
                packet.translation_level = 2;
            }
            else if(address_pscl4 != UINT64_MAX)
            {
                next_address = address_pscl4 << LOG2_PAGE_SIZE | (get_offset(RQ.entry[index].full_addr,IS_PTL3) << 3);				
                packet.translation_level = 3;
            }
            else if(address_pscl5 != UINT64_MAX)
            {
                next_address = address_pscl5 << LOG2_PAGE_SIZE | (get_offset(RQ.entry[index].full_addr,IS_PTL4) << 3);				
                packet.translation_level = 4;
            }
            else
            {
                if(CR3_addr == UINT64_MAX)
                {
                    assert(!CR3_set); //This should be called only once when the process is starting
                    handle_page_fault(L5, &RQ.entry[index], 6); //6 means first level is also not there
                    CR3_set = true;

                    PAGE_TABLE_PAGE* curr_page = L5;
                    uint64_t next_level_base_addr = UINT64_MAX;
                    for (int i = 5; i > 1; i--) //Fill MMU caches
                    {
                        uint64_t offset = get_offset(RQ.entry[index].full_virtual_address, i);
                        assert(curr_page != NULL);
                        next_level_base_addr = curr_page->next_level_base_addr[offset];
                        assert(next_level_base_addr != UINT64_MAX); //Page fault serviced, all levels should be there.
                        curr_page = curr_page->entry[offset];

                        switch(i)
                        {
                            case 5: fill_mmu_cache(PSCL5, next_level_base_addr, &RQ.entry[index], IS_PSCL5);
                                    break;
                            case 4: fill_mmu_cache(PSCL4, next_level_base_addr, &RQ.entry[index], IS_PSCL4);
                                    break;
                            case 3: fill_mmu_cache(PSCL3, next_level_base_addr, &RQ.entry[index], IS_PSCL3);
                                    break;
                            case 2: fill_mmu_cache(PSCL2, next_level_base_addr, &RQ.entry[index], IS_PSCL2);
                                    break;
                        }
                    }

                    uint64_t offset = get_offset(RQ.entry[index].full_virtual_address, IS_PTL1);
                    next_level_base_addr = curr_page->next_level_base_addr[offset];

                    RQ.entry[index].event_cycle = current_core_cycle[cpu] + PAGE_TABLE_LATENCY;
                    RQ.entry[index].data = next_level_base_addr << LOG2_PAGE_SIZE | (RQ.entry[index].full_virtual_address && ((1<<LOG2_PAGE_SIZE) - 1));

                    if (RQ.entry[index].instruction)
                        upper_level_icache[cpu]->return_data(&RQ.entry[index]);
                    else // data
                        upper_level_dcache[cpu]->return_data(&RQ.entry[index]);

                    RQ.remove_queue(&RQ.entry[index]);

                    return;

                }
                next_address = CR3_addr << LOG2_PAGE_SIZE | (get_offset(RQ.entry[index].full_addr,IS_PTL5) << 3);				
                packet.translation_level = 5;
            }

            packet.init_translation_level = packet.translation_level;
            packet.address = next_address >> LOG2_BLOCK_SIZE;
            packet.full_addr = next_address;

            int rq_index = ooo_cpu[cpu].L2C.add_rq(&packet);
            //cout << rq_index<< endl; //@Vishal: Remove this
            /*if(rq_index < -1)
              {
              assert(0);
              }*/
            assert(rq_index == -1); //Packet should not merge as one translation is sent at a time.
            add_mshr(&packet);

            RQ.remove_queue(&RQ.entry[index]);
        }
    }
    else if(PQ.occupancy > 0) 
    {
        if((PQ.entry[PQ.head].event_cycle <= current_core_cycle[cpu]) && (ooo_cpu[cpu].L2C.RQ.occupancy < ooo_cpu[cpu].L2C.RQ.SIZE))
        {
            int index = PQ.head;
            //cout << hex << RQ.entry[index].full_addr << dec << endl;
            assert((PQ.entry[index].full_addr >> 32) != 0xf000000f); //Page table is stored at this address
            assert(PQ.entry[index].full_virtual_address != 0);

            uint64_t address_pscl5 = check_hit(PSCL5,get_index(PQ.entry[index].full_addr,IS_PSCL5),PQ.entry[index].type);
            uint64_t address_pscl4 = check_hit(PSCL4,get_index(PQ.entry[index].full_addr,IS_PSCL4),PQ.entry[index].type);
            uint64_t address_pscl3 = check_hit(PSCL3,get_index(PQ.entry[index].full_addr,IS_PSCL3),PQ.entry[index].type);
            uint64_t address_pscl2 = check_hit(PSCL2,get_index(PQ.entry[index].full_addr,IS_PSCL2),PQ.entry[index].type);

            PACKET packet = PQ.entry[index];
            packet.fill_level = FILL_L1; //@Vishal: This packet will be sent from L2 to PTW, TODO: check if this is done or not
            packet.cpu = cpu;
            packet.prefetch_id = PQ.entry[index].prefetch_id;
            packet.ip = PQ.entry[index].ip; // translation does not have ip
            packet.event_cycle = current_core_cycle[cpu];
            packet.full_virtual_address = PQ.entry[index].full_addr;

            uint64_t next_address = UINT64_MAX;

            if(address_pscl2 != UINT64_MAX)
            {
                next_address = address_pscl2 << LOG2_PAGE_SIZE | (get_offset(PQ.entry[index].full_addr,IS_PTL1) << 3);
                packet.translation_level = 1;
            }
            else if(address_pscl3 != UINT64_MAX)
            {
                next_address = address_pscl3 << LOG2_PAGE_SIZE | (get_offset(PQ.entry[index].full_addr,IS_PTL2) << 3);
                packet.translation_level = 2;
            }
            else if(address_pscl4 != UINT64_MAX)
            {
                next_address = address_pscl4 << LOG2_PAGE_SIZE | (get_offset(PQ.entry[index].full_addr,IS_PTL3) << 3);
                packet.translation_level = 3;
            }
            else if(address_pscl5 != UINT64_MAX)
            {
                next_address = address_pscl5 << LOG2_PAGE_SIZE | (get_offset(RQ.entry[index].full_addr,IS_PTL4) << 3);
                packet.translation_level = 4;
            }
            else {
                if(CR3_addr == UINT64_MAX)
                {
                    assert(!CR3_set); //This should be called only once when the process is starting
                    handle_page_fault(L5, &PQ.entry[index], 6); //6 means first level is also not there
                    CR3_set = true;

                    PAGE_TABLE_PAGE* curr_page = L5;
                    uint64_t next_level_base_addr = UINT64_MAX;
                    for (int i = 5; i > 1; i--) //Fill MMU caches
                    {
                        uint64_t offset = get_offset(PQ.entry[index].full_virtual_address, i);
                        assert(curr_page != NULL);
                        next_level_base_addr = curr_page->next_level_base_addr[offset];
                        assert(next_level_base_addr != UINT64_MAX); //Page fault serviced, all levels should be there.
                        curr_page = curr_page->entry[offset];

                        switch(i)
                        {
                            case 5: fill_mmu_cache(PSCL5, next_level_base_addr, &PQ.entry[index], IS_PSCL5);
                                    break;
                            case 4: fill_mmu_cache(PSCL4, next_level_base_addr, &PQ.entry[index], IS_PSCL4);
                                    break;
                            case 3: fill_mmu_cache(PSCL3, next_level_base_addr, &PQ.entry[index], IS_PSCL3);
                                    break;
                            case 2: fill_mmu_cache(PSCL2, next_level_base_addr, &PQ.entry[index], IS_PSCL2);
                                    break;
                        }
                    }

                    uint64_t offset = get_offset(PQ.entry[index].full_virtual_address, IS_PTL1);
                    next_level_base_addr = curr_page->next_level_base_addr[offset];
                    PQ.entry[index].event_cycle = current_core_cycle[cpu] + PAGE_TABLE_LATENCY;
                    PQ.entry[index].data = next_level_base_addr << LOG2_PAGE_SIZE | (RQ.entry[index].full_virtual_address && ((1<<LOG2_PAGE_SIZE) - 1));

                    if (PQ.entry[index].instruction)
                        upper_level_icache[cpu]->return_data(&PQ.entry[index]);
                    else // data
                        upper_level_dcache[cpu]->return_data(&PQ.entry[index]);

                    PQ.remove_queue(&PQ.entry[index]);

                    return;
                }
                next_address = CR3_addr << LOG2_PAGE_SIZE | (get_offset(PQ.entry[index].full_addr,IS_PTL5) << 3);
                packet.translation_level = 5;
            }
            packet.init_translation_level = packet.translation_level;
            packet.address = next_address >> LOG2_BLOCK_SIZE;
            packet.full_addr = next_address;

            int rq_index = ooo_cpu[cpu].L2C.add_rq(&packet);
            //cout << rq_index<< endl; //@Vishal: Remove this
            /*if(rq_index < -1)
              {
              assert(0);
              }*/
            assert(rq_index == -1); //Packet should not merge as one translation is sent at a time.
            add_mshr(&packet);

            PQ.remove_queue(&PQ.entry[index]);
        }
    }	

}

uint64_t PAGE_TABLE_WALKER::handle_page_fault(PAGE_TABLE_PAGE* page, PACKET *packet, uint8_t pt_level)
{
    bool page_swap = false;

    if(pt_level == 6)
    {
        assert(page == NULL && CR3_addr == UINT64_MAX);
        L5 = new PAGE_TABLE_PAGE();
        CR3_addr = map_translation_page(&page_swap);
        pt_level--;
        write_translation_page(CR3_addr, packet, pt_level);
        page = L5;
    }

    while(pt_level > 1)
    {
        uint64_t offset = get_offset(packet->full_virtual_address, pt_level);

        assert(page != NULL && page->entry[offset] == NULL);

        page->entry[offset] =  new PAGE_TABLE_PAGE();
        page->next_level_base_addr[offset] = map_translation_page(&page_swap);
        write_translation_page(page->next_level_base_addr[offset], packet, pt_level);
        page = page->entry[offset];
        pt_level--;
    }

    uint64_t offset = get_offset(packet->full_virtual_address, pt_level);

    assert(page != NULL && page->next_level_base_addr[offset] == UINT64_MAX);

    page->next_level_base_addr[offset] = map_data_page(packet->instr_id, packet->full_virtual_address, &page_swap);

    //This is done so that latency is added once, not five times
    if (page_swap)
        stall_cycle[cpu] = current_core_cycle[cpu] + SWAP_LATENCY;
    else
        stall_cycle[cpu] = current_core_cycle[cpu] + PAGE_TABLE_LATENCY; 

}

uint64_t PAGE_TABLE_WALKER :: va_to_pa_ptw(uint8_t cpu, uint64_t instr_id, bool translation_page, uint64_t va, uint64_t unique_vpage, bool *page_swap)
{

#ifdef SANITY_CHECK
    if (va == 0) 
        assert(0);
#endif

    uint64_t unique_va = va,
             vpage = unique_vpage ,
             voffset = unique_va & ((1<<LOG2_PAGE_SIZE) - 1);

    // smart random number generator
    uint64_t random_ppage;

    auto pr = page_table.find(vpage);
    auto ppage_check = inverse_table.begin();

    if(!translation_page)
    {
        assert(pr == page_table.end());

        // check unique cache line footprint
        map <uint64_t, uint64_t>::iterator cl_check = unique_cl[cpu].find(unique_va >> LOG2_BLOCK_SIZE);
        if (cl_check == unique_cl[cpu].end()) { // we've never seen this cache line before
            unique_cl[cpu].insert(make_pair(unique_va >> LOG2_BLOCK_SIZE, 0));
            num_cl[cpu]++;
        }
        else
            cl_check->second++;
    }

    if (pr == page_table.end()) { // no VA => PA translation found 

        if (allocated_pages >= DRAM_PAGES) { // not enough memory

            // TODO: elaborate page replacement algorithm
            // here, ChampSim randomly selects a page that is not recently used and we only track 32K recently accessed pages
            uint8_t  found_NRU = 0;
            uint64_t NRU_vpage = 0; // implement it
            //map <uint64_t, uint64_t>::iterator pr2 = recent_page.begin();
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
            *page_swap = true;
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
                if (ppage_check != inverse_table.end() || random_ppage == (UINT64_MAX >> LOG2_PAGE_SIZE)) { // random_ppage is not available //@Vishal: Don't allocale all FFFF page
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
            inverse_table.insert(make_pair(random_ppage, vpage));
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

        if (*page_swap)
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

    /*if (swap)
      stall_cycle[cpu] = current_core_cycle[cpu] + SWAP_LATENCY;
      else
      stall_cycle[cpu] = current_core_cycle[cpu] + PAGE_TABLE_LATENCY;*/

    //cout << "cpu: " << cpu << " allocated unique_vpage: " << hex << unique_vpage << " to ppage: " << ppage << dec << endl;
    //@Vasudha: Perfect DTLB Prefetcher - comment whole function and use the translation from dumped page table
    /*	uint64_t pa;
        auto it = temp_page_table.find(unique_vpage);
        assert(it != temp_page_table.end());
        pa = it->second;
        */
    return pa;
}

uint64_t PAGE_TABLE_WALKER::map_translation_page(bool *page_swap)
{
    uint64_t physical_address = va_to_pa_ptw(cpu, 0, true , next_translation_virtual_address, next_translation_virtual_address >> LOG2_PAGE_SIZE, page_swap);
    next_translation_virtual_address = ( (next_translation_virtual_address >> LOG2_PAGE_SIZE) + 1 ) << LOG2_PAGE_SIZE;

    return physical_address >> LOG2_PAGE_SIZE;


}

uint64_t PAGE_TABLE_WALKER::map_data_page(uint64_t instr_id, uint64_t full_virtual_address, bool *page_swap)
{
    uint64_t physical_address = va_to_pa_ptw(cpu, instr_id, false , full_virtual_address, full_virtual_address >> LOG2_PAGE_SIZE, page_swap);

    return physical_address >> LOG2_PAGE_SIZE;
}

void PAGE_TABLE_WALKER::write_translation_page(uint64_t next_level_base_addr, PACKET *packet, uint8_t pt_level)
{
    //@Vishal: Need to complete it, Problem: If lower level WQ is full, then what to do?
}

void PAGE_TABLE_WALKER::add_mshr(PACKET *packet)
{
    uint32_t index = 0;

    packet->cycle_enqueued = current_core_cycle[packet->cpu];

    MSHR.entry[0] = *packet;
    MSHR.entry[0].returned = INFLIGHT;
    MSHR.occupancy++;
}

void PAGE_TABLE_WALKER::fill_mmu_cache(CACHE &cache, uint64_t next_level_base_addr, PACKET *packet, uint8_t cache_type)
{
    cache.MSHR.entry[0].fill_level = 0;
    cache.MSHR.entry[0].cpu = cpu;
    cache.MSHR.entry[0].address = get_index(packet->full_virtual_address,cache_type);
    cache.MSHR.entry[0].full_addr = get_index(packet->full_virtual_address,cache_type);
    cache.MSHR.entry[0].data = next_level_base_addr;
    cache.MSHR.entry[0].instr_id = packet->instr_id;
    cache.MSHR.entry[0].ip = 0;
    cache.MSHR.entry[0].type = LOAD_TRANSLATION;
    cache.MSHR.entry[0].event_cycle = current_core_cycle[cpu];

    cache.MSHR.next_fill_index = 0;
    cache.MSHR.next_fill_cycle = current_core_cycle[cpu];

    cache.MSHR.occupancy = 1;
    cache.handle_fill();

    //cout << "["<< cache.NAME << "]" << "Miss: "<< cache.sim_miss[cpu][TRANSLATION] << endl;
}

uint64_t PAGE_TABLE_WALKER::get_index(uint64_t address, uint8_t cache_type)
{

    address = address & ( (1L<<57) -1); //Extract Last 57 bits

    int shift = 12;

    switch(cache_type)
    {
        case IS_PSCL5: shift+= 9+9+9+9;
                       break;
        case IS_PSCL4: shift+= 9+9+9;
                       break;
        case IS_PSCL3: shift+= 9+9;
                       break;
        case IS_PSCL2: shift+= 9; //Most siginificant 36 bits will be used to index PSCL2 
                       break;
    }

    return (address >> shift); 
}

uint64_t PAGE_TABLE_WALKER::check_hit(CACHE &cache, uint64_t address, uint8_t type)
{

    uint32_t set = cache.get_set(address);

    if (cache.NUM_SET < set) {
        cerr << "[" << NAME << "_ERROR] " << __func__ << " invalid set index: " << set << " NUM_SET: " << cache.NUM_SET;
        assert(0);
    }

    for (uint32_t way=0; way<cache.NUM_WAY; way++) {
        if (cache.block[set][way].valid && (cache.block[set][way].tag == address)) {

            // COLLECT STATS
            cache.sim_hit[cpu][type]++;
            cache.sim_access[cpu][type]++;


            return cache.block[set][way].data;
        }
    }

    return UINT64_MAX;
}

uint64_t PAGE_TABLE_WALKER::get_offset(uint64_t full_virtual_addr, uint8_t pt_level)
{
    full_virtual_addr = full_virtual_addr & ( (1L<<57) -1); //Extract Last 57 bits

    int shift = 12;

    switch(pt_level)
    {
        case IS_PTL5: shift+= 9+9+9+9;
                      break;
        case IS_PTL4: shift+= 9+9+9;
                      break;
        case IS_PTL3: shift+= 9+9;
                      break;
        case IS_PTL2: shift+= 9;
                      break;
    }

    uint64_t offset = (full_virtual_addr >> shift) & 0x1ff; //Extract the offset to generate next physical address

    return offset; 
}

int  PAGE_TABLE_WALKER::add_rq(PACKET *packet)
{
    // check for duplicates in the read queue
    int index = RQ.check_queue(packet);
    assert(index == -1); //@Vishal: Duplicate request should not be sent.

    // check occupancy
    if (RQ.occupancy == PTW_RQ_SIZE) {
        RQ.FULL++;

        return -2; // cannot handle this request
    }

    // if there is no duplicate, add it to RQ
    index = RQ.tail;

#ifdef SANITY_CHECK
    if (RQ.entry[index].address != 0) {
        cerr << "[" << NAME << "_ERROR] " << __func__ << " is not empty index: " << index;
        cerr << " address: " << hex << RQ.entry[index].address;
        cerr << " full_addr: " << RQ.entry[index].full_addr << dec << endl;
        assert(0);
    }
#endif

    RQ.entry[index] = *packet;

    // ADD LATENCY
    if (RQ.entry[index].event_cycle < current_core_cycle[packet->cpu])
        RQ.entry[index].event_cycle = current_core_cycle[packet->cpu] + LATENCY;
    else
        RQ.entry[index].event_cycle += LATENCY;

    RQ.occupancy++;
    RQ.tail++;
    if (RQ.tail >= RQ.SIZE)
        RQ.tail = 0;

    DP ( if (warmup_complete[RQ.entry[index].cpu]) {
            cout << "[" << NAME << "_RQ] " <<  __func__ << " instr_id: " << RQ.entry[index].instr_id << " address: " << hex << RQ.entry[index].address;
            cout << " full_addr: " << RQ.entry[index].full_addr << dec;
            cout << " type: " << +RQ.entry[index].type << " head: " << RQ.head << " tail: " << RQ.tail << " occupancy: " << RQ.occupancy;
            cout << " event: " << RQ.entry[index].event_cycle << " current: " << current_core_cycle[RQ.entry[index].cpu] << endl; });


#ifdef PRINT_QUEUE_TRACE
    if(packet->instr_id == QTRACE_INSTR_ID)
    {
        cout << "[" << NAME << "_RQ] " <<  __func__ << " instr_id: " << RQ.entry[index].instr_id << " address: " << hex << RQ.entry[index].address;
        cout << " full_addr: " << RQ.entry[index].full_addr << dec;
        cout << " type: " << +RQ.entry[index].type << " head: " << RQ.head << " tail: " << RQ.tail << " occupancy: " << RQ.occupancy;
        cout << " event: " << RQ.entry[index].event_cycle << " current: " << current_core_cycle[RQ.entry[index].cpu] << " cpu: "<<cpu<<endl;
    }
#endif


    if (packet->address == 0)
        assert(0);

    RQ.TO_CACHE++;
    RQ.ACCESS++;

    return -1;
}

int PAGE_TABLE_WALKER::add_wq(PACKET *packet)
{
    assert(0); //@Vishal: No request is added to WQ
}

int PAGE_TABLE_WALKER::add_pq(PACKET *packet)
{
    //check for duplicates in prefetch queue 
    int index = PQ.check_queue(packet);
    assert(index == -1);

    //check occupancy
    if(PQ.occupancy == PTW_PQ_SIZE) {
        PQ.FULL++;
        return -2;	//cannot handle this request
    }

    //if there is no duplicate, add it to PQ
    index = PQ.tail;

#ifdef SANITY_CHECK
    if(PQ.entry[index].address != 0) {
        cerr << "[" << NAME << "_ERROR] " << __func__ << " is not empty index: " << index;
        cerr << " address: " << hex << PQ.entry[index].address;
        cerr << " full_addr: " << hex << PQ.entry[index].full_addr << endl;
        assert(0);
    }
#endif

    PQ.entry[index] = *packet;

    //ADD LATENCY
    if(PQ.entry[index].event_cycle < current_core_cycle[packet->cpu])
        PQ.entry[index].event_cycle = current_core_cycle[packet->cpu] + LATENCY;
    else
        PQ.entry[index].event_cycle += LATENCY;

    PQ.occupancy++;
    PQ.tail++;
    if(PQ.tail >= PQ.SIZE)
        PQ.tail = 0;

    DP( if(warmup_complete[PQ.entry[index].cpu]) {
            cout << "[" << NAME << "_RQ]" << __func__ << "instr_id: " << PQ.entry[index].instr_id << " address: "<< hex << PQ.entry[index].address;
            cout << " full_addr: " << PQ.entry[index].full_addr << dec;
            cout << " type: " << +PQ.entry[index].type << " head: " << PQ.head << " tail: " << PQ.tail << " occupancy: " << PQ.occupancy;
            cout << " event: " << PQ.entry[index].event_cycle << " current: " << current_core_cycle[PQ.entry[index].cpu] << endl; });


#ifdef PRINT_QUEUE_TRACE
    if(packet->instr_id == QTRACE_INSTR_ID)
    {
        cout << "[" << NAME << "_PQ] " <<  __func__ << " instr_id: " << PQ.entry[index].instr_id << " address: " << hex << PQ.entry[index].address;		       
        cout << " full_addr: " << PQ.entry[index].full_addr << dec;
        cout << " type: " << +PQ.entry[index].type << " head: " << PQ.head << " tail: " << PQ.tail << " occupancy: " << PQ.occupancy;
        cout << " event: " << PQ.entry[index].event_cycle << " current: " << current_core_cycle[PQ.entry[index].cpu] << " cpu: "<<cpu<<endl;
    }
#endif

    if (packet->address == 0)
        assert(0);

    PQ.TO_CACHE++;
    return -1;
}

void PAGE_TABLE_WALKER::return_data(PACKET *packet)
{

    int mshr_index = -1;

    // search MSHR
    for (uint32_t index=0; index < MSHR.SIZE; index++) {
        if (MSHR.entry[index].address == packet->address) {
            mshr_index = index;
            break;
        }
    }

    // sanity check
    if (mshr_index == -1) {
        cerr << "[" << NAME << "_MSHR] " << __func__ << " instr_id: " << packet->instr_id << " cannot find a matching entry!";
        cerr << " full_addr: " << hex << packet->full_addr;
        cerr << " address: " << packet->address << dec;
        cerr << " event: " << packet->event_cycle << " current: " << current_core_cycle[packet->cpu] << endl;
        assert(0);
    }

    // MSHR holds the most updated information about this request
    // no need to do memcpy
    MSHR.num_returned++;
    MSHR.entry[mshr_index].returned = COMPLETED;

    assert(MSHR.entry[mshr_index].translation_level > 0);
    MSHR.entry[mshr_index].translation_level--;
}

void PAGE_TABLE_WALKER::increment_WQ_FULL(uint64_t address)
{
    WQ.FULL++;
}

uint32_t PAGE_TABLE_WALKER::get_occupancy(uint8_t queue_type, uint64_t address)
{
    if (queue_type == 0)
        return MSHR.occupancy;
    else if (queue_type == 1)
        return RQ.occupancy;
    else if (queue_type == 2)
        return WQ.occupancy;
    else if (queue_type == 3)
        return PQ.occupancy;
    return 0;
}

uint32_t PAGE_TABLE_WALKER::get_size(uint8_t queue_type, uint64_t address)
{
    if (queue_type == 0)
        return MSHR.SIZE;
    else if (queue_type == 1)
        return RQ.SIZE;
    else if (queue_type == 2)
        return WQ.SIZE;
    else if (queue_type == 3)
        return PQ.SIZE;
    return 0;
}
