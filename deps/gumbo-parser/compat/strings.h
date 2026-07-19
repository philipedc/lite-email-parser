// Windows compatibility shim for <strings.h>
// MSVC doesn't have <strings.h>; it provides _stricmp in <string.h> instead.
#ifndef GUMBO_COMPAT_STRINGS_H
#define GUMBO_COMPAT_STRINGS_H

#ifdef _MSC_VER
#include <string.h>
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else
#include_next <strings.h>
#endif

#endif // GUMBO_COMPAT_STRINGS_H
