#pragma once

#ifdef USE_WSTRING
# define LITERAL(str) L##str
#else // USE_WSTRING
# define LITERAL(str) str
#endif // USE_WSTRING
