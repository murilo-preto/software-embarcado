// O código seguinte define uma variável ex_buf__ que será utilizada para guradar
// o contexto do processador. A macro setjmp salva o contexto do processador no
// momento em que é executada diretamente. Neste caso ela retorna o valor 0. A
// execução de longjmp faz o programa retornar ao ponto em que a macro setjmp
// foi executada e executa o código como se setjmp tivesse retornado o valor inteiro
// especificado na chamada de longjmp. Uma das regras obrigatórioas do padrão
// MISRA proíbe o uso deste método de tratamento de exceções.

/* Copyright (C) 2009-2015 Francesco Nidito
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
* of the Software, and to permit persons to whom the Software is furnished to do
* so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER
2
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE
* SOFTWARE.
*/

#ifndef _TRY_THROW_CATCH_H_
#define _TRY_THROW_CATCH_H_
#include <setjmp.h>
#include <stdio.h>
/* For the full documentation and explanation of the code below, please refer to
 * http://www.di.unipi.it/~nids/docs/longjump_try_trow_catch.html
 */
#define TRY                         \
    do {                            \
        jmp_buf ex_buf__;           \
        switch (setjmp(ex_buf__)) { \
            case 0:                 \
                while (1) {
#define CATCH(x) \
    break;       \
    case x:
#define FINALLY \
    break;      \
    }           \
    default: {
#define E_TRY \
    break;    \
    }         \
    }         \
    }         \
    while (0)
#define THROW(x) longjmp(ex_buf__, x)
#endif /*!_TRY_THROW_CATCH_H_*/
#define FOO_EXCEPTION (1)
#define BAR_EXCEPTION (2)
#define BAZ_EXCEPTION (3)
int main(int argc, char** argv) {
    TRY {
        /* Bloco em que pode haver uma exceção */
        printf("In Try Statement\n");
        /* Se for detectada uma exceção, executa-se a macro THROW */
        THROW(BAR_EXCEPTION);
        printf("I do not appear\n");
    }
    /* Blocos executados se houver uma exceção */
    CATCH(FOO_EXCEPTION) {
        printf("Got Foo!\n");
    }
    CATCH(BAR_EXCEPTION) {
        printf("Got Bar!\n");
    }
    CATCH(BAZ_EXCEPTION) {
        printf("Got Baz!\n");
    }
    FINALLY {
        /* Código que deve ser executado sempre, independente de exceções */
    }
    E_TRY;
    return 0;
}
