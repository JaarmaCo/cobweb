#include "process.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  string_view_t args[] = {
      SV("echo"),
      SV("Hello, world"),
  };
  process_t *process = process_launch(&(process_arguments_t){
      .executable = SV("/usr/bin/echo"),
      .argument_count = 2,
      .arguments = args,
  });

  if (!process) {
    printf("ERROR: %s", strerror(errno));
    exit(1);
  }

  process_wait_result_t *results = NULL;
  process_wait(process, &results);

  return process_get_exit_code(results);
}
