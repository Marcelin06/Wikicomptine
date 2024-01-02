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
#include <sys/types.h>
#include <sys/socket.h>
/* spécifique à internet */
#include <arpa/inet.h> /* inet_pton */
/* spécifique aux comptines */
#include "comptine_utils.h"
#include "comptine_utils.c"

#define PORT_WCP 1234
#define BUFF_SIZE 256

void usage(char *nom_prog)
{
	fprintf(stderr, "Usage: %s addr_ipv4\n"
			"client pour WCP (Wikicomptine Protocol)\n"
			"Exemple: %s 208.97.177.124\n", nom_prog, nom_prog);
}

/** Retourne (en cas de succès) le descripteur de fichier d'une socket
 *  TCP/IPv4 connectée au processus écoutant sur port sur la machine d'adresse
 *  addr_ipv4 */
int creer_connecter_sock(char *addr_ipv4, uint16_t port);

/** Lit la liste numérotée des comptines dans le descripteur fd et les affiche
 *  sur le terminal.
 *  retourne : le nombre de comptines disponibles */
uint16_t recevoir_liste_comptines(int fd);

/** Demande à l'utilisateur un nombre entre 0 (compris) et 
*   nb_comptines (non compris)
*   et retourne la valeur saisie sur 2 octets suivant WCP. */
uint16_t saisir_num_comptine(uint16_t nb_comptines);

/** Demande à l'utilisateur un nombre entre 0 et 3 
 *  et retourne la valeur saisie pour déterminer le choix de sa requete. */
uint16_t saisir_requete_client();

/** Écrit l'entier nc dans le fichier de descripteur fd en network byte order
*   nc étant l'indice d'une comptine dans le tableau du catalogue */
void envoyer_num_comptine(int fd, uint16_t nc);

/** Écrit l'entier r dans le fichier de descripteur fd en network byte order */
void envoyer_requete_client(int fd, uint16_t r);

/** Lit le texte d'une comptine dans le descripteur fd
*   et affiche la comptine sur le terminal */
void afficher_comptine(int fd);

// déssine une petite en-tete dans l'interface client
void en_tete();

/** Envoie un message de confirmation d'une requete de déconnexion
*   par l'utilisateur
*   et ferme le fichier de descripteur fd */
void au_revoir(int fd);

/** envoie un message de déconnexion lorque l'utilisateur
*   sasit une valeur innatendue 
*   et ferme le fichier de descripteur fd */
void au_revoir2(int fd);

/** Lit sur l'entrée standard le titre (255 octets max)
*   et chaque ligne de la comptine (255 octets)
*   et les copie dans le fichier de descripteur fd
*   La sasie du caractère '#' puis ENTRER met fin à la saisie.
*   A la fin deux lignes vides sont ajoutées pour préciser au serveur
*   la fin de la comptine pour qu'il puisse arreter de recopier, 
*   comme le demande le protocole WCP */
void ajouter_comptine(int fd);

/** Lit au clavier le nom d'un fichier comptine à ajouter
*   sans l'extension 'cpt' sur 251 octets (max) puis ajoute l'extension
*   '.cpt' pour former le nom au bon format et écrit
*   ce nom de fichier dans le descripteur de fichier fd
*   et aussi dans la chaine 'dest' */
void envoyer_nom_comptine(int fd, char *dest);

/** reception_confirmation_existence_fichier
*   Lit en network byte order dans le fichier de descripteur fd 
*   la confirmation de l'existence
*   d'un nom de fichier comptine.
*   reçoit 1 si le nom saisi existe déjà, 0 sinon
*   et renvoie le nombre reçu en boutiste machine */
uint16_t rcef(int fd);

