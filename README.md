# vpsm — VPS Manager

A minimal, zero-dependency command-line tool for managing SSH connections
to multiple VPS servers.  Written in C99, no external libraries needed.

```
$ vpsm ls
+------------------+--------------------------+----------------+--------+----------+
| NAME             | HOST                     | USER           | PORT   | AUTH     |
+------------------+--------------------------+----------------+--------+----------+
| acme-prod        | 192.168.1.10             | deploy         | 22     | key      |
| widgets-staging  | staging.widgets.io       | ubuntu         | 2222   | key      |
| old-client       | legacy.oldclient.net     | admin          | 22     | password |
+------------------+--------------------------+----------------+--------+----------+
  3 server(s) total.

$ vpsm acme-prod
Connecting: ssh -p 22 -i /home/user/.ssh/acme_rsa deploy@192.168.1.10
```

---

## Build

```bash
make            # release build  → ./vpsm
make debug      # with ASAN + debug symbols
make install    # install to /usr/local/bin  (may need sudo)
make install PREFIX=$HOME/.local   # user-local install
make clean
```

Requirements: any C99 compiler (gcc, clang, tcc…) and `make`.

---

## Usage

| Command | What it does |
|---|---|
| `vpsm` | List all servers (same as `ls`) |
| `vpsm ls` | List all servers in a table |
| `vpsm <name>` | SSH into the named server |
| `vpsm add` | Interactive wizard to add a server |
| `vpsm remove <name>` | Remove a server (alias: `rm`) |
| `vpsm info <name>` | Show full details of one server |
| `vpsm help` | Print usage |

---

## Config file

Stored at `~/.config/vpsm/servers.conf` — plain text, human-editable.

```ini
# vpsm server list

[acme-prod]
host = 192.168.1.10
user = deploy
port = 22
auth = key
key  = /home/user/.ssh/acme_rsa

[widgets-staging]
host = staging.widgets.io
user = ubuntu
port = 2222
auth = key
key  = /home/user/.ssh/id_ed25519

[old-client]
host = legacy.oldclient.net
user = admin
port = 22
auth = password
```

You may edit this file directly — blank lines and `#` comments are supported.

---



## TODO Extending vpsm

### Add a new command (e.g. `vpsm ping`)

1. Write a handler in `main.c`:
   ```c
   static int cmd_ping(int argc, char **argv,
                       ServerList *list, const char *config_path)
   {
       // your logic here
   }
   ```
2. Add one row to the `dispatch[]` table:
   ```c
   { "ping", cmd_ping },
   ```
That's all — no other file needs to change.

### Add a new field to Server (e.g. `notes`)

1. Add the field to `struct Server` in `vpsm.h`.
2. Parse it in `store_load()` inside `store.c`.
3. Write it in `store_save()` inside `store.c`.
4. Prompt for it in `cmd_add()` inside `main.c`.
5. Display it in `server_print_one()` inside `server.c`.

### Swap the storage backend (e.g. JSON, SQLite)

Replace `store_load()` and `store_save()` in `store.c`.
Keep the function signatures identical.
Update the Makefile LDFLAGS if the new backend needs link flags.
