#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include<signal.h>
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

/*
Declaramos dos estructuras para almacenar los datos de los jugadores y de las partidas y relacionarlos entre ellos.
*/ 
struct partida{
	int estado;
	int jugadores[2];
	int turno;
	char refran[250];
	char panel[250];
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

//Cargamos los refranes en el vector de refranes.
void extraerRefranes(char refranes[][250]){

	FILE * ficheroRefranes = fopen("listadoRefranes.txt","r");

	if(ficheroRefranes != NULL){

		char refran[250];
		int cont = 0;

		while (fgets(refran, 250, ficheroRefranes)){

			refran[strlen(refran) - 1] = '\0';
			strcpy(refranes[cont], refran);
			cont ++;

		}
		fclose(ficheroRefranes);
	}
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

//Devuelve las veces que aparece una vocal en el refrán, o -1 en caso de que no sea una vocal.
int comprobarVocal(int partida, char vocal){
	int nveces = 0;

	if(tolower(vocal)=='a' || tolower(vocal)=='e' || tolower(vocal)=='i' || tolower(vocal)=='o' || tolower(vocal)=='u'){

		for (int i = 0; i < strlen(partidas[partida].refran); ++i){

			if(tolower(partidas[partida].refran[i]) == tolower(vocal)){

				partidas[partida].panel[i] = partidas[partida].refran[i];
				nveces++;
			}
		}
	}
	else{
		nveces = -1;
	}
	return nveces;
}

//Devuelve las veces que aparece una consonante en el refrán, o -1 en caso de que no sea una consonante.
int comprobarConsonante(int partida, char consonante){
	int n = 0;

	if(tolower(consonante)!='a' && tolower(consonante)!='e' && tolower(consonante)!='i' && tolower(consonante)!='o' && tolower(consonante)!='u'){

		for (int i = 0; i < strlen(partidas[partida].refran); ++i){

			if(tolower(partidas[partida].refran[i]) == tolower(consonante)){

				partidas[partida].panel[i] = partidas[partida].refran[i];
				n++;
			}
		}
	}
	else{
		n = -1;
	}

	return n;
}

//Comprueba si la frase introducida se corresponde con la solución (el refrán).
int resolverPanel(char * refran, char * solution){

	for (int i = 0; i < strlen(refran); ++i){

		if(tolower(refran[i]) != tolower(solution[i])){

			return 0;

		}
	}
	return 1;
	
}

//Comprueba si se ha terminado la partida con la última consonante o vocal introducida.
int comprobarFinalPartida(int partida){

	return strcmp(partidas[partida].refran, partidas[partida].panel) == 0;
}