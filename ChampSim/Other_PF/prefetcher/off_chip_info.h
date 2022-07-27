#ifndef __OFF_CHIP_INFO_H
#define __OFF_CHIP_INFO_H

#include <map>

#define STREAM_MAX_LENGTH 1024
#define STREAM_MAX_LENGTH_BITS 10

//#define DEBUG


//Neelu: Storage overhead: 
//PS_Entry: 64 bits key + 10 bits str_addr + 1 valid bit + 2 confidence bits = 77 bits.
class PS_Entry 
{
  public:
    unsigned int str_addr;
    bool valid;
    unsigned int confidence;

    PS_Entry() {
	reset();
    }

    void reset(){
        valid = false;
        str_addr = 0;
        confidence = 0;
    }
    void set(unsigned int addr){
        reset();
        str_addr = addr;
        valid = true;
        confidence = 3;
    }
    void increase_confidence(){
        confidence = (confidence == 3) ? confidence : (confidence+1);
    }
    bool lower_confidence(){
        confidence = (confidence == 0) ? confidence : (confidence-1);
        return confidence;
    }
};

//Neelu: Storage Overhead
//str_addr key 10 bits + phy_addr (we are using virtual) 64 bits + 1 valid bit = 75 bits

class SP_Entry 
{
  public:
    uint64_t phy_addr;
    bool valid;

    void reset(){
        valid = false;
        phy_addr = 0;
    }

    void set(uint64_t addr){
        phy_addr = addr;
        valid = true;
    }
};

class OffChipInfo
{
    public:
        std::map<uint64_t,PS_Entry*> ps_map;
        std::map<unsigned int,SP_Entry*> sp_map;

	//Neelu: Adding some stats.
	std::map<uint64_t, uint64_t> ps_map_access_frequency;
	std::map<unsigned int, uint64_t> sp_map_access_frequency;

	//Windows within which we are trying to capture the number of unique accesses to ps_map and sp_map. 
	uint64_t win1k_ps, win10k_ps, win100k_ps, win1m_ps, total_access_counter_ps, win1k_sp, win10k_sp, win100k_sp, win1m_sp, total_access_counter_sp;

	std::set<uint64_t> win1k_ps_access_keys, win10k_ps_access_keys, win100k_ps_access_keys, win1m_ps_access_keys;
	std::set<unsigned int> win1k_sp_access_keys, win10k_sp_access_keys, win100k_sp_access_keys, win1m_sp_access_keys;

	//Not total accesses, but unique accesses.
	std::map<uint64_t, uint64_t> win1k_ps_map_accesses, win10k_ps_map_accesses, win100k_ps_map_accesses, win1m_ps_map_accesses, win1k_sp_map_accesses, win10k_sp_map_accesses, win100k_sp_map_accesses, win1m_sp_map_accesses;

	//Neelu: Stats declaration ending.

        OffChipInfo()
        {
            reset();
	    win1k_ps = win10k_ps = win100k_ps = win1m_ps = win1k_sp = win10k_sp = win100k_sp = win1m_sp = 1;
	    total_access_counter_ps = total_access_counter_sp = 0;
        }

        void reset()
        {
            ps_map.clear();
            sp_map.clear();
        }
        bool get_structural_address(uint64_t phy_addr, unsigned int& str_addr)
        {
            std::map<uint64_t, PS_Entry*>::iterator ps_iter = ps_map.find(phy_addr);
            if(ps_iter == ps_map.end()) {
#ifdef DEBUG	
                (*outf)<<"In get_structural address of phy_addr "<<phy_addr<<", str addr not found\n";
#endif  
                return false;
            }
            else {
                if(ps_iter->second->valid) {
                    str_addr = ps_iter->second->str_addr;
		    
		    ps_map_access_frequency[phy_addr] = ps_map_access_frequency[phy_addr] + 1;
		    
		    win1k_ps_access_keys.insert(phy_addr);
		    win10k_ps_access_keys.insert(phy_addr); 
		    win100k_ps_access_keys.insert(phy_addr);
		    win1m_ps_access_keys.insert(phy_addr);

		    total_access_counter_ps++;

		    if(total_access_counter_ps % 1000 == 0)
		    {
			win1k_ps_map_accesses.insert(pair<uint64_t, uint64_t>(win1k_ps, win1k_ps_access_keys.size()));
		 	win1k_ps++;	 
			win1k_ps_access_keys.clear();
		    }
		    if(total_access_counter_ps % 10000 == 0)
		    {
			    win10k_ps_map_accesses.insert(pair<uint64_t, uint64_t>(win10k_ps, win10k_ps_access_keys.size()));
			    win10k_ps++;
			    win10k_ps_access_keys.clear();
		    }
		    if(total_access_counter_ps % 100000 == 0)
		    {                                                                           
			    win100k_ps_map_accesses.insert(pair<uint64_t, uint64_t>(win100k_ps, win100k_ps_access_keys.size()));
			    win100k_ps++;
			    win100k_ps_access_keys.clear();
		    }
		    if(total_access_counter_ps % 1000000 == 0)
		    {
			    win1m_ps_map_accesses.insert(pair<uint64_t, uint64_t>(win1m_ps, win1m_ps_access_keys.size()));
			    win1m_ps++;
			    win1m_ps_access_keys.clear();
		    }


#ifdef DEBUG    
                    (*outf)<<"In get_structural address of phy_addr "<<phy_addr<<", str addr is "<<str_addr<<"\n";
#endif
                    return true;
                }
                else {
#ifdef DEBUG    
                    (*outf)<<"In get_structural address of phy_addr "<<phy_addr<<", str addr not valid\n";
#endif
                    return false;
                }
            }			

        }

