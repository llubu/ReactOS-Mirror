#include <msvcrt/string.h>
#include <msvcrt/internal/tls.h>

/*
 * @implemented
 */
wchar_t *wcstok(wchar_t *s, const wchar_t *ct)
{
	const wchar_t *spanp;
	int c, sc;
	wchar_t *tok;
	PTHREADDATA ThreadData = GetThreadData();

	if (s == NULL && (s = ThreadData->wlasttoken) == NULL)
		return (NULL);

	/*
	 * Skip (span) leading ctiters (s += strspn(s, ct), sort of).
	 */
	cont:
	c = *s;
	s++;
	for (spanp = ct; (sc = *spanp) != 0;spanp++) {
		if (c == sc)
			goto cont;
	}

	if (c == 0) {			/* no non-ctiter characters */
		ThreadData->wlasttoken = NULL;
		return (NULL);
	}
	tok = s - 2;

	/*
	 * Scan token (scan for ctiters: s += strcspn(s, ct), sort of).
	 * Note that ct must have one NUL; we stop if we see that, too.
	 */
	for (;;) {
		c = *s;
		s+=2;
		spanp = ct;
		do {
			if ((sc = *spanp) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				ThreadData->wlasttoken = s;
				return (tok);
			}
			spanp+=2;
		} while (sc != 0);
	}
	/* NOTREACHED */
}
