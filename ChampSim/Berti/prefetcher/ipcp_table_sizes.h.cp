//-------------------- Setting the sizes of IPCP prefetcher tables -----------------------
//Neelu: These are the two parameters of interest: CONFIG_1X and SCALE_ONLY_IP_TABLE


//CONFIG_1X keeps the original table sizes from the paper. CONFIG_2X will double the table sizes, and so on. 

//Valid Inputs: PT25X, PT5X, 1X, 2X, 4X, 8X, 16X, 32X, 64X, 128X, 256X. 


#define CONFIG_1X


// SCALE_ONLY_IP_TABLE will affect the size of only the IP_TABLE, and leave the other table sizes same as they are in the paper. 

#define SCALE_ONLY_IP_TABLE



//----------------- From here, the knobs are defined. Don't change the configs. You can add another config. ------------

#ifdef SCALE_ONLY_IP_TABLE
#define NUM_CSPT_ENTRIES 128                                // = 2^NUM_SIG_BITS                                                               
#define NUM_SIG_BITS 7                                      // num of bits in signature        
#define NUM_RST_ENTRIES 8 
#endif

// ------------------------------

#ifdef CONFIG_PT25X

#define NUM_IP_TABLE_L1_ENTRIES 16
#define NUM_IP_INDEX_BITS 4

#ifndef SCALE_ONLY_IP_TABLE
	#define NUM_CSPT_ENTRIES 32                                // = 2^NUM_SIG_BITS
	#define NUM_SIG_BITS 5                                      // num of bits in signature 
	#define NUM_RST_ENTRIES 2  	
#endif

#endif


// ------------------------------        

#ifdef CONFIG_PT5X 

	#define NUM_IP_TABLE_L1_ENTRIES 32 
        #define NUM_IP_INDEX_BITS 5   
#ifndef SCALE_ONLY_IP_TABLE             
        #define NUM_CSPT_ENTRIES 64                                // = 2^NUM_SIG_BITS 
        #define NUM_SIG_BITS 6                                      // num of bits in signature      
	#define NUM_RST_ENTRIES 4  
#endif        
#endif   


// ------------------------------ 

#ifdef CONFIG_1X

	#define NUM_IP_TABLE_L1_ENTRIES 64
	#define NUM_IP_INDEX_BITS 6
#ifndef SCALE_ONLY_IP_TABLE 
	#define NUM_CSPT_ENTRIES 128				   // = 2^NUM_SIG_BITS 
	#define NUM_SIG_BITS 7                                      // num of bits in signature
	#define NUM_RST_ENTRIES 8
#endif
#endif

// ------------------------------

#ifdef CONFIG_2X
	
	#define NUM_IP_TABLE_L1_ENTRIES 128
	#define NUM_IP_INDEX_BITS 7
#ifndef SCALE_ONLY_IP_TABLE 
	#define NUM_CSPT_ENTRIES 256                               // = 2^NUM_SIG_BITS
	#define NUM_SIG_BITS 8                                      // num of bits in signature   
	#define NUM_RST_ENTRIES 16
#endif
#endif

// ------------------------------

#ifdef CONFIG_4X

	#define NUM_IP_TABLE_L1_ENTRIES 256
	#define NUM_IP_INDEX_BITS 8
#ifndef SCALE_ONLY_IP_TABLE 
	#define NUM_CSPT_ENTRIES 512				   // = 2^NUM_SIG_BITS 
	#define NUM_SIG_BITS 9                                      // num of bits in signature 
	#define NUM_RST_ENTRIES 32
#endif
#endif

// ------------------------------

#ifdef CONFIG_8X

	#define NUM_IP_TABLE_L1_ENTRIES 512
	#define NUM_IP_INDEX_BITS 9
#ifndef SCALE_ONLY_IP_TABLE  
	#define NUM_CSPT_ENTRIES 1024                               // = 2^NUM_SIG_BITS
	#define NUM_SIG_BITS 10					      // num of bits in signature 
	#define NUM_RST_ENTRIES 64
#endif
#endif

// ------------------------------  

#ifdef CONFIG_16X
	
	#define NUM_IP_TABLE_L1_ENTRIES 1024
	#define NUM_IP_INDEX_BITS 10
#ifndef SCALE_ONLY_IP_TABLE 
	#define NUM_CSPT_ENTRIES 2048                               // = 2^NUM_SIG_BITS 
	#define NUM_SIG_BITS 11                                      // num of bits in signature  
	#define NUM_RST_ENTRIES 128
#endif
#endif

// ------------------------------  

#ifdef CONFIG_32X
	
	#define NUM_IP_TABLE_L1_ENTRIES 2048
	#define NUM_IP_INDEX_BITS 11
#ifndef SCALE_ONLY_IP_TABLE 
	#define NUM_CSPT_ENTRIES 4096                               // = 2^NUM_SIG_BITS 
	#define NUM_SIG_BITS 12                                      // num of bits in signature 
	#define NUM_RST_ENTRIES 256
#endif
#endif

// ------------------------------

#ifdef CONFIG_64X
	
	#define NUM_IP_TABLE_L1_ENTRIES 4096
	#define NUM_IP_INDEX_BITS 12
#ifndef SCALE_ONLY_IP_TABLE 
	#define NUM_CSPT_ENTRIES 8192                               // = 2^NUM_SIG_BITS 
	#define NUM_SIG_BITS 13                                      // num of bits in signature
	#define NUM_RST_ENTRIES 512
#endif
#endif

// ------------------------------ 

#ifdef CONFIG_128X 
	
	#define NUM_IP_TABLE_L1_ENTRIES 8192
	#define NUM_IP_INDEX_BITS 13
#ifndef SCALE_ONLY_IP_TABLE 
	#define NUM_CSPT_ENTRIES 16384                               // = 2^NUM_SIG_BITS 
	#define NUM_SIG_BITS 14                                      // num of bits in signature
	#define NUM_RST_ENTRIES 1024
#endif
#endif

// ------------------------------ 

#ifdef CONFIG_256X

	#define NUM_IP_TABLE_L1_ENTRIES 16384
	#define NUM_IP_INDEX_BITS 14
#ifndef SCALE_ONLY_IP_TABLE
	#define NUM_CSPT_ENTRIES 32768
	#define NUM_SIG_BITS 15                                      // num of bits in signature 
	#define NUM_RST_ENTRIES 2048
#endif
#endif
