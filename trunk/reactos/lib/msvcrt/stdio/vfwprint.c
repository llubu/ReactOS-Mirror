/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */

#ifdef __USE_W32API
#undef __USE_W32API
#endif

//#include <stdarg.h>
#include <msvcrt/stdarg.h> // robd
#include <msvcrt/crttypes.h> // robd

#include <msvcrt/stdio.h>
#include <msvcrt/malloc.h>
#include <msvcrt/internal/file.h>


int _isnanl(double x);
int _isinfl(double x);
int _isnan(double x);
int _isinf(double x);


int
__vfwprintf(FILE *fp, const wchar_t *fmt0, va_list argp);


/*
 * @implemented
 */
int
vfwprintf(FILE *f, const wchar_t *fmt, va_list ap)
{
	int len;
	wchar_t localbuf[BUFSIZ];

#if 0
	__fileno_lock(fileno(f));
#endif
	if (f->_flag & _IONBF) {
		f->_flag &= ~_IONBF;
		f->_ptr = f->_base = (char *)localbuf;
		f->_bufsiz = BUFSIZ;
		len = __vfwprintf(f,fmt,ap);
		(void)fflush(f);
		f->_flag |= _IONBF;
		f->_base = NULL;
		f->_bufsiz = 0;
		f->_cnt = 0;
	} else
		len = __vfwprintf(f,fmt,ap);
#if 0
	__fileno_unlock(fileno(f));
#endif
	return (ferror(f) ? EOF : len);
}



/*
 *  linux/lib/vsprintf.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/* vsprintf.c -- Lars Wirzenius & Linus Torvalds. */
/*
 * Wirzenius wrote this portably, Torvalds fucked it up :-)
 */

/*
 * Appropiated for the reactos kernel, March 1998 -- David Welch
 */

#include <msvcrt/stdarg.h>

#include <msvcrt/ctype.h>
#include <msvcrt/string.h>
#include <msvcrt/stdio.h>
#include <msvcrt/string.h>
#include <msvcrt/math.h>
#include <msvcrt/internal/ieee.h>


#define ZEROPAD		1	/* pad with zero */
#define SIGN		2	/* unsigned/signed long */
#define PLUS		4	/* show plus */
#define SPACE		8	/* space if plus */
#define LEFT		16	/* left justified */
#define SPECIAL		32	/* 0x */
#define LARGE		64	/* use 'ABCDEF' instead of 'abcdef' */
#define ZEROTRUNC	128	/* truncate zero 's */


static int skip_wtoi(const wchar_t **s)
{
	int i=0;

	while (iswdigit(**s))
		i = i*10 + *((*s)++) - L'0';
	return i;
}


static int do_div(LONGLONG *n,int base)
{
	int __res = ((ULONGLONG) *n) % (unsigned) base;
	*n = ((ULONGLONG) *n) / (unsigned) base;
	return __res;
}


static int number(FILE * f, LONGLONG num, int base, int size, int precision ,int type)
{
	wchar_t c,sign,tmp[66];
	const wchar_t *digits=L"0123456789abcdefghijklmnopqrstuvwxyz";
	int i, done = 0;

	if (type & LARGE)
		digits = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	if (type & LEFT)
		type &= ~ZEROPAD;
	if (base < 2 || base > 36)
		return done;
	c = (type & ZEROPAD) ? L'0' : L' ';
	sign = 0;
	if (type & SIGN) {
		if (num < 0) {
			sign = L'-';
			num = -num;
			size--;
		} else if (type & PLUS) {
			sign = L'+';
			size--;
		} else if (type & SPACE) {
			sign = L' ';
			size--;
		}
	}
	if (type & SPECIAL) {
		if (base == 16)
			size -= 2;
		else if (base == 8)
			size--;
	}
	i = 0;
	if (num == 0)
		tmp[i++]=L'0';
	else while (num != 0)
		tmp[i++] = digits[do_div(&num,base)];
	if (i > precision)
		precision = i;
	size -= precision;
	if (!(type&(ZEROPAD+LEFT)))
		while(size-->0)
		{
			if (putwc(L' ',f) == WEOF)
				return -1;
			done++;
		}

	if (sign)
	{
		if (putwc(sign,f) == WEOF)
			return -1;
		done++;
	}
	if (type & SPECIAL) {
		if (base==8) {
			if (putwc(L'0',f) == WEOF)
				return -1;
			done++;
		}
		else if (base==16) {
		    	if (putwc(L'0', f) == WEOF)
				return -1;
			done++;
			if (putwc(digits[33],f) == WEOF)
				return -1;
			done++;
		}
	}
	if (!(type & LEFT))
		while (size-- > 0)
		{
			if (putwc(c,f) == WEOF)
				return -1;
			done++;
		}
	while (i < precision--)
	{
		if (putwc(L'0', f) == WEOF)
			return -1;
		done++;
	}
	while (i-- > 0)
	{
		if (putwc(tmp[i],f) == WEOF)
			return -1;
		done++;
	}
	while (size-- > 0)
	{
		if (putwc(L' ', f) == WEOF)
			return -1;
		done++;
	}
	return done;
}