        bool get_physical_address(uint64_t& phy_addr, unsigned int str_addr)
        {
            std::map<unsigned int, SP_Entry*>::iterator sp_iter = sp_map.find(str_addr);
            if(sp_iter == sp_map.end()) {
#ifdef DEBUG    
                (*outf)<<"In get_physical_address of str_addr "<<str_addr<<", phy addr not found\n";
#endif
                return false;
            }
            else {
                if(sp_iter->second->valid) {
                    phy_addr = sp_iter->second->phy_addr;
		    sp_map_access_frequency[str_addr] = sp_map_access_frequency[str_addr] + 1;

		    win1k_sp_access_keys.insert(str_addr);
                    win10k_sp_access_keys.insert(str_addr);
                    win100k_sp_access_keys.insert(str_addr);
                    win1m_sp_access_keys.insert(str_addr);

                    total_access_counter_sp++;

                    if(total_access_counter_sp % 1000 == 0)
                    {
                        win1k_sp_map_accesses.insert(pair<uint64_t, uint64_t>(win1k_sp, win1k_sp_access_keys.size()));
                        win1k_sp++;
                        win1k_sp_access_keys.clear();
                    }
                    if(total_access_counter_sp % 10000 == 0)
                    {                                                                           
                            win10k_sp_map_accesses.insert(pair<uint64_t, uint64_t>(win10k_sp, win10k_sp_access_keys.size()));
                            win10k_sp++;
                            win10k_sp_access_keys.clear();
                    }
                    if(total_access_counter_sp % 100000 == 0)
                    {                                                                                           
			    win100k_sp_map_accesses.insert(pair<uint64_t, uint64_t>(win100k_sp, win100k_sp_access_keys.size()));
                            win100k_sp++;                                  
			    win100k_sp_access_keys.clear();
		    }
                    if(total_access_counter_sp % 1000000 == 0)
                    {                                                                           
                            win1m_sp_map_accesses.insert(pair<uint64_t, uint64_t>(win1m_sp, win1m_sp_access_keys.size()));
                            win1m_sp++;
                            win1m_sp_access_keys.clear();
                    }


#ifdef DEBUG    
                    (*outf)<<"In get_physical_address of str_addr "<<str_addr<<", phy addr is "<<phy_addr<<"\n";
#endif
                    return true;
                }
                else {
#ifdef DEBUG    
                    (*outf)<<"In get_physical_address of str_addr "<<str_addr<<", phy addr not valid\n";
#endif

                    return false;
                }
            }

        }
        void update(uint64_t phy_addr, unsigned int str_addr)
        {
#ifdef DEBUG    
            (*outf)<<"In off_chip_info update, phy_addr is "<<phy_addr<<", str_addr is "<<str_addr<<"\n";
#endif

            //PS Map Update
            std::map<uint64_t, PS_Entry*>::iterator ps_iter = ps_map.find(phy_addr);
            if(ps_iter == ps_map.end()) {
                PS_Entry* ps_entry = new PS_Entry();
                ps_map[phy_addr] = ps_entry;
                ps_map[phy_addr]->set(str_addr);
		ps_map_access_frequency[phy_addr] = 0;
            }
            else {
                ps_iter->second->set(str_addr);
            }	

            //SP Map Update
            std::map<unsigned int, SP_Entry*>::iterator sp_iter = sp_map.find(str_addr);
            if(sp_iter == sp_map.end()) {
                SP_Entry* sp_entry = new SP_Entry();
                sp_map[str_addr] = sp_entry;
                sp_map[str_addr]->set(phy_addr);
		sp_map_access_frequency[str_addr] = 0;
            }
            else {
                sp_iter->second->set(phy_addr);
            }	

        }

