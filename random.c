#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

#define STATESIZE 128
#define NTHREADS 20
// How many iterations into the future do we want to look?
#define LOOKAHEAD 50

int * magic_number;
int magic_number_count;
unsigned int start_seed;

// custom structs
struct random_data_c {
	int32_t *fptr;              /* Front pointer.  */
	int32_t *rptr;              /* Rear pointer.  */
	int32_t *state;             /* Array of state values.  */
	int32_t *end_ptr;           /* Pointer behind state table.  */
};

int random_c(struct random_data_c * buf, int32_t * result) {
	int32_t *state;

	state = buf->state;

	int32_t *fptr = buf->fptr;
	int32_t *rptr = buf->rptr;
	int32_t *end_ptr = buf->end_ptr;
	int32_t val;

	val = *fptr += *rptr;
	/* Chucking least random bit.  */
	*result = (val >> 1) & 0x7fffffff;
	++fptr;
	if (fptr >= end_ptr) {
		fptr = state;
		++rptr;
	} else {
		++rptr;
		if (rptr >= end_ptr)
			rptr = state;
	}
	buf->fptr = fptr;
	buf->rptr = rptr;
	return 0;
}

int srandom_c(unsigned int seed, struct random_data_c * buf) {
	int32_t *state;
	long int i;
	int32_t word;
	int32_t *dst;
	int kc;

	state = buf->state;
	state[0] = seed;

	dst = state;
	word = seed;
	kc = 31; // buf->rand_deg
	for (i = 1; i < kc; ++i) {
		/* This does:
		state[i] = (16807 * state[i - 1]) % 2147483647;
		but avoids overflowing 31 bits.  */
		long int hi = word / 127773;
		long int lo = word % 127773;
		word = 16807 * lo - 2836 * hi;
		if (word < 0)
			word += 2147483647;
		*++dst = word;
	}

	buf->fptr = &state[3]; // buf->rand_sep
	buf->rptr = &state[0];
	kc *= 10;
	while (--kc >= 0) {
		int32_t discard;
		random_c(buf, &discard);
	}

	return 0;
}

int initstate_c(unsigned int seed, char * arg_state, struct random_data_c * buf) {
	int32_t *state = &((int32_t *) arg_state)[1]; /* First location.  */
	/* Must set END_PTR before srandom.  */
	buf->end_ptr = &state[31];

	buf->state = state;

	srandom_c(seed, buf);

	state[-1] = (buf->rptr - state) * 5 + 3;

	return 0;
}


void * thread_run(void * arg) {
        int thread_id = *((int *) arg);
        free(arg);

        struct random_data_c * rand_state;
        char * rand_statebuf;

        // Allocate memory for our random_data
        rand_state = malloc(sizeof(struct random_data_c));
        // Allocate memory for our rand_statebuf
        rand_statebuf = malloc(STATESIZE);

        // Loop over every seed we're meant to and re-initialize our state appropriately
	// Take away 1 from start_seed because a seed of 0 will work this way
        for (int64_t i = thread_id + start_seed - 1; i < (pow(2, 32) - 1); i += NTHREADS) {
                memset(rand_statebuf, 0, sizeof(*rand_statebuf));

                // Initialize our random buffer
                initstate_c(i, rand_statebuf, rand_state);

		// Generate random numbers a bunch of times
		for (int n = 0; n < LOOKAHEAD; n++) {
			// Generate a random value
			int random_number;
			int res;

			if ((res = random_c(rand_state, &random_number)) != 0) {
				printf("Failed to generate random number with error: %i\r\n", res);
				pthread_exit(NULL);
			}

			// See if it's the magic number
			for (int x = 0; x < magic_number_count; x++) {
				if (random_number == magic_number[x]) {
					printf("Identified magic number %i. Seed: %li. Iteration: %i (%i)\r\n", magic_number[x], i, n, thread_id);
					fflush(stdout);
					//exit(0);
				}
			}
		}
        }

        // Cleanup
        free(rand_state);
        free(rand_statebuf);

        return NULL;
}

int main (int argc, char *argv[]) {
        // Do our input parameter parsing
        if (argc != 3) {
                printf("Usage: ./random start-seed random-id-to-find other-id-to-find\r\n");
                return 1;
        }

	start_seed = atoi(argv[1]);
	printf("Start seed is %u\r\n", start_seed);

	magic_number_count = argc - 1;
	magic_number = malloc(magic_number_count * sizeof(magic_number[0]));

	for (int i = 0; i < magic_number_count; i++) {
		magic_number[i] = atoi(argv[i + 1]);
		printf("Magic number %i\r\n", magic_number[i]);
	}

	printf("Searching for magic numbers...\r\n");

        pthread_t * thread_ids;
        thread_ids = (pthread_t*)calloc(NTHREADS, sizeof(pthread_t));

        // Spin up the new threads
        for (int t = 1; t < NTHREADS + 1; t++) {
                int * arg = malloc(sizeof(*arg));
                *arg = t;
                pthread_create(&thread_ids[t], NULL, &thread_run, arg);
        }

        // Wait for them to finish
        for (int t = 1; t < NTHREADS + 1; t++) {
                pthread_join(thread_ids[t], NULL);
        }

        printf("Completed\r\n");

        return 0;
        // for seed srandom(1), we get random() = 1804289383
}

