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

// -----------------------------------------------------------------------------------------
void swap(double* a, double* b){
  double temp = *a;
  *a = *b;
  *b = temp;
}

int partition(double* arr, int l, int r){
  int lst = arr[r], i = l, j = l;
  while(j < r) {
    if(arr[j] < lst){
      swap(&arr[i], &arr[j]);
      i++;
    }
    j++;
  } 
  swap(&arr[i], &arr[j]);
  return i;
}

int randomPartition(double* arr, int l, int r, unsigned int *seed){
  int n = r - l + 1;
  int pivot = rand_r(seed) % n;
  swap(&arr[l + pivot], &arr[r]);
  return partition(arr, l, r); 
}

void medianUtil(double* arr, int l, int r, int k, double* a, double* b, unsigned int *seed){
  if(l <= r){
    int partitionIndex = randomPartition(arr, l, r, seed);
    if(partitionIndex == k) {
      *b = arr[partitionIndex];
      if(*a != -1)
        return;
    }

    else if(partitionIndex == k - 1){
      *a = arr[partitionIndex];
      if(*b != -1)
        return;
    }
    if(partitionIndex >= k)
      return medianUtil(arr, l, partitionIndex - 1, k, a, b, seed);
    else
      return medianUtil(arr, partitionIndex + 1, r, k, a, b, seed);
  }
  return;
}

double median(double* arr, int n, int thread_id){
  double a = -1.0, b = -1.0;
  double ans;
  unsigned int seed = thread_id + 1;

  if(n % 2 == 1){
    medianUtil(arr, 0, n-1, n / 2, &a, &b, &seed);
    ans = b;
  } else {
    medianUtil(arr, 0, n-1, n / 2, &a, &b, &seed);
    ans = (a + b)/2;
  }
  return ans;
}

// ----------------------------------------------------



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
  


  double *thread_local_tmp_maior = NULL;
  double *thread_local_tmp_menor = NULL; 
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

    #pragma omp master
    {
        printf("Tempo após MEDIA ALUNOS: %.5f\n", omp_get_wtime() - tempo);
    }

