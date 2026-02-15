/**
 * store.c - Flat-file persistence for the server list.
 *
 * File format (one server per stanza, blank-line separated):
 *
 *   [name]
 *   host     = example.com
 *   user     = root
 *   port     = 22
 *   auth     = password
 *   key      = /home/user/.ssh/id_rsa
 *
 * Lines starting with '#' are comments and are ignored.
 * The 'key' field is omitted when auth = password.
 *
 * To swap in a different backend (e.g. JSON, SQLite):
 *   1. Replace store_load() and store_save() in this file.
 *   2. Keep the function signatures identical.
 *   3. Update the Makefile if new link flags are needed.
 */

#include "vpsm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

VpsmStatus store_config_path(char *buf, size_t buf_len)
{
    const char *home = getenv("HOME");
    if (!home || home[0] == '\0') {
        fprintf(stderr, "vpsm: $HOME is not set.\n");
        return VPSM_ERR_IO;
    }

    int n = snprintf(buf, buf_len, "%s/%s/%s",
                     home, VPSM_CONFIG_DIR, VPSM_CONFIG_FILE);

    if (n < 0 || (size_t)n >= buf_len) {
        fprintf(stderr, "vpsm: config path too long.\n");
        return VPSM_ERR_IO;
    }
    return VPSM_OK;
}

/*
 * Ensure the config directory exists, creating it if necessary.
 * Only handles a single level of VPSM_CONFIG_DIR (e.g. ".config/vpsm").
 */
static VpsmStatus ensure_config_dir(void)
{
    const char *home = getenv("HOME");
    if (!home) return VPSM_ERR_IO;

    /* Step 1: ~/.config */
    char dir[VPSM_MAX_KEYPATH];
    snprintf(dir, sizeof(dir), "%s/.config", home);
    if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
        perror("vpsm: mkdir ~/.config");
        return VPSM_ERR_IO;
    }

    /* Step 2: ~/.config/vpsm */
    snprintf(dir, sizeof(dir), "%s/%s", home, VPSM_CONFIG_DIR);
    if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
        perror("vpsm: mkdir config dir");
        return VPSM_ERR_IO;
    }

    return VPSM_OK;
}

/*
 * Parse "key = value" into key_out and val_out.
 * Returns 1 on success, 0 if line doesn't match the pattern.
 */
static int parse_kv(const char *line, char *key_out, char *val_out, size_t sz)
{
    const char *eq = strchr(line, '=');
    if (!eq) return 0;

    size_t klen = (size_t)(eq - line);
    if (klen >= sz) return 0;

    strncpy(key_out, line, klen);
    key_out[klen] = '\0';
    trim(key_out);

    safe_strncpy(val_out, eq + 1, sz);
    trim(val_out);

    return 1;
}

VpsmStatus store_load(const char *path, ServerList *list)
{
    server_list_init(list);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        /* No config file yet is not an error – first run */
        if (errno == ENOENT) return VPSM_OK;
        perror("vpsm: open config");
        return VPSM_ERR_IO;
    }

    char line[VPSM_MAX_LINE];
    Server cur;
    int    in_entry = 0;     /* are we inside a [name] stanza? */

    memset(&cur, 0, sizeof(cur));

    while (fgets(line, sizeof(line), fp)) {
        trim(line);

        /* Skip blank lines and comments */
        if (line[0] == '\0' || line[0] == '#') {
            /* A blank line finalises the current stanza */
            if (in_entry && cur.name[0] != '\0') {
                if (cur.port == 0) cur.port = 22;
                VpsmStatus st = server_add(list, &cur);
                if (st != VPSM_OK && st != VPSM_ERR_DUP)
                    fprintf(stderr, "vpsm: warning: could not load '%s'\n",
                            cur.name);
                memset(&cur, 0, sizeof(cur));
                in_entry = 0;
            }
            continue;
        }

        /* Section header: [name] */
        if (line[0] == '[') {
            /* Save previous stanza if any */
            if (in_entry && cur.name[0] != '\0') {
                if (cur.port == 0) cur.port = 22;
                server_add(list, &cur);
                memset(&cur, 0, sizeof(cur));
            }
            size_t len = strlen(line);
            if (line[len - 1] == ']') {
                safe_strncpy(cur.name, line + 1, sizeof(cur.name));
                cur.name[len - 2] = '\0';   /* strip trailing ] */
                trim(cur.name);
                in_entry = 1;
            }
            continue;
        }

        /* Key = value inside a stanza */
        if (in_entry) {
            char key[64], val[VPSM_MAX_KEYPATH];
            if (!parse_kv(line, key, val, sizeof(val))) continue;

            if (str_eq(key, "host"))
                safe_strncpy(cur.host, val, sizeof(cur.host));
            else if (str_eq(key, "user"))
                safe_strncpy(cur.user, val, sizeof(cur.user));
            else if (str_eq(key, "port"))
                cur.port = atoi(val);
            else if (str_eq(key, "auth"))
                cur.auth = str_eq(val, "key") ? AUTH_KEY : AUTH_PASSWORD;
            else if (str_eq(key, "key"))
                safe_strncpy(cur.key_path, val, sizeof(cur.key_path));
        }
    }

    /* Flush the last stanza (file may not end with a blank line) */
    if (in_entry && cur.name[0] != '\0') {
        if (cur.port == 0) cur.port = 22;
        server_add(list, &cur);
    }

    fclose(fp);
    return VPSM_OK;
}

VpsmStatus store_save(const char *path, const ServerList *list)
{
    if (ensure_config_dir() != VPSM_OK)
        return VPSM_ERR_IO;

    FILE *fp = fopen(path, "w");
    if (!fp) {
        perror("vpsm: write config");
        return VPSM_ERR_IO;
    }

    fprintf(fp,
        "# vpsm server list\n"
        "# Managed by 'vpsm add' / 'vpsm remove'.\n"
        "# You may also edit this file manually.\n\n");

    for (size_t i = 0; i < list->count; i++) {
        const Server *s = &list->entries[i];

        fprintf(fp, "[%s]\n",  s->name);
        fprintf(fp, "host = %s\n", s->host);
        fprintf(fp, "user = %s\n", s->user);
        fprintf(fp, "port = %d\n", s->port);
        fprintf(fp, "auth = %s\n",
                (s->auth == AUTH_KEY) ? "key" : "password");

        if (s->auth == AUTH_KEY && s->key_path[0] != '\0')
            fprintf(fp, "key  = %s\n", s->key_path);

        fprintf(fp, "\n");
    }

    fclose(fp);
    return VPSM_OK;
}
