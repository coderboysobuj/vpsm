/**
 * main.c - CLI entry point and command dispatcher for vpsm.
 *
 * Usage:
 *   vpsm ls                 – list all servers
 *   vpsm <name>             – SSH into named server
 *   vpsm add                – interactive wizard to add a server
 *   vpsm remove <name>      – remove a server
 *   vpsm info <name>        – show full details of one server
 *   vpsm help               – print usage
 *
 * To add a new command:
 *   1. Write a handler:  static int cmd_yourname(int argc, char **argv, ...)
 *   2. Add a row to the dispatch[] table at the bottom of this file.
 *   That's all.
 */

#include "vpsm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Print a prompt and read a line into buf (up to buf_len-1 chars).
 * If default_val is non-NULL and the user presses Enter, default_val is used.
 * Returns 1 on success, 0 on EOF/error.
 */
static int prompt(const char *label, const char *default_val,
                  char *buf, size_t buf_len)
{
    if (default_val && default_val[0] != '\0')
        printf("  %s [%s]: ", label, default_val);
    else
        printf("  %s: ", label);

    fflush(stdout);

    if (!fgets(buf, (int)buf_len, stdin))
        return 0;

    trim(buf);

    if (buf[0] == '\0' && default_val)
        safe_strncpy(buf, default_val, buf_len);

    return 1;
}


/* vpsm ls */
static int cmd_ls(int argc, char **argv,
                  ServerList *list, const char *config_path)
{
    (void)argc; (void)argv; (void)config_path;
    server_print_table(list);
    return EXIT_SUCCESS;
}

/* vpsm info <name> */
static int cmd_info(int argc, char **argv,
                    ServerList *list, const char *config_path)
{
    (void)config_path;
    if (argc < 2) {
        fprintf(stderr, "Usage: vpsm info <name>\n");
        return EXIT_FAILURE;
    }
    Server *s = server_find(list, argv[1]);
    if (!s) {
        fprintf(stderr, "vpsm: server '%s' not found.\n", argv[1]);
        return EXIT_FAILURE;
    }
    server_print_one(s);
    return EXIT_SUCCESS;
}

/* vpsm remove <name> */
static int cmd_remove(int argc, char **argv,
                      ServerList *list, const char *config_path)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: vpsm remove <name>\n");
        return EXIT_FAILURE;
    }

    VpsmStatus st = server_remove(list, argv[1]);
    if (st == VPSM_ERR_NOTFND) {
        fprintf(stderr, "vpsm: server '%s' not found.\n", argv[1]);
        return EXIT_FAILURE;
    }

    st = store_save(config_path, list);
    if (st != VPSM_OK) {
        fprintf(stderr, "vpsm: failed to save config.\n");
        return EXIT_FAILURE;
    }

    printf("Removed server '%s'.\n", argv[1]);
    return EXIT_SUCCESS;
}

/* vpsm add  (interactive wizard) */
static int cmd_add(int argc, char **argv,
                   ServerList *list, const char *config_path)
{
    (void)argc; (void)argv;

    Server s;
    memset(&s, 0, sizeof(s));

    char buf[VPSM_MAX_KEYPATH];

    printf("=== Add a new server ===\n");

    /* Name */
    do {
        if (!prompt("Alias (no spaces)", NULL, s.name, sizeof(s.name)))
            return EXIT_FAILURE;
        if (s.name[0] == '\0')
            printf("  Name cannot be empty.\n");
        else if (strchr(s.name, ' '))
            printf("  Name must not contain spaces.\n");
        else
            break;
    } while (1);

    if (server_find(list, s.name)) {
        fprintf(stderr, "vpsm: server '%s' already exists. "
                        "Remove it first.\n", s.name);
        return EXIT_FAILURE;
    }

    /* Host */
    do {
        if (!prompt("Host / IP", NULL, s.host, sizeof(s.host)))
            return EXIT_FAILURE;
    } while (s.host[0] == '\0' && printf("  Host cannot be empty.\n"));

    /* User */
    if (!prompt("SSH user", "root", s.user, sizeof(s.user)))
        return EXIT_FAILURE;
    if (s.user[0] == '\0')
        safe_strncpy(s.user, "root", sizeof(s.user));

    /* Port */
    if (!prompt("Port", "22", buf, sizeof(buf)))
        return EXIT_FAILURE;
    s.port = (buf[0] != '\0') ? atoi(buf) : 22;
    if (s.port <= 0 || s.port > 65535) s.port = 22;

    /* Auth method */
    do {
        if (!prompt("Auth method (password/key)", "password", buf, sizeof(buf)))
            return EXIT_FAILURE;
    } while (!str_eq(buf, "password") && !str_eq(buf, "key") &&
             printf("  Enter 'password' or 'key'.\n"));

    s.auth = str_eq(buf, "key") ? AUTH_KEY : AUTH_PASSWORD;

    /* Key path (only if key auth) */
    if (s.auth == AUTH_KEY) {
        const char *home = getenv("HOME");
        char def_key[VPSM_MAX_KEYPATH] = "";
        if (home)
            snprintf(def_key, sizeof(def_key), "%s/.ssh/id_rsa", home);

        if (!prompt("Private key path", def_key, s.key_path, sizeof(s.key_path)))
            return EXIT_FAILURE;

        if (s.key_path[0] == '\0' && def_key[0] != '\0')
            safe_strncpy(s.key_path, def_key, sizeof(s.key_path));
    }

    /* Confirm */
    printf("\nAbout to add:\n");
    server_print_one(&s);
    if (!prompt("Confirm? (yes/no)", "yes", buf, sizeof(buf)))
        return EXIT_FAILURE;

    if (!str_eq(buf, "yes") && !str_eq(buf, "y")) {
        printf("Aborted.\n");
        return EXIT_SUCCESS;
    }

    VpsmStatus st = server_add(list, &s);
    if (st != VPSM_OK) {
        fprintf(stderr, "vpsm: could not add server (code %d).\n", (int)st);
        return EXIT_FAILURE;
    }

    st = store_save(config_path, list);
    if (st != VPSM_OK) {
        fprintf(stderr, "vpsm: failed to save config.\n");
        return EXIT_FAILURE;
    }

    printf("Server '%s' saved.\n", s.name);
    return EXIT_SUCCESS;
}

