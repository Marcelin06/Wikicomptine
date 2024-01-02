/* Dieunel Marcelin
   Je déclare qu'il s'agit de mon propre travail*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include "comptine_utils.h"

#define BUFF_SIZE 256

int read_until_nl(int fd, char *buf){

	//variable pour stocker le nombre d'octets lus sans '\n'
	ssize_t nb_octets = 0;


	char *a = malloc(sizeof(char*));
	if(a == NULL){
		perror("malloc in read_until_nl");
		exit(3);
	}
	while(read(fd, a, 1) > 0){
		if(*a == '\n'){
			buf[nb_octets] = *a;
			break;
		}
		buf[nb_octets++] = *a;
	}
	free(a);
	return nb_octets;
}

int est_nom_fichier_comptine(char *nom_fich)
{
	//recherche de la dernière position de '.' s'il existe
	char *lastPosDot = strrchr(nom_fich, '.');
	if(lastPosDot == NULL) return 0;

	//comparaison
	if(strcmp(lastPosDot, ".cpt") == 0) return 1;
	return 0;
}

struct comptine *init_cpt_depuis_fichier(const char *dir_name, const char *base_name)
{
	/*allocation dynamique de struct comptine et de ses champs
	*ils doivent etre tous libérés à l'intérieur de la fonction appelante
	*après utilisation
	*/
	struct comptine *result = malloc(sizeof(struct comptine *));
	if(result == NULL){
		perror("malloc result in init_cpt_depuis_fichier");
		return NULL;
	}

	char *nom_fichier = malloc(sizeof(strlen(base_name) + 1));
	if(nom_fichier == NULL){
		perror("malloc nom_fichier in init_cpt_depuis_fichier");
		return NULL;
	}

	char *titre = malloc(sizeof(char *)*BUFF_SIZE);
	if(titre == NULL){
		perror("malloc titre in init_cpt_depuis_fichier");
		return NULL;
	}

	/*variable pour stocker le chemin relatif du fichier base_name 
	*dans dir_name
	*/
	char *path = malloc(sizeof(strlen(dir_name) + strlen(base_name) + 2));
	if(path == NULL){
		perror("malloc path in init_cpt_depuis_fichier");
		return NULL;
	}

	int fd;

	strcpy(path, dir_name);
	strcat(path, "/");
	strcat(path, base_name);

	if((fd = open(path, O_RDONLY)) < 0){
		perror("open in ini_cpt_depuis_fichier");
		return NULL;
	}

	//remplissage de titre, il contient déjà '\n' après l'appel à read_until_nl
	read_until_nl(fd, titre);
	strcat(titre, "\0");

	//remplissage du nom de fichier
	strcpy(nom_fichier, base_name);
	strcat(nom_fichier, "\0");

	//affectation des champs de result
	result->titre = titre;
	result->nom_fichier = nom_fichier;

	close(fd);
	free(path);
	return result;
}

void liberer_comptine(struct comptine *cpt)
{
	if(cpt == NULL) return;
	free(cpt->titre);
	cpt->titre = NULL;
	free(cpt->nom_fichier);
	cpt->nom_fichier = NULL;
	free(cpt);
	cpt = NULL;
	return;
}

struct catalogue *creer_catalogue(const char *dir_name)
{
	/* À définir */

	/*allocation dynamique de struct catalogue et de ses champs
	*ils doivent etre tous libérés à l'intérieur de la fonction appelante
	*après utilisation
	*/


	struct catalogue *result = malloc(sizeof(struct catalogue));

	if(result == NULL){
		perror("malloc result in creer_catalogue");
		return NULL;
	}

	result->tab = NULL;

	struct comptine **tab = malloc(sizeof(struct comptine*)*BUFF_SIZE);
	if(tab == NULL){
		perror("malloc tab in creer_catalogue");
		//liberer la mémoire allouée précédement
		free(result);
		return NULL;
	}


	result->nb = 0;


	//ouverture du repertoire dir_name
	DIR *dirp = opendir(dir_name);

	if(dirp == NULL){
		perror("opendir dirp in creer_catalogue");
		exit(2);
	}


	/*lecture des entrées du repertoire
	*pour les placer dans le tableau de 
	*pointeurs de comptine
	*/
	struct dirent *ent;
	while((ent = readdir(dirp)) != NULL){
		if((est_nom_fichier_comptine(ent->d_name)) == 1){
			tab[result->nb++] = init_cpt_depuis_fichier(dir_name, ent->d_name);
		}
	}

	//affectation du champ tab
	if(result->tab != NULL){
		free(result->tab);
	}
	result->tab = tab;

	closedir(dirp);

	return result;
}

void liberer_catalogue(struct catalogue *c)
{

	if (c == NULL)
       		return;
  
  	// Libéreration des comptines
 	for (int i = 0; i < c->nb; i++) {
       		 if (c->tab[i] != NULL) {
           		 liberer_comptine(c->tab[i]);
           		 c->tab[i] = NULL;
       		 }
 	 }
  	 

  	 // Libérer le tableau de pointeurs de comptine
  	 free(c->tab);
	 c->tab = NULL;
	 
   	 // Libérer la structure de catalogue
   	 free(c);
   	 c = NULL;
   	 
   	 return;
   	 
}




int is_folder_exist(const char *dir_name, const char *nom_fichier){
	DIR *dirp = opendir(dir_name);
	if(dirp == NULL){
		perror("opendir dirp in is_folder_exist");
		exit(3);
	}

	struct dirent *ent;
	while((ent = readdir(dirp)) != NULL){
		if((strcmp(ent->d_name, nom_fichier) == 0)){
			closedir(dirp);
			return 1;
		}
	}
	closedir(dirp);
	return 0;
}void update_catalogue(const char *dir_name, struct catalogue *c){
	liberer_catalogue(c);
	struct catalogue *b = creer_catalogue(dir_name);
	c = b;
	return;
}



