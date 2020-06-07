CC=/opt/clang/bin/clang
CCFLAGS=--target=wasm32-unknown-unknown-wasm --optimize=3 -nostdlib
LDFLAGS=-Wl,--export-all -Wl,--no-entry -Wl,--allow-undefined

WASM2WAT=/opt/wabt/bin/wasm2wat

.PHONY: all

all: example1.wat example2.wat example3.wat example4.wat example5.wat

example1.wat: example1.wasm
	$(WASM2WAT) example1.wasm -o example1.wat

example1.wasm: example1.c
	$(CC) example1.c $(CCFLAGS) $(LDFLAGS) --output example1.wasm

example2.wat: example2.wasm
	$(WASM2WAT) example2.wasm -o example2.wat

example2.wasm: example2.c
	$(CC) example2.c $(CCFLAGS) $(LDFLAGS) --output example2.wasm

example3.wat: example3.wasm
	$(WASM2WAT) example3.wasm -o example3.wat

example3.wasm: example3.c
	$(CC) example3.c $(CCFLAGS) $(LDFLAGS) --output example3.wasm

example4.wat: example4.wasm
	$(WASM2WAT) example4.wasm -o example4.wat

example4.wasm: example4.c
	$(CC) example4.c $(CCFLAGS) $(LDFLAGS) --output example4.wasm

example5.wat: example5.wasm
	$(WASM2WAT) example5.wasm -o example5.wat

example5.wasm: example5.c memory-allocation.c
	$(CC) example5.c memory-allocation.c $(CCFLAGS) $(LDFLAGS) --output example5.wasm

clean:
	rm -f example1.wasm
	rm -f example1.wat
	rm -f example2.wasm
	rm -f example2.wat
	rm -f example3.wasm
	rm -f example3.wat
	rm -f example4.wasm
	rm -f example4.wat
	rm -f example5.wasm
	rm -f example5.wat
