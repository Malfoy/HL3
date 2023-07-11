#include <iostream>
#include <cmath>
#include <vector>
#include <fstream>
#include <bitset>
#include "xxhash.h"



#define XXH_INLINE_ALL
#define XXH_FORCE_ALIGN_CHECK



using namespace std;



uint64_t asm_log2(const uint64_t x) {
	uint64_t y;
	asm("\tbsr %1, %0\n" : "=r"(y) : "r"(x));
	return y;
}


static double alpha(int m) {
    switch(m) {
    case 16:
    return 0.673;
    case 32:
    return 0.697;
    case 64:
    return 0.709;
    default:
    return 0.7213 / (1.0 + 1.079/m);
    }
}



class HL3{
	public:
		uint32_t M;
		uint8_t Offset;
		uint8_t *Sketch;
		uint32_t Z_occurence;
		uint8_t max_value;
		vector<pair<int8_t,uint32_t>> hits;



		HL3(uint8_t m){
			M=m;
			Z_occurence=M;
			Sketch=new uint8_t[m];
			Offset=0;
			max_value=3;//2bit per register
		}



		~HL3(){
			delete[] Sketch;
		}



		void display(){
			uint8_t maximum(0);
			cout<<(uint)Offset;
			for(uint32_t lih(0);lih<M;++lih){
				cout<<" "<<(uint)Sketch[lih];
				if(maximum<Sketch[lih]){
					maximum=Sketch[lih];
				}
			}
			cout<<"MAX: "<<(uint)maximum<<endl;
		}



		double get_cardinality(){
			double estimation(0);
			for(uint32_t lih(0);lih<M;++lih){
				estimation+=(double)1/(double)pow(4,1+Offset+Sketch[lih]);
			}
			return M /(1.86*estimation);
		}



		uint8_t increment_offset(bool force){
			uint8_t result(0);
			if(force){
				Offset++;
				result++;
				Z_occurence=0;
				for(uint32_t lih(0);lih<M;++lih){
					if(Sketch[lih]!=0){
						Sketch[lih]--;
					}
					if(Sketch[lih]==0){
						Z_occurence++;
					}
				}
			}
			while(Z_occurence==0){
				Offset++;
				result++;
				for(uint32_t lih(0);lih<M;++lih){
					Sketch[lih]--;
					if(Sketch[lih]==0){
						Z_occurence++;
					}
				}
			}
			return result;
		}



		void get_hashes(const uint64_t key, uint minZeroes){
			hits.clear();
			uint64_t seed(666);
			uint32_t hash_id(0);
			uint8_t remaining_bits(63);
			XXH64_hash_t hash = XXH64(&key, 8, seed);
			uint8_t supplementaryzeroes(0);
			while(hash_id<M){
				if(hash==0){
					supplementaryzeroes=remaining_bits+1;
					remaining_bits=63;
					hash = XXH64(&key, 8, ++seed);
				}
				int64_t rl(remaining_bits-asm_log2(hash)+supplementaryzeroes);
				supplementaryzeroes=0;
				if(rl/2 > minZeroes){//BASE4
					hits.push_back({rl/2-minZeroes,hash_id});//BASE4
				}
				remaining_bits-=(rl+1);
				hash-=(uint64_t)1<<(remaining_bits+1);
				hash_id++;
			}
		}



		void insert_key(uint64_t x){
			get_hashes(x,Offset);

			for(uint32_t ih(0);ih<hits.size();++ih){
				while(hits[ih].first>max_value){
					uint8_t delta=increment_offset(true);
					for(uint32_t lih(ih);lih<hits.size();++lih){
						hits[ih].first-=delta;
					}
				}
				if(hits[ih].first>Sketch[hits[ih].second]){
					if(Sketch[hits[ih].second]==0){
						Z_occurence--;
					}
					Sketch[hits[ih].second]=hits[ih].first;
				}
			}
			increment_offset(false);
		}
};
