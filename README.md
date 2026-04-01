# ft_irc - IRC Server (C++98)

A lightweight IRC server implementation in C++98 using non-blocking sockets and `poll()`.
This project was written to support testing and usage with the `irssi` IRC client.

## Features

- Non-blocking TCP server loop with `poll()`
- Multi-client handling with per-client input/output buffers
- Registration flow with password protection (`PASS`, `NICK`, `USER`)
- Channel support with membership, operators, invites, topic, key, and user limit
- User and channel messaging (`PRIVMSG`, `NOTICE`)
- Operator/channel management (`KICK`, `MODE`, `TOPIC`, `INVITE`)
- Numeric replies for common IRC success/error cases
- Graceful shutdown on `SIGINT` / `SIGTERM`

## Build

Requirements:
- `c++` compiler with C++98 support
- `make`

Build commands:

```bash
make
make clean
make fclean
make re
```

Output binary:
- `./ircserv`

## Run

```bash
./ircserv <port> <password>
```

Constraints enforced by the server:
- `port` must be numeric and in range `1024..65535`
- `password` cannot be empty or only spaces

Example:

```bash
./ircserv 6667 supersecret
```

`irssi` quick connect example:

```bash
irssi -c 127.0.0.1 -p 6667 -w supersecret -n alice
```

## Quick Manual Test (with netcat)

In one terminal:

```bash
./ircserv 6667 supersecret
```

In another terminal:

```bash
nc 127.0.0.1 6667
```

Then send (each line ends with Enter):

```text
PASS supersecret
NICK alice
USER alice 0 * :Alice Example
JOIN #test
PRIVMSG #test :hello
```

## Registration Flow

Clients must complete:
1. `PASS <password>`
2. `NICK <nickname>`
3. `USER <username> <mode> <unused> :<realname>`

Notes:
- Before registration, most commands return `451` (`ERR_NOTREGISTERED`).
- Wrong `PASS` returns `464` and connection is closed.
- Nicknames are compared case-insensitively (IRC-style casefolding).

## Supported Commands

Implemented command handlers:

- Connection/session: `CAP` (`LS`, `END`), `PING`, `QUIT`
- Registration: `PASS`, `NICK`, `USER`
- Channels: `JOIN`, `PART`
- Messaging: `PRIVMSG`, `NOTICE`
- Moderation/admin: `KICK`, `MODE`, `TOPIC`, `INVITE`

## Channel Modes

Supported channel modes:

- `+i` / `-i`: invite-only channel
- `+t` / `-t`: topic change restricted to operators
- `+k <key>` / `-k`: channel key
- `+o <nick>` / `-o <nick>`: grant/revoke operator
- `+l <limit>` / `-l`: user limit

## Implemented Numeric Replies (high level)

Examples of handled replies/errors:

- Welcome/topic/names: `001`, `331`, `332`, `353`, `366`
- Common errors: `401`, `403`, `404`, `411`, `412`, `421`, `431`, `432`, `433`
- Channel/operator errors: `441`, `442`, `443`, `461`, `471`, `472`, `473`, `475`, `476`, `482`
- Registration/auth errors: `451`, `462`, `464`

See `include/NumericReplies.hpp` for the complete list used by the server.

## Architecture Overview

Main components:

- `Server`: socket setup, poll loop, dispatch, cleanup, channel/client registries
- `Client`: per-connection identity + registration flags + I/O buffers
- `Channel`: members, operators, invites, topic, and mode state
- `Command`: parsed command name and parameters
- `Utils`: validation and helper functions (nick/channel checks, casefolding, etc.)

Code layout:

- `src/Authentication.cpp` - registration and nick/user/pass validation
- `src/ChannelCommands.cpp` - join/part/names/topic listing logic
- `src/MessagingCommands.cpp` - user/channel messaging
- `src/OperatorCommands.cpp` - mode/topic/invite/kick handling
- `src/ServerCommands.cpp` - dispatch, numeric replies, CAP/PING/QUIT, broadcast
- `src/Server.cpp` - event loop, connection handling, parser, teardown

## Stability Notes

- Client disconnect cleanup removes membership/invite state before erasing client data.
- Broadcast/name-list paths guard against stale member entries.
- Server ignores `SIGPIPE` to avoid process termination on broken sockets.

## Known Scope / Simplifications

This project intentionally focuses on core IRC behavior and does not aim to be a full production IRC daemon.

Potential limitations may include:
- No TLS
- No server-to-server networking
- No persistence
- Limited subset of IRC commands/capabilities compared to full RFC implementations

