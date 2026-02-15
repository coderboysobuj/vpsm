/**
 * ssh.c - Build and exec the ssh command for a given Server.
 *
 * Uses execvp() so the ssh process *replaces* vpsm – no child zombie,
 * no signal handling needed.  The terminal is fully handed to ssh.
 *
 * To support additional SSH options (e.g. ProxyJump, StrictHostKeyChecking):
 *   Add more argv entries before the NULL terminator below.
 */

#include "vpsm.h"

#include <stdio.h>

#ifdef _WIN32
  /* Windows: use system() as a fallback (no execvp) */
  #include <windows.h>
  #define USE_SYSTEM_FALLBACK 1
#else
  #include <unistd.h>
  #define USE_SYSTEM_FALLBACK 0
#endif

/* Maximum number of argv slots for the ssh invocation */
#define SSH_ARGV_MAX 32

VpsmStatus ssh_connect(const Server *s)
{
    char port_str[16];
    char target[VPSM_MAX_USER + 1 + VPSM_MAX_HOST]; /* user@host */

    snprintf(port_str, sizeof(port_str), "%d", s->port);
    snprintf(target,   sizeof(target),   "%s@%s", s->user, s->host);

    /* Build argv dynamically so it's easy to add future flags */
    const char *argv[SSH_ARGV_MAX];
    int i = 0;

    argv[i++] = "ssh";
    argv[i++] = "-p";
    argv[i++] = port_str;

    if (s->auth == AUTH_KEY && s->key_path[0] != '\0') {
        argv[i++] = "-i";
        argv[i++] = s->key_path;
    }

    argv[i++] = target;
    argv[i]   = NULL;   /* execvp sentinel */

    /* Show the user exactly what command will run */
    printf("Connecting: ");
    for (int j = 0; argv[j]; j++)
        printf("%s ", argv[j]);
    printf("\n");

#if USE_SYSTEM_FALLBACK
    /* Windows – build a shell string and call system() */
    char cmd[1024] = {0};
    for (int j = 0; argv[j]; j++) {
        strncat(cmd, argv[j], sizeof(cmd) - strlen(cmd) - 2);
        strncat(cmd, " ",     sizeof(cmd) - strlen(cmd) - 1);
    }
    int rc = system(cmd);
    return (rc == 0) ? VPSM_OK : VPSM_ERR_IO;
#else
    /* POSIX – replace the current process with ssh */
    execvp("ssh", (char *const *)argv);

    /* execvp only returns on error */
    perror("vpsm: execvp ssh");
    return VPSM_ERR_IO;
#endif
}
