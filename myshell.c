#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <pwd.h>

#define MAX_CMD_LEN 1024
#define HISTORY_COUNT 10
#define ALIAS_COUNT 10

char *aliases[ALIAS_COUNT][2]; 
char *history[HISTORY_COUNT];   

int history_index = 0;

/*A função add_history adiciona um comando ao histórico. Ela libera a memória do comando atual 
no índice do histórico e, em seguida, duplica o comando fornecido para esse índice. O índice 
do histórico é então incrementado, ou retorna a zero se atingir o limite do histórico*/

void add_history(char *cmd) {
    free(history[history_index]); // Libera a memória do comando atual no índice
    history[history_index] = strdup(cmd); // Duplica o comando e armazena no histórico
    history_index = (history_index + 1) % HISTORY_COUNT; // Atualiza o índice
}

/*A função get_alias retorna o comando associado a um alias. Ela percorre a lista de aliases e, 
se encontrar o alias fornecido, retorna o comando correspondente. Se o alias não for encontrado, 
retorna NULL.*/

char *get_alias(char *alias) {
    for (int i = 0; i < ALIAS_COUNT; i++) {
        if (aliases[i][0] != NULL && strcmp(aliases[i][0], alias) == 0) {
            return aliases[i][1]; // Retorna o comando se o alias corresponder
        }
    }
    return NULL; // Retorna se o alias não for encontrado
}

/*A função add_alias adiciona um novo alias e seu comando correspondente à lista de aliases. Ela 
percorre a lista de aliases e, se encontrar um espaço vazio ou o alias fornecido, substitui o alias 
e o comando nesse índice. Se a lista de aliases estiver cheia, imprime uma mensagem de erro.*/

void add_alias(char *alias, char *cmd) {
    for (int i = 0; i < ALIAS_COUNT; i++) {
        if (aliases[i][0] == NULL || strcmp(aliases[i][0], alias) == 0) {
            free(aliases[i][0]); // Libera a memória do alias atual
            free(aliases[i][1]); // Libera a memória do comando atual
            aliases[i][0] = strdup(alias); // Duplica o alias e armazena
            aliases[i][1] = strdup(cmd); // Duplica o comando e armazena
            return; // Retorna após adicionar o alias
        }
    }
    printf("Erro: número máximo de aliases atingido\n"); // Imprime se o número máximo de aliases for atingido
}

/*A função get_history retorna o comando no índice fornecido do histórico. Se o índice for inválido, 
imprime uma mensagem de erro e retorna NULL. Caso contrário, calcula o índice correto no array 
circular do histórico e retorna o comando nesse índice.*/

char *get_history(int index) {
    if (index < 1 || index > HISTORY_COUNT) {
        printf("Erro: número de comando inválido\n"); // Imprime se o índice for inválido
        return NULL;
    }
    return history[(history_index - index + HISTORY_COUNT) % HISTORY_COUNT]; // Retorna o comando no índice especificado
}

int main() {
    char cmd[MAX_CMD_LEN]; // Armazena comando
    char hostname[1024]; // Armazena o nome do host
    hostname[1023] = '\0'; // Que a string seja terminada com NULL
    gethostname(hostname, 1023); // Pega o nome do host

    while (1) { 
        time_t rawtime;
        struct tm * timeinfo;
        char buffer[80];
        time (&rawtime); // Obtém o tempo atual
        timeinfo = localtime(&rawtime); // Converte o tempo para a hora local

        strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo); // Formata a hora como uma string
        
        struct passwd *pw;
        pw = getpwuid(geteuid());
        char *username = pw->pw_name; // obtem nome do usuario

        printf("%s@%s[%s] Digite um comando: ", username, hostname, buffer); // Imprime o prompt do shell

        fgets(cmd, MAX_CMD_LEN, stdin); // Lê o comando do usuário

        cmd[strcspn(cmd, "\n")] =  0;

        // Se o comando começa com '!', é uma referência ao histórico
        if (cmd[0] == '!') {
            int index = atoi(cmd + 1); // Converte o número após '!' para um inteiro
            if (index == 0 && cmd[1] != '0') {
                printf("Erro: número de comando inválido\n");
                continue;
            }
            char *history_cmd = get_history(index); // Obtém o comando do histórico
            if (history_cmd != NULL) {
                strncpy(cmd, history_cmd, MAX_CMD_LEN - 1); // Copia o comando do histórico para cmd
                cmd[MAX_CMD_LEN - 1] = '\0'; // Garante que cmd é sempre uma string válida
            }
        } else {
            add_history(cmd); // Adiciona o comando ao histórico
        }

        char *alias_cmd = get_alias(cmd); // Obtém o comando associado ao alias
        if (alias_cmd != NULL) {
            strcpy(cmd, alias_cmd); // Substitui cmd pelo comando do alias
        }

        // Verifica se o comando é um comando interno
        if (strcmp(cmd, "exit") == 0) {
            exit(0); // Sai do shell
        } else if (strncmp(cmd, "cd ", 3) == 0) {
            char *path = cmd + 3; // Obtém o caminho após 'cd '
            if (chdir(path) != 0) { // Muda o diretório atual para o caminho especificado
                perror("cd falhou"); // Imprime se a mudança de diretório falhar
            }
        } else if (strncmp(cmd, "alias ", 6) == 0) {
            char *alias = strtok(cmd + 6, "="); // Obtém o alias após 'alias '
            char *alias_cmd = strtok(NULL, "="); // Obtém o comando após o '='
            if (alias != NULL && alias_cmd != NULL) {
                add_alias(alias, alias_cmd); // Adiciona o alias e o comando associado
            } else {
                printf("Erro: comando alias inválido\n"); // Imprime um erro se o comando alias for inválido
            }
        } else if (strcmp(cmd, "alias") == 0) {
            for (int i = 0; i < ALIAS_COUNT; i++) {
                if (aliases[i][0] != NULL) {
                    printf("%s=%s\n", aliases[i][0], aliases[i][1]); // Imprime todos os aliases e comandos associados
                }
            }
        } else if (strcmp(cmd, "history") == 0) {
            for (int i = 1; i <= HISTORY_COUNT; i++) {
                char *cmd = get_history(i); // Obtém o comando do histórico
                if (cmd != NULL) {
                    printf("%d %s\n", i, cmd); // Imprime o número do comando e o comando
                }
            }
        } else {
            pid_t pid = fork(); // Cria um novo processo
            if (pid < 0) {
                perror("fork falhou"); // Imprime um erro se o fork falhar
            } else if (pid == 0) {
                char *argv[] = {"/bin/sh", "-c", cmd, NULL}; // Cria um array de argumentos para execvp
                if (execvp(argv[0], argv) < 0) { // Substitui o processo atual pelo /bin/sh
                    perror("execvp falhou"); // Imprime um erro se execvp falhar
                }
                exit(0); // Sai do processo filho
            } else {
                waitpid(pid, NULL, 0); // Espera o processo filho terminar
            }
        }
    }
    return 0;
}