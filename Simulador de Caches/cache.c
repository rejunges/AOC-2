// Nomes: Renata Junges; Yan Soares
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
typedef struct{
	int bitVal;
	int tag;
	int dirtyBit; //bit que irá indicar que há necessidade de levar o valor que foi substituido para a memória principal (para não haver inconsistência de dados)
}cache;

//Declarações de variáveis
cache *cacheL1_i,*cacheL1_d, *cacheL2, **cacheL1Mi, **cacheL1Md, **cacheL2M;
FILE *arq;
int leituraL1d=0, leituraL1i=0, leituraL2=0, escritaL1d=0, escritaL1i=0,escritaL2=0; // Número de escritas e leituras
int nsets_L1i, bsize_L1i, assoc_L1i, nsets_L1d, bsize_L1d, assoc_L1d, nsets_L2, bsize_L2, assoc_L2; //referente as caches
int numAcess=0;
int hitL1d=0, hitL1i=0, hitL2=0; //hit da L1 d e L1 i, hit da L2
int missL1i=0, missCompL1i=0, missCapL1i=0, missConfL1i=0; //miss da L1 instruções
int missL1d=0, missCompL1d=0, missCapL1d=0, missConfL1d=0; //miss da L1 dados
int missL2=0, missCompL2=0, missCapL2=0, missConfL2=0; // miss da L2
int XX=400; //Júlio não especificou, na cache os endereços abaixo de XX
int endereco, le; // le é leitura escrita
int sizeOffset, sizeIndice, sizeTag, indice, tag;

//Funções usadas
void mapeamentoDireto(int endereco, int nsets, int bsize);
void conjAssoc(int endereco, int nsets, int bsize);
void totalAssoc(int endereco, int nsets, int bsize);
void leExtensao(char *nomeArq, char *ext);
void configuraCache(int ass, int nsets, int bsize);
void leArq (char *nomeArq, char *ext);
void carregaArgumentos(char *argv[]);
void carregaArgumentosDefault();
void relatorioDeEstatistica ();
void validaArgumentos(char *argv[]);
float logBase2(int num);
void criaCache();
void nomeCache(int ass, int nset);
void dadosRelatorio (int mt, int mcom,int mcap,int mconf, int ht, int escrita, int leitura);
void decisaoCache();
void sizeTagIndice(int endereco,int nsets, int bsize, int assoc);

int main(int argc,char *argv[]){ // argc é o numero de elementos e argv são os elementos, começa no 1( pq o 0 é o ./cache )
	char nomeArq[50], ext[4];
	//configuração default
	if (argc==2){
		strcpy(nomeArq,argv[1]);
		leArq(nomeArq, ext);
		carregaArgumentosDefault();
		//printf("\n################### Configuracao da Cache: Default ###################\n");
	}
	else if(argc==11){
		strcpy(nomeArq,argv[10]);
		leArq(nomeArq, ext);
		validaArgumentos(argv);
		carregaArgumentos(argv);
	}
	else{
		printf ("ERRO: Número de argumentos inválido, tente novamente !");
		exit(1);
	}
	criaCache();//Cria a L1 de instruções e dados, e cria a L2 também

	if (strcmp(ext, "txt")==0){//se o arquivo é texto
		while (fscanf(arq, "%d", &endereco) != EOF){//vai ler linha por linha
			fscanf(arq, "%d", &le);// A segunda linha informa se é leitura=0 ou escrita=1
			numAcess++;
			decisaoCache();
		}
	}
	else { //arquivo binário
		int a,b,c,d;
		while(1){
			//Como é binário ele vai pegar 1byte por fez, e depois fazer o cálculo para conseguir o valor inteiro
			a=fgetc(arq);
			b=fgetc(arq);
			c=fgetc(arq);
			d=fgetc(arq);
			endereco=(a * 256 * 256 * 256) + (b * 256 * 256) + (c * 256) + d;
			//descobrir se é 0 ou 1
			a=fgetc(arq);
			b=fgetc(arq);
			c=fgetc(arq);
			d=fgetc(arq);
			le = (a * 256 * 256 * 256) + (b * 256 * 256) + (c * 256) + d;
			if(feof(arq)){
				break;
			}
			numAcess++;
			decisaoCache();
		}
	}
	fclose(arq);
	relatorioDeEstatistica();
	return 0;
}

void sizeTagIndice(int endereco,int nsets, int bsize, int assoc){
	sizeOffset = logBase2(bsize);   // Tamanho do offset
	sizeIndice = logBase2(nsets);   // Tamanho do indice
	sizeTag = 32-sizeIndice-sizeOffset; // Tamanho da tag

	indice = (endereco)%(nsets);				// considerando 2na n, n é o indice
	printf("SizeOffset:%d\n", sizeOffset);
	printf("SizeIndice:%d\n", sizeIndice);
	printf ("Indice: %d\n\n",indice);
	tag = (endereco >> (sizeOffset >> sizeIndice));                 // o que restar do endereço sem offset e indice
	printf("TAG:%d\n\n", tag);
}

void decisaoCache(){
	if(endereco < XX ){//dados
		//Vai para sua devida configuração
		configuraCache(assoc_L1d, nsets_L1d, bsize_L1d);
	}
	else if(endereco >= XX ){ //Vai para cache de instruções se o endereço for igual a XX ou superior
		configuraCache(assoc_L1i, nsets_L1i, bsize_L1i);
	}
}
void configuraCache(int ass, int nsets, int bsize){
	if((ass==1) && (nsets>1)){
		mapeamentoDireto(endereco, nsets, bsize);
	}
	else if((ass>1) && (nsets>1)){
		conjAssoc(endereco, nsets, bsize);
	}
	else if((ass>1) && (nsets==1)){
		totalAssoc(endereco, nsets, bsize);
	}
}

