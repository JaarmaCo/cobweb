
# Add Template import directive
- [ ]

Directive has the form:
```c
//! Template IMPORT
#include "import.h"
```

And hints to the template engine that the subsequent `#include` points to another template. The include is then
replaced in the output to point to the name of the generated header version (using the same template JSON), and
all mangle rules and types from `import.h` are put into the current scope.

