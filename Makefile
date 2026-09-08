CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -Werror -Isrc -ggdb
LDFLAGS := -lm

SOURCE_FILES := src/string_view.c \
								src/string_builder.c \
								src/sv_read.c \
								src/format.c \
								src/allocator.c \
								src/json.c \
								src/json_parse.c \
								src/json_value_parser.c

SOURCE_OBJECT_FILES := $(patsubst src/%.c,out/%.o,$(SOURCE_FILES))

build: build-tests

test: build-tests
	./out/test/dynamic_array
	./out/test/string_builder
	./out/test/hashmap
	./out/test/json

build-tests: out/test/dynamic_array out/test/string_builder out/test/hashmap out/test/json

clean:
	rm -rf out

out/%.o: src/%.c | out/
	$(CC) $(CFLAGS) -c -o $@ $<

out/test/dynamic_array: test/dynamic_array.c $(SOURCE_OBJECT_FILES)  | out/test/
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test/dynamic_array.c $(SOURCE_OBJECT_FILES) 

out/test/string_builder: test/string_builder.c $(SOURCE_OBJECT_FILES) | out/test/
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test/string_builder.c $(SOURCE_OBJECT_FILES) 

out/test/hashmap: test/hashmap.c  $(SOURCE_OBJECT_FILES) | out/test/
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test/hashmap.c $(SOURCE_OBJECT_FILES) 

out/test/json: test/json.c  $(SOURCE_OBJECT_FILES) | out/test/
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test/json.c $(SOURCE_OBJECT_FILES) 

out/:
	mkdir -p out

out/test/:
	mkdir -p out/test/

.PHONY: test clean build build-tests