int main(int argc, char *argv[])
{
	if (argc != 2) {
		usage(argv[0]);
		return 1;
	}

	int sock = creer_connecter_sock(argv[1], PORT_WCP);

	char buff[BUFF_SIZE];
	for(;;){
		en_tete();
		uint16_t nc; // nombre de comptines
		uint16_t choix;
		int num_requete = saisir_requete_client();


		switch (num_requete) {

			case 1:
				en_tete();
				envoyer_requete_client(sock, 1);
				nc = recevoir_liste_comptines(sock); 

				printf("\nIl y a %d fichiers de comptines\n", nc);

				printf("\nQuelle comptine voulez-vous lire ?");

				choix = saisir_num_comptine(nc);

				if((choix >= 0) && (choix < nc)){
					envoyer_num_comptine(sock, choix);

					en_tete();

					afficher_comptine(sock);

					printf("\nAppuyer sur :");
					printf("\n1 pour voir le menu");
					printf("\nautre pour quitter\n\n");
					printf("Choix : ");

					scanf(" %hd", &choix);

					switch (choix){
						case 1:
							continue;
						default:
							close(sock);
							return 0;
					}
					break;
				}
				else {
					au_revoir2(sock);
					return 0;
				}
			case 2 :
				envoyer_requete_client(sock, 2);
				for(;;){
					envoyer_nom_comptine(sock, buff);

					if(rcef(sock) == 1){
						printf("\nFichier exite déjà : %s\n", buff);
						printf("\nSaisir un autre nom\n");
						sleep(1.5);
						continue;
					}

					else{
						printf("\nNom enregistré : %s\n", buff);
						break;
					} 

				}
				ajouter_comptine(sock);
				break;

			case 3 :
				en_tete();
				envoyer_requete_client(sock, 3);
				nc = recevoir_liste_comptines(sock); 

				printf("\nIl y a %d fichiers de comptines\n", nc);

				printf("\nQuelle comptine voulez-vous supprimer?");

				choix = saisir_num_comptine(nc);

				if((choix >= 0) && (choix < nc)){
					envoyer_num_comptine(sock, choix);

					en_tete();

					printf("\nfichier supprimé");

					printf("\nAppuyer sur :");
					printf("\n1 pour voir le menu");
					printf("\nautre pour quitter\n\n");
					printf("Choix : ");

					scanf(" %hd", &choix);

					switch (choix){
						case 1:
							continue;
						default:
							close(sock);
							return 0;
					}
					break;
				}
				else {
					au_revoir2(sock);
					return 0;
				}

				break;

			case 0 :
				au_revoir(sock);
				return 0;

			default :
				au_revoir2(sock);
				return 0;
		}


	}

	close(sock);
	return 0;
}

int creer_connecter_sock(char *addr_ipv4, uint16_t port)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0){
		perror("appel à socket dans creer_conecter_sock");
		exit(0);
	}

	struct sockaddr_in sa = { .sin_family = AF_INET,
				  .sin_port = htons(port)
				  };
				  
	if(inet_pton(AF_INET, addr_ipv4, &sa.sin_addr) != 1){
	 	perror("addresse ipv4 invalide");
	 	exit(0);
	 }
	 
	socklen_t sl = sizeof(sa);

	if(connect(sock, (struct sockaddr *) &sa, sl) < 0){
		perror("Connect");
		exit(2);
	}
	 
	return sock;
}

uint16_t recevoir_liste_comptines(int fd)
{
	char buff[BUFF_SIZE];
	size_t n;
	uint16_t result = 0;
	while((n = read_until_nl(fd, buff)) > 0){
		write(1, buff, n);
		printf("\n");
		result++;
	}
	return result;
}

uint16_t saisir_num_comptine(uint16_t nb_comptines)
{
	uint16_t ic;
	printf("\nFaites un choix entre 0 et %d : ", nb_comptines - 1);
	if(scanf("%" SCNu16, &ic) < 1){
		perror("WCP requiert un entier non signé sur 2 octets");

		exit(3);
	}
	return ic;
}

void envoyer_num_comptine(int fd, uint16_t nc)
{
	nc = htons(nc);
	if(write(fd, &nc, sizeof(uint16_t)) != sizeof(uint16_t)){
		perror("Erreur d'écriture dans le fichier pour envoyer le numéro de comptine");
		exit(0);
	}
	return;
}

