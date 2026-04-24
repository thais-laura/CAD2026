/* SSC0903 - Computação de Alto Desempenho - Trabalho 1
    Elaborado por:
    Gabriella Castelari Gonçalves     14755082
    Matheus Lopes Ponciano            14598358
    Murilo Cury Pontes                13830417
    Thaís Laura Anício Andrade        14608765
    Yudi Asano Ramos                  12873553
*/

// gcc -Wall -O3 studentsseq.c input.c -o teste-seq -lm -fopenmp
// ./teste-seq Trab01-AvalEstudantes-ExemploArqEntrada0-v2.txt

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>
#include <string.h>

#define THRESHOLD_CHAN 1000
// PROCESSAMENTO DE ENTRADA E GERAÇÃO DAS NOTAS PSEUDO-ALEATÓRIAS
typedef struct {
  int R;             // regioes
  int C;             // cidades por regiao
  int A;             // alunos por cidade
  int N;             // notas por aluno
  int T;             // threads
  unsigned int seed; // semente
} Entrada;

// dados prontos pra processar
// notas[indice_aluno(r,c,a) * N + n] = nota do aluno
typedef struct {
  Entrada entrada;
  int total_alunos; // R * C * A
  long total_notas; // R * C * A * N
  double *notas;    // vetor continuo
} DadosAlunos;

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

// -----------------------------------------------------------------------------------
// DEFINIÇÃO DO TIPO DE DADO DE SAIDA E IMPRESSÃO

// armazena as estatisticas calculadas pra cada nivel (cidade, regiao, brasil)
typedef struct{
  double maior;
  double menor;
  double media;
  double mediana;
  double dsvpdr;
} DadosSaida;

