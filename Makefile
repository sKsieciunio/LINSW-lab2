OBJS := main.o 
worms: $(OBJS)
	$(CC) -o main $(CFLAGS) $(LDFLAGS) $(OBJS) -lperiphery
$(OBJS) : %.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

