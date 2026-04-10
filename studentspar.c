// gcc -Wall -std=c99 -o test_input input.c main.c
// ./test_input Trab01-AvalEstudantes-ExemploArqEntrada0-v2.txt

#include "input.h"

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>
#include <string.h>

typedef struct{
  double maior;
  double menor;
  double media;
  double mediana;
  double dsvpdr;
} DadosSaida;


void imprimir_medias_alunos(double *alunos, Entrada *ent) {
    printf("\n--- Médias dos Alunos ---\n");
    for (int i = 0; i < ent->R; i++) {
        for (int j = 0; j < ent->C; j++) {
            for (int k = 0; k < ent->A; k++) {
                int idx = indice_aluno(ent, i, j, k);
                printf("Região %d | Cidade %d | Aluno %d: Média %.2f \n", 
                       i, j, k, alunos[idx]);
            }
        }
    }
    printf("---------------------------------------------\n");
}

void imprimir_medias_cidades(DadosSaida *cidades, Entrada *ent) {
    printf("\n--- Dados Consolidados por Cidade ---\n");
    for (int i = 0; i < ent->R; i++) {
        printf("Região %d:\n", i);
        for (int j = 0; j < ent->C; j++) {
            int cidade_idx = i * ent->C + j;
            printf("  Cidade %d: Média %.2f | DP %.2f | Mediana %.2f | Menor %.2f | Maior %.2f\n", 
                   j, 
                   cidades[cidade_idx].media, 
                   cidades[cidade_idx].dsvpdr, 
                   cidades[cidade_idx].mediana, 
                   cidades[cidade_idx].menor, 
                   cidades[cidade_idx].maior);
        }
    }
    printf("---------------------------------------------------\n");
}


void imprimir_medias_regioes(DadosSaida *regioes, Entrada *ent) {
    printf("\n--- Dados Consolidados por Região ---\n");
    for (int i = 0; i < ent->R; i++) {
        printf("Região %d: Média %.2f | DP %.2f | Mediana %.2f | Menor %.2f | Maior %.2f\n", 
               i, 
               regioes[i].media, 
               regioes[i].dsvpdr, 
               regioes[i].mediana, 
               regioes[i].menor, 
               regioes[i].maior);
    }
    printf("-------------------------------------------------\n");
}

void imprimir_medias_brasil(DadosSaida brasil) {
  printf("\n--- Resultado Brasil ---\n");
  printf("Média: %.2f | Desvio Padrão: %.2f | Mediana: %.2f | Menor: %.2f | Maior: %.2f\n", 
         brasil.media, 
         brasil.dsvpdr, 
         brasil.mediana, 
         brasil.menor, 
         brasil.maior);
  printf("------------------------\n");
}

int comparador(const void* a, const void *b){
  return (*(double *) a - *(double *) b);
}

// void kway(double *dados, int *indices, int *ends, double* out, int tam_total, int n_vec){
//   for(int i = 0; i < tam_total; i++){
//     double min = 200.0;
//     int min_val_vec = -1;

