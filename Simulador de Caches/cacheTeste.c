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

typedef struct{
	int hit;
	int miss, missComp, missConf, missCap;
	int leitura, escrita;
	int nivel;
}Estatistica;
//Variaveis globais;
FILE *arq;
int numAcess=0;
int endereco, le; // le é leitura escrita
int sizeOffset, sizeIndice, sizeTag, indice, tag;
int XX=400; //Júlio não especificou, na cache os endereços abaixo de XX
int nsets_L1i, bsize_L1i, assoc_L1i, nsets_L1d, bsize_L1d, assoc_L1d, nsets_L2, bsize_L2, assoc_L2; //referente as caches
int tipoL1d=0, tipoL1i=0, tipoL2=0;//tipo=1=MP ; tipo=2=CA ; tipo=3=TA
cache *cacheL1i, *cacheL1d, *cacheL2, **cacheL1Md, **cacheL1Mi, **cacheL2M;
Estatistica L1i, L1d, L2;

//Funções
void totalAssoc(cache *cacheL, int endereco, int nsets, int bsize, int assoc, Estatistica* L);
void conjAssoc(cache **cacheL, int endereco, int nsets, int bsize, int assoc, Estatistica *L);
void mapeamentoDireto(cache* cacheL, int endereco, int nsets, int bsize, int assoc, Estatistica* L);
void sizeTagIndice(int endereco, int nsets, int bsize);
void zeraEstatistica(Estatistica *L);
void dadosRelatorio (Estatistica *L);
void nomeCache(int tipo);
void relatorioDeEstatistica ();
void criaCache();
cache** criaCacheMatriz(int nsets, int assoc);
cache* criaCacheVetor(int nsets);
int defineTipos(int ass, int nsets);
void decisaoCacheUnificada();
void decisaoCacheSeparada();
void validaArgumentos(char *argv[]);
void carregaArgumentos(char *argv[]);
float logBase2(int num);
void leExtensao(char *nomeArq, char *ext);
void carregaArgumentosDefault();
void leArq (char *nomeArq, char *ext);