static int numberf(FILE * f, double __n, wchar_t exp_sign,  int size, int precision, int type)
{
	double exponent = 0.0;
	double e;
	long ie;

	//int x;
	char *buf, *tmp;
	int i = 0;
	int j = 0;
	//int k = 0;

	double frac, intr;
	double p;
	wchar_t sign;
	wchar_t c;
	char ro = 0;
	int result, done = 0;

	double_t *n = (double_t *)&__n;

	if ( exp_sign == L'g' || exp_sign == L'G' || exp_sign == L'e' || exp_sign == L'E' ) {
		ie = ((unsigned int)n->exponent - (unsigned int)0x3ff);
		exponent = ie/3.321928;
	}

	if ( exp_sign == L'g' || exp_sign == L'G' ) {
		type |= ZEROTRUNC;
		if ( exponent < -4 || fabs(exponent) >= precision )
			 exp_sign -= 2; // g -> e and G -> E
	}

	if ( exp_sign == L'e' ||  exp_sign == L'E' ) {
		frac = modf(exponent,&e);
		if ( frac > 0.5 )
			e++;
		else if ( frac < -0.5 )
			e--;

		result = numberf(f,__n/pow(10.0L,e),L'f',size-4, precision, type);
		if (result < 0)
			return -1;
		done += result;
		if (putwc( exp_sign,f) == WEOF)
			return -1;
		done++;
		size--;
		ie = (long)e;
		type = LEFT | PLUS;
		if ( ie < 0 )
			type |= SIGN;

		result = number(f,ie, 10,2, 2,type );
		if (result < 0)
			return -1;
		done += result;
		return done;
	}

	if ( exp_sign == 'f' ) {
		buf = alloca(4096);
		if (type & LEFT) {
			type &= ~ZEROPAD;
		}

		c = (type & ZEROPAD) ? L'0' : L' ';
		sign = 0;
		if (type & SIGN) {
			if (__n < 0) {
				sign = L'-';
				__n = fabs(__n);
				size--;
			} else if (type & PLUS) {
				sign = L'+';
				size--;
			} else if (type & SPACE) {
				sign = L' ';
				size--;
			}
		}

		frac = modf(__n,&intr);

		// # flags forces a . and prevents trucation of trailing zero's

		if ( precision > 0 ) {
			//frac = modfl(__n,&intr);
			i = precision-1;
			while ( i >= 0  ) {
				frac*=10.0L;
				frac = modf(frac, &p);
				buf[i] = (int)p + L'0';
				i--;
			}
			i = precision;
			size -= precision;

			ro = 0;
			if ( frac > 0.5 ) {
				ro = 1;
			}

			if ( precision >= 1 || type & SPECIAL) {
				buf[i++] = '.';
				size--;
			}
		}

		if ( intr == 0.0 ) {
			buf[i++] = L'0';
			size--;
		}
		else {
			while ( intr > 0.0 ) {
				p = intr;
                                intr/=10.0L;
				modf(intr, &intr);

				p -= 10.0*intr;

				buf[i++] = (int)p + L'0';
				size--;
			}
		}

		j = 0;
		while ( j < i && ro == 1 ) {
			if ( buf[j] >= L'0' && buf[j] <= L'8' ) {
				buf[j]++;
				ro = 0;
			}
			else if ( buf[j] == L'9' ) {
				buf[j] = L'0';
			}
			j++;
		}
		if ( ro == 1 )
			buf[i++] = L'1';

		buf[i] = 0;

		size -= precision;
		if (!(type&(ZEROPAD+LEFT)))
			while(size-->0)
			{
				if (putwc(L' ',f) == WEOF)
					return -1;
				done++;
			}
		if (sign)
		{
			if (putwc( sign,f) == WEOF)
				return -1;
			done++;
		}

		if (!(type&(ZEROPAD+LEFT)))
			while(size-->0)
			{
				if (putwc(L' ',f) == WEOF)
					return -1;
				done++;
			}
		if (type & SPECIAL) {
		}

		if (!(type & LEFT))
			while (size-- > 0)
			{
				if (putwc(c,f) == WEOF)
					return -1;
				done++;
			}

		tmp = buf;
		if ( type & ZEROTRUNC && ((type & SPECIAL) != SPECIAL) ) {
			j = 0;
			while ( j < i && ( *tmp == L'0' || *tmp == L'.' )) {
					tmp++;
					i--;
			}
		}
//		else
//			while (i < precision--)
//				putwc(L'0', f);
		while (i-- > 0)
		{
			if (putwc(tmp[i],f) == WEOF)
				return -1;
			done++;
		}
		while (size-- > 0)
		{
			if (putwc(L' ', f) == WEOF)
				return -1;
			done++;
		}
	}
	return done;
}


