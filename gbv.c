#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "gbv.h"

//estrutura do superbloco
typedef struct {
    int num_docs; //numero de documentos
    long int offset_dir; //posicao onde comeca o diretorio
} superbloco;

//cria a biblioteca GBV
int gbv_create(const char *filename) {
    FILE* file = fopen(filename, "wb");
    if(!file) {
        perror("gbv_create fopen"); 
        return -1;
    }

    superbloco sb;
    sb.num_docs = 0; //biblioteca comeca vazia
    sb.offset_dir = sizeof(superbloco); 

    //escreve superbloco
    fwrite(&sb, sizeof(superbloco), 1, file); 
 
    fclose(file);
    printf("biblioteca criada: %s\n", filename); 

    return 0; //retorna 0 em caso de sucesso
}

//abre ou cria a GBV e aloca memoria para os docs
int gbv_open(Library *lib, const char *filename) {
    FILE* file = fopen(filename, "rb+"); 

    //caso o arquivo nao exista entao chama a gbv create
    if(!file) {
        if(gbv_create(filename) != 0) {
            perror("GBV_OPEN nao foi possivel criar arquivo");
            return -1;
        }
    }
    //reabrir o arquivo depois de criar
    file = fopen(filename, "rb+");
    if(!file) {
        perror("GBV_OPEN nao foi possivel abrir arquivo depois de criar");
        return -1;
    }

    superbloco sb;
    //le o superbloco para RAM
    fread(&sb, sizeof(superbloco), 1, file);
    printf("GBV_OPEN superloco: %d documentos, offset_dir: %ld\n", sb.num_docs, sb.offset_dir); 

    //inicializa a lib
    lib->count = sb.num_docs;

    //quando ha documentos
    if(sb.num_docs > 0) {
        lib->docs = malloc(sb.num_docs * sizeof(Document));
        if(!lib->docs) {
            perror("GBV_OPEN erro ao alocar memoria"); //DEPURACAO
            fclose(file);
            return -1;
        }
        
        //pular para o diretorio e ler documento para a RAM
        fseek(file, sb.offset_dir, SEEK_SET);
        fread(lib->docs, sizeof(Document), sb.num_docs, file);

        //Na gbv_open, depois de fread(lib->docs, ...)
        printf("GBV_OPEN documentos carregados (%d):\n", lib->count);
        for(int i = 0; i < lib->count; i++) {
            printf("  [%d] '%s'\n", i, lib->docs[i].name);
        }   
    }
    else {
        lib->docs = NULL; //sem documentos
    }

    fclose(file);
    return 0;
}

//calcula o tamanho do doc a ser adicionado na bib
long calcula_tamanho(FILE* novo_doc) {
    //long pos_atual = ftell(novo_doc); //nao sei se precisa??
    fseek(novo_doc, 0, SEEK_END); //vai pro fim do arquivo
    long tam_doc = ftell(novo_doc); // pega o tamanho do novo doc
    fseek(novo_doc, 0, SEEK_SET); //volta para o inicio
    return tam_doc;
}

//retorna doc duplicado ou -1 caso contrario
int encontra_doc(Library *lib, const char *docname) {
    for(int i = 0; i < lib->count; i++) {
        if(strcmp(lib->docs[i].name, docname) == 0) {
            printf("documento ja existe \n"); //DEPURACAO
            return i;
        }
    }
    return -1;
}

