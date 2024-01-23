#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <omp.h>
#include "xxhash.h"
#include "hl3.h"

// Commenter cette ligne pour désactiver le débogage
//#define DEBUG

// Déclarations des variables globales
uint8_t *bitstream = NULL;
size_t bitstream_size = 0;
#define BASE 4

void generate_bitstream(uint64_t hash) {
	size_t index = 0;
	int zero_count = 0;

	bitstream = malloc(BASE * 8);
	if (bitstream == NULL) {
		fprintf(stderr, "Erreur d'allocation de mémoire pour bitstream.\n");
		exit(EXIT_FAILURE);
	}

	// Logique pour remplir bitstream
	while (hash != 0 && index < BASE * 8) {
		if (hash & 1) {  // Si le bit actuel est 1
			bitstream[index++] = zero_count;  // Enregistrez le nombre de zéros précédents
			zero_count = 0;
		} else {
			zero_count++;
		}
		hash >>= 1;  // Décalez pour vérifier le bit suivant
	}

	bitstream_size = index;
}


// Fonction de benchmark
void bench_hyperlogloglog(uint16_t num_super_counters, uint16_t num_counters_per_super, uint64_t num_elements) {
	HLL hll;
	init_hll(&hll, num_super_counters, num_counters_per_super);

	printf("Benchmark en cours...\n");

	clock_t start_time = clock();
	for (uint64_t i = 1; i <= num_elements; i++) {
		insert_key(&hll, i);
		if (i % 1000000 == 0) { // Afficher un message chaque million d'éléments
			printf("Éléments insérés : %lu / %lu\n", i, num_elements);
		}
	}

	clock_t end_time = clock();
	double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;

	double cardinality = estimate_cardinality(&hll);
	printf("Estimated Cardinality: %lf\n", cardinality);
	printf("Elapsed Time: %lf seconds\n", elapsed_time);

	free_hll(&hll);
}


int main(int argc, char *argv[]) {
	uint16_t num_super_counters = 1024;
	uint16_t num_counters_per_super = 1024;
	uint64_t num_elements = 10000000;

	// Test avec un ensemble de données connu
	uint64_t known_cardinality = 100000; // exemple : 100000 éléments uniques
	HLL hll;
	init_hll(&hll, num_super_counters, num_counters_per_super); // Passez les paramètres ici
	for (uint64_t i = 1; i <= known_cardinality; i++) {
		insert_key(&hll, i);
	}

	double estimated_cardinality = estimate_cardinality(&hll);
	printf("Cardinalité connue : %lu, Cardinalité estimée : %lf\n", known_cardinality, estimated_cardinality);

	bench_hyperlogloglog(num_super_counters, num_counters_per_super, num_elements);

	return 0;

}

void init_hll(HLL *hll, uint16_t num_super_counters, uint16_t num_counters_per_super) {
	hll->mega_counter = 0;
	hll->num_super_counters = num_super_counters;
	hll->num_counters_per_super = num_counters_per_super;

	// Allouer de la mémoire pour les super-compteurs
	hll->super_counters = malloc(num_super_counters * sizeof(SuperCounter));
	if (hll->super_counters == NULL) {
		fprintf(stderr, "Erreur d'allocation de mémoire pour les super-compteurs.\n");
		exit(EXIT_FAILURE);
	}

	// Initialiser chaque super-compteur
	for (int i = 0; i < num_super_counters; ++i) {
		hll->super_counters[i].super_counter = 0;

		// Allouer de la mémoire pour les compteurs dans chaque super-compteur
		hll->super_counters[i].counters = malloc(num_counters_per_super * sizeof(Counter));
		if (hll->super_counters[i].counters == NULL) {
			fprintf(stderr, "Erreur d'allocation de mémoire pour les super-compteurs.\n");
			exit(EXIT_FAILURE);
		}

		// Initialiser les compteurs
		for (int j = 0; j < num_counters_per_super; ++j) {
			hll->super_counters[i].counters[j].counter = 0;
		}
	}

}