int main(int argc,char *argv[]){ // argc é o numero de elementos e argv são os elementos, começa no 1( pq o 0 é o ./cache )
	char nomeArq[50], ext[4];
	//configuração default
	if (argc==2){
		strcpy(nomeArq,argv[1]);
		leArq(nomeArq, ext);
		carregaArgumentosDefault();
		//Significa que todas as caches são mapeamento direto
		tipoL1d=1;
		tipoL1i=1;
		tipoL2=1;
		//printf("\n################### Configuracao da Cache: Default ###################\n");
	}
	else if(argc==11){
		strcpy(nomeArq,argv[10]);
		leArq(nomeArq, ext);
		validaArgumentos(argv);
		carregaArgumentos(argv);//no carrega argumentos já define o tipo
	}
	else{
		printf ("ERRO: Número de argumentos inválido, tente novamente !");
		exit(1);
	}
	criaCache();//Cria as 3 caches
	if (strcmp(ext, "txt")==0){//se o arquivo é texto
		while (fscanf(arq, "%d", &endereco) != EOF){//vai ler linha por linha
			fscanf(arq, "%d", &le);// A segunda linha informa se é leitura=0 ou escrita=1
			numAcess++;
			decisaoCacheSeparada();
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
			decisaoCacheSeparada();
		}
	}
	fclose(arq);
	relatorioDeEstatistica();
	return 0;
}
void totalAssoc(cache *cacheL, int endereco, int nsets, int bsize, int assoc, Estatistica* L){
	int i, flag = 0, flag2 = 0, aux;
	
	if(le == 0){ //leitura pra qualquer cache, tanto L1d ou L1i como L2
		sizeTagIndice(endereco, nsets, bsize);
		for(i=0; i<assoc; i++){
			if(cacheL[i].tag == tag && cacheL[i].bitVal == 1){ //HIT
				(*L).hit++;
				(*L).leitura++;
				flag = 1; //essa flag simboliza se houve um hit (1) ou um miss (continua em 0 e deverá ser feita escrita)
				break;
			}
		}
		if(flag == 0){ //MISS
			for(i=0; i<assoc; i++){ //procura um espaço "livre" na cache para inserir o valor
				if(cacheL[i].bitVal == 0){ //achou um espaço "livre" -> com bit de validade 0
					(*L).miss++;
					(*L).missComp++;
					cacheL[i].bitVal = 1;
					(*L).escrita++;
					(*L).leitura++;
					cacheL[i].tag = tag;
					if((*L).nivel == 1){ //se a cache for nivel 1 (dados ou instruções), tem que tratar o dado no segundo nível também
						decisaoCacheUnificada(); //trata o dado na L2
					}
					flag2 = 1;
					break;
				}
			}
			if(flag2 == 0){ //não achou um espaço livre, tem que fazer substituição randômica -> conferindo o dirtybit do lugar a ser substituído
				(*L).miss++;
				(*L).missConf++;
				aux = rand()%assoc;
				if(cacheL[aux].dirtyBit == 0){ //dirty bit em 0, não há necessidade de passar o valor pro nível inferior antes de substituí-lo
					(*L).escrita++;
					(*L).leitura++;
					cacheL[aux].tag = tag;
					if((*L).nivel == 1){
						decisaoCacheUnificada();
					}
				}
				else{ //dirty bit em 1, precisa passar o valor velho pra L2 antes de substituí-lo
					if((*L).nivel == 1){ //dirty em 1 no primeiro nível, passa o endereço velho pro segundo nível
						int oldEndereco;
						oldEndereco= ((cacheL[aux].tag << sizeIndice) << sizeOffset) | (sizeIndice << sizeOffset) | sizeOffset;
						cacheL[aux].tag = tag;
						(*L).escrita++;
						(*L).leitura++;
						endereco = oldEndereco;
						decisaoCacheUnificada(); //vai tratar na L2 com o endereço velho que estava no bloco antes de ser substituído
					}
					if((*L).nivel == 2){ //se for no segundo nível, só atualiza (não há a memória principal no simulador, então não há para onde levar o valor antigo)
						cacheL[aux].tag = tag;
						(*L).escrita++;
						(*L).leitura++;
					}
				}
			}
		}
	}
	else{ //casos de escrita, difere para L1 e L2
		if(le == 1 && (*L).nivel == 1){
			sizeTagIndice(endereco, nsets, bsize);
			for(i=0; i<assoc; i++){ //confere primeiro se o que quer ser escrito já não existe na cache
				if(cacheL[i].tag == tag && cacheL[i].bitVal == 1){
					(*L).hit++;
					(*L).leitura++;
					flag = 1; //essa flag simboliza se houve um hit (1) ou um miss (continua em 0 e deverá ser feita escrita)
					break;
				}
			}
			if(flag == 0){ //não está na cache, deve ser escrito
				for(i=0; i<assoc; i++){ //procura um espaço "livre" na cache para inserir o valor
					if(cacheL[i].bitVal == 0){ //achou um espaço "livre" -> com bit de validade 0
						(*L).escrita++;
						cacheL[i].bitVal = 1;
						cacheL[i].tag = tag;
						cacheL[i].dirtyBit = 1;
						flag2 = 1;
						break;
					}
				}
				if(flag2 == 0){ //não achou espaço livre para escrever, vai ter que substituir algum bloco já existente
					aux = rand()%assoc;
					if(cacheL[aux].dirtyBit == 0){ //dirty bit em 0, não faz nada na L2
						(*L).escrita++;
						cacheL[aux].tag = tag;
						cacheL[aux].dirtyBit = 1;
					}
					else{ //dirty bit em 1, atualiza na L2
						int oldEndereco;
						oldEndereco = ((cacheL[aux].tag << sizeIndice) << sizeOffset) | (sizeIndice << sizeOffset) | sizeOffset;
						cacheL[aux].tag = tag;
						(*L).escrita++;
						endereco = oldEndereco; //O endereco que vai ser atualizado em L2 é o endereco antigo
						decisaoCacheUnificada();
					}
				}
			}
		}
		if(le == 1 && (*L).nivel == 2){ //escrita na L2, não há preocupação com o dirty bit nesse caso. Primeiro, procura espaço livre para escrever, se não houver, insere em bloco aleatório
			sizeTagIndice(endereco, nsets, bsize);
			for(i=0; i<assoc; i++){ //procura um espaço "livre" na cache para inserir o valor
				if(cacheL[i].bitVal == 0){ //achou um espaço "livre" -> com bit de validade 0
					cacheL[i].bitVal = 1;
					(*L).escrita++;
					cacheL[i].tag = tag;
					flag2 = 1;
					break;
				}
			}
			if(flag2 == 0){
				aux = rand()%assoc;
				cacheL[aux].tag = tag;
				(*L).escrita++;
			}
		}
	}				
}

