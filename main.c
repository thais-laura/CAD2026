// gcc -Wall -std=c99 -o test_input input.c main.c
// ./test_input Trab01-AvalEstudantes-ExemploArqEntrada0-v2.txt

#include "input.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Uso: %s <arquivo_entrada>\n", argv[0]);
    return EXIT_FAILURE;
  }

  // le o arquivo
  Entrada ent;
  if (ler_entrada(argv[1], &ent) != 0)
    return EXIT_FAILURE;

  if (validar_entrada(&ent) != 0)
    return EXIT_FAILURE;

  // aloca e gera notas
  DadosAlunos dados = criar_dados(&ent);
  if (!dados.notas)
    return EXIT_FAILURE;

  // amostra pra conferir
  imprimir_resumo(&dados);
  imprimir_notas(&dados, 0);

  liberar_dados(&dados);
  return EXIT_SUCCESS;
}