#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "gbv.h"
#include "util.h"

//estrutura do superbloco
typedef struct {
    long num_docs; //numero de documentos
    long offset_dir; //posicao onde comeca o diretorio
} superbloco;

//cria a biblioteca GBV
int gbv_create(const char *filename) {
    FILE* file = fopen(filename, "wb");
    if(!file) {
        perror("GBV_CREATE fopen\n"); 
        return -1;
    }

    superbloco sb;
    sb.num_docs = 0; 
    sb.offset_dir = sizeof(superbloco); 

    printf("Tamanho do superbloco: %zu bytes\n", sizeof(superbloco));
    printf("offset_dir definido como: %ld\n", sb.offset_dir);
    printf("\n");

    //escreve superbloco na GBV
    fwrite(&sb, sizeof(superbloco), 1, file); 
 
    fclose(file);
    printf("Biblioteca criada: %s\n", filename); 
    printf("\n");
    
    return 0; 
}

//abre ou cria a GBV e aloca memoria para os docs
int gbv_open(Library *lib, const char *filename) {
    FILE* file = fopen(filename, "rb+"); 

    //caso o arquivo nao exista entao cria
    if(!file) {
        if(gbv_create(filename) != 0) {
            perror("GBV_OPEN nao foi possivel criar arquivo\n");
            return -1;
        }
        //reabrir o arquivo depois de criar
        file = fopen(filename, "rb+");
        if(!file) {
            perror("GBV_OPEN nao foi possivel abrir arquivo depois de criar\n");
            return -1;
        }
    }

    superbloco sb;
    //le o superbloco para RAM
    fread(&sb, sizeof(superbloco), 1, file);
    printf("Informacoes do superbloco: %ld documentos, offset_dir: %ld\n", sb.num_docs, sb.offset_dir); 
    printf("\n");

    //inicializa a lib
    lib->count = sb.num_docs;

    //quando ha documentos
    if(sb.num_docs > 0) {
        lib->docs = malloc(sb.num_docs * sizeof(Document));
        if(!lib->docs) {
            perror("GBV_OPEN erro ao alocar memoria\n");
            fclose(file);
            return -1;
        }
        
        //pular para o diretorio e ler documento para a RAM
        fseek(file, sb.offset_dir, SEEK_SET);
        fread(lib->docs, sizeof(Document), sb.num_docs, file);
    }
    else {
        lib->docs = NULL; //sem documentos
    }

    fclose(file);
    return 0;
}

//calcula o tamanho do doc a ser adicionado na bib
long calcula_tamanho(FILE* novo_doc) {
    fseek(novo_doc, 0, SEEK_END); //vai pro fim do arquivo
    long tam_doc = ftell(novo_doc); // pega o tamanho do novo doc
    fseek(novo_doc, 0, SEEK_SET); //volta para o inicio
    return tam_doc;
}

