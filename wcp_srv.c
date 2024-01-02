/* Dieunel Marcelin
   Je déclare qu'il s'agit de mon propre travail*/

/* fichiers de la bibliothèque standard */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
/* bibliothèque standard unix */
#include <unistd.h> /* close, read, write */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
/* spécifique à internet */
#include <arpa/inet.h> /* inet_pton */
/* spécifique aux comptines */
#include "comptine_utils.h"
#include "comptine_utils.c"

#define PORT_WCP 1234
#define BUFF_SIZE 256

void usage(char *nom_prog)
{
	fprintf(stderr, "Usage: %s repertoire_comptines\n"
			"serveur pour WCP (Wikicomptine Protocol)\n"
			"Exemple: %s comptines\n", nom_prog, nom_prog);
}
/** Retourne en cas de succès le descripteur de fichier d'une socket d'écoute
 *  attachée au port port et à toutes les adresses locales. */
int creer_configurer_sock_ecoute(uint16_t port);

/** Écrit dans le fichier de desripteur fd la liste des comptines présents dans
 *  le catalogue c comme spécifié par le protocole WCP, c'est-à-dire sous la
 *  forme de plusieurs lignes terminées par '\n' :
 *  chaque ligne commence par le numéro de la comptine (son indice dans le
 *  catalogue) commençant à 0, écrit en décimal, sur 6 caractères
 *  suivi d'un espace
 *  puis du titre de la comptine
 *  une ligne vide termine le message */
void envoyer_liste(int fd, struct catalogue *c);

/** Lit dans fd un entier sur 2 octets écrit en network byte order
 *  retourne : cet entier en boutisme machine. */
uint16_t recevoir_num_comptine(int fd);

/** Lit dans fd un entier sur 2 octets écrit en network byte order
 *  retourne : cet entier en boutisme machine. */
uint16_t recevoir_requete_client(int fd);

/** Écrit dans fd la comptine numéro ic du catalogue c comme spécifié par le
 *  protocole WCP, c'est-à-dire :
 *  chaque ligne du fichier de comptine est écrite avec son '\n' final, y
 *  compris son titre
 *  la comptine se trouve dans le répertoire dir_name
 *  deux lignes vides terminent le message */
void envoyer_comptine(int fd, const char *dir_name, struct catalogue *c, uint16_t ic);

/** pointeur de fonction qui servira chaque client après 
*   l'étape de la première connection avec le serveur
*   il prend void* en argument, on utilisera le plus souvent une 
*   vaiable de type struct client qui contient toutes les informations
*   nécessaires permettant de dialoguer avec le client (adresse, port,
*   socket, famille d'adresse, etc.)*/
void *thread_WCP(void *arg);

/** Ecrit dans buff le nom du fichier comtptine
*   contenu dans dans le descripteur fd. L'extension est ".cpt". 
*   Ce détail est assuré par le 
*   protocole du coté client. Le titre ne fait pas plus de 256 otets.*/
void recevoir_nom_comptine(int fd, char *buff);

/** envoie_confirmation_existence_fichier
*   envoie 1 dans le fichier de descripteur fd
*   si le nom de comptine envoyé par le client (contenu dans nom_fichier)
*   existe déjà
*   l'ensemble des fichiers se trouve dans le répertoire dir_name
*   0 sinon */
int ecef(int fd, const char *dir_name, const char *nom_fichier);

/** enregistrer un nom de comptine contenu dans name
*   dans le repertoire dir_name*/
void save_name_comp(const char *dir_name, const char *name);

/** recevoir le texte d'un comptine dans un socket (fd)
*   et le place dans le fichier 'name' du repertoire
*   'dir_name'*/
void add_comptine(int fd, const char *dir_name, const char *name);


/** Efface la comptine d'indice num du catalogue dans le repertoire dir_name
*   renvoie 0 en cas de succès et -1 si échec */
int remove_comptine(struct catalogue *c, const char *dir_name, uint16_t num);