void validaArgumentos(char *argv[]){
	int i;
	for(i=1; i<10; i++){
		if((atoi(argv[i]))<=0){                                // Verifica Argumentos
			printf("ERRO: Argumentos Invalidos!\n");
			exit(1);
		}
	}
}
void leArq (char *nomeArq, char *ext){
	leExtensao(nomeArq, ext);
	if (strcmp(ext,"txt")==0){//extensao txt
		arq = fopen (nomeArq, "r");
		if (arq == NULL){
			printf ("Erro, não foi possível abrir o arquivo");
			exit(1);
		}
	}
	else if(strcmp(ext,"dat")==0){//extensao dat(binaria)
			arq=fopen(nomeArq, "rb");
			if (arq == NULL){
				printf ("Erro, não foi possível abrir o arquivo");
				exit(1);
			 }
		 }
	else{ // extensao inválida
		printf ("ERRO: Extensão do arquivo inválida, tente novamente!");
		exit(1);
	}
}
void relatorioDeEstatistica (){
	printf ("\n\t\t###############################\n\t\t## Relatório de Estatísticas ##\n\t\t###############################\n\n");
	printf ("*Número de acessos: %d\n", numAcess);
	printf("\n\n################### Cache L1 de dados ##################\n\n");
	// Informa qual cache se refere
	nomeCache(assoc_L1d, nsets_L1d);
	dadosRelatorio(missL1d, missCompL1d, missCapL1d, missConfL1d, hitL1d, escritaL1d, leituraL1d);
	printf("\n\n################ Cache L1 de instruções ################\n\n");
	// Informa qual cache se refere
	nomeCache(assoc_L1i, nsets_L1i);
	dadosRelatorio(missL1i, missCompL1i, missCapL1i, missConfL1i, hitL1i, escritaL1i, leituraL1i);
	printf("\n\n####################### Cache L2 #######################\n\n");
	// Informa qual cache se refere
	nomeCache(assoc_L2, nsets_L2);
	dadosRelatorio(missL2, missCompL2, missCapL2, missConfL2, hitL2, escritaL2, leituraL2);
	printf("\n\n########################################################\n\n");

}
void dadosRelatorio (int mt, int mcom,int mcap,int mconf, int ht, int escrita, int leitura){
	float hr, mr; //hit ratio e miss ratio
	hr= (float)(ht*100)/numAcess; // hit total * 100 / num acesso = hit ratio
	mr= (float)(mt*100)/numAcess;
	printf ("\t*Leitura: %d\n", leitura);
	printf ("\t*Escrita: %d\n", escrita);
	printf ("\t*Total de Hit: %d\n", ht);
	printf ("\t*Total de Miss: %d\n", mt);
	printf ("\t\t-Quantidade de Miss Compulsório: %d\n", mcom);
	printf ("\t\t-Quantidade de Miss de Capacidade: %d\n", mcap);
	printf ("\t\t-Quantidade de Miss de Conflito: %d\n", mconf);
	printf ("\t*Hit Ratio: %.2f\n", hr);
	printf ("\t*MIss Ratio: %.2f\n", mr);
}

void nomeCache(int ass, int nset){
	// Informa qual cache se refere
	if((ass==1) && (nset>1)){
		printf("-----> Cache Direta\n\n");
	}
	else if((ass>1) && (nset>1)){
		printf("-----> Cache Conjunto Associativo\n\n");
	}
	else if((ass>1) && (nset==1)){
		printf("-----> Cache Totalmente Associativa\n\n");
	}
}
void carregaArgumentos(char *argv[]){
	nsets_L1i = atoi(argv[1]);               // carrega argumentos
	bsize_L1i = atoi(argv[2]);
	assoc_L1i = atoi(argv[3]);
	nsets_L1d = atoi(argv[4]);
	bsize_L1d = atoi(argv[5]);
	assoc_L1d = atoi(argv[6]);
	nsets_L2 = atoi(argv[7]);
	bsize_L2 = atoi (argv[8]);
	assoc_L2 = atoi (argv[9]);
}
void carregaArgumentosDefault(){
	/*cache com mapeamento direto com tamanho de bloco de 4 bytes
	 * e com 1024 conjuntos (nas duas caches)*/
	nsets_L1i = 8;
	bsize_L1i = 4;
	assoc_L1i = 1;
	nsets_L1d = 8;
	bsize_L1d = 4;
	assoc_L1d = 1;
	nsets_L2 = 8;
	bsize_L2 = 4;
	assoc_L2 = 1;
}
void leExtensao(char *nomeArq, char *ext){
	int i,tam,j=3;
	tam=strlen(nomeArq);
	//Armazena em ext a extensao do arquivo(txt ou dat)
	for (i=tam; i>(tam-4); i--){
		 ext[j]=nomeArq[i];
		 j--;
	}
}
float logBase2(int num){
     return log10(num)/log10(2);         //log base 2

}
void criaCache(){
	int i,j;
	//tratando a L1 de instruções
	if((assoc_L1i>1) && (nsets_L1i>1)){
		//Matriz por causa da associatividade
		cacheL1Mi = malloc((sizeof(cache))*nsets_L1i);
		for(j = 0; j< nsets_L1i; j++){
			cacheL1Mi[j]=malloc((sizeof(cache))*assoc_L1i);
		}
		//"limpa" as posições
		for (i=0; i<nsets_L1i; i++){
			for (j=0; j<assoc_L1i; j++){
				cacheL1Mi[i][j].bitVal = 0;
				cacheL1Mi[i][j].tag = 0;
				cacheL1Mi[i][j].dirtyBit = 0;
			}
		}
	}
	else {
		//Não é necessário matriz
		cacheL1_i = malloc((sizeof(cache))*nsets_L1i);
		//"limpa" as posições
		for(i=0; i<nsets_L1i; i++){
			cacheL1_i[i].tag = 0;
			cacheL1_i[i].bitVal = 0;
			cacheL1_i[i].dirtyBit = 0;
		}
	}
	//Mesmo código acima para L1 de dados
	if((assoc_L1d>1) && (nsets_L1d>1)){
		cacheL1Md = malloc((sizeof(cache))*nsets_L1d);
		for(j = 0; j< nsets_L1i; j++){
			cacheL1Md[j] = malloc((sizeof(cache))*assoc_L1d);
		}
		for (i=0; i<nsets_L1d; i++){
			for (j=0; j<assoc_L1d; j++){
				cacheL1Md[i][j].bitVal = 0;
				cacheL1Md[i][j].tag = 0;
				cacheL1Md[i][j].dirtyBit = 0;
			}
		}
	}
	else {
		cacheL1_d = malloc((sizeof(cache))*nsets_L1d);
		for(i=0; i<nsets_L1d; i++){
			cacheL1_d[i].tag = 0;
			cacheL1_d[i].bitVal = 0;
			cacheL1_d[i].dirtyBit=0;
		}
	}
	//Mesmo código para a L2
	if((assoc_L2>1) && (nsets_L2>1)){
		cacheL2M = malloc((sizeof(cache))*nsets_L2);
		for(j = 0; j< nsets_L1i; j++){
			cacheL2M[j] = malloc((sizeof(cache))*assoc_L2);
		}
		for (i=0; i<nsets_L1d; i++){
			for (j=0; j<assoc_L1d; j++){
				cacheL2M[i][j].bitVal = 0;
				cacheL2M[i][j].tag = 0;
				//cacheL2M[i].[j].dirtyBit=0;
			}
		}
	}
	else {
		cacheL2 = malloc((sizeof(cache))*nsets_L2);
		for(i=0; i<nsets_L1d; i++){
			cacheL2[i].tag = 0;
			cacheL2[i].bitVal = 0;

		}
	}
}

