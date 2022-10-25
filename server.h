#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <ctype.h>


/*
Estados de la partida.
*/
#define ON 1 //Partida en curso
#define OFF 0 //Partida sin empezar

/*
Estados del jugador.
*/
#define USUARIO 1
#define CONTRASENA 2
#define INICIAR_PARTIDA 3
#define JUGAR 4
#define FILAS 6
#define COLUMNAS 7
#define ESPACIO_VACIO ' '
#define FILA_NO_ENCONTRADA -1
#define ERROR_FILA_INVALIDA 4
#define ERROR_COLUMNA_LLENA 2
#define ERROR_NINGUNO 3
#define CONECTA 4
#define CONECTA_ARRIBA 1
#define CONECTA_DERECHA 2
#define CONECTA_ABAJO_DERECHA 3
#define CONECTA_ARRIBA_DERECHA 4
#define NO_CONECTA 0
#define CADENA_TABLERO 119

/*
Declaramos dos estructuras para almacenar los datos de los jugadores y de las partidas y relacionarlos entre ellos.
*/ 
struct partida{
	int estado;
	int jugadores[2];
	int turno;
};

struct jugador{
	int estado;
	char usuario[300];
	int partida;
	int puntuacion;
};

struct partida partidas[10]; //Vector donde se almacenan las partias (10 máx).
struct jugador jugadores[50]; //Vector donde se almacenan los jugadores (50 máx).

//Variables de tipo FILE * para leer y escribir en los ficheros.
FILE * ficheroDatos;
//FILE * ficheroRefranes;

/****************
 * 
 * 
 * FUNCIONES
 * 
 * 
*****************/

//Inicializa los vectores de jugadores y de partidas.
void inicializarVectores(){

	for (int i=0; i<50; ++i){

		strcpy(jugadores[i].usuario, "");

		jugadores[i].estado = USUARIO;
		jugadores[i].partida = -1;
		jugadores[i].puntuacion = 0;

	}

	for (int i=0; i<10; ++i){

		partidas[i].estado = OFF;
		partidas[i].jugadores[0] = -1;
		partidas[i].jugadores[1] = -1;
		partidas[i].turno = 0;

	}
}

//Comprueba si el usuario introducido está almacenado en el fichero.
int comprobarUsuario(char * user){

	strtok(user, "\n");

    char aux[100];

    ficheroDatos = fopen("datos.txt","r");

    if(ficheroDatos == NULL){
        return 0;
    }
    else{
        while(fgets(aux, 100, ficheroDatos)){
            strtok(aux, " ");
            if(strcmp(aux, user) == 0){
            	fclose(ficheroDatos);
                return 1;
            }
        }
    }
    fclose(ficheroDatos);
    return 0;
}

//Comprueba si la contraseña introducida se corresponde con el usuario introducido.
int comprobarContrasena(char * name, char * password){

	ficheroDatos = fopen("datos.txt","r");

	if(ficheroDatos == NULL){
		return 0;
	}
	else{
		char user[300];
		char pw[300];

		while(fscanf(ficheroDatos, "%s %s", user, pw) == 2){

			if(strcmp(user, name) == 0 && strcmp(pw, password) == 0){
				fclose(ficheroDatos);
				return 1;
			}
		}
		fclose(ficheroDatos);
		return 0;
	}
}

//Se inserta un nuevo usuario en el fichero.
int anadirUsuario(char * name, char * password){

	ficheroDatos = fopen("datos.txt","a");

	if(ficheroDatos == NULL){
		return 0;
	}
	else{
		fprintf(ficheroDatos, "%s %s\n", name, password);
		fclose(ficheroDatos);
		return 1;
	}
}

//Cuando un jugador se conecta, lo añade al vector de jugadores.
void nuevoJugador(int i){

	strcpy(jugadores[i].usuario, "");
	jugadores[i].estado = USUARIO;
	jugadores[i].puntuacion = 0;

}


//Se mete al jugador con el socket pasado como argumento en una partida. Retorna el identificador de esa partida.
int encontrarPartida(int descriptor_socket){
	/*
	Se recorre el vector de partidas para comprobar si hay algún jugador esperando en alguna. Si es así, mete en la partida al nuevo jugador y 
	retorna el número de la partida en la que se encuentra.
	*/
	for (int i = 0; i < 10; ++i){

		if(partidas[i].estado == OFF && partidas[i].jugadores[0] != -1){

			partidas[i].jugadores[1] = descriptor_socket;
			partidas[i].turno = 0;
			partidas[i].estado = ON;

			jugadores[descriptor_socket].partida = i;

			return i;
		}
	}

	/*
	Se recorre el vector de partidas y se pone en espera al jugador en una partida hasta que otro quiera jugar.
	*/
	for (int i = 0; i < 10; ++i){

		if(partidas[i].estado == OFF && partidas[i].jugadores[0] == -1){

			partidas[i].jugadores[0] = descriptor_socket;

			jugadores[descriptor_socket].partida = i;

			return i;
		}
	}

	//Si no hay partidas disponibles devuelve -1
	return -1;
}

