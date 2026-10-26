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

/* PRIMITIVA DE SINCRONIZAÇÃO: Declaração do Mutex do Zephyr */
K_MUTEX_DEFINE(estoque_mutex);

/* =========================================================================
 * THREAD 1: Execução de Reposição de Estoque (Com Proteção)
 * ========================================================================= */
void thread_reposicao_estoque(void *p1, void *p2, void *p3)
{
    for (int i = 0; i < NUM_OPERACOES; i++) {
        
        /* ENTRADA NA SEÇÃO CRÍTICA: Bloqueia o acesso de outras threads */
        k_mutex_lock(&estoque_mutex, K_FOREVER);

        int temp = estoque_disponivel;
        LOG_INF("[THREAD REPOSICAO] [LOCK] Leu estoque atual = %d", temp);

        /* Mesmo que ocorra interrupção/preempção durante o sleep,
         * nenhuma outra thread conseguirá entrar na seção crítica. */
        k_msleep(1);

        temp += VALOR_OPERACAO;
        estoque_disponivel = temp;
        LOG_INF("--> [THREAD REPOSICAO] Inseriu %d itens. Novo estoque = %d", 
                VALOR_OPERACAO, estoque_disponivel);

        /* SAÍDA DA SEÇÃO CRÍTICA: Libera o recurso para a próxima thread */
        k_mutex_unlock(&estoque_mutex);

        k_msleep(5); /* Pausa fora do lock para permitir chaveamento */
    }
}

/* =========================================================================
 * THREAD 2: Execução de Venda / Abate de Estoque (Com Proteção)
 * ========================================================================= */
void thread_venda_estoque(void *p1, void *p2, void *p3)
{
    for (int i = 0; i < NUM_OPERACOES; i++) {
        
        /* ENTRADA NA SEÇÃO CRÍTICA: Aguarda a liberação se estiver bloqueado */
        k_mutex_lock(&estoque_mutex, K_FOREVER);

        int temp = estoque_disponivel;
        LOG_INF("[THREAD VENDAS]    [LOCK] Leu estoque atual = %d", temp);

        k_msleep(1);

        temp -= VALOR_OPERACAO;
        estoque_disponivel = temp;
        LOG_INF("--> [THREAD VENDAS]    Vendeu %d itens. Novo estoque = %d", 
                VALOR_OPERACAO, estoque_disponivel);

        /* SAÍDA DA SEÇÃO CRÍTICA: Libera o recurso */
        k_mutex_unlock(&estoque_mutex);

        k_msleep(5); /* Pausa fora do lock */
    }
}

/* Definição das Threads */
K_THREAD_DEFINE(t_reposicao, STACK_SIZE, thread_reposicao_estoque, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(t_vendas, STACK_SIZE, thread_venda_estoque, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);

int main(void)
{
    int estoque_inicial = estoque_disponivel;

    LOG_INF("==================================================");
    LOG_INF("  FRDM-KL25Z: RESOLUCAO DA RACE CONDITION (MUTEX)");
    LOG_INF("==================================================");
    LOG_INF("Estoque Inicial          : %d unidades", estoque_inicial);
    LOG_INF("Operacoes da Reposicao   : %d x (+%d)", NUM_OPERACOES, VALOR_OPERACAO);
    LOG_INF("Operacoes de Vendas      : %d x (-%d)", NUM_OPERACOES, VALOR_OPERACAO);
    LOG_INF("Estoque Esperado ao Final: %d unidades", estoque_inicial);
    LOG_INF("--------------------------------------------------");

    /* Aguarda tempo suficiente para todas as execuções terminarem */
    k_msleep(3000);

    /* Validação da consistência dos dados */
    LOG_INF("==================================================");
    LOG_INF("RESULTADO FINAL COM MUTEX:");
    LOG_INF("Estoque ESPERADO : %d", estoque_inicial);
    LOG_INF("Estoque REAL     : %d", estoque_disponivel);
    
    if (estoque_disponivel == estoque_inicial) {
        LOG_INF("STATUS           : SUCESSO! INTEGRIDADE MANTIDA.");
        LOG_INF("A exclusao mutua via k_mutex impediu a sobrescrita de memoria.");
    } else {
        LOG_INF("STATUS           : ERRO (Falha na sincronizacao).");
    }
    LOG_INF("==================================================");

    return 0;
}