//adiciona novo doc ao GBV ou substitui com mesmo nome
int gbv_add(Library *lib, const char *archive, const char *docname) {
    FILE* file = fopen(archive, "rb+"); //abrir arquivao
    FILE* novo_doc = fopen(docname, "rb"); // doc que vai ser add

    if(!file || !novo_doc) {
        perror("GBV_ADD fopen nao foi possivel abrir os arquivos");
        if(file)
            fclose(file);
        if(novo_doc)
            fclose(file);
        return -1;
    }

    //le o superbloco para a RAM
    superbloco sb;
    fread(&sb, sizeof(superbloco), 1, file);

    //tamanho do doc pela funcao
    long tam_doc = calcula_tamanho(novo_doc); 
    printf("documento %s tem %ld bytes\n", docname, tam_doc); //DEPURACAO

    //verifica se o documento ja existe pela funcao
    int doc_existente = encontra_doc(lib, docname);


    //todos os documentos carregados na GBV
    printf("documentos na GBV (%d):\n", lib->count);
    for(int i = 0; i < lib->count; i++) {
        printf("  [%d] '%s' (offset: %ld)\n", i, lib->docs[i].name, lib->docs[i].offset);
    }
    
    //novo offset onde os dados serao escritos
    long offset_dados;
    if(doc_existente != -1) {
        //substitui o arquivo anterior
        offset_dados = (lib->docs[doc_existente].offset);

    } else {
        //novo doc offset vai pro fim do arquivo
        fseek(file, 0, SEEK_END);
        offset_dados = ftell(file); 
    }

    //escrever infos do doc para a GBV usando buffer
    //pula para o offset de dados
    fseek(file, offset_dados, SEEK_SET);

    //buffer pra passar as infos
    char buffer[BUFFER_SIZE];
    long  bytes_lidos; 
    long total_escrito = 0; //DEPURACAO

    //enquanto houver bytes pra ler no doc
    while((bytes_lidos = fread(buffer, 1, BUFFER_SIZE, novo_doc)) > 0) {       
        fwrite(buffer, 1, bytes_lidos, file);
        total_escrito += bytes_lidos; //DEPURACAO oq isso faz?
    }

    //depois de escrever fecha o documento
    fclose(novo_doc);
    printf("escritos %ld bytes para GBV\n", total_escrito); //DEPURACAO

    //atualizar metadados na RAM
    if(doc_existente != -1) {
        //att doc existente (substituido)
        strcpy(lib->docs[doc_existente].name, docname); //copia de uma string para outra
        lib->docs[doc_existente].size = tam_doc;
        lib->docs[doc_existente].date = time (NULL);
        //offset se mantem pois substitui o existente
        printf("documento atualizado: %s\n", docname); //DEPURACAO

    } else {
        //add novo doc e precisa redimensionar
        Document *att_vetor = realloc(lib->docs, (lib->count + 1) * sizeof(Document));
        if (!att_vetor) {
            perror("gbv_add realloc erro ao realocar");
            fclose(file);
            return -1;
        }
        //atualiza vetor redimensionado
        lib->docs = att_vetor;

        //inicializa a nova pos do vetor com 0 para nao corromper com lixo de memoria
        memset(&lib->docs[lib->count], 0, sizeof(Document));

        //att do novo doc adicionado
        //lib->count eh a prox posicao livre no vetor
        int pos_livre = lib->count;
        strcpy(lib->docs[pos_livre].name, docname);
        lib->docs[pos_livre].size = tam_doc;
        lib->docs[pos_livre].date = time(NULL);
        lib->docs[pos_livre].offset = offset_dados;
    
        //aumenta quantidade de documentos do GBV
        lib->count++;
        sb.num_docs = lib->count; //att superbloco

        printf("novo documento adicionado: %s (total: %d documentos)\n", docname, lib->count); //DEPURACAO
    }

    //att nova posicao do diretorio depois dos arquivos add
    //caso so substitua o doc diretorio permanece o mesmo
    if(doc_existente != -1) {
        fseek(file, sb.offset_dir, SEEK_SET);  

    } else {
        //caso add soma no offset antigo
        sb.offset_dir = offset_dados + tam_doc;
        fseek(file, sb.offset_dir, SEEK_SET);
    }

    //escreve no arquivo
    fwrite(lib->docs, sizeof(Document), lib->count, file);

    //att superbloco
    fseek(file, 0, SEEK_SET);
    fwrite(&sb, sizeof(superbloco), 1, file);

    fclose(file);
    return 0;
}

//remove logicamente os documentos, dados permanecem e exclui metadados
int gbv_remove(Library *lib, const char *docname) {


}

//lista os documentos, exibindo: nome; tamanho em bytes; data de inserção; posição (offset)
int gbv_list(const Library *lib) {

}

//visuaiza o documento em blocos de tamanho fixo e permite navegar pelo conteudo
//n = prox bloco
//p = bloco anterior
//q = sair da visualizacao
int gbv_view(const Library *lib, const char *docname) {

}

/*int gbv_order(Library *lib, const char *archive, const char *criteria) {
    
} */
