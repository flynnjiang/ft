#ifndef __LOG_H__
#define __LOG_H__



struct log_msg_hdr {
    time_t timestamp;
    char *file;
    char *func;
    int line;
    int pid;
    int pname;

    int level;
    int type;
    char *tag;
    size_t content_len;
}

struct log_msg {
    struct log_msg_hdr hdr;
    char *content; /* HEX or String */
};

struct log_msg *log_msg_alloc(size_t content);
void log_msg_free(struct log_msg *msg);

int log_input(struct log_entry *entry);
int log_filter(struct log_entry *entry);
int log_output(struct log_entry *entry);

int log_output_to_syslog(struct log_entry *entry);

/* 需要考虑空间、备份的问题 */
int log_output_to_file(struct log_entry *entry);
int log_output_to_socket(struct log_entry *entry);

int log_rollback();





#endif /* __LOG_H__ */
