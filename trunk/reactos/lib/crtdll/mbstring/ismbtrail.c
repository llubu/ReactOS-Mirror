/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/mbstring/ismblead.c
 * PURPOSE:     Checks for a trailing byte 
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <crtdll/mbstring.h>

  iskanji2()   :   �V�t�g JIS �R�[�h��2�o�C�g��(0x40 <= c <= 0x7E � ��
                     ����0x80  <=  c <= 0xFC) ���ǂ���

int _ismbbtrail(unsigned int c)
{
	return ((_jctype+1)[(unsigned char)(c)] & _KNJ_2);
}

//int _ismbbtrail( unsigned int b)
//{
//	return ((b >= 0x40 && b <= 0x7e ) || (b >= 0x80 && b <= 0xfc ) );
//}


int _ismbstrail( const unsigned char *str, const unsigned char *t)
{
	char *s = str;
	while(*s != 0 && s != t) 
	{
		
		s+= mblen(*s);
	}
		
	return _ismbbtrail( *s)
}