// ---------------------------------------------------------------------------------------------
// CÁLCULO MEDIA, DESVIO PADRAO, <, > CIDADE
    #pragma omp for private(i, j, k) schedule(static) collapse(2)
    for(i=0;i<ent.R;i++)
    {
      for(j=0; j < ent.C; j++)
      {
        double soma_cidade = 0.0, media_cidade = 0.0;
        double menor = 150.0;
        double maior = 0.0;
        int cidade_idx = i * ent.C + j;
        for(k = 0; k < ent.A; k++){
          double nota = alunos[indice_aluno(&ent, i, j, k)];
          soma_cidade += nota;
          if(nota > maior)
                maior = nota;
          if(nota < menor)
            menor = nota;
        }
        
        media_cidade = soma_cidade / ent.A;
        cidades[cidade_idx].media = media_cidade;

        cidades[cidade_idx].maior = maior;
        cidades[cidade_idx].menor = menor;

        double soma_diff_quadrada = 0.0;
        for(k = 0; k < ent.A; k++) {
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

    #pragma omp master
    {
        printf("Tempo após MEDIA/DP/</> CIDADES: %.5f\n", omp_get_wtime() - tempo);
    }

// ---------------------------------------------------------------------------------------------
// CÁLCULO MEDIA/</> REGIOES
    #pragma omp for private(i, j) schedule(static) reduction(+:soma)
    for(i=0;i<ent.R;i++)
    {
      double soma_regioes = 0.0;
      double maior = 0.0;
      double menor = 150.0;
      for(j=0; j < ent.C; j++){
        double nota = cidades[i*ent.C+j].media;
        soma_regioes += cidades[i*ent.C+j].media; 
        if(nota > maior)
          maior = nota;
        if(nota < menor)
          menor = nota;
      }
      
      regioes[i].media = soma_regioes / ent.C;
      soma += regioes[i].media;

      regioes[i].maior = maior;
      regioes[i].menor = menor;
    }
    #pragma omp master 
    {
      printf("Tempo após calculo das médias regioes:%.5f\n", (omp_get_wtime() - tempo));
    }
    // ---------------------------------------------------------------------------------------------
// CÁLCULO MEDIA/</> BRASIL
    #pragma omp single
    {
      brasil.media = soma / ent.R;

      soma = 0;
      printf("Tempo após calculo das médias brasil:%.5f\n", (omp_get_wtime() - tempo));

      
      int num_threads = omp_get_num_threads();
      thread_local_tmp_maior = calloc(num_threads, sizeof(double));
      thread_local_tmp_menor = malloc(num_threads*sizeof(double));
      for(int n = 0; n < num_threads; n++)
        thread_local_tmp_menor[n] = 150.0;
      

      for(i = 0; i < ent.R; i++){
        #pragma omp task
        {
        int thread_num = omp_get_thread_num();
        double maior = regioes[i].maior;
        if(maior > thread_local_tmp_maior[thread_num])
          thread_local_tmp_maior[thread_num] = maior;
        }
        #pragma omp task
        {
          int thread_num = omp_get_thread_num();
          double menor = regioes[i].menor;
          if(menor > thread_local_tmp_menor[thread_num])
            thread_local_tmp_menor[thread_num] = menor;
        }
      }
      #pragma omp taskwait
      {
        double maior = 0.0;
        double menor = 150.0;
        for(int i = 0; i < omp_get_num_threads(); i++){
          if(thread_local_tmp_maior[i] > maior)
            maior = thread_local_tmp_maior[i];
          if(thread_local_tmp_menor[i] < menor)
            menor = thread_local_tmp_menor[i];
        }
        brasil.maior = maior;
        brasil.menor = menor;
      }
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
      printf("Tempo após DP REGIOES: %.5f\n", omp_get_wtime() - tempo);
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
      
      printf("Tempo após DP BRASIL: %.5f\n", (omp_get_wtime() - tempo));
    }
    // ---------------------------------------------------------------------------------------------
    // CÁLCULO MEDIANAS CIDADE
    #pragma omp for private(i, j, k) collapse(2)
    for(i = 0; i < ent.R; i++)
    {
      for(j = 0; j < ent.C; j++)
      {
        int idx_cidade = i*ent.C + j;
        int idx_aluno = indice_aluno(&ent, i, j, 0);
        int thread_num = omp_get_thread_num();
        
        cidades[idx_cidade].mediana = median(&alunos[idx_aluno], ent.A, thread_num);
      }
    }

    #pragma omp master
    {
        printf("Tempo após MEDIANA CIDADES: %.5f\n", omp_get_wtime() - tempo);
    }
    // ---------------------------------------------------------------------------------------------
    // CÁLCULO MEDIANAS REGIÃO
    
    #pragma omp for private(i, j, k) 
    for(i = 0; i < ent.R; i++)
    {
      int num_alunos = ent.C * ent.A;
      int idx_aluno = indice_aluno(&ent, i, 0, 0);
      int thread_num = omp_get_thread_num();
      
      regioes[i].mediana = median(&alunos[idx_aluno], num_alunos, thread_num);
    }

    #pragma omp master
    {
        printf("Tempo após MEDIANA/MIN/MAX REGIAO: %.5f\n", omp_get_wtime() - tempo);
    }

    // ---------------------------------------------------------------------------------------------
    // CÁLCULO MEDIANAS, MAIOR E MENOR BRASIL
    #pragma omp single
    {
      int tot_alunos = ent.R * ent.C * ent.A;
      int thread_num = omp_get_thread_num();
      
      double menor = alunos[0];
      double maior = alunos[0];
  
      for (int t = 1; t < tot_alunos; t++) {
          if (alunos[t] < menor)
              menor = alunos[t];
          if (alunos[t] > maior)
              maior = alunos[t];
      }
  
      brasil.menor = menor;
      brasil.maior = maior;
      
      brasil.mediana = median(alunos, tot_alunos, thread_num);
    }
  }
  tempo = omp_get_wtime() - tempo; 

  // imprimir_medias_alunos(alunos, &ent);
  // imprimir_medias_cidades(cidades, &ent);
  // imprimir_medias_regioes(regioes, &ent);
  // imprimir_medias_brasil(brasil);

  printf("\nTempo de execução final(Med Brasil): %.5f\n", tempo);

  free(alunos);
  free(cidades);
  free(regioes);


  liberar_dados(&dados);
  return EXIT_SUCCESS;
}
