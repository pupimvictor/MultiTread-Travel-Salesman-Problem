#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpi.h>

#define QTDE_CIDADE 52
#define TAM_POPULACAO 512
#define QTDE_ITERACOES 10000
#define QTDE_THREADS 8
#define TX_MUTACAO 20
#define QTD_PROC 4

typedef struct {
    int id;
    int coordX;
    int coordY;
}Cidade;

typedef struct{
    int rota[52];
    float custo;
}Gene;

Cidade cidades[52];
Cidade cida[52];
Gene populacao[TAM_POPULACAO];
Gene populacaoL[TAM_POPULACAO/QTD_PROC];
int pai1, pai2;
Gene novaPopulacao[TAM_POPULACAO];
Gene novaPopulacaoL[TAM_POPULACAO/QTD_PROC];
int rank;
int qtdeProc;


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
    Gene geneGuloso;
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

float custoRotaL(int iGene){
    int custo = 0;
    float distancia = 0;
    for (int i = 0; i < QTDE_CIDADE - 1; i++){
        distancia += sqrt(pow((cida[populacaoL[iGene].rota[i]-1].coordX - cida[populacaoL[iGene].rota[i+1]-1].coordX), 2) + pow((cida[populacaoL[iGene].rota[i]-1].coordY - cida[populacaoL[iGene].rota[i+1]-1].coordY), 2));
    }
    distancia += sqrt(pow((cida[populacaoL[iGene].rota[QTDE_CIDADE-1]-1].coordX - cida[populacaoL[iGene].rota[0]-1].coordX), 2) + pow((cida[populacaoL[iGene].rota[QTDE_CIDADE-1]-1].coordY - cida[populacaoL[iGene].rota[0]-1].coordY), 2));
    return(distancia);
}

int custosIniciais(){
    for (int i = 0; i < TAM_POPULACAO; i++){
        populacao[i].custo = custoRota(i);
    }
}

