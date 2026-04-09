// gcc -Wall -std=c99 -o test_input input.c main.c
// ./test_input Trab01-AvalEstudantes-ExemploArqEntrada0-v2.txt

#include "input.h"

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>

typedef struct{
    double maior;
    double menor;
    double media;
    double mediana;
    double dsvpdr;
} DadosSaida;


void imprimir_medias_alunos(DadosSaida *alunos, Entrada *ent) {
    printf("\n--- Médias e Desvios Padrão dos Alunos ---\n");
    for (int i = 0; i < ent->R; i++) {
        for (int j = 0; j < ent->C; j++) {
            for (int k = 0; k < ent->A; k++) {
                int idx = indice_aluno(ent, i, j, k);
                printf("Região %d | Cidade %d | Aluno %d: Média %.2f \n", 
                       i, j, k, alunos[idx].media);
            }
        }
    }
    printf("---------------------------------------------\n");
}

void imprimir_medias_cidades(DadosSaida *cidades, Entrada *ent) {
    printf("\n--- Médias das Cidades ---\n");
    for (int i = 0; i < ent->R; i++) {
        printf("Região %d:\n", i);
        for (int j = 0; j < ent->C; j++) {
            printf("  Cidade %d: %.2f\n", j, cidades[i*ent->C + j].media);
        }
    }
    printf("-------------------------\n");
}


void imprimir_medias_regioes(DadosSaida *regioes, Entrada *ent) {
    printf("\n--- Médias das Regiões ---\n");
    for (int i = 0; i < ent->R; i++) {
        printf("Região %d: %.2f\n", i, regioes[i].media);
    }
    printf("-------------------------\n");
}

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

  DadosSaida *alunos = calloc(ent.R*ent.C*ent.A, sizeof(DadosSaida));
  DadosSaida *cidades = calloc(ent.R*ent.C, sizeof(DadosSaida));
  DadosSaida *regioes = calloc(ent.R, sizeof(DadosSaida));
  DadosSaida brasil;

  int i, j, k;
  double soma = 0;
  #pragma omp parallel num_threads(ent.T) private(i,k, j) shared(alunos, cidades, regioes, brasil)
  {
    #pragma omp for private(i, j, k) schedule(static) collapse(3)
    for(i = 0; i < ent.R; i++)
    {
      for(j = 0; j < ent.C; j++)
      {
          for(k = 0; k < ent.A; k++)
          {
            double soma = 0;
            // calcula media
            for(int l = 0; l < ent.N; l++)
              soma += pegar_nota(&dados, i, j, k, l);
            
            alunos[indice_aluno(&ent,i, j, k)].media = soma / ent.N;
          }
      } 
    }
  
    #pragma omp for private(i, j, k) schedule(static) collapse(2)
    for(i=0;i<ent.R;i++)
    {
      for(j=0; j < ent.C; j++)
      {
  
        double soma = 0;
        for(k = 0; k < ent.A; k++)
          soma += alunos[i*ent.C + j*ent.A + k].media;
        
        cidades[i*ent.C+j].media = soma / ent.A;
      }
    }

    #pragma omp for private(i, j) schedule(static)
    for(i=0;i<ent.R;i++)
    {
      double soma = 0;
      for(j=0; j < ent.C; j++)
        soma += cidades[i*ent.C+j].media; 
      
      regioes[i].media = soma / ent.C;
    }

    #pragma omp for private(i) schedule(static) reduction(+:soma)
    for(i=0;i<ent.R;i++)
    {
      soma += regioes[i].media;
    }
    #pragma omp master
    {
    brasil.media = soma / ent.R;
    }
  }

  imprimir_medias_alunos(alunos, &ent);
  imprimir_medias_cidades(cidades, &ent);
  imprimir_medias_regioes(regioes, &ent);

  printf("média brasil: %.1f\n", brasil.media);
  liberar_dados(&dados);
  return EXIT_SUCCESS;
}