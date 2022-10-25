#include "server.h"


#define MSG_SIZE 250
#define MAX_CLIENTS 30


/*
 * El servidor ofrece el servicio de un chat
 */

void manejador(int signum);
void salirCliente(int socket, fd_set * readfds, int * numClientes, int arrayClientes[]);


int main ( )
{
   

	/*---------------------------------------------------- 
		Descriptor del socket y buffer de datos                
	-----------------------------------------------------*/
	int sd, new_sd;
	struct sockaddr_in sockname, from;
	char buffer[MSG_SIZE];
    char bufferAux[MSG_SIZE];
	socklen_t from_len;
    fd_set readfds, auxfds;
   	int salida;
   	int arrayClientes[MAX_CLIENTS];
    int numClientes = 0;

   	//contadores
    int i,j,k;
	int recibidos;
   	char identificador[MSG_SIZE];
    int on, ret;

    inicializarVectores();

    char tablero[FILAS][COLUMNAS];
    tableroVacio(tablero);
    char cadenatablero[CADENA_TABLERO];

    srand(time(NULL));
    
	/* --------------------------------------------------
		Se abre el socket 
	---------------------------------------------------*/
  	sd = socket (AF_INET, SOCK_STREAM, 0); // SOCK_STREAM pq trabajamos con TCP
	if (sd == -1){
		perror("No se puede abrir el socket cliente\n");
    		exit (1);	
	}
    
    // Activaremos una propiedad del socket para permitir· que otros
    // sockets puedan reutilizar cualquier puerto al que nos enlacemos.
    // Esto permite· en protocolos como el TCP, poder ejecutar un
    // mismo programa varias veces seguidas y enlazarlo siempre al
    // mismo puerto. De lo contrario habrÌa que esperar a que el puerto
    // quedase disponible (TIME_WAIT en el caso de TCP)
    on=1;
    ret = setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));



	sockname.sin_family = AF_INET;
	sockname.sin_port = htons(2060);
	sockname.sin_addr.s_addr =  INADDR_ANY;

	if (bind (sd, (struct sockaddr *) &sockname, sizeof (sockname)) == -1){
		perror("Error en la operación bind");
		exit(1);
	}
	

   	/*---------------------------------------------------------------------
		Del las peticiones que vamos a aceptar sólo necesitamos el 
		tamaño de su estructura, el resto de información (familia, puerto, 
		ip), nos la proporcionará el método que recibe las peticiones.
   	----------------------------------------------------------------------*/
	from_len = sizeof (from);


	if(listen(sd,1) == -1){ //descriptor de socket y cola
		perror("Error en la operación de listen");
		exit(1);
	}
    
	//Inicializar los conjuntos fd_set
    FD_ZERO(&readfds);
    FD_ZERO(&auxfds);
    FD_SET(sd,&readfds);
    FD_SET(0,&readfds);


    //Capturamos la señal SIGINT (Ctrl+c)
    signal(SIGINT,manejador);
    
	/*-----------------------------------------------------------------------
		El servidor acepta una petición
	------------------------------------------------------------------------ */
		while(1){
            
            //Esperamos recibir mensajes de los clientes (nuevas conexiones o mensajes de los clientes ya conectados)
            
            auxfds = readfds;
            
            salida = select(FD_SETSIZE,&auxfds,NULL,NULL,NULL);
            
            if(salida > 0){
                
                
                for(i=0; i<FD_SETSIZE; i++){
                    
                    //Buscamos el socket por el que se ha establecido la comunicación
                    if(FD_ISSET(i, &auxfds)){
                        
                        if( i == sd){
                            
                            if((new_sd = accept(sd, (struct sockaddr *)&from, &from_len)) == -1){ //devuelve nuevo descriptor(new_sd) para la conexion del cliente
                                perror("Error aceptando peticiones");
                            }
                            else{

                                if(numClientes < MAX_CLIENTS){
                                    arrayClientes[numClientes] = new_sd;
                                    numClientes++;
                                    FD_SET(new_sd,&readfds);
                                
                                    strcpy(buffer, "+Ok. Usuario conectado\n");
                                
                                    send(new_sd,buffer,sizeof(buffer),0);
                                
                                    nuevoJugador(new_sd);
                                }
                                else{ // si hay demasiados clientes conectados
                                    bzero(buffer,sizeof(buffer));
                                    strcpy(buffer,"Demasiados clientes conectados\n");
                                    send(new_sd,buffer,sizeof(buffer),0);
                                    close(new_sd);
                                }
                                
                            }
                            
                            
                        }
                        else if (i == 0){
                            //Se ha introducido información de teclado
                            bzero(buffer, sizeof(buffer));
                            fgets(buffer, sizeof(buffer),stdin);
                            
                            //Controlar si en el servidor se ha introducido "SALIR", cerrando todos los sockets y finalmente saliendo del servidor. (implementar)
                            if(strcmp(buffer,"SALIR\n") == 0){
                             
                                for (j = 0; j < numClientes; j++){
						            bzero(buffer, sizeof(buffer));
						            strcpy(buffer,"Desconexión servidor\n"); 
                                    send(arrayClientes[j],buffer , sizeof(buffer),0);
                                    close(arrayClientes[j]);
                                    FD_CLR(arrayClientes[j],&readfds);
                                }
                                    
                                close(sd);
                                exit(-1);
                                
                            }
                            //Mensajes que se quieran mandar a los clientes (implementar)
                            
                        } 
                        else{
                            bzero(buffer, sizeof(buffer));
                            
                            recibidos = recv(i, buffer, sizeof(buffer), 0); // recicbir mensajes de cliente
                            
                            if(recibidos > 0){
                                
                                //Comparamos si un cliente ha introducido SALIR.
                                if(strcmp(buffer, "SALIR\n") == 0){
                                    
                                    salirCliente(i, &readfds, &numClientes, arrayClientes);

                                    //Si el jugador que ha salido estaba jugando una partida, se informa al otro jugador de que ha abandonado.

                                    if(jugadores[i].estado == JUGAR){

                                        int npartida;

                                        npartida = jugadores[i].partida; //Guardamos en npartida el nº de partida en la que se encontraba jugando.

                                        partidas[npartida].estado = OFF; //Cambiamos el estado de la partida.

                                        int jugador2;

                                        jugador2 = partidas[npartida].jugadores[0];

                                        if(partidas[npartida].jugadores[0] == i){
                                            jugador2 = partidas[npartida].jugadores[1];
                                        }

                                        jugadores[jugador2].estado = INICIAR_PARTIDA;

                                        partidas[npartida].jugadores[0] = -1;
                                        partidas[npartida].jugadores[1] = -1;

                                        bzero(buffer, sizeof(buffer));
                                        strcpy(buffer, "+Ok. Ha salido el otro jugador. Finaliza la partida.\n");
                                        send(jugador2, buffer, sizeof(buffer), 0);
                                    }

                                    bzero(buffer, sizeof(buffer));
                                    strcpy(buffer, "+Ok. Desconexión procesada.\n");
                                    send(i, buffer, sizeof(buffer), 0);
                                }
                                //COMPARAMOS LO QUE SE HA INTRODUCIDO POR PANTALLA CON LAS CADENAS DE CARACTERES SIGUIENTES

                                //Se ha introducido por pantalla un usuario con la estructrua USUARIO "usuario"
                                else if(strncmp(buffer, "USUARIO", 7) == 0){

                                    //Comprueba si el usuario estaba ya logueado anteriormente

                                    if(jugadores[i].estado != USUARIO && jugadores[i].estado != CONTRASENA){

                                        bzero(buffer,sizeof(buffer));
                                        strcpy(buffer,"-Err. Para introducir usuario y contraseña no debe estar ya logueado.\n");
                                        send(i,buffer,sizeof(buffer),0);

                                    }
                                    else{

                                        char * user = buffer + 8; //Almacenamos en user el usuario introducido.
                                        user[strlen(user)-1] = '\0';

                                        if(comprobarUsuario(user) == 1){

                                            jugadores[i].estado = CONTRASENA;
                                            strcpy(jugadores[i].usuario, user);
                                            bzero(buffer, sizeof(buffer));
                                            strcpy(buffer, "+Ok. Usuario correcto.\n");
                                            send(i, buffer, sizeof(buffer), 0);

                                        }
                                        else{

                                            bzero(buffer, sizeof(buffer));
                                            strcpy(buffer, "-Err. Usuario incorrecto.\n");
                                            send(i, buffer, sizeof(buffer), 0);

                                        }
                                    }
                                }
                                //Se ha introducido por pantalla una contraseña con la estructrua PASSWORD "contraseña"
                                else if(strncmp(buffer, "PASSWORD", 8) == 0){

                                    if(jugadores[i].estado != CONTRASENA){

                                        bzero(buffer, sizeof(buffer));
                                        strcpy(buffer, "-Err. No ha introducido previamente un nombre de usuario correcto.\n");
                                        send(i, buffer, sizeof(buffer), 0);
                                    }
                                    else{

                                        bzero(bufferAux, sizeof(bufferAux));

                                        char password[100]; //Almacenamos en password la contraseña introducida
                                        sscanf(buffer, "%s %s", bufferAux, password);

                                        //Comprobamos si la contraseña introducida se corresponde con la contraseña del usuario introducido anteriormente
                                        if(comprobarContrasena(jugadores[i].usuario, password) == 1){

                                            //Si es así, cambiamos el estado del jugador a INICIAR_PARTIDA: ya está listo para jugar
                                            jugadores[i].estado = INICIAR_PARTIDA;
                                            bzero(buffer, sizeof(buffer));
                                            strcpy(buffer, "+Ok. Usuario validado.\n");
                                            send(i, buffer, sizeof(buffer), 0);
                                        }
                                        else{

                                            bzero(buffer, sizeof(buffer));
                                            strcpy(buffer, "-ERR. Error en la validación.\n");
                                            send(i, buffer, sizeof(buffer), 0);
                                        }
                                    }
                                }
                                //Se ha introducido por pantalla un usuario y una contraseña con la estructrua REGISTRO -u "usuario" -p "contraseña"
                                else if(strncmp(buffer, "REGISTRO", 8) == 0){
                                    
                                    bzero(bufferAux, sizeof(bufferAux));
                                    char usuario_[100]; //En esta variable almacenamos el usuario introducido
                                    char contrasena_[100]; // y en esta la contraseña
                                    sscanf(buffer, "%s -u %s -p %s", bufferAux, usuario_, contrasena_);

                                    //Comprobamos si se ha realizado correctamente
                                    if(anadirUsuario(usuario_, contrasena_) == 1){

                                        bzero(buffer, sizeof(buffer));
                                        strcpy(buffer,"+Ok. Registro correcto.\n");
                                        send(i, buffer, sizeof(buffer), 0);
                                    }
                                    else{

                                        bzero(buffer, sizeof(buffer));
                                        strcpy(buffer, "-Err. Error en el registro del nuevo jugador.\n");
                                        send(i, buffer, sizeof(buffer), 0);
                                    }
                                }
                                //Se ha introducido por pantalla la cadena INICIAR-PARTIDA
                                else if(strncmp(buffer, "INICIAR-PARTIDA", 15) == 0){
                                    //Comprobamos si el estado del jugador que lo ha introducido es INICIAR_PARTIDA, es decir, si se ha logueado
                                    //antes. De lo contrario, mostramos mensaje de error.
                                    if(jugadores[i].estado != INICIAR_PARTIDA){
                                        bzero(buffer, sizeof(buffer));
                                        strcpy(buffer, "-Err. El usuario debe estar logueado para poder iniciar una partida.\n");
                                        send(i, buffer, sizeof(buffer), 0);
                                    }

                                    else{
                                        /*
                                        Almacenamos en j el identificador de la partida en la que se ha metido al jugador.
                                        Si no hay partidas disponibles se muestra mensaje de error.
                                        */
                                        int id_partida = encontrarPartida(i);

                                        if(id_partida >= 0){
                                            /*
                                            Si en la partida en la que se encuentra ya había un jugador esperando, se comienza a jugar.
                                            */
                                            if(partidas[id_partida].estado == ON){
                                                
                                                char cadenaaux[] = "+Ok. Empieza la partida.\n\n--------------------\n| Juego 4 en línea |\n--------------------\n";
                                                dibujarTablero(tablero, &cadenatablero);
                                                strcat(cadenaaux, cadenatablero);

                                                bzero(buffer, sizeof(buffer));
                                                strcpy(buffer, cadenaaux);
                                                send(i, buffer, sizeof(buffer), 0);

                                                bzero(buffer, sizeof(buffer));
                                                strcpy(buffer, cadenaaux);
                                                send(partidas[id_partida].jugadores[0], buffer, sizeof(buffer),0);


                                                // Cambiamos el estado de ambos jugadores a JUGAR.
                                                jugadores[i].estado = JUGAR;
                                                jugadores[partidas[id_partida].jugadores[0]].estado = JUGAR;
                                            }

                                            else{

                                                bzero(buffer, sizeof(buffer));
                                                strcpy(buffer, "+Ok. Petición recibida. Quedamos a la espera de más jugadores.\n");
                                                send(i,buffer, sizeof(buffer), 0);
                                            }
                                        }

                                        else{

                                            bzero(buffer, sizeof(buffer));
                                            strcpy(buffer, "-Err. No hay partidas disponibles.\n");
                                            send(i,buffer, sizeof(buffer), 0);
                                        }
                                    }
                                }
                                //Se ha introducido por pantalla una columna.
                                else if(strncmp(buffer, "COLOCAR-FICHA", 13) == 0){
                                    
                                    int turno = partidas[jugadores[i].partida].turno;

                                    /*
                                    Comprobamos si es el turno del jugador que ha introducido la columna.
                                    */

                                    if(partidas[jugadores[i].partida].jugadores[turno] != i && jugadores[i].estado == JUGAR){

                                        bzero(buffer, sizeof(buffer));
                                        strcpy(buffer, "-Err. Debe esperar su turno.\n");
                                        send(i, buffer, sizeof(buffer),0);
                                    }

                                    /*
                                    Comprobamos si el jugador se encuentra en alguna partida.
                                    */

                                    else if(jugadores[i].estado == JUGAR){
                                        char columna = buffer[14];
                                        int columnaInt = columna -'0';
                                        columnaInt = columnaInt -1;

                                        /*
                                        Comprueba si es una columna.
                                        */
                                        

                                        if(columnaInt < 0 || columnaInt > 6){
                                            bzero(buffer, sizeof(buffer));
                                            sprintf(buffer, "-Err. El valor introducido no esta comprendido entre 1 y 7.\n");
                                            send(i, buffer, sizeof(buffer), 0);
                                        }
					                    else{
                                            
//aqui necesitamos un while para que se repita hasta que termine la partida

                                            //if si no hay sitio en la columna
                                            if(obtenerFilaDesocupada(columnaInt,tablero)==FILA_NO_ENCONTRADA){
                                                bzero(buffer, sizeof(buffer));
                                                sprintf(buffer, "--Err. Debe seleccionar otra columna que tenga alguna casilla disponible”.\n");
                                                send(i, buffer, sizeof(buffer), 0);
                                            }
                                            //else si hay sitio
                                            else{
                                                colocarPieza(partidas[jugadores[i].partida].turno, columnaInt, tablero );
                                                char cadenaaux[] = "--------------------\n| Juego 4 en línea |\n--------------------\n";
                                                dibujarTablero(tablero, &cadenatablero);
                                                strcat(cadenaaux, cadenatablero);
                                                    
                                                bzero(buffer, sizeof(buffer));
                                                strcpy(buffer, cadenaaux);
                                                send(partidas[jugadores[i].partida].jugadores[1], buffer, sizeof(buffer),0);

                                                bzero(buffer, sizeof(buffer));
                                                strcpy(buffer, cadenaaux);
                                                send(partidas[jugadores[i].partida].jugadores[0], buffer, sizeof(buffer),0);
                                                
                                                //if si gana la partida
                                                if(ganador(partidas[jugadores[i].partida].turno, tablero)==1){
                                                    
                                                    int jugador2;
                                                    jugador2 = partidas[jugadores[i].partida].jugadores[0];
                                                    // Se envia mensaje a ambos jugadores de que ha termiando la partida
                                                    bzero(buffer,sizeof(buffer));
                                                    sprintf(buffer, "+Ok. Jugador %s ha ganado la partida.\n",  jugadores[i].usuario);
                                                    send(i, buffer, sizeof(buffer), 0);


                                                    if(partidas[jugadores[i].partida].jugadores[0] == i){
                                                        jugador2 = partidas[jugadores[i].partida].jugadores[1];
                                                    }

                                                    bzero(buffer, sizeof(buffer));
                                                    sprintf(buffer, "+Ok. Jugador %s ha ganado la partida.\n",  jugadores[i].usuario);
                                                    send(jugador2, buffer, sizeof(buffer), 0);

                                                    //Termina la partida
                                                    partidas[jugadores[i].partida].estado = OFF;

                                                    partidas[jugadores[i].partida].jugadores[0] = -1;
                                                    partidas[jugadores[i].partida].jugadores[1] = -1;

                                                }
                                                //else if si no hay mas sitio en el tablero
                                                else if(esEmpate(tablero)==1){
                                                    int jugador2;
                                                    jugador2 = partidas[jugadores[i].partida].jugadores[0];

                                                    //Se envia que termina la partida por empate a ambos jugadores
                                                    bzero(buffer,sizeof(buffer));
                                                    sprintf(buffer, "+Ok. Se ha producido un empate.\n");
                                                    send(i, buffer, sizeof(buffer), 0);

                                                    if(partidas[jugadores[i].partida].jugadores[0] == i){
                                                        jugador2 = partidas[jugadores[i].partida].jugadores[1];
                                                    }

                                                    bzero(buffer, sizeof(buffer));
                                                    sprintf(buffer, "+Ok. Se ha producido un empate.\n");
                                                    send(jugador2, buffer, sizeof(buffer), 0);

                                                    //termina la partida
                                                    partidas[jugadores[i].partida].estado = OFF;

                                                    partidas[jugadores[i].partida].jugadores[0] = -1;
                                                    partidas[jugadores[i].partida].jugadores[1] = -1;
                                                }

                                                else{

                                                	if(partidas[jugadores[i].partida].turno == 0){
                                                    	partidas[jugadores[i].partida].turno = 1;
                                                        
                                                        bzero(buffer, sizeof(buffer));
                                                        strcpy(buffer, "+Ok. Es tu turno.\n");
                                                        send(partidas[jugadores[i].partida].jugadores[1], buffer, sizeof(buffer),0);
                                                	}
                                                	else{
                                                    	partidas[jugadores[i].partida].turno = 0;
                                                        
                                                        bzero(buffer, sizeof(buffer));
                                                        strcpy(buffer, "+Ok. Es tu turno.\n");
                                                        send(partidas[jugadores[i].partida].jugadores[0], buffer, sizeof(buffer),0);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                //El valor introducido es otro
                                else{
                                    bzero(buffer, sizeof(buffer));
                                    strcpy(buffer, "-Err. El mensaje introducido no se corresponde con ninguna de las opciones disponibles.\n");
                                    send(i, buffer, sizeof(buffer), 0);
                                }                                                                   
                            }
                            //Si el cliente introdujo ctrl+c
                            if(recibidos == 0){

                                printf("El socket %d, ha introducido ctrl+c\n", i);
                                //Eliminar ese socket
                                salirCliente(i,&readfds,&numClientes,arrayClientes);

                                //Si el jugador que ha salido estaba jugando una partida.
                                if(jugadores[i].estado == JUGAR){

                                    int idpartida = jugadores[i].partida;

                                    //Se cambia el estado de la partida.
                                    partidas[idpartida].estado = OFF;
                                    int jugador2 = partidas[idpartida].jugadores[0];

                                    if(partidas[idpartida].jugadores[0] == i){
                                        jugador2 = partidas[idpartida].jugadores[1];
                                    }

                                    jugadores[jugador2].estado = INICIAR_PARTIDA;
                                    partidas[idpartida].jugadores[0] = -1;
                                    partidas[idpartida].jugadores[1] = -1;

                                    bzero(buffer, sizeof(buffer));
                                    strcpy(buffer,"+Ok. Ha salido el otro jugador. Finaliza la partida.\n");
                                    send(jugador2, buffer, sizeof(buffer), 0);
                                }

                                bzero(buffer, sizeof(buffer));
                                strcpy(buffer, "+Ok. Desconexión procesada.\n");
                                send(i, buffer, sizeof(buffer), 0);

                            }
                        }
                    }
                }
            }
		}

	close(sd);
	return 0;
	
}

void salirCliente(int socket, fd_set * readfds, int * numClientes, int arrayClientes[]){
  
    char buffer[250];
    int j;
    
    close(socket);
    FD_CLR(socket,readfds);
    
    //Re-estructurar el array de clientes
    for (j = 0; j < (*numClientes) - 1; j++)
        if (arrayClientes[j] == socket)
            break;
    for (; j < (*numClientes) - 1; j++)
        (arrayClientes[j] = arrayClientes[j+1]);
    
    (*numClientes)--;
}


void manejador (int signum){
    printf("\nSe ha recibido la señal sigint\n");
    signal(SIGINT,manejador);
}