static int numberfl(FILE * f, long double __n, wchar_t exp_sign,  int size, int precision, int type)
{
	long double exponent = 0.0;
	long double e;
	long ie;

	//int x;
	wchar_t *buf, *tmp;
	int i = 0;
	int j = 0;
	//int k = 0;

	long double frac, intr;
	long double p;
	wchar_t sign;
	wchar_t c;
	char ro = 0;

	int result, done = 0;

	long_double_t *n = (long_double_t *)&__n;

	if ( exp_sign == L'g' || exp_sign == L'G' || exp_sign == L'e' || exp_sign == L'E' ) {
		ie = ((unsigned int)n->exponent - (unsigned int)0x3fff);
		exponent = ie/3.321928;
	}

	if ( exp_sign == L'g' || exp_sign == L'G' ) {
		type |= ZEROTRUNC;
		if ( exponent < -4 || fabs(exponent) >= precision ) 
			 exp_sign -= 2; // g -> e and G -> E
	}

	if ( exp_sign == L'e' || exp_sign == L'E' ) {
		frac = modfl(exponent,&e);
		if ( frac > 0.5 )
			e++;
		else if ( frac < -0.5 )
			e--;

		result = numberf(f,__n/powl(10.0L,e),L'f',size-4, precision, type);
		if (result < 0)
			return -1;
		done += result; 
		if (putwc( exp_sign,f) == WEOF)
			return -1;
		done++;
		size--;
		ie = (long)e;
		type = LEFT | PLUS;
		if ( ie < 0 )
			type |= SIGN;

		result = number(f,ie, 10,2, 2,type );
		if (result < 0)
			return -1;
		done += result;
		return done;
	}

	if ( exp_sign == L'f' ) {
		buf = alloca(4096);
		if (type & LEFT) {
			type &= ~ZEROPAD;
		}

		c = (type & ZEROPAD) ? L'0' : L' ';
		sign = 0;
		if (type & SIGN) {
			if (__n < 0) {
				sign = L'-';
				__n = fabs(__n);
				size--;
			} else if (type & PLUS) {
				sign = L'+';
				size--;
			} else if (type & SPACE) {
				sign = L' ';
				size--;
			}
		}

		frac = modfl(__n,&intr);

		// # flags forces a . and prevents trucation of trailing zero's
		if ( precision > 0 ) {
			//frac = modfl(__n,&intr);
	
			i = precision-1;
			while ( i >= 0  ) {
				frac*=10.0L;
				frac = modfl((long double)frac, &p);
				buf[i] = (int)p + L'0';
				i--;
			}
			i = precision;
			size -= precision;

			ro = 0;
			if ( frac > 0.5 ) {
				ro = 1;
			}

			if ( precision >= 1 || type & SPECIAL) {
				buf[i++] = L'.';
				size--;
			}
		}

		if ( intr == 0.0 ) {
			buf[i++] = L'0';
			size--;
		}
		else {
			while ( intr > 0.0 ) {
				p = intr;
				intr/=10.0L;
				modfl(intr, &intr);

				p -=10.0*intr;

				buf[i++] = (int)p + L'0';
				size--;
			}
		}

		j = 0;
		while ( j < i && ro == 1) {
			if ( buf[j] >= L'0' && buf[j] <= L'8' ) {
				buf[j]++;
				ro = 0;
			}
			else if ( buf[j] == L'9' ) {
				buf[j] = L'0';
			}
			j++;
		}
		if ( ro == 1 )
			buf[i++] = L'1';

		buf[i] = 0;

		size -= precision;
		if (!(type&(ZEROPAD+LEFT)))
			while(size-->0)
				putwc(L' ',f);
		if (sign)
			putwc(sign,f);

		if (!(type&(ZEROPAD+LEFT)))
			while(size-->0)
				putwc(L' ',f);
		if (type & SPECIAL) {
		}

		if (!(type & LEFT))
			while (size-- > 0)
				putwc(c,f);

		tmp = buf;
		if ( type & ZEROTRUNC && ((type & SPECIAL) != SPECIAL) ) {
			j = 0;
			while ( j < i && ( *tmp == L'0' || *tmp == L'.' )) {
				tmp++;
				i--;
			}
		}
//		else
//			while (i < precision--)
//				    putc( '0', f);
		while (i-- > 0)
		{
			if (putwc(tmp[i],f) == WEOF)
				return -1;
			done++;
		}
		while (size-- > 0)
		{
			if (putwc(L' ', f) == WEOF)
				return -1;
			done++;
		}
	}
	return done;
}

