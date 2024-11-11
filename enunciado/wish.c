#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_PATHS 100
#define MAX_ARGS 100
#define MAX_INPUT 1024

// Array para almacenar las rutas de búsqueda
char *paths[MAX_PATHS] = { "/bin" }; // Ruta inicial
int path_count = 1;

// Mensaje de error
const char error_message[30] = "An error has occurred\n";

// Función para imprimir el mensaje de error
void print_error() {
    write(STDERR_FILENO, error_message, strlen(error_message));
}

// Función para ejecutar el comando integrado `cd`
void execute_cd(char *path) {
    if (path == NULL || chdir(path) != 0) {
        print_error();
    }
}

// Función para ejecutar el comando integrado `path`
void execute_path(char **args, int num_args) {
    // Limpiar rutas anteriores
    for (int i = 0; i < path_count; i++) {
        free(paths[i]);
    }
    path_count = 0;

    // Agregar nuevas rutas si existen argumentos
    for (int i = 0; i < num_args; i++) {
        paths[path_count] = strdup(args[i]);  // Duplicar cada ruta
        if (paths[path_count] == NULL) { // Manejo de errores de memoria
            print_error();
            return;
        }
        path_count++;
    }
}

// Función para encontrar el comando en las rutas especificadas
char *find_command(char *command) {
    for (int i = 0; i < path_count; i++) {
        char *full_path = malloc(strlen(paths[i]) + strlen(command) + 2);
        sprintf(full_path, "%s/%s", paths[i], command);
        if (access(full_path, X_OK) == 0) {
            return full_path; // Encontrado y ejecutable
        }
        free(full_path);
    }
    return NULL; // No se encontró
}

// Función para ejecutar comandos externos
void execute_command(char **args) {
    char *command_path = find_command(args[0]);
    if (command_path == NULL) {
        print_error();
        return;
    }

    if (fork() == 0) { // Proceso hijo
        execv(command_path, args);
        print_error();
        exit(1); // Salir si exec falla
    } else { // Proceso padre
        wait(NULL);
    }
    free(command_path);
}

// Función para analizar y ejecutar el comando
void parse_and_execute(char *line) {
    char *args[MAX_ARGS];
    int num_args = 0;

    // Separar la línea en argumentos
    char *token = strtok(line, " \t\n");
    while (token != NULL) {
        args[num_args++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[num_args] = NULL;

    // Verificar si hay algún comando
    if (num_args == 0) {
        return; // Línea vacía
    }

    // Manejar comandos integrados
    if (strcmp(args[0], "exit") == 0) {
        if (num_args == 1) {
            exit(0);
        } else {
            print_error();
        }
    } else if (strcmp(args[0], "cd") == 0) {
        if (num_args == 2) {
            execute_cd(args[1]);
        } else {
            print_error();
        }
    } else if (strcmp(args[0], "path") == 0) {
        execute_path(&args[1], num_args - 1);
    } else {
        // Ejecutar comandos externos
        execute_command(args);
    }
}

// Función principal de la shell
void shell_loop(FILE *input) {
    char line[MAX_INPUT];
    while (1) {
        if (input == stdin) { // Modo interactivo
            printf("wish> ");
        }

        // Leer la línea de entrada
        if (fgets(line, MAX_INPUT, input) == NULL) {
            break; // EOF
        }

        // Analizar y ejecutar el comando
        parse_and_execute(line);
    }
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        // Modo interactivo
        shell_loop(stdin);
    } else if (argc == 2) {
        // Modo batch
        FILE *batch_file = fopen(argv[1], "r");
        if (batch_file == NULL) {
            print_error();
            exit(1);
        }
        shell_loop(batch_file);
        fclose(batch_file);
    } else {
        // Error: demasiados argumentos
        print_error();
        exit(1);
    }
    return 0;
}