void afficher_comptine(int fd)
{
	char buff[BUFF_SIZE];
	size_t n;
	write(1, "\n", 1);

	printf("\n\t\t\t******DÉBUT DE COMPTINE*****\n\n");
	while((n = read(fd, buff, BUFF_SIZE)) > 0){

		//si la chaine contient deux lignes vides
		if(strstr(buff, "\r\n\r\n") != NULL){
			write(1, buff, n - 4);
			break;
		}
		write(1, buff, n);
	}

	printf("\n\t\t\t******FIN DE COMPTINE*****");
	return;

}

void au_revoir(int fd){
	printf("\nVous etes bien déconnecté(e) du serveur WIKICOMPTINES.\nA Bientot !!!\n");
	close(fd);
	return;
}

void au_revoir2(int fd){
	printf("\nOups !!! Une valeur inattendue a été saisie, par conséquent, la connection avec le serveur a été perdue.\n");
	close(fd);
	return;
}

uint16_t saisir_requete_client(){
	uint16_t num_requete;
	printf("Appuyer sur :\n1 Pour afficher la liste des comptines");
	printf("\n2 Pour ajouter une nouvelle comptine");
	printf("\n3 Pour supprimer une comptine");
	printf("\n0 pour quitter\n\n");
	printf("\nChoix [0-3] : ");

	if(scanf("%" SCNu16, &num_requete) < 1){
		perror("WCP requiert un entier non signé sur 2 octets");

		exit(3);
	}
	return num_requete;
}

void envoyer_requete_client(int fd, uint16_t r){
	r = htons(r);
	if(write(fd, &r, sizeof(uint16_t)) != sizeof(uint16_t)){
		perror("Erreur d'écriture dans le fichier pour envoyer la requete du client");
		exit(0);
	}
	return;
}

void en_tete(){
	system("clear");
	printf("\n\t\t********************************************************\n");
	printf("\n\t\t\tSERVEUR WIKICOMPTINES - BIENVENUE A VOUS\n");
	printf("\n\t\t******************************************************\n\n");
	return;

}

void ajouter_comptine(int fd){

	char buff[BUFF_SIZE];
	int nb_octet = 0;


	printf("\nSaisir le titre (%d caractères au plus) : \n", BUFF_SIZE - 1);

	if((nb_octet = read(0, buff, 255)) < 0){
		perror("read in ajouter_comptine");
		exit(3);
	}

	write(fd, buff, nb_octet);
	write(fd, "\n", 1);


	printf("\nTexte du comptine (# puis ENTRER pour terminer) :\n");

	for(;;){
		char buf[BUFF_SIZE];
		fflush(stdin);
		nb_octet = read(0, buf, 255);


		if(buf[nb_octet - 2] == '#'){

			write(fd, buf, nb_octet - 2);
			write(fd, "\r\n\r\n", 4);

			break;
		}

		write(fd, buf, nb_octet);

	}
	printf("\nPATIENTEZ ...\n");
	sleep(2);
	printf("\nCOMPTINE AJOUTÉE AVEC SUCCES\n");
	sleep(2);

	return;


}

void envoyer_nom_comptine(int fd, char *dest){

	char buff[BUFF_SIZE];

	en_tete();
	printf("\nAJOUT DE COMPTINES\n");
	printf("\nnom de la comptine sans extension .cpt : ");

	scanf(" %251s", buff);

	strcat(buff, ".cpt");
	write(fd, buff, strlen(buff));
	strcpy(dest, buff);
	return;
}

uint16_t rcef(int fd){
	uint16_t num[1];

	if(read(fd, num, sizeof(uint16_t)) != sizeof(uint16_t)){
		perror("Recéption confirmation rcef");
		exit(2);
	}

	*num = ntohs(*num);

	return *num;
}


