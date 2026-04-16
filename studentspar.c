// gcc -Wall studentspar.c input.c -o teste-par -lm -fopenmp
// ./teste-par Trab01-AvalEstudantes-ExemploArqEntrada0-v2.txt

#include "input.h"

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>
#include <string.h>

#define THRESHOLD_QUICKSELECT 1000
#define THRESHOLD_CHAN 1000

// armazena as estatisticas calculadas pra cada nivel (cidade, regiao, brasil)
typedef struct{
  double maior;
  double menor;
  double media;
  double mediana;
  double dsvpdr;
} DadosSaida;

// funções de print (não estão na formatação esperada, foi elaborada apenas para verificar os resultados)
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
// RANDOM QUICKSELECT
// algoritmo que escolhe um pivo aleatoriamente no vetor, e troca elementos nas partições
// até os menores ficarem a esquerda e os maiores a direita. Isso é feito até o pivô ser 
// a mediana, e as partições em que a mediana certamente nao está são descartadas (não ordena).

// troca dois elementos de posicao no vetor
void swap(double* a, double* b){
  double temp = *a;
  *a = *b;
  *b = temp;
}

// particao do quickselect: joga menores que o pivo pra esquerda
int partition(double* arr, int l, int r){
  int i = l, j = l;
  double lst = arr[r];
  while(j < r) {
    if(arr[j] < lst){
      swap(&arr[i], &arr[j]);
      i++;
    }
    j++;
  } 
  swap(&arr[i], &arr[r]);
  return i;
}

// escolhe pivo aleatorio (evita o pior caso de O(N^2))
int randomPartition(double* arr, int l, int r, unsigned int *seed){
  int n = r - l + 1;
  int pivot = rand_r(seed) % n;
  swap(&arr[l + pivot], &arr[r]);
  return partition(arr, l, r); 
}
// acha os elementos centrais (a e b), com particoes recursivas chamadas em outras tasks
// se ainda n atingiu THRESHOLD_CHAN
void medianUtil(double* arr, int l, int r, int k, double* a, double* b, unsigned int *seed){
  if(l <= r){
    int partitionIndex = randomPartition(arr, l, r, seed);
    // se achou pivo no meio do vetor e elementos centrais, retorna
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
    // senao, divide, criando ou nao tasks dependendo do tamanho do subvetor
    if(*a == -1 || *b == -1){
      if(partitionIndex >= k){
        medianUtil(arr, l, partitionIndex - 1, k, a, b, seed);
      } else {
        medianUtil(arr, partitionIndex + 1, r, k, a, b, seed);
      }
    }
  }
}

// pega um array desordenado e bagunça até achar a mediana 
double median(double* arr, int n, int thread_id){
  double a = -1.0, b = -1.0;
  double ans;
  unsigned int seed = thread_id + 1;
  #pragma omp taskgroup
  {
    medianUtil(arr, 0, n-1, n / 2, &a, &b, &seed);
  }
  
  
  if(n % 2 == 1){
    ans = b;
  } else {
    ans = (a + b)/2;
  }
  
  return ans;
}

// --------------------------------------------------------------------
// ALGORITMO PARALELO DE CHAN 
//  https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Parallel_algorithm

// struct pra juntar dados de dois blocos na arvore do chan e obter retorno
typedef struct {
  double n;
  double media;
  double M2;
  double maior;
  double menor;
} ValoresChan;

