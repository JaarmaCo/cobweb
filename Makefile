CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -Werror -Isrc -Iout -ggdb
LDFLAGS := -lm

SOURCE_FILES := src/string_view.c \
								src/string_builder.c \
								src/sv_read.c \
								src/format.c \
								src/allocator.c

SOURCE_OBJECT_FILES := $(patsubst src/%.c,out/%.o,$(SOURCE_FILES))

TEMPLATE_FILES := src/dynamic_array.h
TEMPLATE_SOURCES := out/dynamic_array_int.c out/dynamic_array_int.h out/dynamic_array_char.c out/dynamic_array_char.h
TEMPLATE_OBJECTS := out/dynamic_array_int.o out/dynamic_array_char.o

test: build-tests
	./out/test/dynamic_array
	./out/test/string_builder

build-templates: $(TEMPLATE_SOURCES) $(TEMPLATE_OBJECTS)

build-tests: build-templates out/test/dynamic_array out/test/string_builder

clean:
	rm -rf out

out/dynamic_array_%.o: out/dynamic_array_%.c | out/
	$(CC) $(CFLAGS) -c -o $@ $<

out/string_builder.o: src/string_builder.c out/dynamic_array_char.h | out/
	$(CC) $(CFLAGS) -c -o $@ src/string_builder.c

out/%.o: src/%.c | out/
	$(CC) $(CFLAGS) -c -o $@ $<

out/test/dynamic_array: test/dynamic_array.c out/dynamic_array_int.o out/allocator.o | out/test/
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test/dynamic_array.c out/dynamic_array_int.o out/allocator.o

out/test/string_builder: test/string_builder.c $(SOURCE_OBJECT_FILES) | out/test/
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test/string_builder.c $(SOURCE_OBJECT_FILES) out/dynamic_array_char.o

out/dynamic_array_%.c out/dynamic_array_%.h: src/dynamic_array.h | out/
	./c-template --infer $@ -o out -i src/dynamic_array.h

out/:
	mkdir -p out

out/test/:
	mkdir -p out/test/

.PHONY: test clean build-tests build-templates