/** structure qui représente un client par le numéro
*   de socket qu'on échange avec lui et le catalogue à jour
*/
struct client{
	int socket_client;
	struct catalogue *ctlg;
	const char *dir_name;
};

int main(int argc, char *argv[])
{
	if (argc != 2) {
		usage(argv[0]);
		return 1;
	}

	int sock = creer_configurer_sock_ecoute(PORT_WCP);
	struct catalogue *c = creer_catalogue(argv[1]);

	for(;;){
		struct sockaddr_in sa_client;
		socklen_t sl = sizeof(sa_client);

		int sock_cl = accept(sock, (struct sockaddr*) &sa_client, &sl);
		if(sock_cl < 0){
			perror("Accept");
			exit(0);
		}

		struct client *cl = malloc(sizeof(struct client));
		pthread_t th;

		cl->socket_client = sock_cl;
		cl->ctlg = c;
		cl->dir_name = argv[1];

		if(pthread_create(&th, NULL, thread_WCP, cl) < 0){
			perror("pthread_create");
			exit(3);
		}

		pthread_detach(th);

	}

	close(sock);
	free(c);

	return 0;
}

void *thread_WCP(void *arg){

	struct client *cl = arg;
	int sock_cl = cl->socket_client;
	struct catalogue *c = cl->ctlg;
	const char *dir_name = cl->dir_name;



	/* r pour stcoker ce que le client veut faire au tout début
	*choix_client pour le socker le numéro de la comptine à afficher*/
	uint16_t r, choix_client;
	char buff[BUFF_SIZE];

	for(;;){

		r = recevoir_requete_client(sock_cl);

		switch (r) {

			case 1 :
				envoyer_liste(sock_cl, c);

				choix_client = recevoir_num_comptine(sock_cl);

				envoyer_comptine(sock_cl, dir_name, c, choix_client);

				break;

			case 2 :

				for(;;){

					recevoir_nom_comptine(sock_cl, buff);
					if(ecef(sock_cl, dir_name, buff) == 1){
						continue;
					}
					else{
						save_name_comp(dir_name, buff);
						break;
					}
				}

				add_comptine(sock_cl, dir_name, buff);

				update_catalogue(dir_name, c);
				break;

			case 3 :
				envoyer_liste(sock_cl, c);

				choix_client = recevoir_num_comptine(sock_cl);

				sleep(3);
				remove_comptine(c, dir_name, choix_client);
				update_catalogue(dir_name, c);
				break;

			default :
				close(sock_cl);
				liberer_catalogue(cl->ctlg);
				free(cl);
				return NULL;

		}

		continue;

	}

	close(sock_cl);
	liberer_catalogue(cl->ctlg);

	free(cl);

	return NULL;
}

int creer_configurer_sock_ecoute(uint16_t port)
{
	//création de la socket d'écoute
	int sock_listen = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_listen < 0){
		perror("socket d'écoute");
		exit(3);
	}

	//création de la sockaddr locale
	struct sockaddr_in sa = { .sin_family = AF_INET,
				  .sin_port = htons(port),
				  .sin_addr.s_addr = htonl(INADDR_ANY)};
				  
	socklen_t sl = sizeof(sa);
	int opt = 1;

	setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

	/*attachement de la socket d'écoute au port donné
	*et à toutes les addresses */
	if(bind(sock_listen, (struct sockaddr*) &sa, sl) < 0){
		perror("bind socket d'écoute");
		exit(3);
	}

	//configuration de la socket comme socket d'écoute
	if(listen(sock_listen, 128) < 0){
		perror("listen socket d'écoute");
		exit(2);
	}

	return sock_listen;
}

void envoyer_liste(int fd, struct catalogue *c)
{
	for(int i = 0; i < c->nb; i++){
		dprintf(fd, "%6d %s", i, c->tab[i]->titre);
	}

	dprintf(fd, "\n");
	return;
}

