all:
	gcc main.c gx.c net.c util.c -o ps2config-cmd -lssl -lcrypto
clean:
	rm -rf ps2config-cmd
