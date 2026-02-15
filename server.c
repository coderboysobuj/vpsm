/**
 * server.c - In-memory CRUD operations on ServerList.
 *
 * All functions here are pure data operations; no I/O except display.
 */

#include "vpsm.h"

#include <stdio.h>
#include <string.h>

void server_list_init(ServerList *list)
{
    memset(list, 0, sizeof(*list));
}

Server *server_find(ServerList *list, const char *name)
{
    for (size_t i = 0; i < list->count; i++) {
        if (str_eq(list->entries[i].name, name))
            return &list->entries[i];
    }
    return NULL;
}

VpsmStatus server_add(ServerList *list, const Server *s)
{
    if (list->count >= VPSM_MAX_SERVERS)
        return VPSM_ERR_FULL;

    if (server_find(list, s->name) != NULL)
        return VPSM_ERR_DUP;

    list->entries[list->count++] = *s;
    return VPSM_OK;
}

VpsmStatus server_remove(ServerList *list, const char *name)
{
    for (size_t i = 0; i < list->count; i++) {
        if (str_eq(list->entries[i].name, name)) {
            /* Shift remaining entries left to fill the gap */
            list->count--;
            if (i < list->count)
                memmove(&list->entries[i],
                        &list->entries[i + 1],
                        (list->count - i) * sizeof(Server));
            return VPSM_OK;
        }
    }
    return VPSM_ERR_NOTFND;
}

/* Column widths for the table view */
#define COL_NAME  16
#define COL_HOST  24
#define COL_USER  14
#define COL_PORT   6
#define COL_AUTH   8

static void print_separator(void)
{
    printf("+-%*s-+-%*s-+-%*s-+-%*s-+-%*s-+\n",
        COL_NAME, "----------------",
        COL_HOST, "------------------------",
        COL_USER, "--------------",
        COL_PORT, "------",
        COL_AUTH, "--------");
}

static void print_header(void)
{
    print_separator();
    printf("| %-*s | %-*s | %-*s | %-*s | %-*s |\n",
        COL_NAME, "NAME",
        COL_HOST, "HOST",
        COL_USER, "USER",
        COL_PORT, "PORT",
        COL_AUTH, "AUTH");
    print_separator();
}

void server_print_table(const ServerList *list)
{
    if (list->count == 0) {
        printf("No servers configured. Use 'vpsm add' to add one.\n");
        return;
    }

    print_header();
    for (size_t i = 0; i < list->count; i++) {
        const Server *s = &list->entries[i];
        printf("| %-*s | %-*s | %-*s | %-*d | %-*s |\n",
            COL_NAME, s->name,
            COL_HOST, s->host,
            COL_USER, s->user,
            COL_PORT, s->port,
            COL_AUTH, (s->auth == AUTH_KEY) ? "key" : "password");
    }
    print_separator();
    printf("  %zu server(s) total.\n", list->count);
}

void server_print_one(const Server *s)
{
    printf("  Name : %s\n", s->name);
    printf("  Host : %s\n", s->host);
    printf("  User : %s\n", s->user);
    printf("  Port : %d\n", s->port);
    printf("  Auth : %s\n", (s->auth == AUTH_KEY) ? "key" : "password");
    if (s->auth == AUTH_KEY)
        printf("  Key  : %s\n", s->key_path);
}