//nuestro

void tableroVacio(char tablero[FILAS][COLUMNAS]){
    for(int f=0; f<FILAS; f++){
        for(int c=0; c<COLUMNAS; c++){
            tablero[f][c] = ' ';
        }
    }
}

//Obtiene la fila de la columna elegida en la que se quedará la pieza
int obtenerFilaDesocupada(int columna, char tablero[FILAS][COLUMNAS]) {
    for (int i = FILAS - 1; i >= 0; i--) {
        if (tablero[i][columna] == ESPACIO_VACIO) {
            return i;
        }
    }
    return FILA_NO_ENCONTRADA;
}

//Se le dice jugador (o si es la pieza X o O) y columna en la que queremos poner la pieza
int colocarPieza(int jugador, int columna, char tablero[FILAS][COLUMNAS]) {
    int fila = obtenerFilaDesocupada(columna, tablero);
    char pieza;
    if (fila == FILA_NO_ENCONTRADA) {
        return ERROR_COLUMNA_LLENA;
    }
    //Si jugador=0 se usa pieza 'x' si no 'o', el jugador será 0 o 1 dependiendo del turno
    if (jugador==0){
        tablero[fila][columna] = 'x';
	}
    else{
        tablero[fila][columna] = 'o';
    }
    return ERROR_NINGUNO;
}

//Funcion dibuja el tablero
void dibujarTablero(char tablero[FILAS][COLUMNAS], char cadenatablero[CADENA_TABLERO]) {
    strcpy(cadenatablero, "|1|2|3|4|5|6|7|\n| | | | | | | |\n| | | | | | | |\n| | | | | | | |\n| | | | | | | |\n| | | | | | | |\n| | | | | | | |\n");
    
    for (int i = 0; i < FILAS; ++i) {
        for (int j = 0; j < COLUMNAS; ++j) {
            cadenatablero[16*i+2*j+17]=tablero[i][j];
        }
    }
}


//Funcion que nos imporfa si es empate
int esEmpate(char tablero[FILAS][COLUMNAS]) {
    for (int i=0; i < COLUMNAS; ++i) {
        int resultado = obtenerFilaDesocupada(i, tablero);
        if (resultado != FILA_NO_ENCONTRADA) {
            return 0;
        }
    }
    return 1;
}




int ganador(char jugador, char tablero[FILAS][COLUMNAS]) {

    for(int f=3; f<FILAS; f++){
        for(int c=0; c<COLUMNAS; c++){
            if(tablero[f][c]=='x' && tablero[f-1][c]=='x' && tablero[f-2][c]=='x' && tablero[f-3][c]=='x'){
            return 1;
            }
            if(tablero[f][c]=='o' && tablero[f-1][c]=='o' && tablero[f-2][c]=='o' && tablero[f-3][c]=='o'){
            return 1;
            }
        }
    }

    for(int f=0; f<FILAS; f++){
        for(int c=3; c<COLUMNAS; c++){
            if(tablero[f][c]=='x' && tablero[f][c-1]=='x' && tablero[f][c-2]=='x' && tablero[f][c-3]=='x'){
            return 1;
            }
            if(tablero[f][c]=='o' && tablero[f][c-1]=='o' && tablero[f][c-2]=='o' && tablero[f][c-3]=='o'){
            return 1;
            }
        }
    }

    for(int f=3; f<FILAS; f++){
        for(int c=0; c<(COLUMNAS-3); c++){
            if(tablero[f][c]=='x' && tablero[f-1][c+1]=='x' && tablero[f-2][c+2]=='x' && tablero[f-3][c+3]=='x'){
            return 1;
            }
            if(tablero[f][c]=='o' && tablero[f-1][c+1]=='o' && tablero[f-2][c+2]=='o' && tablero[f-3][c+3]=='o'){
            return 1;
            }
        }
    }

    for(int f=3; f<FILAS; f++){
        for(int c=3; c<COLUMNAS; c++){
            if(tablero[f][c]=='x' && tablero[f-1][c-1]=='x' && tablero[f-2][c-2]=='x' && tablero[f-3][c-3]=='x'){
            return 1;
            }
            if(tablero[f][c]=='o' && tablero[f-1][c-1]=='o' && tablero[f-2][c-2]=='o' && tablero[f-3][c-3]=='o'){
            return 1;
            }
        }
    }

    return 0;
}