static int string(FILE *f, const char* s, int len, int field_width, int precision, int flags)
{
	int i, done = 0;
	if (s == NULL)
	{
		s = "<NULL>";
		len = 6;
	}
	else
	{
		if (len == -1)
		{
			len = 0;
			while (s[len] && (unsigned int)len < (unsigned int)precision)
				len++;
		}
		else
		{
			if ((unsigned int)len > (unsigned int)precision)
				len = precision;
		}
	}
	if (!(flags & LEFT))
		while (len < field_width--)
		{
			if (putwc(L' ', f) == WEOF)
				return -1;
			done++;
		}
	for (i = 0; i < len; ++i)
	{
		if (putwc(*s++, f) == WEOF)
			return -1;
		done++;
	}
	while (len < field_width--)
	{
		if (putwc(L' ', f) == WEOF)
			return -1;
		done++;
	}
	return done;
}

static int stringw(FILE *f, const wchar_t* sw, int len, int field_width, int precision, int flags)
{
	int i, done = 0;
	if (sw == NULL)
	{
		sw = L"<NULL>";
		len = 6;
	}
	else
	{
		if (len == -1)
		{
			len = 0;
			while (sw[len] && (unsigned int)len < (unsigned int)precision)
				len++;
		}
		else
		{
			if ((unsigned int)len > (unsigned int)precision)
				len = precision;
		}
	}
	if (!(flags & LEFT))
		while (len < field_width--)
		{
			if (putwc(L' ', f) == WEOF)
				return -1;
			done++;
		}
	for (i = 0; i < len; ++i)
	{
		if (putwc(*sw++, f) == WEOF)
			return -1;
		done++;
	}
	while (len < field_width--)
	{
		if (putwc(L' ', f) == WEOF)
			return -1;
		done++;
	}
	return done;
}

