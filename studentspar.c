// gcc -Wall studentspar.c input.c -o teste-par -lm -fopenmp
// ./teste-par Trab01-AvalEstudantes-ExemploArqEntrada0-v2.txt

#include "input.h"

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>
#include <string.h>

#define THRESHOLD_QUICKMEDIAN_SELECT 1000
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
void imprimir_resultados_alunos(double *alunos, Entrada *ent) {
    printf("\n--- Médias dos Alunos ---\n");
    for (int i = 0; i < ent->R; i++) {
        for (int j = 0; j < ent->C; j++) {
            for (int k = 0; k < ent->A; k++) {
                int idx = indice_aluno(ent, i, j, k);
                printf("Região %d | Cidade %d | Aluno %d: Média %.1f \n", 
                       i, j, k, alunos[idx]);
            }
        }
    }
    printf("---------------------------------------------\n");
}

void imprimir_resultados_cidades(DadosSaida *cidades, Entrada *ent) {
    printf("\n--- Dados Consolidados por Cidade ---\n");
    printf("Cidades     |   Min Nota  |   Max Nota    |   Mediana   |   Média   |   DsvPdr\n");
    for (int i = 0; i < ent->R; i++) {
        for (int j = 0; j < ent->C; j++) {
            int cidade_idx = i * ent->C + j;
            printf("R = %d, C = %d    %.1f         %.1f           %.1f         %.1f       %.1f\n", 
              i, cidade_idx, 
              cidades[j].menor, 
              cidades[j].maior, 
              cidades[j].mediana, 
              cidades[j].media, 
              cidades[j].dsvpdr);
        }
    }
    printf("---------------------------------------------------\n");
}


void imprimir_resultados_regioes(DadosSaida *regioes, Entrada *ent) {
    printf("\n--- Dados Consolidados por Região ---\n");
    printf("Regiões   |   Min Nota  |   Max Nota    |   Mediana   |   Média   |   DsvPdr\n");
    for (int i = 0; i < ent->R; i++) {
        printf("R = %d         %.1f         %.1f           %.1f         %.1f       %.1f\n", 
               i, 
               regioes[i].menor, 
               regioes[i].maior, 
               regioes[i].mediana, 
               regioes[i].media, 
               regioes[i].dsvpdr);
    }
    printf("-------------------------------------------------\n");
}

void imprimir_resultados_brasil(DadosSaida brasil) {
  printf("\n--- Resultado Brasil ---\n");
  printf("      |   Min Nota  |   Max Nota    |   Mediana   |   Média   |   DsvPdr\n");
  printf("          %.1f         %.1f           %.1f         %.1f       %.1f\n", 
        brasil.menor, 
        brasil.maior, 
        brasil.mediana, 
        brasil.media, 
        brasil.dsvpdr);
  printf("------------------------\n");
}

void imprimir_premiacoes(DadosSaida *cidades, DadosSaida *regioes, Entrada *ent){
  int regiao = 0, cidade = 0;
  double media_regiao = 0.0, media_cidade = 0.0;


  for(int i = 0; i < ent->R; i++){
    for(int j = 0; j < ent->C; j++){
      int idx_cidade = i*ent->C + j;
      double media = cidades[idx_cidade].media;
      if(media > media_cidade){
        cidade = idx_cidade;
        media_cidade = media;
      }
    }
    double media = regioes[i].media;
    if(media > media_regiao){
      regiao = i;
      media_regiao = media;
    }
  }
  printf("Premiação       | Reg/Cid | Media Arit\n");
  printf("Melhor Regiao:      R%d      %.2f\n", regiao, media_regiao);
  printf("Melhor cidade:    R%d-C%d     %.2f\n", cidade / ent->C, cidade % ent->C, media_cidade);
}

// --------------------------------------------------------------------
// MEDIAN OF MEDIANS
// https://en.wikipedia.org/wiki/Median_of_medians
// algoritmo para mediana O(N), dividindo o vetor em subvetores em 5
// para achar um bom candidato a pivô para quickselect

// troca dois elementos de posicao no vetor
void swap(double* a, double* b){
  double temp = *a;
  *a = *b;
  *b = temp;
}

// declaração para uso no median_select (recursão cruzada)
int pivot(double* arr, int left, int right);

// ordena os subvetores com insertion (pois o tamanho do vetor é pequeno)
// retorna o indice do elemento central (mediana do subvetor)
static inline int median_of_5(double* arr, int left, int right){ // median_Selection
  for(int i = 1; i < 5; i++){
    double key = arr[left + i];
    int j = left + i-1;
    while(j >= left && arr[j] > key){
      arr[j+1] = arr[j];
      j--;
    }
    arr[j+1] = key;
  }
  return left + (right - left)/2;
}

