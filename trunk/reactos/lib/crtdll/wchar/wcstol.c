unsigned long wcstoul(const wchar_t *cp,wchar_t **endp,unsigned int base)
{
	unsigned long result = 0,value;

	if (!base) {
		base = 10;
		if (*cp == L'0') {
			base = 8;
			cp++;
			if ((*cp == L'x') && iswxdigit(cp[1])) {
				cp++;
				base = 16;
			}
		}
	}
	while (iswxdigit(*cp) && (value = iswdigit(*cp) ? *cp-L'0' : (iswlower(*cp)
	    ? towupper(*cp) : *cp)-L'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (wchar_t *)cp;
	return result;
}

