
#define upalpha ('A' - 'a')

int isalpha(char c)
{
   return(((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')));
}

int isspace(char c)
{
   return(c==' '||c=='\t');
}

char toupper(char c)
{
   if ((c>='a') && (c<='z')) return (c+upalpha);
   return(c);
}

int islower(char c)
{
   if ((c>='a') && (c<='z')) return 1;
   return 0;
}

int isdigit(char c)
{
   if ((c>='0') && (c<='9')) return 1;
   return 0;
}

int isxdigit(char c)
{
   if (((c>='0') && (c<='9')) || ((toupper(c)>='A') && (toupper(c)<='Z')))
     {
	return 1;
     }
   return 0;
}



