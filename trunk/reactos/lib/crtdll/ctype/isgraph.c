/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/ctype.h>

#undef isgraph
int isgraph(int c)
{
  return _isctype(c,_GRAPH);
}

#undef iswgraph
int iswgraph(wint_t c)
{
	return iswctype(c,_GRAPH);
}
