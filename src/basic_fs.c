#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
/*
	File System virtual FOARTE rudimentar.
	Odata montat (trebuie montat intr-un folder gol), in root (/) se gaseste un singur
	fisier, si anume virtual_file, din cere poti doar citi.
	In root poti apela ls.
	Poti citi din virtual_file: cat virtual_file.

	Compileaza cu: gcc -Wall basic_fs.c `pkg-config fuse3 --cflags --libs` -o bs_fs
	Apoi monteaza folosind ./bs_fs [nume_folder_mount]
*/

/*
numele si continutul fisierului nostru
in proiectul real, vom avea noduri din arbore ce contin toate informatiile
*/
char file_content[] = "These are the contents of my virtual file\n";
char filename[] = "virtual_file"; 

/*
implementarea pentru getattr, functie esentiala a sistemului.
ar trebui sa returneze prin parametrul struct stat *st
informatii despre fisier, precum:
-ownerul
-grupul ownerului
-cand a fost creat
-cand a fost modificat ultima data
-modul (folder/fisier + permission bits)
-numarul de hardlinks
-etc.

Vezi struct stat (e chestie standard de unix) pentru detalii.
*/
static int basic_getattr(const char *path, struct stat *st,struct fuse_file_info *fi){
	//fuse_file_info e unused la noi

	//setam owner,group si timpul
    st->st_uid = getuid();
	st->st_gid = getgid();
	//fisierul nostru fiind virtual, timpul de creare/modificare este acum
	st->st_atime = time(NULL);
	st->st_mtime = time(NULL);

	//sistemul nostru contine un singur director, si anume root (/)
    if(strcmp(path,"/") == 0){
		//daca este root, setam modul ca director + permission bits 
		//vezi Linux File Permission pentru mai multe detalii
        st->st_mode = S_IFDIR | 0755;
		//orice director are by-default doua hard-links,si anume '.' din el
		//si 'numele' din parinte, aka /'numefisier'
		//cauta "file hardlinks" pentru mai multe detalii
		st->st_nlink = 2;
    }
    else if(strcmp(path+1,filename) == 0)
	{
		//daca este singurul nostru fisier
		//setam modul ca fisier + permission bits
		st->st_mode = S_IFREG | 0644;
		//fisierul are doar un hardlink, deoarece nu exista '.'
		st->st_nlink = 1;
		//dimensiunea fisierului este lungimea
		st->st_size = strlen(file_content);
	}
	else{
		//daca utilizatorul a cerut orice alt fisier, returnam eroare
		return -ENOENT;
	}
    return 0;
}

//functie ce "citeste un folder"
//adica, obtii toate fisierele/folderele din el
//in cazul nostru, void* buffer, o sa bagam numele fisierelor/folderelor
//pentru a face asta, ne este pusa la dispozitie "functia"
//fuse_fill_dir_t
static int basic_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags){
    //fuse_file_info si fuse_readdir_flags sunt unsued la noi

	//singurul nostru folder este root '/'
	//orice alt folder da eroare
	if (strcmp(path, "/") != 0)
		return -ENOENT;

	//orice folder primeste '.'(el) si '..'(parintele)
	filler(buffer, ".", NULL, 0,0);
	filler(buffer, "..", NULL, 0,0);

	//adaugam singurul nostru fisier
    filler(buffer, filename, NULL, 0, 0);
	
	return 0;
}

//functie de read
//dat fiind un fisier(pathul), un offset si un size,
//se returneaza prin buffer atata informatie
//functia returneaza nr de bytes citit
static int basic_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi ){
    //fuse_file_info e unusued

	//doar daca fisierul cerul este singurul nostru fisier
	if(strcmp(path+1,filename) == 0){
		//dimensiunea sa
		int len = strlen(file_content);
		//daca offsetul - pozitia de unde vrem sa citim, nu a trecut peste lungimea maxima
		if (offset < len) {
			if (offset + size > len) //daca nu mai avem size bytes de la offset la final, citim doar cat avem
				size = len - offset;
			//citim corespunzator
			memcpy(buffer, file_content + offset, size);
		} 
		else
		//in cazul in care offset iesea din fisier
			size = 0;
		return size;
	}
	//daca nu era fisierul nostru
    return -ENOENT;
}

//prin structura asta se face maparea intre
//functiile de sistem si functiile noastre
//sunt mult mai multe, se pot gasi pe net
static const struct fuse_operations file_oper = {
	.getattr	= basic_getattr,
	.readdir	= basic_readdir,
	.read		= basic_read,
};

int main(int argc, char* argv[])
{
	//in main, doar apelam fuse_main de argumente si mapare
    return fuse_main(argc, argv, &file_oper, NULL);
}