int custosParte(){
    for (int i = 0; i < TAM_POPULACAO/qtdeProc; i++){
        populacaoL[i].custo = custoRotaL(i);
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
    for (int i = 1; i < TAM_POPULACAO/qtdeProc; i++){
        if (populacaoL[i].custo < populacaoL[melhorPai].custo){
            melhorPai = i;
        }
    }

    pai1 = melhorPai;
    pai2 = rand() % TAM_POPULACAO/qtdeProc;
    while (pai2 == pai1){
        pai2 = rand() % TAM_POPULACAO/qtdeProc;
    }

    return pai2;
}

void cruzamento(int numT){

    Gene filho1, filho2;
    int posFilho;
    int posPai;

    int pedaco = TAM_POPULACAO / (QTDE_THREADS*qtdeProc);
    int inicioT = numT*pedaco;
    int fimT = inicioT + pedaco;

    for (int cont = inicioT; cont < fimT; cont+=2){
        int pai2L = selecionaPais();
        //printf("[%i|%i]\n", pai1, pai2);
        for (int i = 0; i < QTDE_CIDADE; i++){
            filho1.rota[i] = 0;
            filho2.rota[i] = 0;
        }

        for (int i = 0; i < QTDE_CIDADE/2; i++){//preenche primeira metade do gene com gene de 1 pai
            filho1.rota[i] = populacaoL[pai1].rota[i];
            filho2.rota[i] = populacaoL[pai2L].rota[i];
        }

        posPai = 0;
        posFilho = QTDE_CIDADE/2;

        while(posFilho < QTDE_CIDADE){//preenche segunda metade do gene percorrendo o outro pai a partir da pos 0 e verificando se já está no filho
            int estaNaRota = 0;
            int pos = 0;
            while (estaNaRota != 1 && pos < QTDE_CIDADE){
                if (populacaoL[pai2L].rota[posPai] == filho1.rota[pos]){
                    estaNaRota = 1;
                }
                else{
                    pos++;
                }
            }
            if (estaNaRota == 0){
                filho1.rota[posFilho] = populacaoL[pai2L].rota[posPai];
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
                if (populacaoL[pai1].rota[posPai] == filho2.rota[pos]){
                    estaNaRota = 1;
                }
                else{
                    pos++;
                }
            }
            if (estaNaRota == 0){
                filho2.rota[posFilho] = populacaoL[pai1].rota[posPai];
                posFilho++;
            }
            posPai++;
        }
        novaPopulacaoL[cont] = filho1;
        novaPopulacaoL[cont+1] = filho2;
    }

}


void atualizaPopulacao(){
    for(int i = 0; i < TAM_POPULACAO/qtdeProc; i++){
        populacaoL[i] = novaPopulacaoL[i];
    }
}

void mutacao(){
    int sorteado;
    int alcance;
    int auxMut;
    int posTroca;

    int taxaMutL = TX_MUTACAO/qtdeProc;
    alcance = 100/taxaMutL;
    for (int i = 0; i < TAM_POPULACAO/qtdeProc; i++){
        sorteado = rand() % alcance;
        if(sorteado == 0){
            posTroca = rand() % 50 + 1;
            auxMut = populacaoL[i].rota[0];
            populacaoL[i].rota[0] = populacaoL[i].rota[posTroca];
            populacaoL[i].rota[posTroca] = auxMut;
            populacaoL[i].custo = custoRotaL(i);
        }
    }
}

void * genetico(void * v_ptr){
    int * i_ptr = (int * ) v_ptr;
    int i_Thread = * i_ptr;

    for (int i = 0; i < QTDE_ITERACOES; i++){
        cruzamento(i_Thread);

        pthread_barrier_wait(&barDepoisCruzamento);
        if (i_Thread == 0){
            atualizaPopulacao();
            mutacao();
            custosParte();
        }
        pthread_barrier_wait(&barDepoisAtualizacao);
    }

}

int main(int argc, char *argv[]){

	Gene gen;
	Cidade cidad;
	cidad.id = 5;
	cidad.coordX = 3;
	cidad.coordY = 15;
	


	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &qtdeProc);

	MPI_Datatype gen_component;
	int block_lengths_gen[2];
	MPI_Aint displacementes_gen[2];
	MPI_Aint addresses_gen[3];
	MPI_Datatype list_of_types_gen[2];

	block_lengths_gen[0] = 52;
	block_lengths_gen[1] = 1;
	list_of_types_gen[0] = MPI_INT;
	list_of_types_gen[1] = MPI_FLOAT;
	MPI_Address(&gen, &addresses_gen[0]);
	MPI_Address(&(gen.rota), &addresses_gen[1]);
	MPI_Address(&(gen.custo), &addresses_gen[2]);
	for (int k = 0; k < 2; k++){
		displacementes_gen[k] = addresses_gen[k+1] - addresses_gen[0];
	}
	MPI_Type_struct(2, block_lengths_gen, displacementes_gen, list_of_types_gen, &gen_component);
	MPI_Type_commit(&gen_component);

	MPI_Datatype cidade_component;
	int block_lengths_cidad[3];
	MPI_Aint displacementes_cidad[3];
	MPI_Aint addresses_cidad[4];
	MPI_Datatype list_of_types_cidad[3];
	for (int k = 0; k < 3; k++){
		block_lengths_cidad[k] = 1;
	}
	list_of_types_cidad[0] = list_of_types_cidad[1] = list_of_types_cidad[2] = MPI_INT;
	MPI_Address(&cidad, &addresses_cidad[0]);
	MPI_Address(&(cidad.id), &addresses_cidad[1]);
	MPI_Address(&(cidad.coordX), &addresses_cidad[2]);
	MPI_Address(&(cidad.coordY), &addresses_cidad[3]);
	for (int k = 0; k < 3; k++){
		displacementes_cidad[k] = addresses_cidad[k+1] - addresses_cidad[0];
	}
	MPI_Type_struct(3, block_lengths_cidad, displacementes_cidad, list_of_types_cidad, &cidade_component);
	MPI_Type_commit(&cidade_component);
	//////////////////////////////////////////////////////

	if(rank == 0){
		lerArquivo();
		srand(TAM_POPULACAO);
		criaGeneInicial(TAM_POPULACAO);
		custosIniciais();
		imprimePopulacao();
		printf("\n");		

	}

	if(rank == 0){
		for (int i = 0; i < qtdeProc; i++){
			MPI_Send(&cidades, 52, cidade_component, i, 0, MPI_COMM_WORLD);
			MPI_Send(&populacao, TAM_POPULACAO, gen_component, i, 0, MPI_COMM_WORLD);
		}
	}
	MPI_Recv(&cida, 52, cidade_component, 0, 0, MPI_COMM_WORLD, NULL);
	Gene populaca[TAM_POPULACAO];
	MPI_Recv(&populaca, TAM_POPULACAO, gen_component, 0, 0, MPI_COMM_WORLD, NULL);


	for (int i = 0; i < TAM_POPULACAO/qtdeProc; i++){
		populacaoL[i] = populaca[TAM_POPULACAO/qtdeProc*rank+i];
	}

	///////////////////////////////////////

    pthread_barrier_init(&barAntesGenetico, NULL, QTDE_THREADS);//: calculacustos2, alterar outros
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

    Gene melhor = populacaoL[0];
    for (int i = 0; i < TAM_POPULACAO/qtdeProc; i++){
        if (melhor.custo > populacaoL[i].custo){
            melhor = populacaoL[i];
        }
    }

    MPI_Send(&melhor, 1, gen_component, 0, 0, MPI_COMM_WORLD);

    if (rank == 0){
        Gene melhores[qtdeProc];
        for (int i = 0; i < qtdeProc; i++){
            MPI_Recv(&melhores[i], 1, gen_component, i, 0, MPI_COMM_WORLD, NULL);
        }
        printf("\n\n");

        int iMelhorCusto = 0;
        for (int i = 0; i < qtdeProc; i++){
            if(melhores[i].custo < melhores[iMelhorCusto].custo){
                iMelhorCusto = i;
            }
        }

        printf("Melhor custo entre todos processos e threads: %f\n", melhores[iMelhorCusto].custo);
        printf("Melhor rota:\n");
        for(int i = 0; i < 52; i++){
            printf("[%i]", melhores[iMelhorCusto].rota[i]);
        }
        printf("\n");
    }


	MPI_Finalize();
}