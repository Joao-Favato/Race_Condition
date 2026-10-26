#include <kernel.h>
#include <sys/printk.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(race_condition_demo, LOG_LEVEL_INF);

#define STACK_SIZE 1024
#define THREAD_PRIORITY 7
#define NUM_OPERACOES 10
#define VALOR_OPERACAO 10

/* RECURSO COMPARTILHADO (Estoque de Produtos) */
static volatile int estoque_disponivel = 100;

/* =========================================================================
 * THREAD 1: Execução de Reposição de Estoque (Adiciona itens)
 * ========================================================================= */
void thread_reposicao_estoque(void *p1, void *p2, void *p3)
{
    for (int i = 0; i < NUM_OPERACOES; i++) {
        
        /* 1. Leitura do estado atual */
        int temp = estoque_disponivel;
        LOG_INF("[THREAD REPOSICAO] Leu estoque atual = %d", temp);

        /* 2. Janela de Vulnerabilidade (Preempção do Zephyr) */
        k_msleep(1);

        /* 3. Processamento e Escrita */
        temp += VALOR_OPERACAO;
        estoque_disponivel = temp;
        LOG_INF("--> [THREAD REPOSICAO] Inseriu %d itens. Novo estoque = %d", 
                VALOR_OPERACAO, estoque_disponivel);

        k_msleep(5);
    }
}

/* =========================================================================
 * THREAD 2: Execução de Venda / Abate de Estoque (Remove itens)
 * ========================================================================= */
void thread_venda_estoque(void *p1, void *p2, void *p3)
{
    for (int i = 0; i < NUM_OPERACOES; i++) {
        
        /* 1. Leitura do estado atual */
        int temp = estoque_disponivel;
        LOG_INF("[THREAD VENDAS]    Leu estoque atual = %d", temp);

        /* 2. Janela de Vulnerabilidade (Preempção do Zephyr) */
        k_msleep(1);

        /* 3. Processamento e Escrita */
        temp -= VALOR_OPERACAO;
        estoque_disponivel = temp;
        LOG_INF("--> [THREAD VENDAS]    Vendeu %d itens. Novo estoque = %d", 
                VALOR_OPERACAO, estoque_disponivel);

        k_msleep(5);
    }
}

/* Definição de duas Threads com funções de entrada distintas */
K_THREAD_DEFINE(t_reposicao, STACK_SIZE, thread_reposicao_estoque, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(t_vendas, STACK_SIZE, thread_venda_estoque, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);

int main(void)
{
    int estoque_inicial = estoque_disponivel;

    LOG_INF("==================================================");
    LOG_INF("  FRDM-KL25Z: RACE CONDITION COM THREADS DISTINTAS");
    LOG_INF("==================================================");
    LOG_INF("Estoque Inicial          : %d unidades", estoque_inicial);
    LOG_INF("Operações da Reposicao   : %d x (+%d)", NUM_OPERACOES, VALOR_OPERACAO);
    LOG_INF("Operações de Vendas      : %d x (-%d)", NUM_OPERACOES, VALOR_OPERACAO);
    LOG_INF("Estoque Esperado ao Final: %d unidades", estoque_inicial);
    LOG_INF("--------------------------------------------------");

    /* Aguarda o término de todas as operações */
    k_msleep(3000);

    /* Exibição da divergência no saldo final */
    LOG_INF("==================================================");
    LOG_INF("RESULTADO FINAL:");
    LOG_INF("Estoque ESPERADO : %d", estoque_inicial);
    LOG_INF("Estoque REAL     : %d", estoque_disponivel);
    
    if (estoque_disponivel != estoque_inicial) {
        LOG_INF("STATUS           : RACE CONDITION DETECTADA!");
        LOG_INF("Diferença de %d unidades por sobrescrita de memória.", 
                estoque_disponivel - estoque_inicial);
    } else {
        LOG_INF("STATUS           : Execucao sem perdas aparentes (Raro sem Mutex).");
    }
    LOG_INF("==================================================");

    return 0;
}