        void update_physical(uint64_t phy_addr, unsigned int str_addr)
        {
#ifdef DEBUG    
            std::cout <<"In off_chip_info update, phy_addr is "<<phy_addr<<", str_addr is "<<str_addr<<"\n";
#endif

            //PS Map Update
            std::map<uint64_t, PS_Entry*>::iterator ps_iter = ps_map.find(phy_addr);
            if(ps_iter == ps_map.end()) {
                PS_Entry* ps_entry = new PS_Entry();
                ps_map[phy_addr] = ps_entry;
                ps_map[phy_addr]->set(str_addr);
            }
            else {
                ps_iter->second->set(str_addr);
            }	
        }

        void update_structural(uint64_t phy_addr, unsigned int str_addr)
        {
#ifdef DEBUG    
            (*outf)<<"In off_chip_info update, phy_addr is "<<phy_addr<<", str_addr is "<<str_addr<<"\n";
#endif
            //SP Map Update
            std::map<unsigned int, SP_Entry*>::iterator sp_iter = sp_map.find(str_addr);
            if(sp_iter == sp_map.end()) {
                SP_Entry* sp_entry = new SP_Entry();
                sp_map[str_addr] = sp_entry;
                sp_map[str_addr]->set(phy_addr);
            }
            else {
                sp_iter->second->set(phy_addr);
            }	

        }

        void invalidate(uint64_t phy_addr, unsigned int str_addr)
        {
#ifdef DEBUG    
            (*outf)<<"In off_chip_info invalidate, phy_addr is "<<phy_addr<<", str_addr is "<<str_addr<<"\n";
#endif
            //PS Map Invalidate
            std::map<uint64_t, PS_Entry*>::iterator ps_iter = ps_map.find(phy_addr);
            if(ps_iter != ps_map.end()) {
                ps_iter->second->reset();
                delete ps_iter->second;
                ps_map.erase(ps_iter);
            }
            else {
                //TODO TBD
            }

            //SP Map Invalidate
            std::map<unsigned int, SP_Entry*>::iterator sp_iter = sp_map.find(str_addr);
            if(sp_iter != sp_map.end()) {
                sp_iter->second->reset();
                delete sp_iter->second;
                sp_map.erase(sp_iter);
            }
            else {
                //TODO TBD
            }

        }
        void increase_confidence(uint64_t phy_addr)
        {
#ifdef DEBUG    
            (*outf)<<"In off_chip_info increase_confidence, phy_addr is "<<phy_addr<<"\n";
#endif
            std::map<uint64_t, PS_Entry*>::iterator ps_iter = ps_map.find(phy_addr);
            if(ps_iter != ps_map.end()) {
                ps_iter->second->increase_confidence();
            }
            else {
                assert(0);
            }

        }

        bool lower_confidence(uint64_t phy_addr)
        {
            bool ret = false;

#ifdef DEBUG    
            (*outf)<<"In off_chip_info lower_confidence, phy_addr is "<<phy_addr<<"\n";
#endif

            std::map<uint64_t, PS_Entry*>::iterator ps_iter = ps_map.find(phy_addr);
            if(ps_iter != ps_map.end()) {
                ret = ps_iter->second->lower_confidence();
            }
            else {
                assert(0);
            }
            return ret;

        }
        //void reassign_stream(unsigned int str_addr, unsigned int new_str_addr);
	
