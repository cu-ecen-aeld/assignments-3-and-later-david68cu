# Step by steps


## Building the program

- Navigate to the Directory: Open a terminal and navigate to the directory containing your aesdsocket.c and Makefile:
```cd server/```

- Build the Program: Run the make command to compile the program:

```make```

This will compile aesdsocket.c and create an executable named aesdsocket in the same directory.

- Run the Program
```./aesdsocket```

- Send signals

```bash
ps aux | grep aesdsocket
# Let say you got PID 12345
kill -SIGTERM 12345 or kill -15 12345 # Default termination signal ( graceful shutdown)
kill -SIGINT 12345 or kill -2 12345  # SIGINT intrerrup form keyboras
kill -SIGKILL 12345 or kill -9 12345 # Force kill process( can not be ignored)
```

test for leakeage

```bask
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=/tmp/valgrind-out.txt ./aesdsocket
```