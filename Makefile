all:
	gcc main.c gx.c net.c -o ps2config-cmd
clean:
	rm -rf $(EXECUTABLE)
