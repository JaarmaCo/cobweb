CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -Werror -Isrc -Iout

test: test/dynamic_array
	./test/dynamic_array
	rm ./test/dynamic_array

test/dynamic_array: out/dynamic_array_int.h out/dynamic_array_int.c src/allocator.c
	$(CC) $(CFLAGS) -o test/dynamic_array test/dynamic_array.c out/dynamic_array_int.c src/allocator.c

out/dynamic_array_%: src/dynamic_array.h
	mkdir -p out
	./c-template --infer $@ -o out -i src/dynamic_array.h

.PHONY: test

