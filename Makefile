
out/dynamic_array_%: src/dynamic_array.h
	mkdir -p out
	./c-template --infer $@ -o out -i src/dynamic_array.h

