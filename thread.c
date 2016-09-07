#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define QTDE_CIDADE 52
#define TAM_POPULACAO 512
#define QTDE_ITERACOES 10000
#define QTDE_THREADS 8
#define TX_MUTACAO 20

struct Cidade{
    int id;
    int coordX;
    int coordY;
};

struct Gene{
    int rota[QTDE_CIDADE];
    float custo;
};

struct Cidade cidades[QTDE_CIDADE];
struct Gene populacao[TAM_POPULACAO];
int pai1, pai2;
struct Gene novaPopulacao[TAM_POPULACAO];

pthread_t threads[QTDE_THREADS];
pthread_barrier_t barAntesGenetico;
pthread_barrier_t barDepoisCruzamento;
pthread_barrier_t barDepoisAtualizacao;

int numThread[] = {0,1,2,3,4,5,6,7};


void criaCidade(int id, int cX, int cY){
    cidades[id-1].id = id;
    cidades[id-1].coordX = cX;
    cidades[id-1].coordY = cY;
}

void lerArquivo(){
    char nomeArquivo[] = "berlin52.tsp";
    char c;

    char * atributo;
    char cid[10];
    char cX[10] ;
    char cY[10];

    int tamAtr;
    int l = 0;


    char * cidade, coordX, coordY;

    int fimArquivo = 0;

    int contador = 0;
    char linha[20] = "0";
    int comecoLinha;
    int tamanhoLinha = 0;
    int i = 0;
    FILE * arquivo;
    arquivo = fopen(nomeArquivo, "r");

    c = fgetc(arquivo);
    while(contador < 6){
        c = fgetc(arquivo);
        if(c == '\n')
            contador++;
    }

    while (!fimArquivo && l < QTDE_CIDADE){
        comecoLinha = ftell(arquivo);
        c = fgetc(arquivo);
        while (c != '\n'){
            tamanhoLinha++;
            c = fgetc(arquivo);
        }
        fseek(arquivo, comecoLinha, SEEK_SET);
        fgets(linha, tamanhoLinha, arquivo);
        fseek(arquivo, comecoLinha + tamanhoLinha+1, SEEK_SET);
        tamanhoLinha = 0;
        if (linha[0] == 'E'){
            fimArquivo = 1;
        }

        atributo = strtok(linha, " ");
        tamAtr = strlen(atributo);
        for (int i = 0; i < tamAtr; i++){ // pega numero da cidade
            cid[i] = atributo[i];
        }

        atributo = strtok(NULL, " ");
        tamAtr = strlen(atributo);
        for (int i = 0; i < tamAtr; i++){ // pega coordenada X da cidade
            cX[i] = atributo[i];
        }

        atributo = strtok(NULL, " ");

        tamAtr = strlen(atributo);
        for (int i = 0; i < tamAtr; i++){ // pega coordenada Y da cidade
            cY[i] = atributo[i];
        }

        criaCidade(atoi(cid), atoi(cX), atoi(cY)); // insere id, cX e cY no array de structs de cidades na pos id-1

        for (int i = 0; i < 10; i++){
            cX[i] = ' ';
            cY[i] = ' ';
        }

        l++;
    }
    fclose(arquivo);
}

void criaGeneInicial(int tamPopulacao){
    struct Gene geneGuloso;
    int aleatorio;
    int estaNaRota = -1;
    int iRota = 0;
    int cidadesNaRota = 0;
    int auxTroca;

    for (int contPopulacao = 0; contPopulacao < tamPopulacao; contPopulacao++){
        for (int i = 0; i < QTDE_CIDADE; i++){
            geneGuloso.rota[i] = i+1;
        }

        for (int i = 0; i < QTDE_CIDADE; i++){
            aleatorio = rand() % 51 + 1;
            auxTroca = geneGuloso.rota[i];
            geneGuloso.rota[i] = geneGuloso.rota[aleatorio];
            geneGuloso.rota[aleatorio] = auxTroca;
        }
        populacao[contPopulacao] = geneGuloso;
    }
}

float custoRota(int iGene){
    int custo = 0;
    float distancia = 0;
    for (int i = 0; i < QTDE_CIDADE - 1; i++){
        distancia += sqrt(pow((cidades[populacao[iGene].rota[i]-1].coordX - cidades[populacao[iGene].rota[i+1]-1].coordX), 2) + pow((cidades[populacao[iGene].rota[i]-1].coordY - cidades[populacao[iGene].rota[i+1]-1].coordY), 2));
    }
    distancia += sqrt(pow((cidades[populacao[iGene].rota[QTDE_CIDADE-1]-1].coordX - cidades[populacao[iGene].rota[0]-1].coordX), 2) + pow((cidades[populacao[iGene].rota[QTDE_CIDADE-1]-1].coordY - cidades[populacao[iGene].rota[0]-1].coordY), 2));
    return(distancia);
}

int custosIniciais(){
    for (int i = 0; i < TAM_POPULACAO; i++){
        populacao[i].custo = custoRota(i);
    }
}

void imprimePopulacao(){
    for (int i = 0; i < TAM_POPULACAO; i++){
        for(int j = 0; j < QTDE_CIDADE; j++){
            printf("[%i]", populacao[i].rota[j]);
        }
        printf("<%f>\n", populacao[i].custo);
    }
}