// divide o vetor em 3: menores que o pivô, iguais e maiores
int partition(double* arr, int left, int right, int pivotIndex, int  n){
  double pivotVal = arr[pivotIndex];
  int store = left;
  // joga o pivô pro final  
  swap(&arr[pivotIndex], &arr[right]);

  // agrupa os menores a esquerda
  for(int i = left; i < right -1; i++){
    if(arr[i] < pivotVal){
      swap(&arr[store], &arr[i]);
      store++;
    }
  }
  // agrupa os iguais em seguida
  int storeEq = store;
  for(int i = store; i < right -1; i++){
    if(arr[i] == pivotVal){
      swap(&arr[storeEq], &arr[i]);
      storeEq++;
    }
  }
  // devolve o pivô
  swap(&arr[right], &arr[storeEq]);

  // verifica o grupo do n-esimo eleemnto retorna a posição
  if(n < store) return store;
  if(n<= storeEq) return n;

  return storeEq;
}

// laco principal do quickselect. usa o pivot para restringir
// o array na partição com o n-esimo elemento
int median_select(double* arr, int left, int right, int n){
  while(1){
    // encontrou o elemento
    if(left == right)
      return left;
    // pega um bom pivô
    int pivotIndex = pivot(arr, left, right);
    // particiona o vetor a partir do pivo
    pivotIndex = partition(arr, left, right, pivotIndex, n);
    // se o pivô ficou na posição desejada, retorna ele
    if(n == pivotIndex) return n;
    // senão, descarta a metade inutil do vetor e continua
    else if(n < pivotIndex) right = pivotIndex - 1;
    else left = pivotIndex + 1;
  }
}

// divide o array nos blocos de 5, acha a mediana de cada 1
// move pro inicio do vetor chama recursão para achar a mediana verdadeira
int pivot(double* arr, int left, int right){
  // caso base: com menos de 5 elementos, acha a mediana direto
  if(left - right < 5)
    return median_of_5(arr, left, right);

  // varre o array de 5 em 5 elementos
  #pragma omp for schedule(dynamic)
  for(int i = left; i < right; i +=5){
    int subRight = i + 4;
    // garante q não estoura o tamanho do vetor
    if(subRight > right)
      subRight = right;
    // acha o indice do vetorda mediana no pedaço
    int l_median = median_of_5(arr, i, subRight);
    // joga as sub-medianas pro inicio do array
    swap(&arr[l_median], &arr[left + (int)((i-left)/5)]);
  }
  // indice do meio do grupo de sub-medianas
  int mid = (right-left)/10 + left + 1;
  // chama recursão cruzada para achar a mediana das medianas e a usa como pivô
  return median_select(arr, left,left + (int)((right-left)/5), mid);
}

// encapsula a chamada da seleção para pegar o elemento central
double nth_smallest(double* arr, int n){
  int index = median_select(arr, 0, n - 1, n / 2);
  return arr[index];
}

// chamada principal: lida com o calculo das medianas para tamanhos de array pares e ímpares
double median(double* arr, int n){
  double value;
  if(n %2 == 0)
    value = 0.5 * (nth_smallest(arr, n/2) + nth_smallest(arr, n/2-1)); 
  else
    value = nth_smallest(arr, n/2);
  return value;
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
      // imprimir_resultados_alunos(alunos, &ent);
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
       // int thread_num = omp_get_thread_num();
        
        cidades[idx_cidade].mediana = median(&alunos[idx_aluno], ent.A);
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
      //int thread_num = omp_get_thread_num();
      
      regioes[i].mediana = median(&alunos[idx_aluno], num_alunos);
    }

    #pragma omp master
    {
        printf("Tempo após MEDIANA REGIAO: %.5f\n", omp_get_wtime() - tempo);
    }

    // ---------------------------------------------------------------------------------------------
    // MEDIANAS BRASIL
    
    
     #pragma omp single
     {
      //int thread_num = omp_get_thread_num();
      brasil.mediana = median(alunos, tot_alunos);
     }

    
  }


  tempo = omp_get_wtime() - tempo; 

  
  imprimir_resultados_cidades(cidades, &ent);
  imprimir_resultados_regioes(regioes, &ent);
  imprimir_resultados_brasil(brasil);

  printf("\nTempo de execução final(Med Brasil): %.5f\n", tempo);

  free(alunos);
  free(cidades);
  free(regioes);


  liberar_dados(&dados);
  return EXIT_SUCCESS;
}
