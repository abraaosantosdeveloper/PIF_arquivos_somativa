# Sistema de Manutenção de Contas — Arquivo Binário em C

Resolução da atividade da disciplina, cobrindo manipulação de arquivos binários com registros de tamanho fixo, acesso direto via `fseek()`, leitura e escrita com `fread()`/`fwrite()` e reinício de leitura com `rewind()`.

---

## Estrutura do repositório

```
.
├── contas.c     # Sistema completo com menu interativo
├── contas.dat   # Arquivo binário gerado em tempo de execução
└── README.md
```

> `contas.dat` é criado automaticamente na primeira execução. Não é necessário criá-lo manualmente.

---

## Como compilar e executar

Pré-requisito: GCC instalado.

```bash
gcc -Wall -o contas contas.c
./contas
```

---

## Explicação do sistema

### A struct `Conta` — registros de tamanho fixo

```c
typedef struct {
    int   id;           /* número da conta; 0 = posição vazia  */
    char  nome[50];     /* tamanho FIXO — obrigatório           */
    float saldo;
    int   ativo;        /* 1 = ativa  |  0 = encerrada         */
} Conta;
```

O campo `nome[50]` ter tamanho fixo é um requisito técnico fundamental. Se fosse `char*`, cada registro ocuparia um tamanho diferente em disco e o `fseek()` não conseguiria saltar para posições específicas de forma confiável.

---

## Explicação por funcionalidade

### `fseek()` — acesso direto a qualquer posição

```c
void ir_para(FILE *fp, int pos) {
    fseek(fp, (long)pos * sizeof(Conta), SEEK_SET);
}
```

**Como funciona:**  
`fseek` move o cursor interno do arquivo para o byte `pos * sizeof(Conta)`. Como todos os registros têm o mesmo tamanho, multiplicar o índice pelo tamanho da struct leva diretamente ao início do registro desejado — sem ler os anteriores.

| Posição | Byte inicial no arquivo    |
|---------|---------------------------|
| 0       | `0 * sizeof(Conta)` = 0   |
| 1       | `1 * sizeof(Conta)` = 64  |
| 2       | `2 * sizeof(Conta)` = 128 |
| n       | `n * sizeof(Conta)`       |

---

### `fwrite()` — gravação de registro

```c
int gravar_registro(FILE *fp, int pos, const Conta *c) {
    ir_para(fp, pos);
    return (int) fwrite(c, sizeof(Conta), 1, fp);
}
```

**Como funciona:**  
`fwrite(ponteiro, tamanho_de_um_elemento, quantidade, arquivo)` copia os bytes da struct diretamente para o disco. Não há conversão para texto — os dados ficam em formato binário, exatamente como estão na memória.

---

### `fread()` — leitura de registro

```c
int ler_registro(FILE *fp, int pos, Conta *c) {
    ir_para(fp, pos);
    return (int) fread(c, sizeof(Conta), 1, fp);
}
```

**Como funciona:**  
`fread` lê `sizeof(Conta)` bytes da posição atual e os preenche diretamente nos campos da struct. O retorno indica quantos elementos foram lidos com sucesso (esperado: 1).

---

### `rewind()` — reinício da leitura

```c
void opcao_rewind(void) {
    FILE *fp = abrir_arquivo("rb");
    rewind(fp);   /* equivale a fseek(fp, 0, SEEK_SET) + limpa flag de erro */
    listar(fp);
    fclose(fp);
}
```

**Diferença entre `rewind` e `fseek` para o byte 0:**  
`rewind(fp)` faz duas coisas: reposiciona o cursor no início *e* limpa o indicador de erro do arquivo. `fseek(fp, 0, SEEK_SET)` apenas reposiciona. Para releituras após operações que possam ter gerado erros, `rewind` é a escolha mais segura.

---

### Encerramento de conta — exclusão lógica

```c
/* Opção 4: não apaga — apenas marca como inativa */
c.ativo = 0;
gravar_registro(fp, i, &c);
```

**Por que não deletar fisicamente?**  
Em arquivos de acesso direto, remover um registro do meio exigiria deslocar todos os seguintes — uma operação cara. A solução adotada é a **exclusão lógica**: o registro permanece no disco com `ativo = 0`, liberando a posição para reuso no próximo cadastro.

---

### Limpeza de buffer com `%*c`

```c
scanf("%d%*c", &opcao);   /* lê inteiro e descarta o '\n' do ENTER */
scanf("%f%*c", &saldo);   /* lê float  e descarta o '\n' do ENTER  */
```

**Por que é necessário?**  
`scanf` com `%d` ou `%f` lê o número mas deixa o `\n` no buffer. Se o próximo comando for um `fgets`, ele leria essa quebra de linha como entrada vazia. O especificador `%*c` consome exatamente um caractere (o `\n`) sem atribuí-lo a nenhuma variável, evitando o problema do "ENTER duplo".

---

## Resumo visual do arquivo binário

```
contas.dat (arquivo em disco)
┌──────────────────┬──────────────────┬──────────────────┬─────┐
│   Registro 0     │   Registro 1     │   Registro 2     │ ... │
│  sizeof(Conta)   │  sizeof(Conta)   │  sizeof(Conta)   │     │
│  bytes = 64      │  bytes = 64      │  bytes = 64      │     │
│  id=101          │  id=102          │  id=0 (vazio)    │     │
│  nome="Ana"      │  nome="Bruno"    │  nome=""         │     │
│  saldo=1500.0    │  saldo=3200.5    │  saldo=0.0       │     │
│  ativo=1         │  ativo=0 (enc.)  │  ativo=0         │     │
└──────────────────┴──────────────────┴──────────────────┴─────┘
         ▲                   ▲
         │                   │
   fseek(fp, 0*64)     fseek(fp, 1*64)
   acesso direto        acesso direto
```