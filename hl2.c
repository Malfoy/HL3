#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <limits.h> // For INT_MAX
#include "xxhash.h"
#include "hl2.h"

#define ALPHA_INF (1.0 / (2.0 * log(2.0)))

uint64_t asm_log2(const uint64_t x) {
    if (x == 0) return 64;

    uint64_t tmp = x; // Utilisez une variable temporaire pour le décalage
    int n = 0;
    while ((tmp & (1ULL << 63)) == 0) {
        n = n + 1;
        tmp = tmp << 1;
    }
    return n;
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

// 2puissance le nombre de zero
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
    int zeros = 0;
    double sum = 0.0;

    for (size_t i = 0; i < (1 << hll->p); i++) {
        if (hll->registers[i] == 0) {
            zeros++;
        } else {
            sum += pow(2.0, -hll->registers[i]);
        }
    }

    if (zeros != 0) {
        // Utiliser la correction pour les petits ensembles
        estimate = (1 << hll->p) * log((double)(1 << hll->p) / zeros);
    } else {
        // Utiliser la formule standard HyperLogLog
        double alpha_m = ALPHA_INF * (1 << (hll->p)) * (1 << (hll->p));
        estimate = alpha_m / sum;
    }

    return estimate;
}


void bench_hyperlogloglog(uint16_t num_super_counters, uint16_t num_counters_per_super, uint64_t num_elements) {
    HyperLogLog* hll = createHyperLogLog(num_super_counters, num_counters_per_super);

    printf("Benchmark en cours...\n");

    clock_t start_time = clock();
    for (uint64_t i = 1; i <= num_elements; i++) {
        ajouter(hll, &i, sizeof(i));
        if (i % 1000000 == 0) {
            printf("Éléments insérés : %lu / %lu\n", i, num_elements);
        }
    }

    clock_t end_time = clock();
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    double cardinality = estimate_cardinality(hll);
    printf("Cardinalité estimée : %lf\n", cardinality);
    printf("Temps écoulé : %lf secondes\n", elapsed_time);

    destroyHyperLogLog(hll);
}



int main(int argc, char *argv[]) {
    uint64_t known_cardinality = 1000000; // exemple : 1000000 éléments uniques
    HyperLogLog* hll = createHyperLogLog(10, 8); // Ajustez p et q selon vos besoins

    for (uint64_t i = 1; i <= known_cardinality; i++) {
        ajouter(hll, &i, sizeof(i)); // Ajoute un élément à la structure HyperLogLog
    }

    bench_hyperlogloglog(10, 8, 1000000000);
    double estimated_cardinality = estimate_cardinality(hll);
    printf("Cardinalité connue : %lu, Cardinalité estimée : %lf\n", known_cardinality, estimated_cardinality);
    destroyHyperLogLog(hll);
    return 0;
}


