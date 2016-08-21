#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/types.h>

#include <fcntl.h>



#define DISK "eeprom"
#define PARA_SIZE 64

#define LFS_DIR 1<<6
#define LFS_FILE 2<<6

typedef uint16_t para_ptr;
typedef uint16_t md_ptr;

struct md
{
	uint8_t type_sizehi;  //---  highest 2 bits -> type
		              //---  lower 6 bits are the higher 6 bits of size
	uint8_t sizelo;       //---  lower 8 bits of size
	para_ptr para1;
	para_ptr para2;
	md_ptr next_md;
};


struct md_page
{
	struct md metadata[PARA_SIZE/sizeof(struct md)];
};

struct data_page
{
	uint8_t data[PARA_SIZE];
};

struct lfs_descriptor
{
	char lfs_header[16];
	uint16_t nr_md_paras;
	uint16_t nr_data_paras;
	struct md root_md;
};

struct lfs_info
{
	int diskfd;
	struct lfs_descriptor *p_lfsd;
};


void lfs_make_root(struct lfs_descriptor *p_lfsd)
{
	struct md *p_root_md = &(p_lfsd->root_md);
	memset(p_root_md, 0, sizeof(struct md));
	p_root_md->type_sizehi = LFS_DIR;
	p_root_md->para1 = 7;

	//lseek(diskfd, 0, SEEK_SET);
	//write(diskfd, lfsd, sizeof(struct lfs_descriptor));
	
	
	//lseek(diskfd, 1*PARA_SIZE, SEEK_SET);
	//write(diskfd, &root_md, sizeof(struct md));

}

int lfs_mkfs(int diskfd, struct lfs_descriptor *p_lfsd)
{
	int nr_md_paras = 0;
	int i = 0;
	uint8_t zeropara[PARA_SIZE];
	populate_lfs_descriptor(diskfd, p_lfsd);
	nr_md_paras = p_lfsd->nr_md_paras;
	
	memset(&zeropara, 0, PARA_SIZE);
	lseek(diskfd, 0, SEEK_SET);
	for(i=1; i<=nr_md_paras+1; i++)
	{	
		write(diskfd, zeropara, PARA_SIZE);
	}
	
	lseek(diskfd, 0, SEEK_SET);
	write(diskfd, p_lfsd, sizeof(struct lfs_descriptor));
}

int populate_lfs_descriptor(int diskfd, struct lfs_descriptor *p_lfsd)
{
	off_t disk_size = 0;
	struct stat buf;
	int nr_paras = 0;
	int nr_md_per_mdpara = PARA_SIZE/sizeof(struct md);
	int nr_md_paras = 0;
	int nr_data_paras = 0;

	fstat(diskfd, &buf);
	disk_size = buf.st_size;
	printf("size of disk = %lu bytes.\n", disk_size);
	nr_paras = (disk_size/PARA_SIZE) - 1;
	//printf("nr_paras = %d.\n", nr_paras);
	//nr_md_per_mdpara = PARA_SIZE/sizeof(struct md);
	//printf("nr_md_per_mdpara = %d  --  %d  %u.\n", nr_md_per_mdpara, PARA_SIZE, sizeof(struct md));

	nr_md_paras = nr_paras/(nr_md_per_mdpara+1);
	nr_data_paras = nr_paras - nr_md_paras;

	printf("md_paras : %d     data_paras : %d\n", nr_md_paras, nr_data_paras);

	memcpy(p_lfsd->lfs_header, "<LFS FILESYSTEM>", sizeof(p_lfsd->lfs_header));
	p_lfsd->nr_md_paras = nr_md_paras;
	p_lfsd->nr_data_paras = nr_data_paras;
	


	lfs_make_root(p_lfsd);

	return 0;
}

int validate_dir_name(char *p_str)
{
	int ret = 0;
	char *i = NULL;
	char *j = NULL;
	char delim = '/';
	size_t len = 0;

	if(NULL == p_str)
	{
		ret = -1;
	}
	else if('/' != p_str[0])
	{
		ret = -2;
	}
	len = strlen(p_str);
	i = p_str;
	j = i+1;
	for(;j<p_str+len;)
	{
		printf("TEST: %s\n", i);
		//i must point to /
		//therefore j must not be a /
		if((*i != delim) || (*j == delim))
		{
			return -3;
		}
		else if(*j == '\0')
			break;


		for(j=i+1; j<p_str+len; j++)
		{
			if(*j == delim)
				break;
			else if(0 == isalnum(*j))
			{
				//this is not alphanumeric.
				printf(":( -> %c   :", *j);
				return -4;
			}
		}
		//j points to a delim

		if (j-i > 6)
		{
			return -5;
		}
		i=j;
		j++;
	}
	return 0;
}

	
uint16_t get_size(struct md *p_md)
{
	uint16_t size = 0;
	size = (p_md->type_sizehi)&0x3F;
	size = (size<<8)|(p_md->sizelo);
	return size;
}

struct md* get_parent_dir_md(struct lfs_info *p_lfs_info, char *path)
{
	return &(p_lfs_info->p_lfsd->root_md);
}

int make_file(struct lfs_info *p_lfs_info, char *p_parent, char *p_filename)
{
	int ret = 0;
	uint16_t dir_size = 0;
	struct md *p_parent_dir_md;

	ret = validate_dir_name(p_parent);

	printf("ret = %d.\n", ret);
	
	p_parent_dir_md = get_parent_dir_md(p_lfs_info, p_parent);
	
	printf("type = %d,   para1 = %d.\n", ((p_parent_dir_md->type_sizehi)>>6)&3, p_parent_dir_md->para1);

	dir_size = get_size(p_parent_dir_md);

	printf("dir_size = %u.\n", dir_size);




}
	

int main()
{
	int diskfd = open(DISK, O_RDWR);
	struct lfs_descriptor lfsd;
	struct lfs_info lfsinfo;
	if(0 > diskfd)
	{
		printf("cannot open "DISK"\n");
		return -1;
	}

	lfs_mkfs(diskfd, &lfsd);

	lfsinfo.diskfd = diskfd;
	lfsinfo.p_lfsd = &lfsd;

	make_file(&lfsinfo, "/", "test");




	return 0;
}






	

