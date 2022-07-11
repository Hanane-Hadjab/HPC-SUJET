#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_SIZE 20

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

void sort(int* arr, int start, int end) {
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
	sort(arr, start, left-1);
	sort(arr, right, end);
}


int main(int argc, char** argv) {
	
	int nbTask;
	int myRank;
	int array_temp_size;
	int* arr;
	int* array_temp;

	
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nbTask);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

	if(myRank == 0) {
		
		array_temp_size = ARRAY_SIZE / nbTask;
		
		if(ARRAY_SIZE % nbTask != 0) {
			array_temp_size++;
		}

		// ALlouer de la mémoire 
		MPI_Alloc_mem(array_temp_size * nbTask * sizeof(int), MPI_INFO_NULL, &arr);

		int z;
    	for (z = 0; z < ARRAY_SIZE; z++) {
        	arr[z] = ARRAY_SIZE - z;
    	}

		int i;
		for(i = ARRAY_SIZE; i < array_temp_size * nbTask; i++) {
			arr[i] = __INT_MAX__;
		}
	}

	// Synchroniser entre les processus 
	MPI_Barrier(MPI_COMM_WORLD);

	// Envoyer l'adresse de tableau à tous les autres processus
	MPI_Bcast(&array_temp_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

	MPI_Alloc_mem(array_temp_size * sizeof(int), MPI_INFO_NULL, &array_temp);

	// Envoyer le contenu de tableau arr vers le tableau array_temp
	MPI_Scatter(arr, array_temp_size, MPI_INT, array_temp, array_temp_size, MPI_INT, 0, MPI_COMM_WORLD);

	sort(array_temp, 0, array_temp_size-1);

	MPI_Barrier(MPI_COMM_WORLD);

	int i = 1;
	if(myRank == 0) {
		MPI_Alloc_mem(nbTask * array_temp_size * sizeof(int), MPI_INFO_NULL, &arr);
	}

	int status = MPI_Gather(array_temp, array_temp_size, MPI_INT, arr, array_temp_size, MPI_INT, 0, MPI_COMM_WORLD);

	MPI_Free_mem(array_temp);

	if(myRank == 0) {
		int *tab1;
		int *tab2;
		int *tab;

		// Allouer de la mémoire pour les deux tableux

		int size = ARRAY_SIZE / 2;

		MPI_Alloc_mem(size * sizeof(int), MPI_INFO_NULL, &tab1);

		MPI_Alloc_mem(size * sizeof(int), MPI_INFO_NULL, &tab2);

		MPI_Alloc_mem(ARRAY_SIZE * sizeof(int), MPI_INFO_NULL, &tab);
		
		for (int i = 0; i < ARRAY_SIZE / 2; ++i)
		{
			tab1[i] = arr[i];
		}

		int j = 0;
		for (int i = ARRAY_SIZE/2; i < ARRAY_SIZE; ++i)
		{
			tab2[j] = arr[i];
			++j;
		}

		printf("\n Affichage \n");

		printf("\n Le premier tableau \n");

		for (int l = 0; l < ARRAY_SIZE/2; l++) {
			printf("%d ", tab1[l]);
		}


		printf("\n Le deuxieme tableau \n");

		for (int k = 0; k < ARRAY_SIZE/2; k++) {
			printf("%d ", tab2[k]);
		}
		
		printf("Le tableau apres le tri est \n");

		for (int i = 0; i < ARRAY_SIZE/2; ++i)
		{
			tab[i] = tab2[i];
			printf(" tab de 2 %d \n", tab[i]);
		}
		int m = 0;
		for (int i = ARRAY_SIZE/2; i < ARRAY_SIZE; ++i)
		{
			tab[i] = tab1[m];
			++m;
			printf(" tab de !1 %d \n", tab[i]);
		}

		for (int l = 0; l < ARRAY_SIZE; l++) {
			printf("%d ", tab[l]);
		}
		printf("\n");
	}

	MPI_Finalize();

	return 0;
}
