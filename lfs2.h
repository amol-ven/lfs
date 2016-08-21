#define DISK "media"
#define PARA_SIZE 512

typedef uint16_t para_ptr;
typedef uint16_t md_ptr;

struct para_md
{
	uint16_t size_in_para;
	para_ptr para;
} __attribute__((packed));

struct md
{
	struct para_md para1;
	struct para_md para2;
	md_ptr next_md;
	uint16_t reserved0;
	uint16_t reserved1;
	uint16_t reserved2;
} __attribute__((packed));


struct md_para
{
	struct md metadata[PARA_SIZE/sizeof(struct md)];
};

struct data_para
{
	uint8_t data[PARA_SIZE];
};

struct lfs_descriptor
{
	char lfs_header[16];
	struct md root_md;
	uint16_t nr_md_paras;
	uint16_t nr_data_paras;
	uint16_t reserved0;
	uint16_t reserved1;
	uint16_t reserved2;
	uint16_t reserved3;
	uint16_t reserved4;
	uint16_t reserved5;
};

struct lfs_info
{
	int diskfd;
	struct lfs_descriptor *p_lfsd;
};

struct dentry
{
	char fname[13];
	uint8_t type;
	md_ptr file_mdptr;
};

typedef enum
{
	e_MD_PARA,
	e_DATA_PARA
}para_type_e;

typedef enum
{
	e_PARA_SFCNT,
	e_NEED_PARA
}file_para_state;
