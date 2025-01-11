all:
	gcc main.c gx.c net.c util.c -o ps2config-cmd -lssl -lcrypto

clean:
	rm -rf ps2config-cmd

dependencies:
	sudo apt-get update
	sudo apt-get install -y gcc libssl-dev make zlib1g-dev

dependencies_apple_silicon:
	brew install openssl zlib

linux: dependencies all

apple_silicon: dependencies_apple_silicon
	gcc main.c gx.c net.c util.c -arch arm64 -I/opt/homebrew/opt/openssl/include -L/opt/homebrew/opt/openssl/lib -o ps2config-cmd -lssl -lcrypto
