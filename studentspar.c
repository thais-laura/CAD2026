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
    printf("\n--- Médias dos Alunos ---\n");
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
    printf("\n--- Médias e Desvios Padrão das Cidades ---\n");
    for (int i = 0; i < ent->R; i++) {
        printf("Região %d:\n", i);
        for (int j = 0; j < ent->C; j++) {
            int cidade_idx = i * ent->C + j;
            printf("  Cidade %d: Média %.2f | Desvio Padrão %.2f\n", j, cidades[cidade_idx].media, cidades[cidade_idx].dsvpdr);
        }
    }
    printf("---------------------------------------------\n");
}


void imprimir_medias_regioes(DadosSaida *regioes, Entrada *ent) {
    printf("\n--- Médias e Desvios Padrão das Regiões ---\n");
    for (int i = 0; i < ent->R; i++) {
        printf("Região %d: Média %.2f | Desvio Padrão %.2f\n", i, regioes[i].media, regioes[i].dsvpdr);
    }
    printf("-------------------------------------------\n");
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
  DadosSaida brasil = {0};

  int i, j, k;
  double soma = 0;
  double *somas_dsvpdr_regiao = calloc(ent.R, sizeof(double));
  #pragma omp parallel num_threads(ent.T) private(i,k, j) shared(alunos, cidades, regioes, brasil)
  {
// CÁLCULO MEDIA ALUNOS
    #pragma omp for private(i, j, k) schedule(static) collapse(3)
    for(i = 0; i < ent.R; i++)
    {
      for(j = 0; j < ent.C; j++)
      {
          for(k = 0; k < ent.A; k++)
          {
            double soma_aluno = 0;
            // calcula media
            for(int l = 0; l < ent.N; l++)
              soma_aluno += pegar_nota(&dados, i, j, k, l);
            
            alunos[indice_aluno(&ent,i, j, k)].media = soma_aluno / ent.N;
          }
      } 
    }
// ---------------------------------------------------------------------------------------------
// CÁLCULO MEDIA E DESVIO PADRAO CIDADE
    #pragma omp for private(i, j, k) schedule(static) collapse(2)
    for(i=0;i<ent.R;i++)
    {
      for(j=0; j < ent.C; j++)
      {
        double soma_cidade = 0, media_cidade = 0;
        int cidade_idx = i * ent.C + j;
        for(k = 0; k < ent.A; k++){
          int aluno_idx = indice_aluno(&ent, i, j, k);
          soma_cidade += alunos[aluno_idx].media;
        }
        
        media_cidade = soma_cidade / ent.A;
        cidades[cidade_idx].media = media_cidade;

        double soma_diff_quadrada = 0;
        for(k = 0; k < ent.A; k++)
        {
          int aluno_idx = indice_aluno(&ent, i, j, k);
          soma_diff_quadrada += pow(alunos[aluno_idx].media - media_cidade, 2);

        }
        if (ent.A > 1){
            cidades[cidade_idx].dsvpdr = sqrt(soma_diff_quadrada / (ent.A - 1));
        } else {
            cidades[cidade_idx].dsvpdr = 0; // Desvio padrão é 0 se houver apenas um aluno
        }
      }
    }
// ---------------------------------------------------------------------------------------------
// CÁLCULO MEDIA REGIOES
    #pragma omp for private(i, j) schedule(static)
    for(i=0;i<ent.R;i++)
    {
      double soma_regioes = 0;
      for(j=0; j < ent.C; j++)
        soma_regioes += cidades[i*ent.C+j].media; 
      
      regioes[i].media = soma_regioes / ent.C;
    }
    // ---------------------------------------------------------------------------------------------
// CÁLCULO MEDIA BRASIL
    #pragma omp for private(i) schedule(static) reduction(+:soma)
    for(i=0;i<ent.R;i++)
    {
      soma += regioes[i].media;
    }
    #pragma omp master
    {
    brasil.media = soma / ent.R;

    soma = 0;
    }
  
// ---------------------------------------------------------------------------------------------
// CÁLCULO DESVIO PADRÃO PARA REGIOES
    #pragma omp for private(i, j, k) schedule(dynamic) collapse(2)
    for(i = 0; i < ent.R;i++)
    {
      for(j=0;j < ent.C; j++)
      {
        double media_regiao = regioes[i].media;
        double soma_diff_local = 0;
        for(k=0; k < ent.A; k++){
          int aluno_idx = indice_aluno(&ent, i, j, k);
          soma_diff_local += pow(alunos[aluno_idx].media - media_regiao, 2);
        }
        #pragma omp atomic
        somas_dsvpdr_regiao[i] += soma_diff_local;
      }
    }
    #pragma omp master
    {
      for(i = 0; i < ent.R; i++){
        int total_alunos_regiao = ent.C * ent.A;
        if (total_alunos_regiao > 1) {
            regioes[i].dsvpdr = sqrt(somas_dsvpdr_regiao[i] / (total_alunos_regiao - 1));
        } else {
            regioes[i].dsvpdr = 0;
        }
      }
    }
// ---------------------------------------------------------------------------------------------
// CÁLCULO DESVIO PADRÃO BRASIL
    #pragma omp for private(i,j,k) schedule(static) collapse(3) reduction(+:soma)
    for(i = 0; i < ent.R; i++)
    {
      for(j=0;j < ent.C; j++)
      {
        for(k=0; k < ent.A; k++)
        {
          double media_brasil = brasil.media;
          int idx_aluno = indice_aluno(&ent, i, j, k);
          soma += pow(alunos[idx_aluno].media - media_brasil, 2);
        }
      }
    }
    #pragma omp master
    {
        int total_alunos_brasil = ent.R*ent.C*ent.A;
        if(total_alunos_brasil > 1)
          brasil.dsvpdr = sqrt(soma / (total_alunos_brasil - 1));
        else
          brasil.dsvpdr = 0;
    }

  }

  free(somas_dsvpdr_regiao);
  imprimir_medias_alunos(alunos, &ent);
  imprimir_medias_cidades(cidades, &ent);
  imprimir_medias_regioes(regioes, &ent);

  printf("\n--- Resultado Brasil ---\n");
  printf("Média: %.2f | Desvio Padrão: %.2f\n", brasil.media, brasil.dsvpdr);
  printf("------------------------\n");
  liberar_dados(&dados);
  return EXIT_SUCCESS;
}