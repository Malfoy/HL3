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



void printBinary(uint64_t number) {
    std::bitset<64> binary(number);
    std::cout << binary << '\n';
}



void get_hashes(const uint64_t key,uint H, uint minZeroes,vector<pair<uint,uint>>& result){
    result.clear();
    uint64_t seed(666);
    uint hash_id(0);
    uint remaining_bits(63);
    XXH64_hash_t hash = XXH64(&key, 8, seed);
    uint supplementaryzeroes(0);
    while(hash_id<H){
        if(hash==0){
            supplementaryzeroes=remaining_bits+1;
            remaining_bits=63;
            hash = XXH64(&key, 8, ++seed);
        }
        uint64_t rl(remaining_bits-asm_log2(hash)+supplementaryzeroes);
        supplementaryzeroes=0;
        if(rl > minZeroes){
            result.push_back({rl-minZeroes,hash_id});
        }
        remaining_bits-=(rl+1);
        hash-=(uint64_t)1<<(remaining_bits+1);
        hash_id++;
    }

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



void sketch_histogram(const vector<uint64_t>& S){
    vector<uint64_t> result;
    for(uint i(0);i<S.size();++i){
        if(S[i]+1>result.size()){
            result.resize(S[i]+1,0);
        }
        result[S[i]]++;
    }
    for(uint i(0);i<result.size();++i){
       for(uint j(0);j<result[i];++j){
            cout<<"*";
        } 
        cout<<"\n";
    }
    cout<<endl;
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


void bench_hyperlogloglog(uint nb_hashes,uint64_t nb_inserted_element){
    cout<<"I use "<<nb_hashes<<" to estimate card of "<<nb_inserted_element<<endl;
    uint nH(nb_hashes);
    uint min_value(0);
    uint min_value_occurence(nH);
    uint print(1000);
    vector<uint64_t> sketchsimple(nH);
    vector<pair<uint,uint>> hits;
    for(uint64_t ne(1);ne<nb_inserted_element;ne++){
        if(ne%print==0){
            vector<uint> histo(100,0);;
            uint64_t maxval(0);
            for(uint i(0);i<nH;++i){    
                histo[sketchsimple[i]]++;
                maxval=max(maxval,sketchsimple[i]);
            }
            for(uint i(0);i<=maxval;++i){    
                cout<< histo[i]<<" ";
            }
            cout<<endl;
            cout<<"exp: "<<min_value<<" "<<intToString(ne)<<":  "<<maxval+1<<endl;
            print*=10;
        }
        get_hashes(ne,nH,min_value,hits);
        for(uint32_t ih(0);ih<hits.size();++ih){
            if(hits[ih].first>sketchsimple[hits[ih].second]){
                if(hits[ih].first<63){
                    if(sketchsimple[hits[ih].second]==0){
                        min_value_occurence--;
                    }
                    sketchsimple[hits[ih].second]=hits[ih].first;
                }
            }
        }
        while(min_value_occurence==0){
            min_value++;
            for(uint32_t lih(0);lih<nH;++lih){
                sketchsimple[lih]--;
                if(sketchsimple[lih]==0){
                    min_value_occurence++;
                }
            }
        }
    }
    double estimation(0);
    for(uint32_t lih(0);lih<nH;++lih){
        estimation+=(double)1/(double)pow(2,1+min_value+sketchsimple[lih]);
    }
    cout<<alpha(nH)* nH /estimation<<endl;
    cout<<(alpha(nH)* nH /estimation)/nb_inserted_element<<endl;
}



int main() {
    bench_hyperlogloglog(100,10000000);
    return 0;
}