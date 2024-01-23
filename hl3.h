#ifndef HL3_H
#define HL3_H

#include <stdint.h>

// Déclarations des structures et des fonctions

// Define the counter structure
typedef struct {
    uint8_t counter;  // Smaller word size counter
} Counter;

// Define the structure for super counters
typedef struct {
    uint32_t super_counter;
    Counter *counters; // Tableau de compteurs
} SuperCounter;

// Define the HLL structure
typedef struct {
    uint64_t mega_counter;
    SuperCounter *super_counters;
    Counter **counters;
    uint16_t num_super_counters;
    uint16_t num_counters_per_super;
} HLL;

// Prototypes des fonctions
void init_hll(HLL *hll, uint16_t num_super_counters, uint16_t num_counters_per_super);
void insert_key(HLL *hll, uint64_t key);
double estimate_cardinality(const HLL *hll);
void free_hll(HLL *hll);
void display(const HLL *hll);
uint16_t calculate_counter_index(int zero_count, uint16_t num_counters_per_super);

#endif // HL3_H

