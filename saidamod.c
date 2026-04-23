// gcc saidamod.c input.c -o aval -lm
// ./aval arq_entradas.csv
#include "input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define QTD_PAR 10
#define REP 10

double calcula_tempo_medio(double tempos[REP]){
    double soma = 0.0;
    for(int i=0; i<REP; i++)
        soma += tempos[i];
    return soma / REP;
}

double calcula_tempo_std(double tempos[REP], double media){
    double soma = 0.0;
    for(int i=0; i<REP; i++)
        soma += (tempos[i] - media) * (tempos[i] - media);
    return sqrt(soma / REP);
}

void ler_linha_csv(char *linha, Entrada *out){
    char *token = strtok(linha,",");
    // ent -> struct R, C, A, N, T, seed
    // linha ->      R, C, A, N, seed
    out->R = atoi(token);
    token = strtok(NULL,",");
    
    out->C = atoi(token);
    token = strtok(NULL,",");

    out->A = atoi(token);
    token = strtok(NULL,",");

    out->N = atoi(token);
    token = strtok(NULL,",");

    out->T = 1;

    out->seed = (unsigned int)(atoi(token));
    token = strtok(NULL,",");
}

void write_teste(Entrada *out){
    FILE *fp = fopen("arq_teste.txt", "w");

    fprintf(fp, "%d\n", out->R);
    fprintf(fp, "%d\n", out->C);
    fprintf(fp, "%d\n", out->A);
    fprintf(fp, "%d\n", out->N);
    fprintf(fp, "%d\n", out->T);
    fprintf(fp, "%u\n", out->seed);

    fclose(fp);
}

int main(int argc, char *argv[]){
    if (argc != 2) {
    fprintf(stderr, "Uso: %s <arquivo_entrada>\n", argv[0]);
    return EXIT_FAILURE;
    }

    FILE *arq_entradas = fopen(argv[1], "r");
    if(!arq_entradas) {
        printf("Nao encontrou arquivo de entradas csv\n");
        return -1;
    }

    // estrutura do arquivo: R, C, A, N, Total(R*C*A*N), T, seed, media_tempo_seq_, std_tempo_seq_, media_tempo_par_, std_tempo_par_, speedup, eficiencia
    FILE *fp;
    // tenta abrir para leitura
    fp = fopen("resultados.csv", "r");
    if (fp == NULL) {
        // arquivo não existe → cria e escreve cabeçalho
        printf("Criando resultados.csv\n");
        fp = fopen("resultados.csv", "w");
        fprintf(fp, "R,C,A,N,Total,T,seed,Tseq_mean,Tseq_std,Tpar_mean,Tpar_std,Speedup,Eficiencia\n");
    }
    fclose(fp);
    
    double tempo_seq[REP];
    double tempos_par[QTD_PAR][REP];
    double media_seq;
    double media_par[QTD_PAR];
    double std_seq;
    double std_par[QTD_PAR];
    double speedup, eficiencia;

    char row[1024];
    Entrada ent;
    // Primeira linha de cabeçalho.
    fgets(row, sizeof row, arq_entradas);
    printf("%s\n", row);
    while(fgets(row, sizeof(row), arq_entradas)){
        char *tmp = strdup(row);
        ler_linha_csv(tmp, &ent);
        
        write_teste(&ent);
        
        fp = fopen("resultados.csv", "a");

        // Algoritmo sequencial
        for(int i=0; i<REP; i++){
            system("./teste-seq arq_teste.txt");
            
            FILE *seqtesteptr = fopen("res_seq.txt","r");
            if (seqtesteptr == NULL) {
                printf("Erro abrindo arquivo de Resultado Sequencial\n");
                return 1;
            }
            fscanf(seqtesteptr,"%lf", &tempo_seq[i]);
            fclose(seqtesteptr);
        }
        // médias e desvios-padrão dos tempos sequenciais
        media_seq = calcula_tempo_medio(tempo_seq);
        std_seq = calcula_tempo_std(tempo_seq, media_seq);
        
        // Algoritmo paralelo
        // carga pequena
        for(int i=0;i<QTD_PAR;i++){
            ent.T=i+1;
            write_teste(&ent);
            for(int r=0; r<REP; r++){
                system("./teste-par arq_teste.txt");
            
                FILE *partesteptr = fopen("res_par.txt","r");
                if (partesteptr == NULL) {
                printf("Erro abrindo arquivo de Resultado Paralelo\n");
                return 1;
                }
                fscanf(partesteptr,"%lf", &tempos_par[i][r]);
                fclose(partesteptr);
            }

            media_par[i] = calcula_tempo_medio(tempos_par[i]);
            std_par[i] = calcula_tempo_std(tempos_par[i], media_par[i]);

            speedup = media_seq / media_par[i];
            eficiencia = speedup / ent.T;

            fprintf(fp, "%d,%d,%d,%d,%d,%d,%u,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
                ent.R, ent.C, ent.A, ent.N, ent.R*ent.C*ent.A*ent.N, ent.T, ent.seed,
                media_seq, std_seq, media_par[i], std_par[i], speedup, eficiencia);
        }

        fclose(fp);
        free(tmp);
    }
    
    fclose(arq_entradas);
    
    return 0;
}

/*
import pandas as pd

df = pd.read_csv("resultados.csv")
# já tem o cabeçalho!!!
cargas = df.groupby(["R","C","A","N"])
*/