// chan paralelizado com divide and conquer e tasks 
// quando atinge threshold, começa a mesclar blocos pela fórmula
void chanRecursivo(DadosSaida* in, int start, int end, int medias_por_indice_in, ValoresChan* out){
  int tam = end - start + 1;
  if(tam <= THRESHOLD_CHAN){
    out->n = 0.0;
    out->media = 0.0;
    out->M2 = 0.0;
    out->maior = 0.0;
    out->menor = 150.0;

    for(int j = start; j <= end; j++){
      double media = in[j].media;
      double dsvpdr = in[j].dsvpdr;
      // obtem m2 retrocedendo o desvio padrao (dsv = sqrt(m2/n-1))
      double M2 = dsvpdr * dsvpdr * (medias_por_indice_in > 1 ? medias_por_indice_in - 1 : 0);

      double maior_atual = in[j].maior;
      double menor_atual = in[j].menor;

      if(out->n == 0){
        out->n = medias_por_indice_in;
        out->media = media;
        out->M2 = M2;
        out->maior = maior_atual;
        out->menor = menor_atual;
      } else {
        // combina elementos do array pela formula
        double delta = media - out->media;
        double n_novo = out->n + medias_por_indice_in;
        
        out->M2 = M2 + (delta * delta) * (out->n * medias_por_indice_in) / n_novo;
        out->media += delta * (medias_por_indice_in / n_novo);
        out->n = n_novo;

        if (maior_atual > out->maior) out->maior = maior_atual;
        if (menor_atual < out->menor) out->menor = menor_atual;
      }
    }
    return;
  }
  // divide and conquer com tasks
  int mid = start + (end - start) / 2;
  ValoresChan left, right;

  #pragma omp task shared(left)
  {chanRecursivo(in, start, mid, medias_por_indice_in, &left);}

  #pragma omp task shared(right)
  {chanRecursivo(in, mid + 1, end, medias_por_indice_in, &right);}
  // espera as duas metades ficarem prontas pra juntar 
  #pragma omp taskwait
  {
    double n_novo = left.n + right.n;
    double delta = right.media - left.media;

    out->n = n_novo;
    out->M2 = left.M2 + right.M2 + (delta * delta) * (left.n * right.n) / n_novo;
    out->media = left.media + delta * (right.n / n_novo);
    out->maior = (left.maior > right.maior) ? left.maior : right.maior;
    out->menor = (left.menor < right.menor) ? left.menor : right.menor;
  }
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
  // imprimir_notas(&dados, 0);

  double tempo = omp_get_wtime();

  double *alunos = calloc(ent.R*ent.C*ent.A, sizeof(double));
  DadosSaida *cidades = calloc(ent.R*ent.C, sizeof(DadosSaida));
  DadosSaida *regioes = calloc(ent.R, sizeof(DadosSaida));
  DadosSaida brasil = {0};

  int tot_alunos = ent.C*ent.R*ent.A;
  int num_notas = ent.N;
  double *notas = dados.notas;

  int i, j, k;
  // regiao paralela geral
  #pragma omp parallel num_threads(ent.T) private(i,k, j) shared(alunos, cidades, regioes, brasil)
  {
// MEDIA ALUNOS
    #pragma omp for private(i, j, k) schedule(static) collapse(3)
    for(i = 0; i < ent.R; i++)
    {
      for(j = 0; j < ent.C; j++)
      {
        for(k = 0; k < ent.A; k++)
        {     
          double soma_aluno = 0.0;
          int idx = indice_aluno(&ent, i, j, k) * num_notas;
          
          // vetorização: obriga o processador a somar várias notas no mesmo ciclo de clock
          #pragma omp simd reduction(+:soma_aluno) linear(idx:1)
          for(int l = 0; l < num_notas; l++) {
            soma_aluno += notas[idx + l];
          }

          alunos[indice_aluno(&ent,i, j, k)] = soma_aluno / num_notas;
        }
      } 
    }

    #pragma omp master
    {
        printf("Tempo após MEDIA ALUNOS: %.5f\n", omp_get_wtime() - tempo);
    }

// ---------------------------------------------------------------------------------------------
// MEDIA, DESVIO PADRAO, <, > CIDADE
    #pragma omp for private(i, j, k) schedule(static) collapse(2)
    for(i=0;i<ent.R;i++)
    {
      for(j=0; j < ent.C; j++)
      {
        double m2 = 0, media_cidade = 0.0;
        double menor = 150.0;
        double maior = 0.0;
        int cidade_idx = i * ent.C + j;

        for(k = 0; k < ent.A; k++){
          double nota = alunos[indice_aluno(&ent, i, j, k)];

          if(nota > maior) maior = nota;
          if(nota < menor) menor = nota;
          // welford (calcula media e m2 incrementalmente)
          double delta = nota - media_cidade;
          media_cidade += delta / (k + 1);
          double delta2 = nota - media_cidade; // Usa a nova média
          m2 += delta * delta2;
        }
        
        cidades[cidade_idx].media = media_cidade;
        cidades[cidade_idx].dsvpdr = (ent.A > 1) ? sqrt(m2 / (ent.A - 1)) : 0.0;
        cidades[cidade_idx].maior = maior;
        cidades[cidade_idx].menor = menor;
      }
    }

    #pragma omp master
    {
        printf("Tempo após MEDIA/DP/</> CIDADES: %.5f\n", omp_get_wtime() - tempo);
    }

// ---------------------------------------------------------------------------------------------
// MEDIA, DESVIO PADRAO, <, > REGIOES
// dynamic pois as tasks podem demorar tempos diferentes (devido ao rand e dados diferentes)
    #pragma omp for schedule(dynamic)
    for(i = 0; i < ent.R; i++){
      ValoresChan resultado_regiao;

      int start = i*ent.C;
      int end = start + ent.C - 1;
      // desce a arvore de recursao juntando as cidades de cada regiao
      chanRecursivo(cidades, start, end, ent.A, &resultado_regiao);

      regioes[i].media = resultado_regiao.media;
      regioes[i].dsvpdr = (resultado_regiao.n > 1) ? sqrt(resultado_regiao.M2 / (resultado_regiao.n - 1)) : 0.0;
      regioes[i].maior = resultado_regiao.maior;
      regioes[i].menor = resultado_regiao.menor;
    }
    
    #pragma omp master 
    {
      printf("Tempo após calculo das regioes:%.5f\n", (omp_get_wtime() - tempo));
    }
    // ---------------------------------------------------------------------------------------------
// MEDIA, DESVIO PADRAO, <, > BRASIL
    #pragma omp single
    {
      ValoresChan resultado_brasil;

      int start = 0;
      int end = start + ent.R - 1;

      // desce a arvore de recursão juntando as regioes
      chanRecursivo(regioes, start, end, ent.C, &resultado_brasil);

      brasil.media = resultado_brasil.media;
      brasil.dsvpdr = (resultado_brasil.n > 1) ? sqrt(resultado_brasil.M2 / (resultado_brasil.n - 1)) : 0.0;
      brasil.maior = resultado_brasil.maior;
      brasil.menor = resultado_brasil.menor;
    }

    // --------------------------------------------------------------------------------------------------------
    #pragma omp single
    {
      // printa os alunos e corrige o tempo de execução (n lembro se é obrigatório printar isso)
      double antes = omp_get_wtime();
      // precisa imprimir as médias antes de reordenar o vetor alunos (quickselect)
      // imprimir_medias_alunos(alunos, &ent);
      double depois = omp_get_wtime();
      tempo -= (depois - antes);
    }

    // ---------------------------------------------------------------------------------------------
    // MEDIANAS CIDADE
    #pragma omp for private(i, j, k) collapse(2)
    for(i = 0; i < ent.R; i++)
    {
      for(j = 0; j < ent.C; j++)
      {
        int idx_cidade = i*ent.C + j;
        int idx_aluno = indice_aluno(&ent, i, j, 0); // primeiro aluno de cada cidade
        int thread_num = omp_get_thread_num();
        
        cidades[idx_cidade].mediana = median(&alunos[idx_aluno], ent.A, thread_num);
      }
    }

    #pragma omp master
    {
        printf("Tempo após MEDIANA CIDADES: %.5f\n", omp_get_wtime() - tempo);
    }
    // ---------------------------------------------------------------------------------------------
    // MEDIANAS REGIÃO
    
    #pragma omp for private(i, j, k) 
    for(i = 0; i < ent.R; i++)
    {
      int num_alunos = ent.C * ent.A;
      int idx_aluno = indice_aluno(&ent, i, 0, 0); // primeiro aluno de cada regiao
      int thread_num = omp_get_thread_num();
      
      regioes[i].mediana = median(&alunos[idx_aluno], num_alunos, thread_num);
    }

    #pragma omp master
    {
        printf("Tempo após MEDIANA REGIAO: %.5f\n", omp_get_wtime() - tempo);
    }

    // ---------------------------------------------------------------------------------------------
    // MEDIANAS BRASIL
    #pragma omp single
    {
      int thread_num = omp_get_thread_num();
    
      brasil.mediana = median(alunos, tot_alunos, thread_num);
    }
  }
  tempo = omp_get_wtime() - tempo; 

  
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
