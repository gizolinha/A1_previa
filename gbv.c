#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
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

    //inicializa o superbloco
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

//calcula o tamanho do doc a ser adicionado ou removido na bib
long calcula_tamanho(FILE* novo_doc) {
    fseek(novo_doc, 0, SEEK_END); //vai pro fim do arquivo
    long tam_doc = ftell(novo_doc); // pega o tamanho do novo doc
    fseek(novo_doc, 0, SEEK_SET); //volta para o inicio
    return tam_doc;
}

//retorna doc duplicado ou -1 caso contrario
int encontra_doc(const Library *lib, const char *docname) {
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
    long bytes_lidos; 
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
int gbv_remove(Library *lib, const char *archive, const char *docname) {
    if(!lib || !docname) {
        perror("biblioteca ou doc invalidos");
        return -1;
    }

    //abre o arquivo pra atualizar
    FILE *file = fopen(archive, "rb+");
    if (!file) {
        perror("GBV_REMOVE erro ao abrir biblioteca\n");
        return -1;
    }
    
    //encontra o doc a ser removido com a funcao
    int doc_remove = encontra_doc(lib,docname);
    if(doc_remove == -1) {
        printf("Documento nao encontrado ou ja removido\n");
        return -1;
    }

    //infos sobre documento a ser removido
    long offset_doc_removido = lib->docs[doc_remove].offset; //onde esta o arquivo a ser removido
    long tam_doc_removido = lib->docs[doc_remove].size;
    long offset_pos_remocao = offset_doc_removido + tam_doc_removido;
    printf("Removendo documento - Offset_doc: %ld, Tamanho: %ld\n", offset_pos_remocao, tam_doc_removido);
    printf("\n");

    //le o superbloco
    superbloco sb;
    fread(&sb, sizeof(superbloco), 1, file);
    fseek(file, 0, SEEK_SET);

    long prox_offset_dados = sb.offset_dir;
    
    //encontra inicio do rpox documento se existir
    for(int i = 0; i < lib->count; i++) {
        if(lib->docs[i].offset > offset_doc_removido) {
            prox_offset_dados = lib->docs[i].offset;
            break;
        }
    }

    long tam_mover = prox_offset_dados - offset_pos_remocao;
    //DEBUG
    //printf(" DEBUG: Movendo %ld bytes de dados...\n", tam_mover); //SO PARA DEBUG

    //shift dos dados, movendo para frente
    if(tam_mover > 0) {
        char buffer[BUFFER_SIZE];
        long bytes_faltando = tam_mover;
        long offset_leitura = offset_pos_remocao;
        long offset_escrita = offset_pos_remocao;

        //DEBUG
        //printf("DEBUG: Movendo %ld bytes de dados...\n", tam_mover); //SO PARA DEBUG

        //move em blocos do tam do buffer
        while (bytes_faltando > 0) {
            long bytes_ler;

            //quantos bytes ler no bloco atual
            if(bytes_faltando < BUFFER_SIZE) 
                //ultimo bloco, menos bytes que tam buffer
                bytes_ler = bytes_faltando;
            else
                //le max de bytes do buffer
                bytes_ler = BUFFER_SIZE;  
            
            //le o offset atual
            fseek(file, offset_leitura, SEEK_SET);
            long bytes_lidos = fread(buffer, 1, bytes_ler, file);

            if(bytes_lidos > 0) {
                //escreve no novo offset
                fseek(file, offset_escrita, SEEK_SET);
                fwrite(buffer, 1, bytes_lidos, file);

                offset_leitura += bytes_lidos;
                offset_escrita += bytes_lidos;
                bytes_faltando -= bytes_lidos;
            } else 
                break;
        }
        
    }

    //atualiza offset dos documentos movidos
    for(int i = 0; i < lib->count; i++) {
        if(lib->docs[i].offset > offset_pos_remocao)
            lib->docs[i].offset -= tam_doc_removido;
    }

    //remove metadadaos
    //se nao for o ultimo elemento do vetor, reorganiza
    if(doc_remove < lib->count -1) {
        //move todos os elementos para tras
        for(int i = doc_remove; i < lib->count -1; i++)
            lib->docs[i] = lib->docs[i + 1];
    }
    //reduz a contadora
    lib->count--;
    sb.num_docs = lib->count;  
    //REVISAR COMO FUNCIONA
    sb.offset_dir -= tam_doc_removido;
    printf("offset_dir apos remocao = %ld\n", sb.offset_dir);
    printf("\n");

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

    //atualiza o arquivo
    fseek(file, 0, SEEK_SET);
    fwrite(&sb, sizeof(superbloco), 1, file);

    //reescreve o diretorio e os docs
    fseek(file, sb.offset_dir, SEEK_SET);
    //se nao tiver apenas um doc no arquivo, reescreva os docs
    if(lib->count > 0)
        fwrite(lib->docs, sizeof(Document), lib->count, file); 

    //trunca o arquivo
    long novo_tam_file = sb.offset_dir + (lib->count * sizeof(Document));
    printf("Novo tamanho da GBV = %ld\n", novo_tam_file);
    printf("\n");

    fflush(file); //garante que todos os dados foram escritos NAO SEI SE PRECISA 

    //REVISAR OQ E ISSO
    long file_descritor = fileno(file);
    if(ftruncate(file_descritor, novo_tam_file) != 0) {
        perror("GBV REMOVE erro ao reduzir arquivo");
        return -1;
    }

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
           "NOME", "TAMANHO (B)", "DATA INSERIDO", "OFFSET DADOS");
    
  
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

//exibe os caracteres bloco atual na funaco view
void exibe_bloco( const char *buffer, long bytes_lidos) {
    //exibe conteudo
    for(int i = 0; i < bytes_lidos; i++) {
        //caracteres que podem ser impresssos segunda ASCII entre 32 e 126
        if(buffer[i] >= 32 && buffer[i] <= 126) 
            printf("%c", buffer[i]);
        else    
            printf("."); //nao podem ser impressos
    }
    printf("\n");
}

//visuaiza o documento em blocos de tamanho fixo e permite navegar pelo conteudo do doc
//n = prox bloco
//p = bloco anterior
//q = sair da visualizacao
int gbv_view(const Library *lib , const char *archive, const char *docname) {
    if(!archive || !docname) {
        perror("GBV VIEW arquivo ou doc invalidos");
        return -1;
    }

    //doc a ser visualizado
    int doc_view = encontra_doc(lib, docname);

    FILE *file = fopen(archive, "rb");
    if(!file) {
        perror("GBV ADD erro ao abrir arquivo");
        return -1;
    }

    //variaveis para visualizacao do doc
    Document doc = lib->docs[doc_view];
    long bloco_atual = 0;
    //arredonda para garantir leitura de todos o blocos corretamente
    //ex se (1000 + 511) ÷ 512 = 2.95 blocos (arredonda para 2 ao inves de 1 bloco que perde todo o resto)
    //ex se fosse um arquivo menor e a div desse 0.75 ele truncaria para 0 blocos
    long total_blocos = (doc.size + BUFFER_SIZE - 1) / BUFFER_SIZE;

    printf("view doc: %s (%ld bytes, %ld blocos)\n",docname, doc.size, total_blocos);

    //buffer para ler o doc
    char buffer[BUFFER_SIZE];
    char comando;

    do {
        //calcula a posicao no arquivo em que o bloco a ser visto esta
        long offset_bloco = doc.offset + (bloco_atual * BUFFER_SIZE);// quanto bytes pular desde o inicio do doc
        //bytes que faltam para ler no doc
        long bytes_faltando = doc.size - (bloco_atual * BUFFER_SIZE);
        long bytes_ler;

        //quantos bytes ler no bloco atual
        if(bytes_faltando < BUFFER_SIZE) 
            //ultimo bloco, menos bytes que tam buffer
            bytes_ler = bytes_faltando;
        else
            //le max de bytes do buffer
            bytes_ler = BUFFER_SIZE;

        //le o bloco apra o buffer e exibe
        fseek(file, offset_bloco, SEEK_SET); //inicio do bloco
        long bytes_lidos = fread(buffer, 1, bytes_ler, file); 
        exibe_bloco(buffer, bytes_lidos);

        //comandos de navegacao
        printf("digite comando p/q/n\n");
        comando = getchar();
        //esvazia buffer para nao bugar
        while(getchar() != '\n'); 

        switch(comando) {
            case 'n': //prox bloco
                if(bloco_atual < total_blocos - 1) 
                    bloco_atual++;            
                else
                    printf("ULTIMO BLOCO, use q para sair ou p para voltar\n");
                break;
            case 'p': //bloco anterior
                if(bloco_atual > 0)
                    bloco_atual--;
                else
                    printf("PRIMEIRO BLOCO, use q para sair ou n para avancar\n");
                break;
            case 'q': //sair da view
                printf("saida da visualizacao do doc\n");
                break;
            default:
                printf("comando invalido\n");
        }   
        printf("\n");

    } while(comando != 'q');

    fclose(file);
    return 0;
}

//libera memoria
void gbv_close(Library *lib) {
    if (lib && lib->docs) {
        free(lib->docs);
        lib->docs = NULL;
        lib->count = 0;
    }
}



