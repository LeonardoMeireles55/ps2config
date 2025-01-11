all:
	gcc main.c gx.c net.c util.c -o ps2config-cmd -lssl -lcrypto
clean:
	rm -rf ps2config-cmd
apple_silicon:
	gcc main.c gx.c net.c util.c -arch arm64 -I/opt/homebrew/opt/openssl/include -L/opt/homebrew/opt/openssl/lib -o ps2config-cmd -lssl -lcrypto