void conjAssoc(int endereco, int nsets, int bsize){
	int i, aux, flagAchouL1=0, flagAchouIgual=0, oldaux;

	if(endereco < XX && le == 0){ //dado e leitura
		sizeTagIndice(endereco,nsets, bsize, assoc_L1d); //calcula o indice e tag
		for(i=0; i<assoc_L1d; i++){ //procura em todas as "colunas" para ver se há o valor
			if(cacheL1Md[indice][i].bitVal == 1 && cacheL1Md[indice][i].tag == tag){ //hit em L1
				hitL1d++;
				leituraL1d++;
				flagAchouL1=1;
				break;
			}
		}
		if(flagAchouL1 == 0){ //caso dê miss
			missL1d++;
			aux = rand()%assoc_L1d;
			if(cacheL1Md[indice][aux].bitVal == 0){ //miss compulsório
				missCompL1d++;
				escritaL1d++;
				cacheL1_d[aux].tag = tag;
				cacheL1_d[aux].bitVal = 1;
				sizeTagIndice(endereco, nsets_L2, bsize_L2, assoc_L2);
				for(i=0; i<assoc_L2; i++){
					if(cacheL2M[indice][i].bitVal == 1 && cacheL2M[indice][i].tag == tag){ //hit em L2
						hitL2++;
						flagAchouIgual = 1;
						break;
					}
				}
				if(flagAchouIgual == 0){ //miss em L2, deve pegar o valor da memória e escrever em L2 (já está escrito em L1)
					aux = rand()%assoc_L2;
					if(cacheL2[aux].bitVal == 1){ //miss de conflito
						missConfL2++;
						escritaL2++;
						cacheL2[aux].tag = tag;
					}
					if(cacheL2[aux].bitVal == 0){ //miss compulsório
						missCompL2++;
						escritaL2++;
						cacheL2[aux].tag = tag;
						cacheL2[aux].bitVal = 1;
					}
				}
			}
			else{ //bit de validade é 1, já há um valor inserido no bloco desejado
				if(cacheL1Md[indice][aux].bitVal == 1){
					missConfL1d++;
					if(cacheL1Md[indice][aux].dirtyBit == 0){ //dirtybit em 0
						escritaL1d++;
						cacheL1_d[aux].tag = tag;
						sizeTagIndice(endereco, nsets_L2, bsize_L2, assoc_L2);
						for(i=0; i<assoc_L2; i++){	//por precaução, procura em L2 se já há o valor 
							if(cacheL2M[indice][i].tag == tag && cacheL2M[indice][i].bitVal == 1){
								hitL2++;
								leituraL2++;
								flagAchouIgual = 1;
								break;
							}
						}
						if(flagAchouIgual == 0){ //só vai escrever em L2  se já não houver o valor lá
							aux = rand()%assoc_L2;
							escritaL2++;
							cacheL2M[indice][aux].bitVal = 1;
							cacheL2M[indice][aux].tag = tag;
						}
					}
					else{ //dirtybit em 1
						if(cacheL1Md[indice][aux].dirtyBit == 1){
							int oldEndereco;
							oldaux = aux; //armazena o "aleatório" que esta sendo usado na L1, pois é escolhido outro aleatorio pra L2 de acordo com sua associatividade
							oldEndereco= ((cacheL1Md[indice][aux].tag << sizeIndice) << sizeOffset) | (sizeIndice << sizeOffset) | sizeOffset;
							sizeTagIndice(oldEndereco,nsets_L2, bsize_L2, assoc_L2);
							aux = rand()%assoc_L2;
							cacheL2M[indice][aux].tag= tag; //Atualiza valor
							cacheL2M[indice][aux].bitVal=1;
							escritaL2++;
							sizeTagIndice(endereco,nsets, bsize, assoc_L1d); //dados da L1
							cacheL1Md[indice][oldaux].tag=tag;
							escritaL1d++;
						}
					}
				}
			}
		}
	}
	if(endereco > XX && le == 0){ //instrução e leitura
		sizeTagIndice(endereco,nsets, bsize, assoc_L1i);
		for(i=0; i<assoc_L1i; i++){ //procura em todas as "colunas" para ver se há o valor
			if(cacheL1Mi[indice][i].bitVal == 1 && cacheL1Mi[indice][i].tag == tag){ //hit em L1
				hitL1i++;
				leituraL1i++;
				flagAchouL1=1;
				break;
			}
		}
		if(flagAchouL1 == 0){ //caso dê miss
			missL1d++;
			aux = rand()%assoc_L1i;
			if(cacheL1Mi[indice][aux].bitVal == 0){ //miss compulsório
				missCompL1i++;
				escritaL1i++;
				cacheL1Mi[indice][aux].tag = tag;
				cacheL1Mi[indice][aux].bitVal = 1;
				sizeTagIndice(endereco, nsets_L2, bsize_L2, assoc_L2);
				for(i=0; i<assoc_L2; i++){
					if(cacheL2M[indice][i].bitVal == 1 && cacheL2M[indice][i].tag == tag){ //hit em L2
						hitL2++;
						flagAchouIgual = 1;
						break;
					}
				}
				if(flagAchouIgual == 0){ //miss em L2, deve pegar o valor da memória e escrever em L2 (já está escrito em L1)
					aux = rand()%assoc_L2;
					if(cacheL2M[indice][aux].bitVal == 1){ //miss de conflito
						missConfL2++;
						escritaL2++;
						cacheL2M[indice][aux].tag = tag;
					}
					if(cacheL2[aux].bitVal == 0){ //miss compulsório
						missCompL2++;
						escritaL2++;
						cacheL2M[indice][aux].tag = tag;
						cacheL2M[indice][aux].bitVal = 1;
					}
				}
			}
			else{ //bit de validade é 1, já há um valor inserido no bloco desejado
				if(cacheL1Mi[indice][aux].bitVal == 1){
					missConfL1i++;
					if(cacheL1Mi[indice][aux].dirtyBit == 0){ //dirtybit em 0
						escritaL1i++;
						cacheL1Mi[indice][aux].tag = tag;
						sizeTagIndice(endereco, nsets_L2, bsize_L2, assoc_L2);
						for(i=0; i<assoc_L2; i++){	//por precaução, procura em L2 se já há o valor 
							if(cacheL2M[indice][i].tag == tag && cacheL2M[indice][i].bitVal == 1){
								hitL2++;
								leituraL2++;
								flagAchouIgual = 1;
								break;
							}
						}
						if(flagAchouIgual == 0){ //só vai escrever em L2  se já não houver o valor lá
							aux = rand()%assoc_L2;
							escritaL2++;
							cacheL2M[indice][aux].bitVal = 1;
							cacheL2M[indice][aux].tag = tag;
						}
					}
					else{ //dirtybit em 1
						if(cacheL1Mi[indice][aux].dirtyBit == 1){
							int oldEndereco;
							oldaux = aux; //armazena o "aleatório" que esta sendo usado na L1, pois é escolhido outro aleatorio pra L2 de acordo com sua associatividade
							oldEndereco= ((cacheL1Mi[indice][aux].tag << sizeIndice) << sizeOffset) | (sizeIndice << sizeOffset) | sizeOffset;
							sizeTagIndice(oldEndereco,nsets_L2, bsize_L2, assoc_L2);
							aux = rand()%assoc_L2;
							cacheL2M[indice][aux].tag= tag; //Atualiza valor
							cacheL2M[indice][aux].bitVal=1;
							escritaL2++;
							sizeTagIndice(endereco,nsets, bsize, assoc_L1i); //dados da L1
							cacheL1Mi[indice][oldaux].tag=tag;
							escritaL1i++;
						}
					}
				}
			}
		}
	}
	if(endereco < XX && le == 1){ //dado e escrita
		sizeTagIndice(endereco,nsets, bsize, assoc_L1d);
		for(i=0; i<=assoc_L1d; i++){ //procura para ver se o endereço a ser escrito já não está na cache
			if(cacheL1Md[indice][i].tag == tag){
				flagAchouIgual = 1; //o endereço a ser escrito já está escrito na cache
				break;
			}
		}
		if(flagAchouIgual == 0){ //o bloco a ser escrito não existe na cache, deve realmente ser escrito
			for(i=0; i<=assoc_L1d; i++){
				if(cacheL1Md[indice][i].bitVal == 0){ //vai procurar na L1 um lugar "vago" para escrever
				escritaL1d++;
				hitL1d++; //escreveu na L1 de dados
				cacheL1Md[indice][i].tag = tag;
				cacheL1Md[indice][i].bitVal = 1;
				cacheL1Md[indice][i].dirtyBit = 1;
				flagAchouL1=1;
				break;
				}
			}
			if(flagAchouL1 == 0){ //não achou um bloco vago para escrever, vai escrever em um aleatório
				aux = rand()%assoc_L1d;
				if(cacheL1Md[indice][aux].dirtyBit == 0){ //se o dirty do bloco a ser escrito for 0, apenas escreve
					escritaL1d++;
					cacheL1Md[indice][aux].tag = tag;
					cacheL1Md[indice][aux].dirtyBit = 1;
				}
				else{
					if(cacheL1Md[indice][aux].dirtyBit == 1){ //se o dirty está em 1, é porque o valor a ser escrito não está na L2
						int oldEndereco;
						//soma em binário para conseguir o endereço antigo para atualizar L2, write-back
						oldEndereco= ((cacheL1Md[indice][aux].tag << sizeIndice) << sizeOffset) | (sizeIndice << sizeOffset) | sizeOffset;
						sizeTagIndice(oldEndereco,nsets_L2, bsize_L2, assoc_L2);
						oldaux = aux;
						aux = rand()%assoc_L2;
						cacheL2M[indice][aux].tag= tag; //Atualiza valor
						cacheL2M[indice][aux].bitVal=1;
						escritaL2++;
						sizeTagIndice(endereco,nsets, bsize, assoc_L2); //dados da L1
						cacheL1Md[indice][oldaux].tag=tag;
						escritaL1d++;
					}
				}
			}
		}
	}
	if(endereco > XX && le == 1){ //instrução e escrita
		sizeTagIndice(endereco,nsets, bsize, assoc_L1i);
		for(i=0; i<=assoc_L1i; i++){ //procura para ver se o endereço a ser escrito já não está na cache
			if(cacheL1Mi[indice][i].tag == tag){
				flagAchouIgual = 1; //o endereço a ser escrito já está escrito na cache
				break;
			}
		}
		if(flagAchouIgual == 0){ //o bloco a ser escrito não existe na cache, deve realmente ser escrito
			for(i=0; i<=assoc_L1i; i++){
				if(cacheL1Mi[indice][i].bitVal == 0){ //vai procurar na L1 um lugar "vago" para escrever
				escritaL1i++;
				hitL1i++; //escreveu na L1 de dados
				cacheL1Mi[indice][i].tag = tag;
				cacheL1Mi[indice][i].bitVal = 1;
				cacheL1Mi[indice][i].dirtyBit = 1;
				flagAchouL1=1;
				break;
				}
			}
			if(flagAchouL1 == 0){ //não achou um bloco vago para escrever, vai escrever em um aleatório
				aux = rand()%assoc_L1i;
				if(cacheL1Mi[indice][aux].dirtyBit == 0){ //se o dirty do bloco a ser escrito for 0, apenas escreve
					escritaL1i++;
					cacheL1Mi[indice][aux].tag = tag;
					cacheL1Mi[indice][aux].dirtyBit = 1;
				}
				else{
					if(cacheL1Mi[indice][aux].dirtyBit == 1){ //se o dirty está em 1, é porque o valor a ser escrito não está na L2
						int oldEndereco;
						//soma em binário para conseguir o endereço antigo para atualizar L2, write-back
						oldEndereco= ((cacheL1Mi[indice][aux].tag << sizeIndice) << sizeOffset) | (sizeIndice << sizeOffset) | sizeOffset;
						sizeTagIndice(oldEndereco,nsets_L2, bsize_L2, assoc_L2);
						oldaux = aux;
						aux = rand()%assoc_L2;
						cacheL2M[indice][aux].tag= tag; //Atualiza valor
						cacheL2M[indice][aux].bitVal=1;
						escritaL2++;
						sizeTagIndice(endereco,nsets, bsize, assoc_L1i); //dados da L1
						cacheL1Mi[indice][oldaux].tag=tag;
						escritaL1i++;
					}
				}
			}
		}
	}
}

