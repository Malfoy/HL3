#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <limits.h> // For INT_MAX
#include "xxhash.h"
#include "hl2.h"

#define ALPHA_INF (1.0 / (2.0 * log(2.0)))
//#define DEBUG_ASM
#define DEBUG


void print_binary(uint64_t x) {
	for (int i = 63; i >= 0; i--) {
		uint64_t mask = 1ULL << i;
		uint64_t bit = (x & mask) >> i;
		putchar('0' + bit);
		if (i % 4 == 0 && i > 0) {
			putchar(' ');
		}
	}
}

uint64_t asm_log2(const uint64_t x) {
	if (x == 0) return 64;
	uint64_t tmp = x;
	int n = 0;
	while ((tmp & (1ULL << 63)) == 0) {
		n = n + 1;
		tmp = tmp << 1;
	}
#ifdef DEBUG_ASM
	printf("Nombre de zéros de tête pour %d: \n", n);
	print_binary(x);
	printf("\n");
#endif
	return n+1;
}

HyperLogLog* createHyperLogLog(unsigned char p, unsigned char q) {
	size_t size = (size_t)(1 << p);
	HyperLogLog* hll = malloc(sizeof(HyperLogLog));
	hll->p = p;
	hll->q = q;
	hll->registers = calloc(size, sizeof(unsigned char));
	hll->counts = calloc(q + 2, sizeof(int));
	hll->counts[0] = size;
	hll->minCount = 0;
	hll->registerValueFilter = ~((uint_fast64_t)0);
	return hll;
}

void destroyHyperLogLog(HyperLogLog* hll) {
	free(hll->registers);
	free(hll->counts);
	free(hll);
}

// Ajouter un élément à l'HyperLogLog
void ajouter(HyperLogLog* hll, const void* element, size_t longueur) {
	unsigned long hash = XXH64(element, longueur, 0);
	size_t index = hash % (1 << hll->p);
	int rang = (hash != 0) ? asm_log2(hash) : 64; // 64 est le nombre de bits dans uint64_t
	if (hll->registers[index] < rang) {
		hll->registers[index] = rang;
	}
}

// Fusionner deux HyperLogLogs
void merge(HyperLogLog* dest, const HyperLogLog* src) {
	size_t size = (1 << dest->p);
	for (size_t i = 0; i < size; i++) {
		if (src->registers[i] > dest->registers[i]) {
			dest->registers[i] = src->registers[i];
		}
	}
}



double estimate_cardinality(const HyperLogLog* hll) {
    double estimate = 0.0;
    double sum = 0.0;
    size_t i =0;
    for (i = 0; i < (1 << hll->p); i++) {
        sum += pow(2.0, -hll->registers[i]);
    }

#ifdef DEBUG
        printf("Valeur du registre [%zu] : %d, somme partielle : %f\n", i, hll->registers[i], sum);
#endif
    // Utiliser la formule standard HyperLogLog
    double alpha_m = (1<< hll->p);//ALPHA_INF * (1 << (hll->p)) * (1 << (hll->p));
#ifdef DEBUG
    printf("alpha_m : %f\n", alpha_m);
#endif
    estimate = alpha_m / sum;
#ifdef DEBUG
    printf("Estimation de la cardinalité : %f\n", estimate);
#endif

    return estimate;
}


void bench_hyperlogloglog(uint16_t num_super_counters, uint16_t num_counters_per_super, uint64_t num_elements) {
	HyperLogLog* hll = createHyperLogLog(num_super_counters, num_counters_per_super);

	printf("Benchmark en cours...\n");

	clock_t start_time = clock();
	for (uint64_t i = 1; i <= num_elements; i++) {
		ajouter(hll, &i, sizeof(i));
		if (i % 100000000 == 0) {
			printf("Éléments insérés : %lu / %lu\n", i, num_elements);
		}
	}

	clock_t end_time = clock();
	double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;

	double cardinality = estimate_cardinality(hll);
	printf("Cardinalité estimée : %lf\n", (cardinality/num_elements)*100);
	printf("Temps écoulé : %lf secondes\n", elapsed_time);

	destroyHyperLogLog(hll);
}



int main(int argc, char *argv[]) {

	uint64_t test_value = 0x02;
	printf("Test avec la valeur : %016lx\n", test_value);
	printf("Nombre de 0 :%ld\n",asm_log2(test_value));

	print_binary(test_value);
	printf("\n");
	asm_log2(test_value);
	
	bench_hyperlogloglog(10, 8, 1000000000);
	return 0;
}