//     for(int j = 0; j < n_vec; j++){
//       if(indices[j] < ends[j]){
//         double val = dados[indices[j]];
//         if (val < min) {
//             min = val;
//             min_val_vec = j;
//         }
//       }
//     }
//     if(min_val_vec != -1){
//       out[i] = min;
//       indices[min_val_vec]++;
//     }
//   }
// }

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
  // imprimir_notas(&dados, 0);

  double tempo = omp_get_wtime();

  double *alunos = calloc(ent.R*ent.C*ent.A, sizeof(double));
  DadosSaida *cidades = calloc(ent.R*ent.C, sizeof(DadosSaida));
  DadosSaida *regioes = calloc(ent.R, sizeof(DadosSaida));
  DadosSaida brasil = {0};

  int i, j, k;
  double soma = 0.0;

  double *somas_dsvpdr_regiao = calloc(ent.R, sizeof(double));
  


  // double **thread_local_tmp_notas = NULL;
  // int **thread_local_idx = NULL;
  // int **thread_local_ends = NULL;

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
            double soma_aluno = 0.0;
            // calcula media
            for(int l = 0; l < ent.N; l++)
              soma_aluno += pegar_nota(&dados, i, j, k, l);
            
            alunos[indice_aluno(&ent,i, j, k)] = soma_aluno / ent.N;
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
        double soma_cidade = 0.0, media_cidade = 0.0;
        int cidade_idx = i * ent.C + j;
        for(k = 0; k < ent.A; k++){
          int aluno_idx = indice_aluno(&ent, i, j, k);
          soma_cidade += alunos[aluno_idx];
        }
        
        media_cidade = soma_cidade / ent.A;
        cidades[cidade_idx].media = media_cidade;

        double soma_diff_quadrada = 0.0;
        for(k = 0; k < ent.A; k++)
        {
          int aluno_idx = indice_aluno(&ent, i, j, k);
          soma_diff_quadrada += pow(alunos[aluno_idx] - media_cidade, 2);

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
    #pragma omp for private(i, j) schedule(static) reduction(+:soma)
    for(i=0;i<ent.R;i++)
    {
      double soma_regioes = 0.0;
      for(j=0; j < ent.C; j++)
        soma_regioes += cidades[i*ent.C+j].media; 
      
      regioes[i].media = soma_regioes / ent.C;
      soma += regioes[i].media;
    }
    // ---------------------------------------------------------------------------------------------
// CÁLCULO MEDIA BRASIL
    #pragma omp master
    {
    brasil.media = soma / ent.R;

    soma = 0;
    printf("Tempo após calculo das médias:%.5f\n", (omp_get_wtime() - tempo));
    }
  
// ---------------------------------------------------------------------------------------------
// CÁLCULO DESVIO PADRÃO PARA REGIOES
    #pragma omp for private(j, k) schedule(static) collapse(2) reduction(+:somas_dsvpdr_regiao[0:ent.R])
    for(i = 0; i < ent.R; i++)
    {
      for(j=0;j < ent.C; j++)
      {
        double media_regiao = regioes[i].media;
        for(k=0; k < ent.A; k++){
          int aluno_idx = indice_aluno(&ent, i, j, k);
          // Cada thread acumula em sua cópia privada de somas_dsvpdr_regiao. Sem contenção!
          somas_dsvpdr_regiao[i] += pow(alunos[aluno_idx] - media_regiao, 2);
        }
      }
    }
    #pragma omp master 
    {
      for(i = 0; i < ent.R; i++){
        int total_alunos_regiao = ent.C * ent.A;
        if (total_alunos_regiao > 1) {
            regioes[i].dsvpdr = sqrt(somas_dsvpdr_regiao[i] / (total_alunos_regiao - 1));
        } else {
            regioes[i].dsvpdr = 0.0;
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
          soma += pow(alunos[idx_aluno] - media_brasil, 2);
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
      
      printf("Tempo após calculo dos DPs :%.5f\n", (omp_get_wtime() - tempo));
    }
    // ---------------------------------------------------------------------------------------------
    // CÁLCULO MEDIANAS, MAIOR E MENOR CIDADE
    #pragma omp for private(i, j, k) collapse(2)
    for(i = 0; i < ent.R; i++)
    {
      for(j = 0; j < ent.C; j++){
        int idx_cidade = i*ent.C + j;
        int idx_aluno = indice_aluno(&ent, i, j, 0);
        qsort(&alunos[idx_aluno], ent.A, sizeof(double), comparador);
        if(ent.A % 2 == 0)
          cidades[idx_cidade].mediana = (alunos[idx_aluno + (ent.A / 2) - 1] + alunos[idx_aluno + (ent.A / 2)])/2.0;
        else
          cidades[idx_cidade].mediana = alunos[idx_aluno + (ent.A/2)];

        cidades[idx_cidade].menor = alunos[idx_aluno];
        cidades[idx_cidade].maior = alunos[idx_aluno + ent.A - 1];
      }
    }
    // ---------------------------------------------------------------------------------------------
    // CÁLCULO MEDIANAS, MAIOR E MENOR REGIÃO
    // #pragma omp single 
    // {
    //   printf("Tempo após calculo das medianas cidade:%.5f\n", (omp_get_wtime() - tempo));

    //   int num_threads = omp_get_num_threads();
    //   thread_local_tmp_notas = malloc(num_threads * sizeof(double*));
    //   thread_local_idx = malloc(num_threads * sizeof(int*));
    //   thread_local_ends = malloc(num_threads * sizeof(int*));

    //   for (int t = 0; t < num_threads; t++){
    //     thread_local_tmp_notas[t] = malloc(ent.C * ent.A * sizeof(double));
    //     thread_local_idx[t] = malloc(ent.C * sizeof(int));
    //     thread_local_ends[t] = malloc(ent.C * sizeof(int));
    //   }
    // }

    // #pragma omp for private(i, j, k) schedule(dynamic)
    // for(i = 0; i < ent.R; i++)
    // {
    //   int tid = omp_get_thread_num();
    //   int tot_alunos = ent.C * ent.A;

    //   double *tmp_notas = thread_local_tmp_notas[tid];
    //   int *idx = thread_local_idx[tid];
    //   int *ends = thread_local_ends[tid];

    //   for(int j = 0; j < ent.C; j++){
    //     idx[j] = indice_aluno(&ent, i, j, 0);
    //     ends[j] = idx[j] + ent.A;
    //   }
    //   kway(alunos, idx, ends, tmp_notas, tot_alunos, ent.C);
      
    //   if(tot_alunos % 2 == 0){
    //     regioes[i].mediana = (tmp_notas[tot_alunos / 2 - 1] + tmp_notas[tot_alunos / 2]) / 2.0; 
    //   } else {
    //     regioes[i].mediana = tmp_notas[tot_alunos/2];
    //   }

    //   regioes[i].menor = tmp_notas[0];
    //   regioes[i].maior = tmp_notas[tot_alunos - 1];

    //   memcpy(&alunos[indice_aluno(&ent, i, 0, 0)], tmp_notas, tot_alunos*sizeof(double));
    // }

    // #pragma omp single 
    // {
    //   printf("Tempo após calculo das medianas região:%.5f\n", (omp_get_wtime() - tempo));

    //   int num_threads = omp_get_num_threads();
    //   for (int t = 0; t < num_threads; t++) {
    //     free(thread_local_tmp_notas[t]);
    //     free(thread_local_idx[t]);
    //     free(thread_local_ends[t]);
    //   }
    //   free(thread_local_tmp_notas);
    //   free(thread_local_idx);
    //   free(thread_local_ends);
    // }
    #pragma omp for private(i, j, k) 
    for(i = 0; i < ent.R; i++)
    {
      int num_alunos = ent.C * ent.A;
      int idx_aluno = indice_aluno(&ent, i, 0, 0);
      qsort(&alunos[idx_aluno], num_alunos, sizeof(double), comparador);
      if(ent.A % 2 == 0)
        regioes[i].mediana = (alunos[idx_aluno + (ent.A / 2) - 1] + alunos[idx_aluno + (ent.A / 2)])/2.0;
      else
        regioes[i].mediana = alunos[idx_aluno + (ent.A/2)];

      regioes[i].menor = alunos[idx_aluno];
      regioes[i].maior = alunos[idx_aluno + ent.A - 1];
      
    }

    // ---------------------------------------------------------------------------------------------
    // CÁLCULO MEDIANAS, MAIOR E MENOR BRASIL
    // #pragma omp single
    // {
    //   int tot_alunos = ent.R * ent.C * ent.A;
    //   double *tmp_notas = malloc(tot_alunos * sizeof(double));
    //   int *idx = malloc(ent.R * sizeof(int));
    //   int *ends = malloc(ent.R * sizeof(int));
    //   for(int i = 0; i < ent.R; i++){
    //     idx[i] = indice_aluno(&ent, i, 0, 0);
    //     ends[i] = idx[i] + ent.C * ent.A;
    //   }
    //   kway(alunos, idx, ends, tmp_notas, tot_alunos, ent.R);

    //   if(tot_alunos % 2 == 0){
    //     brasil.mediana = (tmp_notas[tot_alunos / 2 - 1] + tmp_notas[tot_alunos / 2]) / 2.0; 
    //   } else {
    //     brasil.mediana = tmp_notas[tot_alunos/2];
    //   }

    //   brasil.menor = tmp_notas[0];
    //   brasil.maior = tmp_notas[tot_alunos - 1];

    //   free(idx);
    //   free(ends);
    //   free(tmp_notas);
    // }
    #pragma omp single
    {
      int tot_alunos = ent.R * ent.C * ent.A;

      qsort(alunos, tot_alunos, sizeof(double), comparador);

      if(tot_alunos > 0) {
        if(tot_alunos % 2 == 0){
          brasil.mediana = (alunos[tot_alunos / 2 - 1] + alunos[tot_alunos / 2]) / 2.0; 
        } else {
          brasil.mediana = alunos[tot_alunos/2];
        }

        brasil.menor = alunos[0];
        brasil.maior = alunos[tot_alunos - 1];
      }
    }
  }
  tempo = omp_get_wtime() - tempo; 

  // imprimir_medias_alunos(alunos, &ent);
  // imprimir_medias_cidades(cidades, &ent);
  // imprimir_medias_regioes(regioes, &ent);
  // imprimir_medias_brasil(brasil);

  printf("\nTempo de execução: %.5f\n", tempo);

  free(alunos);
  free(cidades);
  free(regioes);


  liberar_dados(&dados);
  return EXIT_SUCCESS;
}