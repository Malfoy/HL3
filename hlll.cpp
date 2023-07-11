#include <iostream>
#include <cmath>
#include <vector>
#include <fstream>
#include <bitset>
#include "xxhash.h"
#include "hlll.h"



#define XXH_INLINE_ALL
#define XXH_FORCE_ALIGN_CHECK



using namespace std;





void printBinary(uint64_t number) {
    std::bitset<64> binary(number);
    std::cout << binary << '\n';
}







uint64_t murmur64(uint64_t h) {
  h ^= h >> 33;
  h *= 0xff51afd7ed558ccdL;
  h ^= h >> 33;
  h *= 0xc4ceb9fe1a85ec53L;
  h ^= h >> 33;
  return h;
}



uint64_t xorshift64star(uint64_t x) {
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    return x * 0x2545F4914F6CDD1DULL;
}



string intToString(uint64_t n) {
  if (n < 1000) {
    return to_string(n);
  }
  string end(to_string(n % 1000));
  if (end.size() == 3) {
    return intToString(n / 1000) + "," + end;
  }
  if (end.size() == 2) {
    return intToString(n / 1000) + ",0" + end;
  }
  return intToString(n / 1000) + ",00" + end;
}








void bench_hyperlogloglog(uint nb_hashes,uint64_t nb_inserted_element){
	HL3 hl3((nb_hashes));
	uint64_t print(1000);
	for(uint64_t ne(1);ne<nb_inserted_element;ne++){
		hl3.insert_key(ne);
		if(ne%print==0){
			hl3.display();
            cout<<hl3.get_cardinality()/ne<<endl;
            print*=2;
        }
	}
}

int main() {
    bench_hyperlogloglog(64,1000*1000*1000);
    return 0;
}