void imprimir_resultados_cidades(DadosSaida *cidades, Entrada *ent) {
    printf("\n--- Dados Consolidados por Cidade ---\n");
    printf("Cidades     |   Min Nota  |   Max Nota    |   Mediana   |   Média   |   DsvPdr\n");
    for (int i = 0; i < ent->R; i++) {
        for (int j = 0; j < ent->C; j++) {
            int cidade_idx = i * ent->C + j;
            printf("R = %d, C = %d    %.1f         %.1f           %.1f         %.1f       %.1f\n", 
              i, j, 
              cidades[cidade_idx].menor, 
              cidades[cidade_idx].maior, 
              cidades[cidade_idx].mediana, 
              cidades[cidade_idx].media, 
              cidades[cidade_idx].dsvpdr);
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
  printf("          %.1f           %.1f            %.1f          %.1f        %.1f\n", 
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
  printf("Melhor Regiao:  |   R%d    | %.1f\n", regiao, media_regiao);
  printf("Melhor cidade:  | R%d-C%d   | %.1f\n", cidade / ent->C, cidade % ent->C, media_cidade);
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
        
        out->M2 = out->M2 + M2 + (delta * delta) * (out->n * medias_por_indice_in) / n_novo;
        out->media += delta * (medias_por_indice_in / n_novo);
        out->n = n_novo;

        if (maior_atual > out->maior) out->maior = maior_atual;
        if (menor_atual < out->menor) out->menor = menor_atual;
      }
    }
    return;
  }

  int mid = start + (end - start) / 2;
  ValoresChan left, right;

  chanRecursivo(in, start, mid, medias_por_indice_in, &left);
  chanRecursivo(in, mid + 1, end, medias_por_indice_in, &right);

  double n_novo = left.n + right.n;
  double delta = right.media - left.media;

  out->n = n_novo;
  out->M2 = left.M2 + right.M2 + (delta * delta) * (left.n * right.n) / n_novo;
  out->media = left.media + delta * (right.n / n_novo);
  out->maior = (left.maior > right.maior) ? left.maior : right.maior;
  out->menor = (left.menor < right.menor) ? left.menor : right.menor;
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

  double tempo = omp_get_wtime();

  double *alunos = calloc(ent.R*ent.C*ent.A, sizeof(double));
  DadosSaida *cidades = calloc(ent.R*ent.C, sizeof(DadosSaida));
  DadosSaida *regioes = calloc(ent.R, sizeof(DadosSaida));
  DadosSaida brasil = {0};

  int i, j, k;

  int tot_alunos = ent.C*ent.R*ent.A;
  int num_notas = ent.N;
  double *notas = dados.notas;

// MEDIA ALUNOS
    
    for(i = 0; i < ent.R; i++)
    {
      for(j = 0; j < ent.C; j++)
      {
          for(k = 0; k < ent.A; k++)
          {
            double soma_aluno = 0.0;
            int idx = indice_aluno(&ent, i, j, k) * num_notas;
            // calcula media
            for(int l = 0; l < num_notas; l++)
              soma_aluno += notas[idx + l];
            
            alunos[indice_aluno(&ent,i, j, k)] = soma_aluno / ent.N;
          }
      } 
    }   

// ---------------------------------------------------------------------------------------------
// MEDIA, DESVIO PADRAO, <, > CIDADE
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



// ---------------------------------------------------------------------------------------------
// MEDIA, DESVIO PADRAO, <, > REGIOES
   
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
    
    // ---------------------------------------------------------------------------------------------
// MEDIA, DESVIO PADRAO, <, > BRASIL

      ValoresChan resultado_brasil;

      int start = 0;
      int end = start + ent.R - 1;

      // desce a arvore de recursão juntando as regioes
      chanRecursivo(regioes, start, end, ent.C, &resultado_brasil);

      brasil.media = resultado_brasil.media;
      brasil.dsvpdr = (resultado_brasil.n > 1) ? sqrt(resultado_brasil.M2 / (resultado_brasil.n - 1)) : 0.0;
      brasil.maior = resultado_brasil.maior;
      brasil.menor = resultado_brasil.menor;

      // -----------------------------------------------------------------------------------------------
      // printa os alunos e corrige o tempo de execução (n lembro se é obrigatório printar isso)
      double antes = omp_get_wtime();
      // precisa imprimir as médias antes de reordenar o vetor alunos (quickselect)
      // imprimir_resultados_alunos(alunos, &ent);
      double depois = omp_get_wtime();
      tempo -= (depois - antes);
    

    // ---------------------------------------------------------------------------------------------
    // MEDIANAS CIDADE
    for(i = 0; i < ent.R; i++)
    {
      for(j = 0; j < ent.C; j++)
      {
        int idx_cidade = i*ent.C + j;
        int idx_aluno = indice_aluno(&ent, i, j, 0); // primeiro aluno de cada cidade
        
        cidades[idx_cidade].mediana = median(&alunos[idx_aluno], ent.A);
      }
    }

    
    // ---------------------------------------------------------------------------------------------
    // MEDIANAS REGIÃO
    
 
    for(i = 0; i < ent.R; i++)
    {
      int num_alunos = ent.C * ent.A;
      int idx_aluno = indice_aluno(&ent, i, 0, 0); // primeiro aluno de cada regiao
      
      regioes[i].mediana = median(&alunos[idx_aluno], num_alunos);
    }



  // ---------------------------------------------------------------------------------------------
  // MEDIANAS BRASIL
    
  brasil.mediana = median(alunos, tot_alunos);
    
  
  tempo = omp_get_wtime() - tempo; 

  
  imprimir_resultados_cidades(cidades, &ent);
  imprimir_resultados_regioes(regioes, &ent);
  imprimir_resultados_brasil(brasil);
  imprimir_premiacoes(cidades, regioes, &ent);

  printf("\nTempo de resposta em segundos, sem considerar E/S: %.3fs\n", tempo);

  free(alunos);
  free(cidades);
  free(regioes);


  liberar_dados(&dados);
  return EXIT_SUCCESS;
}
