#include <stdlib.h>
#include <stdio.h>

#define DIM1(array, type) (sizeof(array) / sizeof(type))

int DIM2(int *array) {
    return sizeof(array) / sizeof(int);
}

int main() {
    int arr[10];
    printf("The dimension of the array is %d\n", DIM1(arr, int));

    // O problema da DIM2 é que o array é passado como um ponteiro (pelo pre-compilador)
    // Logo, sizeof(array) vai retornar o tamanho do ponteiro e não do array.
    printf("The dimension of the array is %d\n", DIM2(arr));
    return 0;
}