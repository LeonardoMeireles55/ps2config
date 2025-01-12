all:
	gcc ./src/main.c ./src/gx.c ./src/net.c ./src/util.c -o ./bin/ps2config-cmd -lssl -lcrypto
clean:
	rm -rf ./bin/ps2config-cmd
