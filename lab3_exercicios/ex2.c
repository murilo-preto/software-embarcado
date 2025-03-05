// O que faz o codigo seguinte? Qual sua utilidade num ambiente com reduzida
// capacidade de memória?

// O cógido troca o valor das duas variáveis sem ocupar nenhuma variável adicional
// Ademais, são utilizadas operações BITWISE (eficientes)

#include <stdio.h>
#include <stdlib.h>

void swap(int *a, int *b);

main() {
    int x = 10, y = 8;
    swap(&x, &y);

    printf("x=%d y=%d", x, y);
}


/**
 * @brief Swaps the values of two integers using bitwise XOR operation.
 *
 * This function takes two pointers to integers and swaps their values
 * without using a temporary variable. It uses the bitwise XOR operation
 * to achieve the swap in three steps:
 * 1. *a ^= *b: This sets *a to the result of *a XOR *b.
 * 2. *b ^= *a: This sets *b to the result of *b XOR (*a XOR *b), which simplifies to *a.
 * 3. *a ^= *b: This sets *a to the result of (*a XOR *b) XOR *a, which simplifies to *b.
 *
 * @param a Pointer to the first integer.
 * @param b Pointer to the second integer.
 */
void swap(int *a, int *b) {
    *a ^= *b, *b ^= *a, *a ^= *b; 
}
