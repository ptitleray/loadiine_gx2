#ifndef _FS_H_
#define _FS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
#define MAX_CLIENT 32

struct bss_t {
    int global_sock;
    int socket_fs[MAX_CLIENT];
    void *pClient_fs[MAX_CLIENT];
    volatile int lock;
    char mount_base[255];
    char save_base[255];
};

#define bss_ptr (*(struct bss_t **)0x100000e4)
#define bss (*bss_ptr)

void PatchMethodHooks(void);

#ifdef __cplusplus
}
#endif

#endif /* _FS_H */