void conjAssoc(cache **cacheL, int endereco, int nsets, int bsize, int assoc, Estatistica* L){
	int i, flag = 0, flag2 = 0, aux;
	if(le == 0){ //leitura pra qualquer cache, tanto L1d ou L1i como L2
		sizeTagIndice(endereco, nsets, bsize);
		for(i=0; i<assoc; i++){
			if(cacheL[indice][i].tag == tag && cacheL[indice][i].bitVal == 1){
				(*L).hit++;
				(*L).leitura++;
				flag = 1; //essa flag simboliza se houve um hit (1) ou um miss (continua em 0 e deverá ser feita escrita)
				break;
			}
		}
		if(flag == 0){ //MISS
			for(i=0; i<assoc; i++){ //procura um espaço "livre" na cache para inserir o valor
				if(cacheL[indice][i].bitVal == 0){ //achou um espaço "livre" -> com bit de validade 0
					(*L).miss++;
					(*L).missComp++;
					cacheL[indice][i].bitVal = 1;
					(*L).escrita++;
					(*L).leitura++;
					cacheL[indice][i].tag = tag;
					if((*L).nivel == 1){ //se a cache for nivel 1 (dados ou instruções), tem que tratar o dado no segundo nível também
						decisaoCacheUnificada(); //trata o dado na L2
					}
					flag2 = 1;
					break;
				}
			}
			if(flag2 == 0){ //não achou um espaço livre, tem que fazer substituição randômica -> conferindo o dirtybit do lugar a ser substituído
				(*L).miss++;
				(*L).missConf++;
				aux = rand()%assoc;
				if(cacheL[indice][aux].dirtyBit == 0){ //dirty bit em 0, não há necessidade de passar o valor pro nível inferior antes de substituí-lo
					(*L).escrita++;
					(*L).leitura++;
					cacheL[indice][aux].tag = tag;
					if((*L).nivel == 1){
						decisaoCacheUnificada();
					}
				}
				else{ //dirty bit em 1, precisa passar o valor velho pra L2 antes de substituí-lo
					if((*L).nivel == 1){ //dirty em 1 no primeiro nível, passa o endereço velho pro segundo nível
						int oldEndereco;
						oldEndereco= ((cacheL[indice][aux].tag << sizeIndice) << sizeOffset) | (sizeIndice << sizeOffset) | sizeOffset;
						cacheL[indice][aux].tag = tag;
						(*L).escrita++;
						(*L).leitura++;
						endereco = oldEndereco;
						decisaoCacheUnificada(); //vai tratar na L2 com o endereço velho que estava no bloco antes de ser substituído
					}
					if((*L).nivel == 2){ //se for no segundo nível, só atualiza (não há a memória principal no simulador, então não há para onde levar o valor antigo)
						cacheL[indice][aux].tag = tag;
						(*L).escrita++;
						(*L).leitura++;
					}
				}
			}
		}
	}
	else{ //casos de escrita, difere para L1 e L2
		if(le == 1 && (*L).nivel == 1){
			sizeTagIndice(endereco, nsets, bsize);
			for(i=0; i<assoc; i++){ //confere primeiro se o que quer ser escrito já não existe na cache
				if(cacheL[indice][i].tag == tag && cacheL[indice][i].bitVal == 1){
					(*L).hit++;
					(*L).leitura++;
					flag = 1; //essa flag simboliza se houve um hit (1) ou um miss (continua em 0 e deverá ser feita escrita)
					break;
				}
			}
			if(flag == 0){ //não está na cache, deve ser escrito
				for(i=0; i<assoc; i++){ //procura um espaço "livre" na cache para inserir o valor
					if(cacheL[indice][i].bitVal == 0){ //achou um espaço "livre" -> com bit de validade 0
						(*L).escrita++;
						cacheL[indice][i].bitVal = 1;
						cacheL[indice][i].tag = tag;
						cacheL[indice][i].dirtyBit = 1;
						flag2 = 1;
						break;
					}
				}
				if(flag2 == 0){ //não achou espaço livre para escrever, vai ter que substituir algum bloco já existente
					aux = rand()%assoc;
					if(cacheL[indice][aux].dirtyBit == 0){ //dirty bit em 0, não faz nada na L2
						(*L).escrita++;
						cacheL[indice][aux].tag = tag;
						cacheL[indice][aux].dirtyBit = 1;
					}
					else{ //dirty bit em 1, atualiza na L2
						int oldEndereco;
						oldEndereco = ((cacheL[indice][aux].tag << sizeIndice) << sizeOffset) | (sizeIndice << sizeOffset) | sizeOffset;
						cacheL[indice][aux].tag = tag;
						(*L).escrita++;
						endereco = oldEndereco; //O endereco que vai ser atualizado em L2 é o endereco antigo
						decisaoCacheUnificada();
					}
				}
			}
		}
		if(le == 1 && (*L).nivel == 2){ //escrita na L2, não há preocupação com o dirty bit nesse caso. Primeiro, procura espaço livre para escrever, se não houver, insere em bloco aleatório
			sizeTagIndice(endereco, nsets, bsize);
			for(i=0; i<assoc; i++){ //procura um espaço "livre" na cache para inserir o valor
				if(cacheL[indice][i].bitVal == 0){ //achou um espaço "livre" -> com bit de validade 0
					cacheL[indice][i].bitVal = 1;
					(*L).escrita++;
					cacheL[indice][i].tag = tag;
					flag2 = 1;
					break;
				}
			}
			if(flag2 == 0){
				aux = rand()%assoc;
				cacheL[indice][aux].tag = tag;
				(*L).escrita++;
			}
		}
	}				
}
void mapeamentoDireto(cache *cacheL, int endereco, int nsets, int bsize, int assoc, Estatistica* L){
	//L1 ou L2 se for leitura, e os tratamentos caso seja cache L1 são feitos dentro desse laço
	if(le==0){
		sizeTagIndice(endereco,nsets, bsize);
		if(cacheL[indice].bitVal == 0){//Se o bit validade é 0  pq é o primeiro acesso a ela
			(*L).miss++;                                   // Miss total de dados sobe
			(*L).missComp++;                              // Esse é compulsorio
			cacheL[indice].bitVal = 1;       // validade = 1
			(*L).escrita++; //escreverá em L1
			(*L).leitura++; //Vai escrever mas também vai ler
			cacheL[indice].tag = tag;
			if((*L).nivel==1){ 	// Se for L1(dado ou instrucao) e leitura
				//Trata a L2-> se um dado está na memória i ela precisa estar em i+1
				decisaoCacheUnificada();
			}
		}
		else {//bit validade 1, já usou essa memoria, vai ocorrer hit ou substituicao
			if(cacheL[indice].tag == tag){
				(*L).hit++; //encontrou, hit
				(*L).leitura++;
			}
			else {     //nao encontrou, precisa confererir o dirty
				(*L).miss++;
				(*L).missConf++;                // miss conflito
				if(cacheL[indice].dirtyBit == 0){
					(*L).escrita++;
					(*L).leitura++;
					cacheL[indice].tag = tag;           // seta novo tag
					if((*L).nivel==1){ 	// Se for L1(dado ou instrucao) e leitura
						//Trata a L2
						decisaoCacheUnificada();
					}
				}
				else {// dirtyBit == 1
					if ((*L).nivel==1){
						int oldEndereco;
						//soma em binário para conseguir o endereço antigo para atualizar L2, write-back
						oldEndereco= ((cacheL[indice].tag << sizeIndice) << sizeOffset) | (sizeIndice << sizeOffset) | sizeOffset;
						//Atualiza os dados de L1
						cacheL[indice].tag=tag;
						(*L).escrita++;
						(*L).leitura++;
						//Trata L2, porém com o endereco antigo
						endereco=oldEndereco; //O endereco que vai ser atualizado em L2 é o endereco antigo
						decisaoCacheUnificada();
					}
					//Se L2 que tiver o dirty, nao precisa passar para a memoria principal, entao só atualiza nela
					else{
						cacheL[indice].tag=tag;
						(*L).escrita++;
						(*L).leitura++;
					}
				}
			}
		}
	}
	// L1(dado ou instrucao) escrita
	else if(le==1 && (*L).nivel==1){ // metodo write-back
		//precisa fazer algo por causa do write-back
		sizeTagIndice(endereco,nsets, bsize);
		if(cacheL[indice].bitVal == 0){//Se o bit validade é 0  pq é o primeiro acesso a ela
			//nao dá miss, porque seria um miss write e eles serão desconsiderados
			cacheL[indice].bitVal = 1;       // validade = 1
			(*L).escrita++; //escreverá em L1
			cacheL[indice].tag = tag;
			cacheL[indice].dirtyBit=1; //ocorreu uma escrita e nao ocorreu atualização em L2
		}
		else {//bit validade 1, já usou essa memoria, vai ter que atualizar L2 e depois L1
			if(cacheL[indice].tag == tag){
				//hitL1d++; //encontrou, hit
				//é escrita, o que ia escrever já está escrita, nao faz nada então.
			}
			else {     //nao encontrou, precisa confererir o dirty
				if(cacheL[indice].dirtyBit == 0){
					(*L).escrita++;
					cacheL[indice].tag = tag;     // seta novo tag
					cacheL[indice].dirtyBit=1;	//não está atualizado em L2, quando se for fazer a leitura/escrita de algum valor diferente, irá atualizar em L2 (WRITE-BACK)
				}
				else {// dirtyBit ==1, dai precisa atualizar L2
					int oldEndereco;
					//soma em binário para conseguir o endereço antigo para atualizar L2, write-back
					oldEndereco= ((cacheL[indice].tag << sizeIndice) << sizeOffset) | (sizeIndice << sizeOffset) | sizeOffset;
					cacheL[indice].tag=tag;
					(*L).escrita++;
					//Trata L2, porém com o endereco antigo
					endereco=oldEndereco; //O endereco que vai ser atualizado em L2 é o endereco antigo
					decisaoCacheUnificada();
				}
			}
		}
	}
	//escrita em L2
	else if(le==1 && (*L).nivel==2){
		sizeTagIndice(endereco,nsets, bsize);
		cacheL[indice].tag= tag; //Atualiza valor
		cacheL[indice].bitVal=1;
		(*L).escrita++;
	}
}
void sizeTagIndice(int endereco, int nsets, int bsize){
	sizeOffset = logBase2(bsize);   // Tamanho do offset
	sizeIndice = logBase2(nsets);   // Tamanho do indice
	sizeTag = 32-sizeIndice-sizeOffset; // Tamanho da tag
	indice = endereco%nsets;				// considerando 2na n, n é o indice
	tag = (endereco >> (sizeOffset >> sizeIndice));                 // o que restar do endereço sem offset e indice
}
void zeraEstatistica(Estatistica* L){
	//Acredito que esses valores já venham zerados, mas é sempre melhor garantir
	(*L).hit=0;
	(*L).miss=0;
	(*L).missComp=0;
	(*L).missConf=0;
	(*L).missCap=0;
	(*L).leitura=0;
	(*L).escrita=0;
}
void dadosRelatorio (Estatistica* L){
	float hr, mr; //hit ratio e miss ratio
	hr= (float)((*L).hit*100)/numAcess; // hit total * 100 / num acesso = hit ratio
	mr= (float)((*L).miss*100)/numAcess;
	printf ("\t*Leitura: %d\n", (*L).leitura);
	printf ("\t*Escrita: %d\n", (*L).escrita);
	printf ("\t*Total de Hit: %d\n", (*L).hit);
	printf ("\t*Total de Miss: %d\n", (*L).miss);
	printf ("\t\t-Quantidade de Miss Compulsório: %d\n", (*L).missComp);
	printf ("\t\t-Quantidade de Miss de Capacidade: %d\n",(*L).missCap);
	printf ("\t\t-Quantidade de Miss de Conflito: %d\n", (*L).missConf);
	printf ("\t*Hit Ratio: %.2f\n", hr);
	printf ("\t*Miss Ratio: %.2f\n", mr);
}
void nomeCache(int tipo){
	// Informa qual cache se refere
	if(tipo==1){
		printf("-----> Cache Direta\n\n");
	}
	else if(tipo==2){
		printf("-----> Cache Conjunto Associativo\n\n");
	}
	else if(tipo==3){
		printf("-----> Cache Totalmente Associativa\n\n");
	}
}
void relatorioDeEstatistica (){
	printf ("\n\t\t###############################\n\t\t## Relatório de Estatísticas ##\n\t\t###############################\n\n");
	printf ("*Número de acessos: %d\n", numAcess);
	printf("\n\n################### Cache L1 de dados ##################\n\n");
	// Informa qual cache se refere
	nomeCache(tipoL1d);
	dadosRelatorio(&L1d);
	printf("\n\n################ Cache L1 de instruções ################\n\n");
	// Informa qual cache se refere
	nomeCache(tipoL1i);
	dadosRelatorio(&L1i);
	printf("\n\n####################### Cache L2 #######################\n\n");
	// Informa qual cache se refere
	nomeCache(tipoL2);
	dadosRelatorio(&L2);
	printf("\n\n########################################################\n\n");
}
void criaCache(){
	zeraEstatistica(&L1i);
	L1i.nivel=1;
	zeraEstatistica(&L1d);
	L1d.nivel=1;
	zeraEstatistica(&L2);
	L2.nivel=2;
	//cria L1 de dados
	if (tipoL1d==1 || tipoL1d==3){
		//cria cache dado vetor -> Mapeamento direto ou Totalmente Associativa
		cacheL1d=criaCacheVetor(nsets_L1d);
	}
	else{
		//cria cache dado matriz->Conjunto Associativo
		cacheL1Md=criaCacheMatriz(nsets_L1d, assoc_L1d);
	}
	//cria L1 de instrucoes
	if (tipoL1i==1 || tipoL1i==3){
		//cria cache dado vetor -> Mapeamento direto ou Totalmente Associativa
		cacheL1i=criaCacheVetor(nsets_L1i);
	}
	else{
		//cria cache dado matriz->Conjunto Associativo
		cacheL1Mi=criaCacheMatriz(nsets_L1i, assoc_L1i);
	}
	//cria L2 
	if (tipoL2==1 || tipoL2==3){
		//cria cache dado vetor -> Mapeamento direto ou Totalmente Associativa
		cacheL2=criaCacheVetor(nsets_L2);
	}
	else{
		//cria cache dado matriz->Conjunto Associativo
		cacheL2M=criaCacheMatriz(nsets_L2, assoc_L2);
	}
}
cache** criaCacheMatriz(int nsets, int assoc){
	int i,j;
	cache **cacheM;
	//Matriz por causa da associatividade
	cacheM = malloc((sizeof(cache))*nsets);
	for(j = 0; j< nsets; j++){
		cacheM[j]=malloc((sizeof(cache))*assoc);
	}
	//"limpa" as posições
	for (i=0; i<nsets; i++){
		for (j=0; j<assoc; j++){
			cacheM[i][j].bitVal = 0;
			cacheM[i][j].tag = 0;
			cacheM[i][j].dirtyBit = 0;
		}
	}
	return cacheM;
}
cache* criaCacheVetor(int nsets){
	//Não é necessário matriz
	int i;
	cache *cacheV;
	cacheV = malloc((sizeof(cache))*nsets);
	//"limpa" as posições
	for(i=0; i<nsets; i++){
		cacheV[i].tag = 0;
		cacheV[i].bitVal = 0;
		cacheV[i].dirtyBit = 0;
	}
	return cacheV;
}
int defineTipos(int ass, int nsets){
	int tipo=0;
	if((ass==1) && (nsets>1)){
		tipo=1; //mapeamento direto
	}
	else if((ass>1) && (nsets>1)){
		tipo=2; //conjunto associativa
	}
	else if((ass>1) && (nsets == 1)){ //só é totalmente associativa se o numero de associatividade é igual ao numero de blocos
		tipo=3; //totalmente associativa
	}
	return tipo;
}
void decisaoCacheUnificada(){
	//Referente a cache L2 ( unica unificada
	if(tipoL2==1){
		mapeamentoDireto(cacheL2, endereco, nsets_L2, bsize_L2, assoc_L2, &L2);
	}
	else if(tipoL2==2){
		conjAssoc(cacheL2M, endereco, nsets_L2, bsize_L2, assoc_L2, &L2);
	}
	else if(tipoL2==3){
		totalAssoc(cacheL2 ,endereco, nsets_L2, bsize_L2, assoc_L2, &L2);
	}
}
void decisaoCacheSeparada(){
	if(endereco < XX ){//dados
		//Vai para sua devida configuração
		if(tipoL1d==1){
			mapeamentoDireto(cacheL1d, endereco, nsets_L1d, bsize_L1d, assoc_L1d, &L1d);
		}
		else if(tipoL1d==2){
			conjAssoc(cacheL1Md, endereco, nsets_L1d, bsize_L1d, assoc_L1d, &L1d);
		}
		else if(tipoL1d==3){
			totalAssoc(cacheL1d ,endereco, nsets_L1d, bsize_L1d, assoc_L1d, &L1d);
		}
	}
	else if(endereco >= XX ){ //Vai para cache de instruções se o endereço for igual a XX ou superior
		//Vai para sua devida configuração
		if(tipoL1i==1){
			mapeamentoDireto(cacheL1i, endereco, nsets_L1i, bsize_L1i, assoc_L1i, &L1i);
		}
		else if(tipoL1i==2){
			conjAssoc(cacheL1Mi, endereco, nsets_L1i, bsize_L1i, assoc_L1i, &L1i);
		}
		else if(tipoL1i==3){
			totalAssoc(cacheL1i ,endereco, nsets_L1i, bsize_L1i, assoc_L1i, &L1i);
		}
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
	//Define tipos
	tipoL1d= defineTipos(assoc_L1d, nsets_L1d);
	tipoL1i= defineTipos(assoc_L1i, nsets_L1i);
	tipoL2= defineTipos(assoc_L2, nsets_L2);
	
}
float logBase2(int num){
     return log10(num)/log10(2);         //log base 2

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