void totalAssoc(int endereco, int nsets, int bsize){
	int i, aux, flagAchouL1=0, DouI = 0, flagAchouIgual = 0, oldaux;

	if(endereco<XX)DouI = 0;
	else DouI = 1;

	if(DouI == 0){ //endereço menor que valor estipulado para divisão entre memória de dados e memória de instruções - entrou na mem. de dados
		if(le == 0){ //leitura
			sizeTagIndice(endereco,nsets, bsize, assoc_L1d);
			for(i=0; i<=assoc_L1d; i++){
				if((cacheL1_d[i].tag == tag) && (cacheL1_d[i].bitVal == 1)){
					leituraL1d++;
					hitL1d++; //achou na Cache L1 de dados
					flagAchouL1=1;
					break; //sai da iteração, pois já achou o dado desejado em alguma posição da cache (totalmente associativa pode estar em QUALQUER bloco)
				}
			}
			if(flagAchouL1 == 0){
				missL1d++; //não achou na Cache L1 de dados, vai ter que escrever em algum lugar aleatório e tratar isso depois na L2
				aux = rand()%assoc_L1d;
				if(cacheL1_d[aux].bitVal == 0){
					missCompL1d++;
					escritaL1d++;
					cacheL1_d[aux].tag = tag;
					cacheL1_d[aux].bitVal = 1;
					sizeTagIndice(endereco, nsets_L2, bsize_L2, assoc_L2);
					//procura na L2 se já há o valor sendo buscado, se não houver, busca da memória e escreve em L2
					for(i=0; i<assoc_L2; i++){
						if(cacheL2[i].tag == tag && cacheL2[i].bitVal == 1){ //achou na L2, da hit
							hitL2++;
							flagAchouIgual = 1;
							break;
						}
					}
					if(flagAchouIgual == 0){ //não deu HIT na L2, tem que puxar o valor da memória principal para algum bloco aleatório da L2
						aux = rand()%assoc_L2;
						if(cacheL2[aux].bitVal == 1){
							missConfL2++;
							escritaL2++;
							cacheL2[aux].tag = tag;
						}
						if(cacheL2[aux].bitVal == 0){
							missCompL2++;
							escritaL2++;
							cacheL2[aux].tag = tag;
							cacheL2[aux].bitVal = 1;
						}
					}
				}
				else{ //já há um valor no bloco que quer inserir
					if(cacheL1_d[aux].bitVal == 1){
						missConfL1d++;
						if(cacheL1_d[aux].dirtyBit == 0){ //só substitui
							escritaL1d++;
							cacheL1_d[aux].tag = tag;
							sizeTagIndice(endereco, nsets_L2, bsize_L2, assoc_L2);
							for(i=0; i<assoc_L2; i++){	//por precaução, procura em L2 se já há o valor 
								if(cacheL2[i].tag == tag && cacheL2[i].bitVal == 1){
									hitL2++;
									leituraL2++;
									flagAchouIgual = 1;
									break;
								}
							}
							if(flagAchouIgual == 0){ //só vai escrever em L2  se já não houver o valor lá
								aux = rand()%assoc_L2;
								escritaL2++;
								cacheL2[aux].bitVal = 1;
								cacheL2[aux].tag = tag;
							}
						}
						else{
							if(cacheL1_d[aux].dirtyBit == 1){
								int oldEndereco;
								oldaux = aux; //armazena o "aleatório" que esta sendo usado na L1, pois é escolhido outro aleatorio pra L2 de acordo com sua associatividade
								oldEndereco= ((cacheL1_d[aux].tag << sizeIndice) << sizeOffset) | (sizeIndice << sizeOffset) | sizeOffset;
								sizeTagIndice(oldEndereco,nsets_L2, bsize_L2, assoc_L2);
								aux = rand()%assoc_L2;
								cacheL2[aux].tag= tag; //Atualiza valor
								cacheL2[aux].bitVal=1;
								escritaL2++;
								sizeTagIndice(endereco,nsets, bsize, assoc_L1d); //dados da L1
								cacheL1_d[oldaux].tag=tag;
								escritaL1d++;
							}
						}
					}
				}
			}
		}
		if(le == 1){ //escrita
			sizeTagIndice(endereco,nsets, bsize, assoc_L1d);
			for(i=0; i<=assoc_L1d; i++){ //procura para ver se o endereço a ser escrito já não está na cache
				if(cacheL1_d[i].tag == tag){
					flagAchouIgual = 1; //o endereço a ser escrito já está escrito na cache
					break;
				}
			}
			if(flagAchouIgual == 0){ //o bloco a ser escrito não existe na cache, deve realmente ser escrito
				for(i=0; i<=assoc_L1d; i++){
					if(cacheL1_d[i].bitVal == 0){ //vai procurar na L1 um lugar "vago" para escrever
					escritaL1d++;
					hitL1d++; //escreveu na L1 de dados
					cacheL1_d[i].tag = tag;
					cacheL1_d[i].bitVal = 1;
					cacheL1_d[i].dirtyBit = 1;
					flagAchouL1=1;
					break;
					}
				}
				if(flagAchouL1 == 0){ //não achou um bloco vago para escrever, vai escrever em um aleatório
					aux = rand()%assoc_L1d;
					if(cacheL1_d[aux].dirtyBit == 0){ //se o dirty do bloco a ser escrito for 0, apenas escreve
						escritaL1d++;
						cacheL1_d[aux].tag = tag;
						cacheL1_d[aux].dirtyBit = 1;
					}
					else{
						if(cacheL1_d[aux].dirtyBit == 1){ //se o dirty está em 1, é porque o valor a ser escrito não está na L2
							int oldEndereco;
							//soma em binário para conseguir o endereço antigo para atualizar L2, write-back
							oldEndereco= ((cacheL1_d[indice].tag << sizeIndice) << sizeOffset) | (sizeIndice << sizeOffset) | sizeOffset;
							sizeTagIndice(oldEndereco,nsets_L2, bsize_L2, assoc_L2);
							oldaux = aux;
							aux = rand()%assoc_L2;
							cacheL2[aux].tag= tag; //Atualiza valor
							cacheL2[aux].bitVal=1;
							escritaL2++;
							sizeTagIndice(endereco,nsets, bsize, assoc_L1d); //dados da L1
							cacheL1_d[oldaux].tag=tag;
							escritaL1d++;
						}
					}
				}
			}
		}
	}
	else{ //endereço é INSTRUÇÃO (acima do XX -> IouD = 1)
		if(le == 0){ //leitura
			sizeTagIndice(endereco,nsets, bsize, assoc_L1i);
			for(i=0; i<=assoc_L1i; i++){
				if((cacheL1_i[i].tag == tag) && (cacheL1_i[i].bitVal == 1)){
					leituraL1i++;
					hitL1i++; //achou na Cache L1 de dados
					flagAchouL1=1;
					break; //sai da iteração, pois já achou o dado desejado em alguma posição da cache (totalmente associativa pode estar em QUALQUER bloco)
				}
			}
			if(flagAchouL1 == 0){
				missL1i++; //não achou na Cache L1 de dados, vai ter que escrever em algum lugar aleatório e tratar isso depois na L2
				aux = rand()%assoc_L1i;
				if(cacheL1_i[aux].bitVal == 0){
					missCompL1i++;
					escritaL1i++;
					cacheL1_i[aux].tag = tag;
					cacheL1_i[aux].bitVal = 1;
					sizeTagIndice(endereco, nsets_L2, bsize_L2, assoc_L2);
					//procura na L2 se já há o valor sendo buscado, se não houver, busca da memória e escreve em L2
					for(i=0; i<assoc_L2; i++){
						if(cacheL2[i].tag == tag && cacheL2[i].bitVal == 1){ //achou na L2, da hit
							hitL2++;
							leituraL2++;
							flagAchouIgual = 1;
							break;
						}
					}
					if(flagAchouIgual == 0){ //não deu HIT na L2, tem que puxar o valor da memória principal para algum bloco aleatório da L2
						aux = rand()%assoc_L2;
						if(cacheL2[aux].bitVal == 1){
							missConfL2++;
							escritaL2++;
							cacheL2[aux].tag = tag;
						}
						if(cacheL2[aux].bitVal == 0){
							missCompL2++;
							escritaL2++;
							cacheL2[aux].tag = tag;
							cacheL2[aux].bitVal = 1;
						}
					}
				}
				else{ //já há um valor no bloco que quer inserir
					if(cacheL1_i[aux].bitVal == 1){
						missConfL1i++;
						if(cacheL1_i[aux].dirtyBit == 0){ //só substitui
							escritaL1i++;
							cacheL1_i[aux].tag = tag;
							sizeTagIndice(endereco, nsets_L2, bsize_L2, assoc_L2);
							for(i=0; i<assoc_L2; i++){	//por precaução, procura em L2 se já há o valor 
								if(cacheL2[i].tag == tag && cacheL2[i].bitVal == 1){
									hitL2++;
									flagAchouIgual = 1;
									break;
								}
							}
							if(flagAchouIgual == 0){ //só vai escrever em L2  se já não houver o valor lá
								aux = rand()%assoc_L2;
								escritaL2++;
								cacheL2[aux].bitVal = 1;
								cacheL2[aux].tag = tag;
							}
						}
						else{
							if(cacheL1_i[aux].dirtyBit == 1){
								int oldEndereco;
								oldaux = aux; //armazena o "aleatório" que esta sendo usado na L1, pois é escolhido outro aleatorio pra L2 de acordo com sua associatividade
								oldEndereco= ((cacheL1_i[aux].tag << sizeIndice) << sizeOffset) | (sizeIndice << sizeOffset) | sizeOffset;
								sizeTagIndice(oldEndereco,nsets_L2, bsize_L2, assoc_L2);
								aux = rand()%assoc_L2;
								cacheL2[aux].tag= tag; //Atualiza valor
								cacheL2[aux].bitVal=1;
								escritaL2++;
								sizeTagIndice(endereco,nsets, bsize, assoc_L1i); //dados da L1
								cacheL1_i[oldaux].tag=tag;
								escritaL1i++;
							}
						}
					}
				}
			}
		}
		if(le == 1){ //escrita
			sizeTagIndice(endereco,nsets, bsize, assoc_L1i);
			for(i=0; i<=assoc_L1i; i++){ //procura para ver se o endereço a ser escrito já não está na cache
				if(cacheL1_i[i].tag == tag){
					flagAchouIgual = 1; //o endereço a ser escrito já está escrito na cache
					break;
				}
			}
			if(flagAchouIgual == 0){ //o bloco a ser escrito não existe na cache, deve realmente ser escrito
				for(i=0; i<=assoc_L1i; i++){
					if(cacheL1_i[i].bitVal == 0){ //vai procurar na L1 um lugar "vago" para escrever
					escritaL1i++;
					hitL1i++; //escreveu na L1 de dados
					cacheL1_i[i].tag = tag;
					cacheL1_i[i].bitVal = 1;
					cacheL1_i[i].dirtyBit = 1;
					flagAchouL1=1;
					break;
					}
				}
				if(flagAchouL1 == 0){ //não achou um bloco vago para escrever, vai escrever em um aleatório
					aux = rand()%assoc_L1i;
					if(cacheL1_i[aux].dirtyBit == 0){ //se o dirty do bloco a ser escrito for 0, apenas escreve
						escritaL1i++;
						cacheL1_i[aux].tag = tag;
						cacheL1_i[aux].dirtyBit = 1;
					}
					else{
						if(cacheL1_i[aux].dirtyBit == 1){ //se o dirty está em 1, é porque o valor a ser escrito não está na L2
							int oldEndereco;
							//soma em binário para conseguir o endereço antigo para atualizar L2, write-back
							oldEndereco= ((cacheL1_i[indice].tag << sizeIndice) << sizeOffset) | (sizeIndice << sizeOffset) | sizeOffset;
							sizeTagIndice(oldEndereco,nsets_L2, bsize_L2, assoc_L2);
							oldaux = aux;
							aux = rand()%assoc_L2;
							cacheL2[aux].tag= tag; //Atualiza valor
							cacheL2[aux].bitVal=1;
							escritaL2++;
							sizeTagIndice(endereco,nsets, bsize, assoc_L1i); //dados da L1
							cacheL1_i[oldaux].tag=tag;
							escritaL1i++;
						}
					}
				}
			}
		}
	}
}

