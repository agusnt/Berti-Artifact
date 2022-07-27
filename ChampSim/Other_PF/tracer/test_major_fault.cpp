#include<bits/stdc++.h>
int main()
{
	uint64_t size = 2UL*1024*1024*1024;
	int *arr = (int*)malloc(size*sizeof(int));
	while(1)
	{
		for(uint64_t i = 0; i < size; i+=1024)
			arr[i] = 0;
	}	
}