uint16_t recevoir_num_comptine(int fd)
{
	uint16_t num[1];

	if(read(fd, num, sizeof(uint16_t)) != sizeof(uint16_t)){
		perror("Recéption numéro de comptine");
		exit(2);
	}

	*num = ntohs(*num);
	return *num;
}

void envoyer_comptine(int fd, const char *dir_name, struct catalogue *c, uint16_t ic)
{
	int fd_ic;
	char *path = malloc(BUFF_SIZE*sizeof(char));
	strcpy(path, dir_name);
	strcat(path, "/");
	strcat(path, c->tab[ic]->nom_fichier);

	if((fd_ic = open(path, O_RDONLY)) < 0){
		perror("open in envoyer_comptine");
		exit(0);
	}

	char buff[BUFF_SIZE];

	size_t n;

	while((n = read(fd_ic, buff, 256)) > 0){

		write(fd, buff, n);

		if(n < 256) break;
	}

	/** ajout de 2 lignes vides pour spécifier la fin
	*   de la comptine */
	write(fd, "\r\n\r\n", 4);


	return;
}

uint16_t recevoir_requete_client(int fd){

	uint16_t num[1];

	if(read(fd, num, sizeof(uint16_t)) != sizeof(uint16_t)){
		perror("Recéption requete client");
		exit(2);
	}

	*num = ntohs(*num);

	return *num;
}

void recevoir_nom_comptine(int fd, char *buff){

	int n;
	n = read(fd, buff, 255);
	if((n < 0) || (n > 255)){
		perror("read recevoir_nom_comptine");
		exit(3);
	}

	return; 
}

int ecef(int fd, const char *dir_name, const char *nom_fichier){

	uint16_t n;
	int m;
	if(is_folder_exist(dir_name, nom_fichier) == 1){
		n = 1;
		m = 1;
	}
	else{
		n = 0;
		m = 0;
	}

	n = htons(n);
	write(fd, &n, sizeof(uint16_t));

	return m;
}

void save_name_comp(const char *dir_name, const char *name){

	char *path = malloc(sizeof(strlen(dir_name) + strlen(name) + 2));
	if(path == NULL){
		perror("malloc path in save_name_comp");
		exit(3);
	}

	int fd;

	strcpy(path, dir_name);
	strcat(path, "/");
	strcat(path, name);

	if((fd = open(path, O_RDWR | O_CREAT, 0660)) < 0){
		perror("open in save_name_comp");
		exit(3);
	}

	close(fd);
	free(path);
	return;
}

void add_comptine(int fd, const char *dir_name, const char *name){

	int nb_octet;

	char *path = malloc(sizeof(strlen(dir_name) + strlen(name) + 2));
	if(path == NULL){
		perror("malloc path in add_nom_comptine");
		exit(3);
	}

	strcpy(path, dir_name);
	strcat(path, "/");
	strcat(path, name);

	int f_name = open(path, O_RDWR);

	if(f_name < 0){
		perror("open in add_comptine");
		exit(3);
	}

	for(;;){
		char *buf = malloc(BUFF_SIZE);
		if((nb_octet = read(fd, buf, 256)) <= 0){
			break;
		}

		//si la chaine contient deux lignes vides
		if(strstr(buf, "\r\n\r\n") != NULL){
			write(f_name, buf, nb_octet - 4);
			break;
		}

		write(f_name, buf, nb_octet);
		free(buf);
	}
	write(f_name, "\n", 1);

	free(path);

	return;
}



int remove_comptine(struct catalogue *c, const char *dir_name, uint16_t num){
	int status;
	char *path = malloc(sizeof(dir_name) + sizeof(c->tab[num]->nom_fichier + 2));

	strcpy(path, dir_name);
	strcat(path, c->tab[num]->nom_fichier);
	status = remove(path);
	return status;
}