int __vfwprintf(FILE *f, const wchar_t *fmt, va_list args)
{
	int len;
	ULONGLONG num;
	int base;
	long double _ldouble;
	double _double;
	const char *s;
	const wchar_t* sw;
	int result, done = 0;

	int flags;		/* flags to number() */

	int field_width;	/* width of output field */
	int precision;		/* min. # of digits for integers; max
				   number of chars for from string */
	int qualifier = 0;	/* 'h', 'l', 'L' or 'I64' for integer fields */

	for (; *fmt ; ++fmt) {
		if (*fmt != L'%') {
			if (putwc(*fmt,f) == WEOF)
				return -1;
			done++;
			continue;
		}

		/* process flags */
		flags = 0;
		repeat:
			++fmt;		/* this also skips first '%' */
			switch (*fmt) {
				case L'-': flags |= LEFT; goto repeat;
				case L'+': flags |= PLUS; goto repeat;
				case L' ': flags |= SPACE; goto repeat;
				case L'#': flags |= SPECIAL; goto repeat;
				case L'0': flags |= ZEROPAD; goto repeat;
				}

		/* get field width */
		field_width = -1;
		if (isxdigit(*fmt))
			field_width = skip_wtoi(&fmt);
		else if (*fmt == L'*') {
			++fmt;
			/* it's the next argument */
			field_width = va_arg(args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		/* get the precision */
		precision = -1;
		if (*fmt == L'.') {
			++fmt;
			if (iswdigit(*fmt))
				precision = skip_wtoi(&fmt);
			else if (*fmt == L'*') {
				++fmt;
				/* it's the next argument */
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		/* get the conversion qualifier */
		qualifier=0;
		// %Z can be just stand alone or as size_t qualifier
		if ( *fmt == 'Z' ) {
			qualifier = *fmt;
			switch ( *(fmt+1)) {
				case L'o':
				case L'b':
				case L'X':
				case L'x':
				case L'd':
				case L'i':
				case L'u':
					++fmt;
					break;
				default:
					break;
			}
		} else if (*fmt == L'h' || *fmt == L'l' || *fmt == L'L' || *fmt == L'w') {
			qualifier = *fmt;
			++fmt;
		} else if (*fmt == L'I' && *(fmt+1) == L'6' && *(fmt+2) == L'4') {
			qualifier = *fmt;
			fmt += 3;
		}

		// go fine with ll instead of L
		if ( *fmt == L'l' ) {
			++fmt;
			qualifier = L'L';
		}

		/* default base */
		base = 10;

		switch (*fmt) {
		case L'c': /* finished */
			if (!(flags & LEFT))
				while (--field_width > 0)
				{
					if (putwc(L' ', f) == WEOF)
						return -1;
					done++;
				}
			if (qualifier == L'h')
			{
				if (putwc((wchar_t) va_arg(args, int), f) == WEOF)
					return -1;
			}
			else
			{
				if (putwc((wchar_t) va_arg(args, int), f) == WEOF)
					return -1;
			}
			done++;
			while (--field_width > 0)
			{
				if (putwc(L' ', f) == WEOF)
					return -1;
				done++;
			}
			continue;

		case L'C': /* finished */
			if (!(flags & LEFT))
				while (--field_width > 0)
				{
					if (putwc(L' ', f) == WEOF)
						return -1;
					done++;
				}
			if (qualifier == L'l' || qualifier == L'w')
			{
				if (putwc((unsigned char) va_arg(args, int), f) == WEOF)
					return -1;
			}
			else
			{
				if (putwc((unsigned char) va_arg(args, int), f) == WEOF)
					return -1;
			}
			done++;
			while (--field_width > 0)
			{
				if (putwc(L' ', f) == WEOF)
					return -1;
				done++;
			}
			continue;

		case L's': /* finished */
			if (qualifier == L'h') {
				/* print ascii string */
				s = va_arg(args, char *);
				result = string(f, s, -1, field_width, precision, flags);
			} else {
				/* print unicode string */
				sw = va_arg(args, wchar_t *);
				result = stringw(f, sw, -1, field_width, precision, flags);
			}
			if (result < 0)
				return -1;
			done += result;
			continue;

		case L'S':
			if (qualifier == L'l' || qualifier == L'w') {
				/* print unicode string */
				sw = va_arg(args, wchar_t *);
				result = stringw(f, sw, -1, field_width, precision, flags);
			} else {
				/* print ascii string */
				s = va_arg(args, char *);
				result = string(f, s, -1, field_width, precision, flags);
			}
			if (result < 0)
				return -1;
			done += result;
			continue;

		case L'Z': /* finished */
			if (qualifier == L'w') {
				/* print counted unicode string */
				PUNICODE_STRING pus = va_arg(args, PUNICODE_STRING);
				if ((pus == NULL) || (pus->Buffer)) {
					sw = NULL;
					len = -1;
				} else {
					sw = pus->Buffer;
				}
				result = stringw(f, sw, len, field_width, precision, flags);
			} else {
				/* print counted ascii string */
				PANSI_STRING pus = va_arg(args, PANSI_STRING);
				if ((pus == NULL) || (pus->Buffer)) {
					s = NULL;
					len = -1;
				} else {
					s = pus->Buffer;
					len = pus->Length;
				}
				result = string(f, s, len, field_width, precision, flags);
			}
			if (result < 0)
				return -1;
			done += result;
			continue;

		case L'e': /* finished */
		case L'E':
		case L'f':
		case L'g':
		case L'G':
			if (qualifier == L'l' || qualifier == L'L' ) {
				_ldouble = va_arg(args, long double);
			
				if ( _isnanl(_ldouble) ) {
					sw = L"Nan";
					len = 3;
					while ( len > 0 ) {
						if (putwc(*sw++,f) == WEOF)
							return -1;
						done++;
						len --;
					}
				}
				else if ( _isinfl(_ldouble) < 0 ) {
					sw = L"-Inf";
					len = 4;
					while ( len > 0 ) {
						if (putwc(*sw++,f) == WEOF)
							return -1;
						done++;
						len --;
					}
				}
				else if ( _isinfl(_ldouble) > 0 ) {
					sw = L"+Inf";
					len = 4;
					while ( len > 0 ) {
						if (putwc(*sw++,f) == WEOF)
							return -1;
						done++;
						len --;
					}
				} else {
					if ( precision == -1 )
						precision = 6;
					result = numberfl(f,_ldouble,*fmt,field_width,precision,flags);
					if (result < 0)
						return -1;
					done += result;
				}
			} else {
				_double = (double)va_arg(args, double);

				if ( _isnan(_double) ) {
					sw = L"Nan";
					len = 3;
					while ( len > 0 ) {
						if (putwc(*sw++,f) == WEOF)
							return -1;
						done++;
						len --;
					}
				} else if ( _isinf(_double) < 0 ) {
					sw = L"-Inf";
					len = 4;
					while ( len > 0 ) {
						if (putwc(*sw++,f) == WEOF)
							return -1;
						done++;
						len --;
					}
				} else if ( _isinf(_double) > 0 ) {
					sw = L"+Inf";
					len = 4;
					while ( len > 0 ) {
						if (putwc(*sw++,f) == WEOF)
							return -1;
						done++;
						len --;
					}
				} else {
					if ( precision == -1 )
						precision = 6;
					result = numberf(f,_double,*fmt,field_width,precision,flags);
					if (result < 0)
						return -1;
					done += result;
				}
			}
			continue;

		case L'p':
			if (field_width == -1) {
				field_width = 2*sizeof(void *);
				flags |= ZEROPAD;
			}
			result = number(f,
				        (unsigned long) va_arg(args, void *), 16,
					field_width, precision, flags);
			if (result < 0)
				return -1;
			done += result;
			continue;

		case L'n':
			if (qualifier == L'l') {
				long * ip = va_arg(args, long *);
				*ip = 0;
			} else {
				int * ip = va_arg(args, int *);
				*ip = 0;
			}
			continue;

		/* integer number formats - set up the flags and "break" */
		case L'o':
			base = 8;
			break;

		case L'b':
			base = 2;
			break;

		case L'X':
			flags |= LARGE;
		case L'x':
			base = 16;
			break;

		case L'd':
		case L'i':
			flags |= SIGN;
		case L'u':
			break;

		default:
			if (*fmt != L'%')
			{
				if (putwc(L'%', f) == WEOF)
					return -1;
				done++;
			}
			if (*fmt)
			{
				if (putwc(*fmt, f) == WEOF)
					return -1;
				done++;
			}
			else
				--fmt;
			continue;
		}

		if (qualifier == L'I')
			num = va_arg(args, ULONGLONG);
		else if (qualifier == L'l')
			num = va_arg(args, unsigned long);
		else if (qualifier == L'h') {
			if (flags & SIGN)
				num = va_arg(args, int);
			else
				num = va_arg(args, unsigned int);
		}
		else if (flags & SIGN)
			num = va_arg(args, int);
		else
			num = va_arg(args, unsigned int);
		result = number(f, num, base, field_width, precision, flags);
		if (result < 0)
			return -1;
		done += result;
	}
	//putwc(L'\0',f);
	return done;
}

/* EOF */
