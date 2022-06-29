#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_SIZE 16

void echange(int* arr, int i, int j) {
	int temp = arr[i];
	arr[i] = arr[j];
	arr[j] = temp;
}

void partition(int* arr, int start, int end, int* left, int* right) {
	int piv = arr[*left];
	*left = start;
	*right = start;
	int ubound = end;
	int temp;
	while(*right <= ubound) {
		if(arr[*right] < piv) {
			temp = arr[*left];
			arr[*left] = arr[*right];
			arr[*right] = temp;
			*left += 1;
			*right += 1;
		} else if(arr[*right] > piv) {
			temp = arr[*right];
			arr[*right] = arr[ubound];
			arr[ubound] = temp;
			ubound--;
		} else {
			*right += 1;
		}
	}
}

void quicksort(int* arr, int start, int end) {
	if(start >= end) {
		return;
	}

	int mid = end - (end-start)/2;
	int left;
	if(arr[start] <= arr[mid] && arr[start] <= arr[end]) {
		if(arr[mid] <= arr[end]) {
			left =  mid;
		} else {
			left = end;
		}
	} else if(arr[mid] <= arr[start] && arr[mid] <= arr[end]) {
		if(arr[start] <= arr[end]) {
			left = start;
		} else {
			left = end;
		}
	} else {
		if(arr[mid] <= arr[start]) {
			left = mid;
		} else {
			left = start;
		}
	}

	int right = left;
	partition(arr, start, end, &left, &right);
	quicksort(arr, start, left-1);
	quicksort(arr, right, end);
}


int next_gap(int gap) {
	if(gap <= 1) {
		return 0;
	}
	return (gap / 2) + (gap % 2);
}

void compare_split(int* self_arr, int size, int self_myRank, int rank1, int rank2) {
	int other_arr[size];

	MPI_Status status;

	MPI_Sendrecv(self_arr, size, MPI_INT, ((self_myRank == rank1) ? rank2 : rank1), 0, other_arr, size, MPI_INT, ((self_myRank == rank1) ? rank2 : rank1), 0, MPI_COMM_WORLD, &status);

	int i, j, gap = size + size;

	if(self_myRank == rank1) {
		for (gap = next_gap(gap); gap > 0; gap = next_gap(gap)) { 
			for (i = 0; i + gap < size; i++) {
				if (self_arr[i] > self_arr[i + gap]) { 
					echange(self_arr, i, i + gap); 
				}
			}

			for (j = gap > size ? gap-size : 0 ; i < size && j < size; i++, j++) { 
				if (self_arr[i] > other_arr[j]) {
					int temp = self_arr[i];
					self_arr[i] = other_arr[j];
					other_arr[j] = temp;
				}
			}
	
			if (self_myRank == rank2 && j < size) { 
				for (j = 0; j + gap < size; j++) { 
					if (other_arr[j] > other_arr[j + gap]) {
						echange(other_arr, j, j + gap);
					}
				}
			} 
		}
	} else {

		for (gap = next_gap(gap); gap > 0; gap = next_gap(gap)) { 
			for (i = 0; i + gap < size; i++) {
				if (other_arr[i] > other_arr[i + gap]) { 
					echange(other_arr, i, i + gap); 
				}
			}
	
			for (j = gap > size ? gap-size : 0 ; i < size && j < size; i++, j++) { 
				if (other_arr[i] > self_arr[j]) {
					int temp = other_arr[i];
					other_arr[i] = self_arr[j];
					self_arr[j] = temp;
				}
			}
	
			if (self_myRank == rank2 && j < size) { 
				for (j = 0; j + gap < size; j++) { 
					if (self_arr[j] > self_arr[j + gap]) {
						echange(self_arr, j, j + gap);
					}
				}
			} 
		}
	}
}

void populate(int n, int *array) {
    int i;
    for (i = 0; i < n; i++) {
        array[i] = n - i;
    }
}

void print(int n, int *array) {
    int i;
    for (i = 0; i < n; i++) {
        printf("%d ", array[i]);
    }
}

int main(int argc, char** argv) {
	
	int nbTask;
	int myRank;
	int chunk_size;
	int* arr;
	int* chunk;

	
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nbTask);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

	if(myRank == 0) {
		
		chunk_size = ARRAY_SIZE / nbTask;
		
		if(ARRAY_SIZE % nbTask != 0) {
			chunk_size++;
		}

		MPI_Alloc_mem(chunk_size * nbTask * sizeof(int), MPI_INFO_NULL, &arr);

		int i;

		populate(ARRAY_SIZE, arr);

		for(i = ARRAY_SIZE; i < chunk_size * nbTask; i++) {
			arr[i] = __INT_MAX__;
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);

	MPI_Bcast(&chunk_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

	MPI_Alloc_mem(chunk_size * sizeof(int), MPI_INFO_NULL, &chunk);

	MPI_Scatter(arr, chunk_size, MPI_INT, chunk, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);

	if(myRank == 0) {
		MPI_Free_mem(arr);
	}

	quicksort(chunk, 0, chunk_size-1);

	MPI_Barrier(MPI_COMM_WORLD);

	int i = 1;
	
	for(i = 0; i < nbTask; i++) {
		if(i%2 == 0) {
			if((myRank % 2 == 0) && (myRank < nbTask - 1)) {
				compare_split(chunk, chunk_size, myRank, myRank, myRank+1);
			} else if (myRank % 2 == 1) {
				compare_split(chunk, chunk_size, myRank, myRank-1, myRank);
			}
		} else {
			if((myRank % 2 == 1) && (myRank <= nbTask - 2)) {
				compare_split(chunk, chunk_size, myRank, myRank, myRank+1);
			} else if((myRank %2 == 0) && (myRank > 0)) {
				compare_split(chunk, chunk_size, myRank, myRank-1, myRank);
			}
		}
		MPI_Barrier(MPI_COMM_WORLD);
	}

	if(myRank == 0) {
		MPI_Alloc_mem(nbTask * chunk_size * sizeof(int), MPI_INFO_NULL, &arr);
	}

	int status = MPI_Gather(chunk, chunk_size, MPI_INT, arr, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);

	MPI_Free_mem(chunk);

	if(myRank == 0) {
		printf("le tableau apres le tri est \n");
		print(ARRAY_SIZE, arr);
		printf("\n");
	}

	MPI_Finalize();

	return 0;
}