void mapeamentoDireto(int endereco,int nsets, int bsize){
	//Substitução nao é randomica pq é mapeamentoDireto

	printf ("Endereco:%d 	Indice:%d\n", endereco, indice);
	if(endereco < XX && le==0){   // DADOS  e leitura
		sizeTagIndice(endereco,nsets, bsize, assoc_L1d);
		if(cacheL1_d[indice].bitVal == 0){//Se o bit validade é 0  pq é o primeiro acesso a ela
			missL1d++;                                   // Miss total de dados sobe
			missCompL1d++;                              // Esse é compulsorio
			cacheL1_d[indice].bitVal = 1;       // validade = 1
			escritaL1d++; //escreverá em L1d
			cacheL1_d[indice].tag = tag;
			//Trata a L2-> se um dado está na memória i ela precisa estar em i+1
			sizeTagIndice(endereco,nsets_L2, bsize_L2, assoc_L2); //dados L2
			if (cacheL2[indice].bitVal == 0 ){//vai ter que buscar da memória
				missL2++;
				missCompL2++; //Compulsório
				cacheL2[indice].bitVal=1;
				escritaL2++;
				cacheL2[indice].tag=tag;
			}
			else{ if (cacheL2[indice].bitVal==1 && cacheL2[indice].tag == tag){
				hitL2++;
				leituraL2++;
			}
				 if (cacheL2[indice].bitVal==1 && cacheL2[indice].tag != tag){ //dado não está em L2, pega da memória principal e traz pra L2
					missL2++;
					missConfL2++;
					cacheL2[indice].tag = tag;
					escritaL2++;
				}
			}
		}
		else {//bit validade 1, já usou essa memoria, vai ocorrer hit ou substituicao
			if(cacheL1_d[indice].tag == tag){
				hitL1d++; //encontrou, hit
				leituraL1d++;
			}
			else {     //nao encontrou, precisa confererir o dirty
				missL1d++;
				missConfL1d++;                // miss conflito
				if(cacheL1_d[indice].dirtyBit == 0){
					escritaL1d++;
					cacheL1_d[indice].tag = tag;           // seta novo tag
					//Trata L2
					sizeTagIndice(endereco,nsets_L2, bsize_L2, assoc_L2); //dados L2
					if(cacheL2[indice].tag != tag){ //só vai escrever se o que houver na L2 não for o dado desejado
						escritaL2++;
						cacheL2[indice].bitVal=1;
						cacheL2[indice].tag= tag;
					}
					if(cacheL2[indice].tag == tag){ //se já houver em L2 o dado desejado, apenas da hit
						leituraL2++;
						hitL2++;
					}
				}
				else {// dirtyBit ==1
					int oldEndereco;
					//soma em binário para conseguir o endereço antigo para atualizar L2, write-back
					oldEndereco= ((cacheL1_d[indice].tag << sizeIndice) << sizeOffset) | (sizeIndice << sizeOffset) | sizeOffset;
					sizeTagIndice(oldEndereco,nsets_L2, bsize_L2, assoc_L2);
					cacheL2[indice].tag= tag; //Atualiza valor
					cacheL2[indice].bitVal=1;
					escritaL2++;
					sizeTagIndice(endereco,nsets, bsize, assoc_L1d); //dados da L1
					cacheL1_d[indice].tag=tag;
					escritaL1d++;
				}
			}
		}
	}
	else if(endereco >= XX && le==0){ // INSTRUÇOES
		sizeTagIndice(endereco,nsets, bsize, assoc_L1i);
		if(cacheL1_i[indice].bitVal == 0){//Se o bit validade é 0  pq é o primeiro acesso a ela
			missL1i++;                                   // Miss total de dados sobe
			missCompL1i++;                              // Esse é compulsorio
			cacheL1_i[indice].bitVal = 1;       // validade = 1
			escritaL1i++; //escreverá em L1d
			cacheL1_i[indice].tag = tag;
			//Trata a L2-> se um dado está na memória i ela precisa estar em i+1
			sizeTagIndice(endereco,nsets_L2, bsize_L2, assoc_L2); //dados L2
			if (cacheL2[indice].bitVal == 0 ){//vai ter que buscar da memória
				missL2++;
				missCompL2++; //Compulsório
				cacheL2[indice].bitVal=1;
				escritaL2++;
				cacheL2[indice].tag=tag;
			}
			else{ if (cacheL2[indice].bitVal==1 && cacheL2[indice].tag == tag){
				hitL2++;
				leituraL2++;
			}
				 if (cacheL2[indice].bitVal==1 && cacheL2[indice].tag != tag){
					missL2++;
					missConfL2++;
					cacheL2[indice].tag = tag;
					escritaL2++;
				}
			}
		}
		else {//bit validade 1, já usou essa memoria, vai ocorrer hit ou substituicao
			if(cacheL1_i[indice].tag == tag){
				hitL1i++; //encontrou, hit
				leituraL1i++;
			}
			else {     //nao encontrou, precisa confererir o dirty
				missL1i++;
				missConfL1i++;                // miss conflito
				if(cacheL1_i[indice].dirtyBit == 0){
					escritaL1i++;
					cacheL1_i[indice].tag = tag;           // seta novo tag
					//Trata L2
					sizeTagIndice(endereco,nsets_L2, bsize_L2, assoc_L2); //dados L2
					if(cacheL2[indice].tag != tag){
						escritaL2++;
						cacheL2[indice].bitVal=1;
						cacheL2[indice].tag= tag;
					}
					if(cacheL2[indice].tag == tag){
						leituraL2++;
						hitL2++;
					}	//dessa maneira, L1 e L2 estarão sempre coerentes, por isso o dirty bit não é atualizado para 1
				}
				else {// dirtyBit ==1
					int oldEndereco;
					//soma em binário para conseguir o endereço antigo para atualizar L2, write-back
					oldEndereco= ((cacheL1_i[indice].tag << sizeIndice) << sizeOffset) | (sizeIndice << sizeOffset) | sizeOffset;
					sizeTagIndice(oldEndereco,nsets_L2, bsize_L2, assoc_L2);
					cacheL2[indice].tag= tag; //Atualiza valor
					cacheL2[indice].bitVal=1;
					escritaL2++;
					sizeTagIndice(endereco,nsets, bsize, assoc_L1i); //dados da L1
					cacheL1_i[indice].tag=tag;
					escritaL1i++;
				}
			}
		}
	}
	else if(endereco < XX && le==1){//dados para escrita metodo write-back
		//precisa fazer algo por causa do write-back
		sizeTagIndice(endereco,nsets, bsize, assoc_L1d);
		if(cacheL1_d[indice].bitVal == 0){//Se o bit validade é 0  pq é o primeiro acesso a ela
			cacheL1_d[indice].bitVal = 1;       // validade = 1
			escritaL1d++; //escreverá em L1d
			cacheL1_d[indice].tag = tag;
			cacheL1_d[indice].dirtyBit=1; //ocorreu uma escrita e nao ocorreu atualização em L2
		}
		else {//bit validade 1, já usou essa memoria, vai ter que atualizar L2 e depois L1
			if(cacheL1_d[indice].tag == tag){
				hitL1d++; //encontrou, hit
				//é escrita, o que ia escrever já está escrita, nao faz nada então.
			}
			else {     //nao encontrou, precisa confererir o dirty
				if(cacheL1_d[indice].dirtyBit == 0){
					escritaL1d++;
					cacheL1_d[indice].tag = tag;           // seta novo tag
					cacheL1_d[indice].dirtyBit=1;	//não está atualizado em L2, quando se for fazer a leitura/escrita de algum valor diferente, irá atualizar em L2 (WRITE-BACK)
				}
				else {// dirtyBit ==1, dai precisa atualizar L2
					int oldEndereco;
					//soma em binário para conseguir o endereço antigo para atualizar L2, write-back
					oldEndereco= ((cacheL1_d[indice].tag << sizeIndice) << sizeOffset) | (sizeIndice << sizeOffset) | sizeOffset;
					sizeTagIndice(oldEndereco,nsets_L2, bsize_L2, assoc_L2);
					cacheL2[indice].tag= tag; //Atualiza valor
					cacheL2[indice].bitVal=1;
					escritaL2++;
					sizeTagIndice(endereco,nsets, bsize, assoc_L1d); //dados da L1
					cacheL1_d[indice].tag=tag;
					escritaL1d++;
				}
			}
		}
	}
	else if(endereco >= XX && le==1){//instruçoes para escrita metodo write-back
		//precisa fazer algo por causa do write-back
		sizeTagIndice(endereco,nsets, bsize, assoc_L1i);
		if(cacheL1_i[indice].bitVal == 0){//Se o bit validade é 0  pq é o primeiro acesso a ela
			cacheL1_i[indice].bitVal = 1;       // validade = 1
			escritaL1i++; //escreverá em L1d
			cacheL1_i[indice].tag = tag;
			cacheL1_i[indice].dirtyBit=1; //ocorreu uma escrita e nao ocorreu atualização em L2
		}
		else {//bit validade 1, já usou essa memoria, vai ter que atualizar L2 e depois L1
			if(cacheL1_i[indice].tag == tag){
				hitL1i++; //encontrou, hit
				//é escrita, o que ia escrever já está escrita, nao faz nada então. CONFERIR
			}
			else {     //nao encontrou, precisa confererir o dirty
				if(cacheL1_i[indice].dirtyBit == 0){
					escritaL1i++;
					cacheL1_i[indice].tag = tag;           // seta novo tag
					cacheL1_i[indice].dirtyBit=1;
				}
				else {// dirtyBit ==1, dai precisa atualizar L2
					int oldEndereco;
					//soma em binário para conseguir o endereço antigo para atualizar L2, write-back
					oldEndereco= ((cacheL1_i[indice].tag << sizeIndice) << sizeOffset) | (sizeIndice << sizeOffset) | sizeOffset;
					sizeTagIndice(oldEndereco,nsets_L2, bsize_L2, assoc_L2);
					cacheL2[indice].tag= tag; //Atualiza valor
					cacheL2[indice].bitVal=1;
					escritaL2++;
					sizeTagIndice(endereco,nsets, bsize, assoc_L1i); //dados da L1
					cacheL1_i[indice].tag=tag;
					escritaL1i++;
				}
			}
		}
	}
}
