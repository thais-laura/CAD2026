
#include "input.h"
#include <stdio.h>
#include <stdlib.h>

int ler_entrada(const char *caminho, Entrada *out) {
  FILE *fp = fopen(caminho, "r");
  if (!fp) {
    fprintf(stderr, "Erro: nao abriu '%s'.\n", caminho);
    return -1;
  }

  // 6 linhas: R, C, A, N, T, seed
  int lidos = fscanf(fp, "%d %d %d %d %d %u", &out->R, &out->C, &out->A,
                     &out->N, &out->T, &out->seed);
  fclose(fp);

  if (lidos != 6) {
    fprintf(stderr, "Erro: leitura incompleta (%d de 6).\n", lidos);
    return -1;
  }
  return 0;
}

int validar_entrada(const Entrada *ent) {
  // todos > 0, seed pode ser 0
  if (ent->R <= 0) {
    fprintf(stderr, "Erro: R deve ser > 0.\n");
    return -1;
  }
  if (ent->C <= 0) {
    fprintf(stderr, "Erro: C deve ser > 0.\n");
    return -1;
  }
  if (ent->A <= 0) {
    fprintf(stderr, "Erro: A deve ser > 0.\n");
    return -1;
  }
  if (ent->N <= 0) {
    fprintf(stderr, "Erro: N deve ser > 0.\n");
    return -1;
  }
  if (ent->T <= 0) {
    fprintf(stderr, "Erro: T deve ser > 0.\n");
    return -1;
  }
  return 0;
}

// os alunos ficam num vetor so
// primeiro todos da cidade 0 da regiao 0, depois cidade 1 da regiao 0, etc.
// ex: com C=4, A=6 -> aluno (r=1, c=2, a=3) = 1*4*6 + 2*6 + 3 = 39
int indice_aluno(const Entrada *ent, int r, int c, int a) {
  return r * (ent->C * ent->A) + c * ent->A + a;
}

// cada aluno tem N notas seguidas no vetor, entao a posicao eh:
// indice_do_aluno * N + numero_da_nota
// cast pra long pq com muitos alunos o int pode estourar
long pos_nota(const Entrada *ent, int r, int c, int a, int n) {
  return (long)indice_aluno(ent, r, c, a) * ent->N + n;
}

// &dados->entrada pega o endereco do campo entrada dentro da struct
double pegar_nota(const DadosAlunos *dados, int r, int c, int a, int n) {
  return dados->notas[pos_nota(&dados->entrada, r, c, a, n)];
}

DadosAlunos criar_dados(const Entrada *ent) {
  DadosAlunos dados = {0};
  dados.entrada = *ent;
  dados.total_alunos = ent->R * ent->C * ent->A;
  dados.total_notas = (long)dados.total_alunos * ent->N;

  dados.notas = (double *)malloc((size_t)dados.total_notas * sizeof(double));
  if (!dados.notas) {
    fprintf(stderr, "Erro: falha ao alocar memoria.\n");
    return dados;
  }

  srand(ent->seed);

  // gera notas na ordem: regiao, cidade, aluno, nota
  // rand() % 101 da valores inteiros 0-100, guardados como double
  for (int r = 0; r < ent->R; r++)
    for (int c = 0; c < ent->C; c++)
      for (int a = 0; a < ent->A; a++)
        for (int n = 0; n < ent->N; n++)
          dados.notas[pos_nota(ent, r, c, a, n)] = (double)(rand() % 101);

  return dados;
}

void liberar_dados(DadosAlunos *dados) {
  if (!dados)
    return;
  free(dados->notas);
  dados->notas = NULL;
}

void imprimir_resumo(const DadosAlunos *dados) {
  const Entrada *e = &dados->entrada;
  printf("R=%d  C=%d  A=%d  N=%d  T=%d  seed=%u\n", e->R, e->C, e->A, e->N,
         e->T, e->seed);
  printf("Total alunos: %d  |  Total notas: %ld\n\n", dados->total_alunos,
         dados->total_notas);
}

void imprimir_notas(const DadosAlunos *dados, int max) {
  const Entrada *ent = &dados->entrada;
  int cont = 0;

  for (int r = 0; r < ent->R && (max <= 0 || cont < max); r++) {
    printf("Regiao %d\n", r);
    for (int c = 0; c < ent->C && (max <= 0 || cont < max); c++)
      for (int a = 0; a < ent->A && (max <= 0 || cont < max); a++, cont++) {
        printf("  R=%d C=%d A=%d  |", r, c, a);
        for (int n = 0; n < ent->N; n++)
          printf(" %5.1f", pegar_nota(dados, r, c, a, n));
        printf("\n");
      }
  }
}