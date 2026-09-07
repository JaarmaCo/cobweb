CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -Werror -Isrc -Iout -ggdb
LDFLAGS := -lm

SOURCE_FILES := src/string_view.c \
								src/string_builder.c \
								src/sv_read.c \
								src/format.c \
								src/allocator.c

SOURCE_OBJECT_FILES := $(patsubst src/%.c,out/%.o,$(SOURCE_FILES))

TEMPLATE_FILES := src/dynamic_array.h src/hashmap.h
TEMPLATE_SOURCES := \
										out/dynamic_array_int.c \
										out/dynamic_array_int.h \
										out/dynamic_array_char.c \
										out/dynamic_array_char.h \
										out/hashmap_sv_i.h \
										out/hashmap_sv_i.c \
										out/hashmap_i_sv.h \
										out/hashmap_i_sv.c
TEMPLATE_OBJECTS := \
										out/dynamic_array_int.o \
										out/dynamic_array_char.o \
										out/hashmap_sv_i.o \
										out/hashmap_i_sv.o

test: build-tests
	./out/test/dynamic_array
	./out/test/string_builder
	./out/test/hashmap

build-templates: $(TEMPLATE_SOURCES) $(TEMPLATE_OBJECTS)

build-tests: build-templates out/test/dynamic_array out/test/string_builder out/test/hashmap

clean:
	rm -rf out

out/dynamic_array_%.o: out/dynamic_array_%.c | out/
	$(CC) $(CFLAGS) -c -o $@ $<

out/hashmap_%.o: out/hashmap_%.c | out/
	$(CC) $(CFLAGS) -c -o $@ $<

out/string_builder.o: src/string_builder.c out/dynamic_array_char.h | out/
	$(CC) $(CFLAGS) -c -o $@ src/string_builder.c

out/%.o: src/%.c | out/
	$(CC) $(CFLAGS) -c -o $@ $<

out/test/dynamic_array: test/dynamic_array.c out/dynamic_array_int.o out/allocator.o | out/test/
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test/dynamic_array.c out/dynamic_array_int.o out/allocator.o

out/test/string_builder: test/string_builder.c $(SOURCE_OBJECT_FILES) | out/test/
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test/string_builder.c $(SOURCE_OBJECT_FILES) out/dynamic_array_char.o

out/test/hashmap: test/hashmap.c $(TEMPLATE_OBJECTS) $(SOURCE_OBJECT_FILES) | out/test/
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test/hashmap.c $(SOURCE_OBJECT_FILES) $(TEMPLATE_OBJECTS)

out/dynamic_array_%.c out/dynamic_array_%.h: src/dynamic_array.h | out/
	./c-template --infer $@ -o out -i src/dynamic_array.h

out/hashmap_%.c out/hashmap_%.h: src/hashmap.h templates/sv-x-int.json | out/
	./c-template -o out -i src/hashmap.h -J templates/sv-x-int.json

out/:
	mkdir -p out

out/test/:
	mkdir -p out/test/

.PHONY: test clean build-tests build-templates

