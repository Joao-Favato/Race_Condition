#include <kernel.h>
#include <sys/printk.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(race_condition_demo, LOG_LEVEL_INF);

#define STACK_SIZE 1024
#define THREAD_PRIORITY 7
#define ITERACOES_POR_THREAD 10

/* RECURSO COMPARTILHADO (Variável Global) */
static volatile int contador_compartilhado = 0;

/* Função executada concorrentemente pelas Threads */
void thread_trabalhadora(void *p1, void *p2, void *p3)
{
    int thread_id = (int)(intptr_t)p1;

    for (int i = 0; i < ITERACOES_POR_THREAD; i++) {
        
        /* --- INÍCIO DA SEÇÃO CRÍTICA (DESPROTEGIDA) --- */

        /* 1. LEITURA: A thread lê o valor atual da variável global */
        int valor_local = contador_compartilhado;
        LOG_INF("[Thread %d] LEU contador = %d", thread_id, valor_local);

        /* 2. ATRASO ARTIFICIAL (Janela de Vulnerabilidade):
         * k_msleep faz a thread atual entrar em estado BLOCKED/SLEEPING.
         * O escalonador do Zephyr é forçado a trocar de contexto e dar 
         * execução para a outra thread ANTES que esta escreva a atualização.
         */
        k_msleep(1);

        /* 3. MODIFICAÇÃO */
        valor_local = valor_local + 1;

        /* 4. ESCRITA: Sobrescreve a memória com o valor incrementado */
        contador_compartilhado = valor_local;
        LOG_INF("--> [Thread %d] ESCREVEU contador = %d", thread_id, contador_compartilhado);

        /* --- FIM DA SEÇÃO CRÍTICA --- */

        k_msleep(5); /* Pausa fora da seção crítica */
    }
}

/* Criação de 2 Threads concorrentes no Zephyr com a mesma prioridade */
K_THREAD_DEFINE(thread_A, STACK_SIZE, thread_trabalhadora, (void *)1, NULL, NULL, THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(thread_B, STACK_SIZE, thread_trabalhadora, (void *)2, NULL, NULL, THREAD_PRIORITY, 0, 0);

int main(void)
{
    LOG_INF("==================================================");
    LOG_INF("  FRDM-KL25Z: DEMONSTRACAO DE RACE CONDITION      ");
    LOG_INF("==================================================");
    LOG_INF("2 Threads incrementando o recurso %d vezes cada.", ITERACOES_POR_THREAD);
    LOG_INF("Valor Final Esperado: %d", ITERACOES_POR_THREAD * 2);
    LOG_INF("--------------------------------------------------");

    /* Aguarda tempo suficiente para as threads concluírem o ciclo */
    k_msleep(3000);

    /* Exibição dos resultados e evidência da perda de atualização */
    LOG_INF("==================================================");
    LOG_INF("RESULTADO FINAL DA EXECUCAO:");
    LOG_INF("Valor ESPERADO : %d", ITERACOES_POR_THREAD * 2);
    LOG_INF("Valor REAL     : %d", contador_compartilhado);
    
    int perdas = (ITERACOES_POR_THREAD * 2) - contador_compartilhado;
    if (perdas > 0) {
        LOG_INF("STATUS         : RACE CONDITION DETECTADA!");
        LOG_INF("Perda de %d incremento(s) devido a leituras inconsistentes.", perdas);
    } else {
        LOG_INF("STATUS         : Nenhuma perda detectada (Execucao atipica).");
    }
    LOG_INF("==================================================");

    return 0;
}