//retorna doc duplicado ou -1 caso contrario
int encontra_doc(Library *lib, const char *docname) {
    for(int i = 0; i < lib->count; i++) {
        if(strcmp(lib->docs[i].name, docname) == 0) {
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
        perror("GBV_ADD fopen nao foi possivel abrir os arquivos\n");
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
    printf("O documento %s tem %ld bytes\n", docname, tam_doc);
    printf("\n");

    //verifica se o documento ja existe pela funcao
    int doc_existente = encontra_doc(lib, docname);

    //novo offset onde os dados serao escritos
    long offset_dados;
    if(doc_existente != -1) {
        //substitui o arquivo anterior
        printf("O documento ja existe, sera substituido\n"); 
        printf("\n");
        offset_dados = (lib->docs[doc_existente].offset);

    } else {
        //doc fica no lugar do antigo dir
        offset_dados = sb.offset_dir;  
        //printf("Novo offset_dados calculado = %ld\n", offset_dados);
        //att offset_dir para depois do novo documento add
        sb.offset_dir = offset_dados + tam_doc;
        printf("Novo sb.offset_dir = %ld\n", sb.offset_dir);
        printf("\n");

    }

    //escrever infos do doc para a GBV usando buffer
    //pula para o offset de dados
    fseek(file, offset_dados, SEEK_SET);

    //buffer pra passar as infos
    char buffer[BUFFER_SIZE];
    long  bytes_lidos; 
    long total_escrito = 0; 

    //enquanto houver bytes pra ler no doc
    while((bytes_lidos = fread(buffer, 1, BUFFER_SIZE, novo_doc)) > 0) {       
        fwrite(buffer, 1, bytes_lidos, file);
        //printf("Foram lidos  %ld bytes\n", bytes_lidos);
        total_escrito += bytes_lidos; 
    }

    //depois de escrever na GBV fecha o documento
    fclose(novo_doc);
    printf("Foram escritos com sucesso %ld bytes para GBV\n", total_escrito); 
    printf("\n");

    //atualizar metadados na RAM
    if(doc_existente != -1) {
        //att doc
        strcpy(lib->docs[doc_existente].name, docname);
        lib->docs[doc_existente].size = tam_doc;
        lib->docs[doc_existente].date = time (NULL);
        //offset se mantem pois substitui o existente
        printf("Documento atualizado: %s\n", docname); 

    } else {
        //add novo doc e precisa redimensionar
        Document* att_vetor = realloc(lib->docs, (lib->count + 1) * sizeof(Document));
        if(!att_vetor) {
            perror("GBV_ADD realloc erro ao realocar\n");
            fclose(file);
            return -1;
        }
        //atualiza vetor redimensionado
        lib->docs = att_vetor;

        //inicializa a nova pos do vetor com 0 para nao corromper com lixo de memoria
        memset(&lib->docs[lib->count], 0, sizeof(Document));

        //att do novo doc adicionado
        int pos_livre = lib->count;
        strcpy(lib->docs[pos_livre].name, docname);
        lib->docs[pos_livre].size = tam_doc;
        lib->docs[pos_livre].date = time(NULL);
        lib->docs[pos_livre].offset = offset_dados;
    
        //aumenta quantidade de documentos do GBV
        lib->count++;
        sb.num_docs = lib->count; 

        printf("Novo documento adicionado: %s (total: %d documentos)\n", docname, lib->count);
        printf("\n");
    }

    fseek(file, sb.offset_dir, SEEK_SET);  
    //reescreve no arquivo os docs att
    fwrite(lib->docs, sizeof(Document), lib->count, file); 

    //att superbloco sobrescreve
    fseek(file, 0, SEEK_SET);
    fwrite(&sb, sizeof(superbloco), 1, file);

    fclose(file);
    return 0;
}


//remove logicamente os documentos, dados permanecem e exclui metadados
int gbv_remove(Library *lib, const char *docname) {
    if(!lib || !docname) {
        perror("biblioteca ou doc invalidos");
        return -1;
    }

    //abre o arquivo pra atualizar
    FILE *file = fopen("biblioteca.gbv", "rb+");
    if (!file) {
        perror("GBV_REMOVE erro ao abrir biblioteca\n");
        return -1;
    }
    
    //encontra o doc a ser removido com a funcao
    int doc_remove = encontra_doc(lib,docname);

    if(doc_remove == -1) {
        printf("Documento nao encontrado\n");
        return -1;
    }

    //se nao for o ultimo elemento do vetor, reorganiza
    if(doc_remove < lib->count -1) {
        //move todos os elementos para tras
        for(int i = doc_remove; i < lib->count -1; i++)
            lib->docs[i] = lib->docs[i + 1];
    }
    //reduz a contadora
    lib->count--;
     
    //redimensiona o vetor
    if(lib->count > 0) {
        Document* att_vetor = realloc(lib->docs, lib->count * sizeof(Document));
        if(!att_vetor) {
            perror("GBV_ADD realloc erro ao realocar\n");
            fclose(file);
            return -1;
        }
        //atualiza a lib com vetor realocado
        lib->docs = att_vetor;
    }

    //att superbloco 
    superbloco sb;
    fread(&sb, sizeof(superbloco), 1, file);
    sb.num_docs = lib->count;   
    //sobrescreve o superbloco
    fseek(file, 0, SEEK_SET);
    fwrite(&sb, sizeof(superbloco), 1, file);

    //reescreve o diretorio e os docs
    fseek(file, sb.offset_dir, SEEK_SET);
    //se nao tiver apenas um doc no arquivo, reescreva os docs
    if(lib->count > 0)
        fwrite(lib->docs, sizeof(Document), lib->count, file); 

    fclose(file);
    printf("Documento '%s' removido\n", docname);

    return 0;
} 

//lista os documentos, exibindo: nome; tamanho em bytes; data de inserção; posição (offset)
int gbv_list(const Library *lib) {
    if(!lib) {
        perror("biblioteca nao inicializada\n");
        return -1;
    }

    if(lib->count == 0) {
        perror("biblioteca vazia\n");
        return -1;
    }

    //formatacao apra ficar legivel
    printf("%-30s %-12s %-20s %-10s\n", 
           "NOME", "TAMANHO", "DATA INSERIDO", "OFFSET DADOS");
    
  
    //dados dos docs
    for (int i = 0; i < lib->count; i++) {
        char buffer_data[20];
        format_date(lib->docs[i].date, buffer_data, sizeof(buffer_data));
        
        printf("%-30s %-12ld %-20s %-10ld\n", 
               lib->docs[i].name,
               lib->docs[i].size,
               buffer_data,
               lib->docs[i].offset);
    }
    printf("\n");

    printf("Total de documentos: %d\n", lib->count);

    return 0;
}

//visuaiza o documento em blocos de tamanho fixo e permite navegar pelo conteudo
//n = prox bloco
//p = bloco anterior
//q = sair da visualizacao
int gbv_view(const Library *lib, const char *docname) {

}

//libera memoria
void gbv_close(Library *lib) {
    if (lib && lib->docs) {
        free(lib->docs);
        lib->docs = NULL;
        lib->count = 0;
    }
}


/*int gbv_order(Library *lib, const char *archive, const char *criteria) {
    
} */

