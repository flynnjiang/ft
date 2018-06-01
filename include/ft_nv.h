#ifndef __FT_NV_H__
#define __FT_NV_H__


#define NV_ROOT_PATH    "nv/"
#define NV_BACKUP_ROOT_PATH "nv-backup/"

#define NV_INIT_FLAG_FILE   ".__nv_init__"


int ft_nv_init(const char *root_path);

#if 0
char *ft_nv_read(const char *name, char *buf, size_t buf_size);
int ft_nv_write(const char *name, char *value);
int ft_nv_compare(const char *name, char *value);

int ft_tmpnv_read();
int ft_tmpnv_write();

int ft_nv_import(const char *fpath, cosnt char *root_path);
int ft_nv_export(const char *fpath);
#endif


#endif /* __FT_NV_H__ */
