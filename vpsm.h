/**
 * vpsm.h - VPS Manager: types, constants, and public API
 */

#ifndef VPSM_H
#define VPSM_H

#include <stddef.h>


#define VPSM_MAX_NAME      64
#define VPSM_MAX_HOST      256
#define VPSM_MAX_USER      64
#define VPSM_MAX_KEYPATH   512
#define VPSM_MAX_SERVERS   512
#define VPSM_MAX_LINE      1024

/* default storage file: ~/.config/vpsm/servers.conf */
#define VPSM_CONFIG_DIR    ".config/vpsm"
#define VPSM_CONFIG_FILE   "servers.conf"

/* Auth method */
typedef enum {
    AUTH_PASSWORD = 0,   /* ssh user@host  (terminal prompts for password)    */
    AUTH_KEY      = 1    /* ssh -i keyfile user@host                          */
} AuthMethod;

/* Core data type */

typedef struct {
    char       name[VPSM_MAX_NAME];      /* logical client/server alias       */
    char       host[VPSM_MAX_HOST];      /* hostname or IP                    */
    char       user[VPSM_MAX_USER];      /* SSH username                      */
    int        port;                     /* SSH port (default 22)             */
    AuthMethod auth;                     /* password or key                   */
    char       key_path[VPSM_MAX_KEYPATH]; /* path to private key (AUTH_KEY)  */
} Server;

/* Server list (in-memory store) */

typedef struct {
    Server  entries[VPSM_MAX_SERVERS];
    size_t  count;
} ServerList;

/* Return codes */

typedef enum {
    VPSM_OK          =  0,
    VPSM_ERR_ARGS    = -1,
    VPSM_ERR_IO      = -2,
    VPSM_ERR_FULL    = -3,
    VPSM_ERR_DUP     = -4,
    VPSM_ERR_NOTFND  = -5,
    VPSM_ERR_SYNTAX  = -6
} VpsmStatus;

/* store.h interface */
/*    Swap out the implementation in store.c to change the storage backend.   */

VpsmStatus store_load(const char *path, ServerList *list);
VpsmStatus store_save(const char *path, const ServerList *list);
VpsmStatus store_config_path(char *buf, size_t buf_len);

/* server.h interface */

void       server_list_init(ServerList *list);
VpsmStatus server_add(ServerList *list, const Server *s);
VpsmStatus server_remove(ServerList *list, const char *name);
Server    *server_find(ServerList *list, const char *name);
void       server_print_table(const ServerList *list);
void       server_print_one(const Server *s);

/* ssh.h interface */

VpsmStatus ssh_connect(const Server *s);

/* util.h interface */

void  die(const char *fmt, ...);
void  trim(char *s);
int   str_eq(const char *a, const char *b);
void  safe_strncpy(char *dst, const char *src, size_t n);

#endif /* VPSM_H */
