// Onde está o erro?

#include <stdio.h>
#include <stdlib.h>

main() {
    char* str1 = "5 eh igual a ";

    // char* str2 = "5%de 20\n";
    // Faltou escapar o caractere especial (%)
    
    char* str2 = "5%% de 20\n";
    printf(str1);
    printf(str2);
}