	void print_stats()
	{
		/*for (auto i : win1k_ps_map_accesses) 
		{
			cout << "1k PS: " << i.first << " - " << i.second << endl;
		}
		for (auto i : win10k_ps_map_accesses)
                {
                        cout << "10k PS: " << i.first << " - " << i.second << endl;
                }
		for (auto i : win100k_ps_map_accesses)
                {
                        cout << "100k PS: " << i.first << " - " << i.second << endl;
                }
		for (auto i : win1m_ps_map_accesses)
                {
                        cout << "1m PS: " << i.first << " - " << i.second << endl;
                }
		for (auto i : win1k_sp_map_accesses)
                {
                        cout << "1k SP: " << i.first << " - " << i.second << endl;
                }
		for (auto i : win10k_sp_map_accesses)
                {
                        cout << "10k SP: " << i.first << " - " << i.second << endl;
                }
		for (auto i : win100k_sp_map_accesses)
                {
                        cout << "100k SP: " << i.first << " - " << i.second << endl;
                }
		for (auto i : win1m_sp_map_accesses)
                {
                        cout << "1m SP: " << i.first << " - " << i.second << endl;
                }*/


		uint64_t bucket[4] = {0}, stream_start_str_addr = 0, stream_end_str_addr = 0; 

		uint64_t stream_id = 0, next_stream_start_addr = STREAM_MAX_LENGTH, unreferenced50 = 0, unreferenced75 = 0, unreferenced90 = 0, stream_length_1 = 0, unreferenced50_ps_entries = 0, unreferenced75_ps_entries = 0, unreferenced90_ps_entries = 0;
		for (auto i : sp_map)
		{
			if(i.first >= next_stream_start_addr)
			{
				next_stream_start_addr += STREAM_MAX_LENGTH;
				uint64_t unreferenced_addr_counter = 0;		
				uint32_t stream_length = stream_end_str_addr - stream_start_str_addr + 1;
				assert(stream_length <= STREAM_MAX_LENGTH);
				if(stream_length > 0 && stream_end_str_addr != 0)
				{
					if(stream_length == 1)
						stream_length_1++;
					//cout << "Stream Start Addr: " << stream_start_str_addr << endl;
					//cout << "Stream End Addr: " << stream_end_str_addr << endl;
					//iterate over stream and check if references are zero.
					unsigned int stream_addr = stream_start_str_addr;
					for(int i = 0; i < stream_length; i++)
					{
						if(sp_map.find(stream_addr) != sp_map.end())
						{
							if(ps_map_access_frequency[sp_map[stream_addr]->phy_addr] == 0)
							{
								unreferenced_addr_counter++;
								//uint64_t first_one, second, third,
								uint64_t first_one = (stream_start_str_addr+stream_end_str_addr)/2;
								uint64_t second = (stream_start_str_addr+first_one)/2;
								uint64_t third = (first_one+1+stream_end_str_addr)/2;
								if(stream_addr < first_one)
								{
									//First half - 2 buckets.
									if(stream_addr < second)
										bucket[0]++;
									else
										bucket[1]++;
								}
								else
								{
									//Second half - 2 buckets.
									if(stream_addr < third)
										bucket[2]++;
									else
										bucket[3]++;
								}
							}	
						}
						stream_addr++;
					}
					//cout << " Num of unreferenced addresses in stream " << stream_id << " - " << unreferenced_addr_counter << " with stream length - " << stream_length << endl;
					if(unreferenced_addr_counter >= (stream_length/2))
					{
						unreferenced50++;
						unreferenced50_ps_entries += unreferenced_addr_counter;
					}
					if(unreferenced_addr_counter >= (stream_length*3/4))
					{
						unreferenced75++;
						unreferenced75_ps_entries += unreferenced_addr_counter;
					}
					if(unreferenced_addr_counter >= (stream_length*9/10))
					{
						unreferenced90++;
						unreferenced90_ps_entries += unreferenced_addr_counter;
					}

				}
				stream_start_str_addr = i.first;
				stream_id++;
			}
			stream_end_str_addr = i.first;
		}
		cout << " Num of streams " << stream_id-1 << " unreferenced50: " << unreferenced50 << " unreferenced75 " << unreferenced75 << " unreferenced90 " << unreferenced90 << endl;
		cout << " unreferenced50_ps_entries: " << unreferenced50_ps_entries << " unreferenced75_ps_entries: " << unreferenced75_ps_entries << " unreferenced90_ps_entries: " << unreferenced90_ps_entries << endl;

		cout << "Stream length 1 count: " << stream_length_1 << endl;

		for(int i = 0; i < 4; i++)
			cout << "Zero access bucket " << i << " freq: " << bucket[i] << endl;

		uint64_t sp_access_freq[6] = {0}, ps_access_freq[6] = {0};	//0-99, 100-499, 500-999, 1000-1499, 1500 or more.
		for (auto i : sp_map_access_frequency)
		{
			if(i.second >= 1 && i.second <= 24)
				sp_access_freq[0]++;
			else if(i.second >= 25 && i.second <= 49)
                                sp_access_freq[1]++;
			else if(i.second >= 50 && i.second <= 74)
                                sp_access_freq[2]++;
			else if(i.second >= 75 && i.second <= 99)
                                sp_access_freq[3]++;
			else if(i.second >= 100)
                                sp_access_freq[4]++;
			else if(i.second == 0)
				sp_access_freq[5]++;
		}

		for (auto i : ps_map_access_frequency)
                {
                        if(i.second >= 1 && i.second <= 24)
                                ps_access_freq[0]++;
                        else if(i.second >= 25 && i.second <= 49)
                                ps_access_freq[1]++;
                        else if(i.second >= 50 && i.second <= 74)
                                ps_access_freq[2]++;
                        else if(i.second >= 75 && i.second <= 99)
                                ps_access_freq[3]++;
                        else if(i.second >= 100)
                                ps_access_freq[4]++;
			else if(i.second == 0)
				ps_access_freq[5]++;
                }

	
		for( int i = 0; i < 6; i++)
		{
			cout << "PS Access Freq index - " << i << " value: " << ps_access_freq[i] << endl;
			cout << "SP Access Freq index - " << i << " value: " << sp_access_freq[i] << endl;
		}
   		
	}
	
};

#endif