/* vpsm help */
static int cmd_help(int argc, char **argv,
                    ServerList *list, const char *config_path)
{
    (void)argc; (void)argv; (void)list; (void)config_path;

    printf(
        "Usage: vpsm <command> [args]\n\n"
        "Commands:\n"
        "  ls                 List all configured servers\n"
        "  <name>             SSH into the named server\n"
        "  add                Interactively add a new server\n"
        "  remove <name>      Remove a server from the list\n"
        "  info   <name>      Show full details for a server\n"
        "  help               Show this help message\n\n"
        "Config file: ~/.config/vpsm/servers.conf\n"
    );
    return EXIT_SUCCESS;
}

/*
 * To add a new command, add ONE row here and write the handler above.
 * Nothing else needs to change.
 */

typedef int (*CmdFn)(int argc, char **argv,
                     ServerList *list, const char *config_path);

typedef struct {
    const char *name;
    CmdFn       fn;
} Command;

static const Command dispatch[] = {
    { "ls",     cmd_ls     },
    { "list",   cmd_ls     },   /* alias */
    { "add",    cmd_add    },
    { "remove", cmd_remove },
    { "rm",     cmd_remove },   /* alias */
    { "info",   cmd_info   },
    { "help",   cmd_help   },
    { "--help", cmd_help   },
    { "-h",     cmd_help   },
    { NULL,     NULL       }    /* sentinel */
};

int main(int argc, char **argv)
{
    /* Resolve config file path once */
    char config_path[VPSM_MAX_KEYPATH];
    if (store_config_path(config_path, sizeof(config_path)) != VPSM_OK)
        return EXIT_FAILURE;

    /* Load persisted servers */
    ServerList list;
    if (store_load(config_path, &list) != VPSM_OK) {
        fprintf(stderr, "vpsm: failed to load config from %s\n", config_path);
        return EXIT_FAILURE;
    }

    /* With no arguments, default to listing servers */
    const char *subcmd = (argc >= 2) ? argv[1] : "ls";

    /* Check dispatch table first */
    for (const Command *c = dispatch; c->name; c++) {
        if (str_eq(subcmd, c->name)) {
            /* argv+1 points to the subcommand; argc-1 is remaining count.
             * When argc==1 (no args), subcmd is "ls" by default, and we
             * safely pass argc=0, argv+1 (which points past end, but
             * cmd_ls ignores its arguments entirely). */
            int sub_argc = (argc >= 2) ? argc - 1 : 0;
            char **sub_argv = (argc >= 2) ? argv + 1 : argv;
            return c->fn(sub_argc, sub_argv, &list, config_path);
        }
    }

    /*
     * Not a known command → treat it as a server name and connect.
     * This lets you type:  vpsm myserver
     */
    Server *s = server_find(&list, subcmd);
    if (!s) {
        fprintf(stderr,
            "vpsm: unknown command or server '%s'.\n"
            "Run 'vpsm help' for usage or 'vpsm ls' to list servers.\n",
            subcmd);
        return EXIT_FAILURE;
    }

    return (ssh_connect(s) == VPSM_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
