# Quick GDB Remote Debugging Guide

## Your Setup
You have two sides:
- **Host** (your PC) — runs `gdb`
- **Target** (board) — runs `gdbserver`

---

## On the Target Board

```bash
gdbserver :2345 /usr/bin/sequencer
# or attach to already running process
gdbserver :2345 --attach <PID>
```

---

## On the Host PC

You need the **unstripped binary** (with debug symbols). It should be in:
```bash
output/build/sequencer-1.0/sequencer
```

Start GDB:
```bash
# Use the cross gdb, not system gdb
output/host/bin/arm-linux-gnueabihf-gdb output/build/sequencer-1.0/sequencer
```

Then inside GDB connect to the board:
```gdb
(gdb) target remote 192.168.137.XX:2345
```

---

## Essential GDB Commands

### Breakpoints
```gdb
break main          # break at function
break sequencer.c:42  # break at file:line
info breakpoints    # list all breakpoints
delete 1            # delete breakpoint #1
```

### Running
```gdb
continue            # run until next breakpoint
next                # next line (step over functions)
step                # next line (step into functions)
finish              # run until current function returns
```

### Inspecting Variables
```gdb
print my_var        # print variable value
print *ptr          # dereference pointer
print arr[0]        # array element
display my_var      # print var automatically at every stop
info locals         # show all local variables
info args           # show function arguments
```

### Call Stack
```gdb
backtrace           # show call stack (where did we come from)
frame 2             # switch to stack frame #2
up / down           # move between frames
```

### Memory
```gdb
x/10x 0xaddress     # examine 10 hex words at address
x/s 0xaddress       # examine as string
```

### Watchpoints (break when variable changes)
```gdb
watch my_var        # stop when my_var is written
rwatch my_var       # stop when my_var is read
```

---

## Typical Debug Session Flow

```gdb
(gdb) target remote 192.168.137.XX:2345
(gdb) break main
(gdb) continue          # start execution
(gdb) next              # step through code
(gdb) print some_var    # inspect value
(gdb) continue          # run to next breakpoint
(gdb) backtrace         # if it crashes, see where
(gdb) quit
```

---

## Tips
- Build with `-g` flag to include debug symbols — add `SEQUENCER_CFLAGS += -g` in your `.mk`
- If GDB says **"no debugging symbols"** → binary was stripped, use the one from `output/build/` not from the target
- `Ctrl+C` in GDB pauses execution on the target so you can inspect state anytime