int selecionaPais(){
    int melhorPai = 0;
    for (int i = 1; i < TAM_POPULACAO; i++){
        if (populacao[i].custo < populacao[melhorPai].custo){
            melhorPai = i;
        }
    }

    pai1 = melhorPai;
    pai2 = rand() % TAM_POPULACAO;
    while (pai2 == pai1){
        pai2 = rand() % TAM_POPULACAO;
    }

    return pai2;
}

void cruzamento(int numT){

    struct Gene filho1, filho2;
    int posFilho;
    int posPai;

    int pedaco = TAM_POPULACAO / QTDE_THREADS;
    int inicioT = pedaco * numT;
    int fimT = inicioT + pedaco;

    for (int cont = inicioT; cont < fimT; cont+=2){
        int pai2L = selecionaPais();
        for (int i = 0; i < QTDE_CIDADE; i++){
            filho1.rota[i] = 0;
            filho2.rota[i] = 0;
        }

        for (int i = 0; i < QTDE_CIDADE/2; i++){//preenche primeira metade do gene com gene de 1 pai
            filho1.rota[i] = populacao[pai1].rota[i];
            filho2.rota[i] = populacao[pai2L].rota[i];
        }

        posPai = 0;
        posFilho = QTDE_CIDADE/2;

        while(posFilho < QTDE_CIDADE){//preenche segunda metade do gene percorrendo o outro pai a partir da pos 0 e verificando se já está no filho
            int estaNaRota = 0;
            int pos = 0;
            while (estaNaRota != 1 && pos < QTDE_CIDADE){
                if (populacao[pai2L].rota[posPai] == filho1.rota[pos]){
                    estaNaRota = 1;
                }
                else{
                    pos++;
                }
            }
            if (estaNaRota == 0){
                filho1.rota[posFilho] = populacao[pai2L].rota[posPai];
                posFilho++;
            }
            posPai++;
        }

        posPai = 0;
        posFilho = QTDE_CIDADE/2;
        while(posFilho < QTDE_CIDADE){
            int estaNaRota = 0;
            int pos = 0;
            while (estaNaRota != 1 && pos < QTDE_CIDADE){
                if (populacao[pai1].rota[posPai] == filho2.rota[pos]){
                    estaNaRota = 1;
                }
                else{
                    pos++;
                }
            }
            if (estaNaRota == 0){
                filho2.rota[posFilho] = populacao[pai1].rota[posPai];
                posFilho++;
            }
            posPai++;
        }

        novaPopulacao[cont] = filho1;
        novaPopulacao[cont+1] = filho2;
    }

}

void atualizaPopulacao(){
    for(int i = 0; i < TAM_POPULACAO; i++){
        populacao[i] = novaPopulacao[i];
    }
}

void mutacao(){
    int sorteado;
    int alcance;
    int auxMut;
    int posTroca;

    alcance = 100/TX_MUTACAO;
    for (int i = 0; i < TAM_POPULACAO; i++){
        sorteado = rand() % alcance;
        if(sorteado == 0){
            posTroca = rand() % 50 + 1;
            auxMut = populacao[i].rota[0];
            populacao[i].rota[0] = populacao[i].rota[posTroca];
            populacao[i].rota[posTroca] = auxMut;
            populacao[i].custo = custoRota(i);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void * genetico(void * v_ptr){
    int * i_ptr = (int * ) v_ptr;
    int i_Thread = * i_ptr;


    if (i_Thread == 0){//etapas de inicializacao que serao executadas somente uma vez e somente por 1 thread
        lerArquivo();
        srand(100);
        criaGeneInicial(TAM_POPULACAO);
        custosIniciais();
        imprimePopulacao();
        printf("\n");

    }
    pthread_barrier_wait(&barAntesGenetico);


    for (int i = 0; i < QTDE_ITERACOES; i++){
        cruzamento(i_Thread);

        pthread_barrier_wait(&barDepoisCruzamento);
        if (i_Thread == 0){
            atualizaPopulacao();
            mutacao();
            custosIniciais();

        }
        pthread_barrier_wait(&barDepoisAtualizacao);


    }

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(){

    pthread_barrier_init(&barAntesGenetico, NULL, QTDE_THREADS);
    pthread_barrier_init(&barDepoisCruzamento, NULL, QTDE_THREADS);
    pthread_barrier_init(&barDepoisAtualizacao, NULL, QTDE_THREADS);
    for (int t = 0; t < QTDE_THREADS; t++){
        pthread_create(&threads[t], NULL, genetico, &numThread[t]);
    }

    for (int t = 0; t < QTDE_THREADS; t++){
        pthread_join(threads[t], NULL);
    }
    pthread_barrier_destroy(&barAntesGenetico);
    pthread_barrier_destroy(&barDepoisCruzamento);
    pthread_barrier_destroy(&barDepoisAtualizacao);


    int iMelhorCusto = 0;
    for (int i = 0; i < TAM_POPULACAO; i++){
        if(populacao[i].custo < populacao[iMelhorCusto].custo){
            iMelhorCusto = i;
        }
    }

    printf("\n\n");
    printf("Melhor custo entre todos processos e threads: %f\n", populacao[iMelhorCusto].custo);
    printf("Melhor rota:\n");
    for(int i = 0; i < 52; i++){
        printf("[%i]", populacao[iMelhorCusto].rota[i]);
    }
    printf("\n");  

}
