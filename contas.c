/*
 * Sistema de Manutenção de Contas Bancárias
 *
 * Arquivo binário de registros fixos.
 * Acesso direto via fseek(), leitura/escrita com fread()/fwrite(),
 * reinício da leitura com rewind().
 */

#include <stdio.h>
#include <string.h>

#define ARQUIVO "contas.dat"
#define MAX_NOME 50
#define CONTA_VAZIA 0 /* id == 0 indica posição livre */

/* ------------------------------------------------------------------ */
/*  Estrutura de registro — tamanho FIXO para acesso direto            */
/* ------------------------------------------------------------------ */
typedef struct
{
    int id;              /* número da conta; 0 = posição vazia      */
    char nome[MAX_NOME]; /* nome do titular                         */
    float saldo;         /* saldo atual                             */
    int ativo;           /* 1 = conta ativa  |  0 = encerrada       */
} Conta;

/* ------------------------------------------------------------------ */
/*  Utilitários                                                        */
/* ------------------------------------------------------------------ */

/* Abre o arquivo em modo leitura+escrita binário.
   Cria o arquivo se não existir (modo "a+b" para não truncar). */
FILE *abrir_arquivo(const char *modo)
{
    FILE *fp = fopen(ARQUIVO, modo);
    if (!fp)
    {
        perror("Erro ao abrir arquivo");
    }
    return fp;
}

/* Calcula o número de registros gravados no arquivo */
int total_registros(FILE *fp)
{
    fseek(fp, 0, SEEK_END);
    long bytes = ftell(fp);
    return (int)(bytes / sizeof(Conta));
}

/* Posiciona o cursor no registro de índice 'pos' (base 0) */
void ir_para(FILE *fp, int pos)
{
    fseek(fp, (long)pos * sizeof(Conta), SEEK_SET);
}

/* Lê um registro na posição 'pos' */
int ler_registro(FILE *fp, int pos, Conta *c)
{
    ir_para(fp, pos);
    return (int)fread(c, sizeof(Conta), 1, fp);
}

/* Grava um registro na posição 'pos' */
int gravar_registro(FILE *fp, int pos, const Conta *c)
{
    ir_para(fp, pos);
    return (int)fwrite(c, sizeof(Conta), 1, fp);
}

void linha(void)
{
    printf("--------------------------------------------------\n");
}

void cabecalho(void)
{
    printf("\n==================================================\n");
    printf("       SISTEMA DE MANUTENCAO DE CONTAS\n");
    printf("==================================================\n");
}

/* ------------------------------------------------------------------ */
/*  1. Cadastrar cliente em posição específica                         */
/* ------------------------------------------------------------------ */
void cadastrar(void)
{
    FILE *fp = abrir_arquivo("r+b"); /* abre existente para leitura+escrita */
    if (!fp)
    {
        /* Arquivo ainda não existe — cria */
        fp = abrir_arquivo("w+b");
        if (!fp)
            return;
    }

    int pos;
    Conta c;

    printf("\n--- CADASTRAR NOVO CLIENTE ---\n");
    printf("Posicao do registro (0 = primeira): ");
    scanf("%d%*c", &pos);

    int total = total_registros(fp);

    /* Verifica se a posição já está ocupada por conta ativa */
    if (pos < total)
    {
        ler_registro(fp, pos, &c);
        if (c.ativo == 1)
        {
            printf("ERRO: Posicao %d ja esta ocupada pela conta #%d (%s).\n",
                   pos, c.id, c.nome);
            fclose(fp);
            return;
        }
    }

    printf("Numero da conta  : ");
    scanf("%d%*c", &c.id);
    printf("Nome do titular  : ");
    fgets(c.nome, MAX_NOME, stdin);
    c.nome[strcspn(c.nome, "\n")] = '\0'; /* remove '\n' do fgets */
    printf("Saldo inicial R$ : ");
    scanf("%f%*c", &c.saldo);
    c.ativo = 1;

    /* Se a posição pedida for além do fim, preenche lacunas com vazios */
    if (pos > total)
    {
        Conta vazio = {0, "", 0.0f, 0};
        for (int i = total; i < pos; i++)
        {
            gravar_registro(fp, i, &vazio);
        }
    }

    gravar_registro(fp, pos, &c);
    printf("Conta #%d cadastrada na posicao %d com sucesso!\n", c.id, pos);

    fclose(fp);
}

/* ------------------------------------------------------------------ */
/*  2. Consultar cliente pelo número da conta                          */
/* ------------------------------------------------------------------ */
void consultar(void)
{
    FILE *fp = abrir_arquivo("rb");
    if (!fp)
        return;

    int numero;
    printf("\n--- CONSULTAR CLIENTE ---\n");
    printf("Numero da conta: ");
    scanf("%d%*c", &numero);

    int total = total_registros(fp);
    int encontrado = 0;
    Conta c;

    for (int i = 0; i < total; i++)
    {
        ler_registro(fp, i, &c);
        if (c.id == numero && c.ativo == 1)
        {
            linha();
            printf("Posicao no arquivo : %d\n", i);
            printf("Numero da conta    : %d\n", c.id);
            printf("Titular            : %s\n", c.nome);
            printf("Saldo              : R$ %.2f\n", c.saldo);
            printf("Status             : Ativa\n");
            linha();
            encontrado = 1;
            break;
        }
    }

    if (!encontrado)
    {
        printf("Conta #%d nao encontrada ou encerrada.\n", numero);
    }

    fclose(fp);
}

