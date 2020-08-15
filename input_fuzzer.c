#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

int count = 0;
int crashes = 0;
time_t start, now;
pthread_mutex_t lock;

typedef struct {
	char **corpus;
	char *target;
	char *target_flags;
	char *corpus_dir;
	char *crash_dir;
	int corpus_count;
} Thread_args;
	

size_t get_corpus(const char *path, char ***ls) {
	size_t count = 0;
	DIR *dp = NULL;
	struct dirent *ep = NULL;

	dp = opendir(path);
	if (dp == NULL) {
		fprintf(stderr, "No such directory: '%s'\n", path);
		return 0;
	}

	*ls = NULL;
	ep = readdir(dp);
	while (ep != NULL) {
		count++;
		ep = readdir(dp);
	}

	rewinddir(dp);
	*ls = calloc(count, sizeof(char *));

	count = 0;
	ep = readdir(dp);
	while (ep != NULL) {

		char *tmpstr = calloc(PATH_MAX + 1, sizeof(char));

		if (strcmp(ep->d_name, ".") != 0 && strcmp(ep->d_name, "..") != 0) {
			tmpstr = strcpy(tmpstr, path);
			strcat(tmpstr, ep->d_name);
			(*ls)[count++] = strdup(tmpstr);
		}
		ep = readdir(dp);

		free(tmpstr);
	}

	closedir(dp);
	return count;
}

void *fuzz(void *void_args) {
	pthread_mutex_lock(&lock);
	Thread_args *thread_args = (Thread_args*)void_args;
	pthread_mutex_unlock(&lock);

	char *chosen = NULL;

	for(;;) {
		// chose random file to input
		int r = rand() % thread_args->corpus_count;
		chosen = thread_args->corpus[r];

		// read file as char array
		FILE *fileptr;
		char *buffer;
		long filelen;
		fileptr = fopen(chosen, "rb");

		if (fileptr == NULL) {
			printf("fopen() failed: input = %s", chosen);
			printf("\t%s\n", strerror(errno));
			fclose(fileptr);
			break;
		}

		fseek(fileptr, 0, SEEK_END);
		filelen = ftell(fileptr);
		rewind(fileptr);

		buffer = (char *) malloc(filelen * sizeof(char));
		fread(buffer, filelen, 1, fileptr);
		
		fclose(fileptr);

		/* mutate 8 bytes in buffer */
		char mutation[8];

		// get 8 random bytes
		for (int i = 0; i < 8; i++) {
			mutation[i] = rand();
		}

		// put random bytes on random position in file
		for (int i = 0; i < 8; i++) {
			buffer[rand() % filelen] = mutation[i];
		}

		// write mutated file to disc
		char *mut_fname = calloc(PATH_MAX + 1, sizeof(char));
		char tmpstr[FILENAME_MAX + 1]; 

		mut_fname = strcpy(mut_fname, thread_args->crash_dir);

		strcat(mut_fname, "crash_");

		sprintf(tmpstr, "%d", count);
		strcat(mut_fname, tmpstr);

		FILE *mutated_file = fopen(mut_fname, "w");
		fwrite(buffer, sizeof(char), filelen, mutated_file);
		fclose(mutated_file);
		free(buffer);

		// run the mutated input through target program
		char command[_POSIX_ARG_MAX];
		strcpy(command, "timeout 1 ");
		strcat(command, thread_args->target);
		strcat(command, " ");
		strcat(command, thread_args->target_flags);
		strcat(command, " ");
		strcat(command, mut_fname);
		strcat(command, " > /dev/null 2>&1");
		int status = system(command);

		// if crash found
		if (status == 35584) {
			printf("Program exited with SIGSEV\n");
			pthread_mutex_lock(&lock);
			crashes++;
			pthread_mutex_unlock(&lock);
		} else {
			// if no crash, remove file from disc
			char rm[_POSIX_ARG_MAX];
			strcpy(rm, "rm ");
			strcat(rm, mut_fname);
			system(rm);
		}

		// report performance
		now = time(NULL) - start;
		int time_spent = now;
		printf("Time: %d\t", time_spent);
		printf("Cases: %d\t", count);
		printf("Crashes: %d\t", crashes);
		double fcps = (double)count / (double)time_spent;
		printf("FCPS: %f\n", fcps);

		free(mut_fname);

		pthread_mutex_lock(&lock);
		count++;
		pthread_mutex_unlock(&lock);
	}
	
	return NULL;
}

int main(int argc, char *argv[]) {
	start = time(NULL);

	if (pthread_mutex_init(&lock, NULL) != 0)
	{
		printf("Mutex init failed\n");
		return 1;
	}

	if (argc == 1) {
		printf("Usage: ./fuzz <threads> </path/to/target/binary> <\"flags\"> </path/to/corpus_dir/> </path/to/crashes_dir/>");
		exit(0);
	}

	int jobs           = atoi(argv[1]);
	char *target       = argv[2];
	char *target_flags = argv[3];
	char *corpus_dir   = argv[4];
	char *crash_dir    = argv[5];

	printf("****************************\n");
	printf("Threads engaged: %d\n", jobs);
	printf("Fuzzing target: %s\n", target);
	printf("Target flags: %s\n", target_flags);
	printf("Corpus located at: %s\n", corpus_dir);
	printf("Crashes will be stored in %s\n", crash_dir);
	printf("****************************\n");

	char **corpus;
	size_t corpus_count;

	corpus_count = get_corpus(corpus_dir, &corpus);

	Thread_args *thread_args  = malloc(sizeof(*thread_args));
	thread_args->corpus       = corpus;
	thread_args->corpus_count = corpus_count;
	thread_args->target       = target;
	thread_args->target_flags = target_flags;
	thread_args->corpus_dir   = corpus_dir;
	thread_args->crash_dir    = crash_dir;

	printf("Thread args at: %p\n", thread_args);
	printf("Corpus at: %p\n", thread_args->corpus);

	srand(time(NULL));

	// Multithreading incoming
	pthread_t thread_id[jobs];

	for (int i = 0; i < jobs; i++) {
		pthread_create(&thread_id[i], NULL, fuzz, thread_args);
	}

	for (int i = 0; i < jobs; i++) {
		pthread_join(thread_id[i], NULL);
	}

	for (int i = 0; i < corpus_count; i++) {
		free(corpus[i]);
	}

	pthread_mutex_destroy(&lock);
	free(corpus);
	free(thread_args);
}
