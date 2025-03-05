// Execute o código seguinte. Retire o comentário da diretiva define e execute
// novamente. O que houve? Para que serve a macro assert?

#define NDEBUG
// NDEBUG ignora as asserções

#include <assert.h>
int main() {
    int vetor[3];
    int i;

    for (i = 0; i <= 3; i++) {
        assert(i < 3);
        vetor[i] = i * i;
    }
}