void insert_key(HLL *hll, uint64_t key) {
	uint64_t hash = XXH64(&key, sizeof(key), 0);
#ifdef DEBUG
	printf("Key: %lu, Hash: %lu\n", key, hash);
#endif
	generate_bitstream(hash);  // Générer le flux de bits à partir du hachage

	for (size_t i = 0; i < bitstream_size; ++i) {
		int zero_count = bitstream[i];
		uint16_t super_index = hash % hll->num_super_counters;
		uint16_t counter_index = calculate_counter_index(zero_count, hll->num_counters_per_super);

		//for (int i = 0; i < 64; ++i) {
		if ((hash >> i) & 1) {
			if (zero_count > 0) {
#ifdef DEBUG
				printf("Zero Count: %d\n", zero_count);
				printf("Hash Bit: %ld\n", (hash >> i) & 1);
#endif

				SuperCounter *selected_super = &(hll->super_counters[super_index]);
				counter_index = calculate_counter_index(zero_count, hll->num_counters_per_super);

#ifdef DEBUG
				printf("Super Index: %u, Counter Index: %u\n", super_index, counter_index);
				printf("Before Update: Super Counter: %u, Counter: %u\n", 
						selected_super->super_counter, 
						selected_super->counters[counter_index].counter);
#endif

				// Mettre à jour le compteur si zero_count est plus grand que la valeur actuelle du compteur
				if (zero_count > selected_super->counters[counter_index].counter) {
					selected_super->counters[counter_index].counter = zero_count;
				}

				if (zero_count > selected_super->super_counter) {
					selected_super->super_counter = zero_count;
				}

				if (selected_super->super_counter > hll->mega_counter) {
					hll->mega_counter = selected_super->super_counter;
				}

#ifdef DEBUG
				printf("After Update: Super Counter: %u, Counter: %u\n", 
						selected_super->super_counter, 
						selected_super->counters[counter_index].counter);
#endif
			}
			zero_count = 0;
		} else {
			zero_count++;
		}
	}
	free(bitstream);
	bitstream = NULL; 
	}



	uint16_t calculate_counter_index(int zero_count, uint16_t num_counters_per_super) {
		int k = 0;
		while (num_counters_per_super >> k > zero_count) { // Calcul de l'indice en utilisant le logarithme base 2
			++k;
		}
		return k % num_counters_per_super; // Limitation de l'index à la taille maximale des conteneurs par super-conteur
	}

	double estimate_cardinality(const HLL *hll) {
		double alpha_m_square;  // Constante spécifique à l'algorithme HLL
		int m;                  // Nombre total de compteurs
		double sum = 0.0;       // Somme pour la moyenne harmonique

		m = hll->num_super_counters * hll->num_counters_per_super;

		if (m == 16) {
			alpha_m_square = 0.673;
		} else if (m == 32) {
			alpha_m_square = 0.697;
		} else if (m == 64) {
			alpha_m_square = 0.709;
		} else {
			alpha_m_square = 0.7213 / (1 + 1.079 / m);
		}

		// Calculer la somme harmonique
		for (int i = 0; i < hll->num_super_counters; ++i) {
			for (int j = 0; j < hll->num_counters_per_super; ++j) {
				sum += 1.0 / pow(2,hll->super_counters[i].counters[j].counter);
			}
		}

		// Retourner l'estimation de la cardinalité
		return alpha_m_square * m * m / sum;
	}

	void display(const HLL *hll) {
		printf("Mega Counter: %lu\n", hll->mega_counter); // Afficher le mega-compteur

		// Parcourir et afficher les super-compteurs et les compteurs
		for (uint16_t i = 0; i < hll->num_super_counters; ++i) {
			printf("Super Counter %d: %u\n", i, hll->super_counters[i].super_counter);
			printf("Counters: ");

			for (uint16_t j = 0; j < hll->num_counters_per_super; ++j) {
				printf("%u ", hll->super_counters[i].counters[j].counter);
			}

			printf("\n");
		}
	}

	void free_hll(HLL *hll) {
		// Libérer la mémoire pour chaque tableau de compteurs dans les super-compteurs
		for (int i = 0; i < hll->num_super_counters; ++i) {
			free(hll->super_counters[i].counters);
		}

		// Libérer la mémoire pour le tableau de super-compteurs
		free(hll->super_counters);
	}