/* ------------------------------------------------------------------ */
/*  3. Atualizar saldo                                                 */
/* ------------------------------------------------------------------ */
void atualizar_saldo(void)
{
    FILE *fp = abrir_arquivo("r+b");
    if (!fp)
        return;

    int numero;
    printf("\n--- ATUALIZAR SALDO ---\n");
    printf("Numero da conta: ");
    scanf("%d%*c", &numero);

    int total = total_registros(fp);
    int encontrado = 0;
    Conta c;

    for (int i = 0; i < total; i++)
    {
        ler_registro(fp, i, &c);
        if (c.id == numero && c.ativo == 1)
        {
            printf("Titular         : %s\n", c.nome);
            printf("Saldo atual     : R$ %.2f\n", c.saldo);
            printf("Novo saldo R$   : ");
            scanf("%f%*c", &c.saldo);

            gravar_registro(fp, i, &c);
            printf("Saldo atualizado com sucesso!\n");
            encontrado = 1;
            break;
        }
    }

    if (!encontrado)
    {
        printf("Conta #%d nao encontrada ou encerrada.\n", numero);
    }

    fclose(fp);
}

/* ------------------------------------------------------------------ */
/*  4. Encerrar conta (marca ativo = 0, preserva o registro)           */
/* ------------------------------------------------------------------ */
void encerrar_conta(void)
{
    FILE *fp = abrir_arquivo("r+b");
    if (!fp)
        return;

    int numero;
    printf("\n--- ENCERRAR CONTA ---\n");
    printf("Numero da conta: ");
    scanf("%d%*c", &numero);

    int total = total_registros(fp);
    int encontrado = 0;
    Conta c;

    for (int i = 0; i < total; i++)
    {
        ler_registro(fp, i, &c);
        if (c.id == numero && c.ativo == 1)
        {
            char conf;
            printf("Confirma encerramento da conta de %s? (s/n): ", c.nome);
            scanf("%c%*c", &conf);

            if (conf == 's' || conf == 'S')
            {
                c.ativo = 0;
                gravar_registro(fp, i, &c);
                printf("Conta #%d encerrada. Posicao %d liberada.\n", numero, i);
            }
            else
            {
                printf("Operacao cancelada.\n");
            }
            encontrado = 1;
            break;
        }
    }

    if (!encontrado)
    {
        printf("Conta #%d nao encontrada ou ja encerrada.\n", numero);
    }

    fclose(fp);
}

/* ------------------------------------------------------------------ */
/*  5. Listar todos os clientes                                        */
/* ------------------------------------------------------------------ */
void listar(FILE *fp)
{
    int total = total_registros(fp);

    if (total == 0)
    {
        printf("Nenhum registro no arquivo.\n");
        return;
    }

    printf("\n%-5s %-6s %-30s %12s  %s\n",
           "Pos", "Conta", "Titular", "Saldo (R$)", "Status");
    linha();

    Conta c;
    for (int i = 0; i < total; i++)
    {
        ler_registro(fp, i, &c);
        if (c.id == CONTA_VAZIA)
        {
            printf("%-5d %-6s %-30s %12s  %s\n",
                   i, "---", "(vazio)", "---", "Livre");
        }
        else
        {
            printf("%-5d %-6d %-30s %12.2f  %s\n",
                   i, c.id, c.nome, c.saldo,
                   c.ativo ? "Ativa" : "Encerrada");
        }
    }
    linha();
}

void opcao_listar(void)
{
    FILE *fp = abrir_arquivo("rb");
    if (!fp)
        return;
    printf("\n--- LISTAGEM DE CLIENTES ---\n");
    listar(fp);
    fclose(fp);
}

/* ------------------------------------------------------------------ */
/*  6. Rewind — reinicia leitura do início e lista novamente           */
/* ------------------------------------------------------------------ */
void opcao_rewind(void)
{
    FILE *fp = abrir_arquivo("rb");
    if (!fp)
        return;

    printf("\n--- REWIND: releitura do arquivo do inicio ---\n");

    /* rewind() equivale a fseek(fp, 0, SEEK_SET) + limpa flag de erro */
    rewind(fp);

    printf("Cursor reposicionado para o byte 0. Relendo registros...\n\n");
    listar(fp);

    fclose(fp);
}

/* ------------------------------------------------------------------ */
/*  Menu principal                                                     */
/* ------------------------------------------------------------------ */
int main(void)
{
    int opcao;

    do
    {
        cabecalho();
        printf("  1. Cadastrar novo cliente\n");
        printf("  2. Consultar cliente pelo numero da conta\n");
        printf("  3. Atualizar saldo\n");
        printf("  4. Encerrar conta\n");
        printf("  5. Listar todos os clientes\n");
        printf("  6. Rewind (releitura do inicio do arquivo)\n");
        printf("  7. Encerrar programa\n");
        linha();
        printf("Opcao: ");
        scanf("%d%*c", &opcao);

        switch (opcao)
        {
        case 1:
            cadastrar();
            break;
        case 2:
            consultar();
            break;
        case 3:
            atualizar_saldo();
            break;
        case 4:
            encerrar_conta();
            break;
        case 5:
            opcao_listar();
            break;
        case 6:
            opcao_rewind();
            break;
        case 7:
            printf("\nEncerrando sistema. Ate logo!\n");
            break;
        default:
            printf("\nOpcao invalida. Tente novamente.\n");
            break;
        }

        if (opcao != 7)
        {
            printf("\nPressione ENTER para continuar...");
            getchar();
        }

    } while (opcao != 7);

    printf("\nPressione ENTER para encerrar");
    getchar();

    return 0;
}