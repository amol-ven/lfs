#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/types.h>

#include <fcntl.h>


#include "lfs2.h"




#define LFS_DIR 1<<6
#define LFS_FILE 2<<6




void lfs_make_root(struct lfs_descriptor *p_lfsd)
{
	struct md *p_root_md = &(p_lfsd->root_md);
	memset(p_root_md, 0, sizeof(struct md));
	//p_root_md->type_sizehi = LFS_DIR;
	p_root_md->para1.para = 0;
	p_root_md->para1.size_in_para = 0;

	//lseek(diskfd, 0, SEEK_SET);
	//write(diskfd, lfsd, sizeof(struct lfs_descriptor));
	
	
	//lseek(diskfd, 1*PARA_SIZE, SEEK_SET);
	//write(diskfd, &root_md, sizeof(struct md));

}

int lfs_mkfs(int diskfd, struct lfs_descriptor *p_lfsd)
{
	int nr_md_paras = 0;
	int i = 1;
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
	//nr_paras = (disk_size/PARA_SIZE) - 1;
	nr_paras = (disk_size/PARA_SIZE);
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


int read_para(struct lfs_info *p_lfs_info, uint16_t para_number, para_type_e para_type, void *para)
{
	uint16_t para_in_media = 0;
	int out_of_bounds = 0;
	size_t bytes_read = 0;
	int ret = -1;
	if(e_MD_PARA == para_type)
	{
		para_in_media = para_number;
		if(para_in_media >= p_lfs_info->p_lfsd->nr_md_paras)
		{
			out_of_bounds = 1;
		}
	}
	else
	{
		para_in_media = p_lfs_info->p_lfsd->nr_md_paras + para_number;
		if(para_in_media >= (p_lfs_info->p_lfsd->nr_md_paras + p_lfs_info->p_lfsd->nr_data_paras))
		{
			out_of_bounds = 1;
		}
	}
	if(0 == out_of_bounds)
	{
		lseek(p_lfs_info->diskfd, 0 , SEEK_SET);
		bytes_read = read(p_lfs_info->diskfd, para, PARA_SIZE);
		if(PARA_SIZE == bytes_read)
		{
			ret = 0;
		}
		else
		{
			printf("cannot read from media.\n");
		}
	}
	return ret;		
}

para_ptr md_ptr_to_para_ptr(md_ptr mdptr)
{
	return mdptr/(PARA_SIZE/sizeof(struct md));
}

uint16_t md_in_para(md_ptr mdptr)
{
	return mdptr%(PARA_SIZE/sizeof(struct md));
}



uint16_t get_size(md_ptr mdptr, struct lfs_info *p_lfs_info)
{
	uint16_t size = 0;
	struct md_para mdpara;
	para_ptr paraptr = md_ptr_to_para_ptr(mdptr);
	struct md *p_md = NULL;

	
	printf(" --mdptr: %u\n", mdptr);
	printf(" --paraptr: %u\n", paraptr);



	do
	{
		int ret = read_para(p_lfs_info, paraptr, e_MD_PARA, &mdpara);
		printf("ret = %d\n", ret);
		p_md = &(mdpara.metadata[md_in_para(mdptr)]);
		size += p_md->para1.size_in_para;
		size += p_md->para2.size_in_para;
		mdptr = p_md->next_md;
		printf("para = %u\n", p_md->para1.para);
		printf("size = %u\n", p_md->para1.size_in_para);
	}
	while(mdptr >= 3);

	return size;

}




/*
struct md* get_parent_dir_md(struct lfs_info *p_lfs_info, char *path)
{
	return &(p_lfs_info->p_lfsd->root_md);
}
*/
md_ptr get_parent_dir_md(struct lfs_info *p_lfs_info, char *path)
{
	//return &(p_lfs_info->p_lfsd->root_md);
	return 1;
}


void  

int make_file(struct lfs_info *p_lfs_info, char *p_parent, char *p_filename)
{
	int ret = 0;
	uint16_t dir_size = 0;
	md_ptr parent_dir_md = 0;
	size_t spare_space_needed = 0;

	ret = validate_dir_name(p_parent);

	printf("ret = %d.\n", ret);
	
	parent_dir_md = get_parent_dir_md(p_lfs_info, p_parent);
	
	//printf("para1 = %d.\n", p_parent_dir_md->para1.para);

	dir_size = get_size(parent_dir_md, p_lfs_info);

	printf("dir_size = %u.\n", dir_size);

	spare_space_needed = sizeof(struct dentry);


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

	
	printf("sizeof md_para        = %u\n"
	       "sizeof md             = %u\n"
	       "sizeof data_para      = %u\n"
	       "sizeof lfs_descriptor = %u\n",
	       (unsigned int)sizeof(struct md_para),
	       (unsigned int)sizeof(struct md), 
	       (unsigned int)sizeof(struct data_para),
	       (unsigned int)sizeof(struct lfs_descriptor));


	return 0;
}






	

