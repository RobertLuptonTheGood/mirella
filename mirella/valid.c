#include "mirella.h"
/*
 * Used for the validation suite, mirvalid.m
 *
 * interrupt test
 */
void 
test_interrupt()
{
    int i;
    int count = pop;
    
    cprint("Test of ^C interrupt\n");
    for(i=0;i<count;i++){
        scrprintf("%4d\r",i);
        if(dopause()) break;
    }
}

/*
 * u_char, shift, and mprintf test
 */
void 
test_uchar()
{
    char temp[100];
    char c = 65;
    int c1,c2;
    int iv1 = 1,iv2 = 2;
    char *string = "hello";
    float floatvar = 3.1415926536;
    double dvar = 137.0;
    
    sprintf(temp,"\nc = %d; ",c);
    cprint(temp);
    c |= 0x80;  /* set the sign bit */
    c1 = c;
    sprintf(temp,"c|0x80 = %d", c1);
    cprint(temp);
    cprint("\nif c|0x80 is NEGATIVE, your machine has signed chars");
    cprint("\nif POSITIVE, not, and you must #define DEFUCHAR in msysdef.h");
    cprint("\nand recompile at least mirgraph.c");
    c1 = -32768;
    c2 = c1>>8;
    sprintf(temp,"\n\n(-32768) >> 8 = i = %d",c2);
    cprint(temp);
    cprint(
"\nIf i is negative, your C/machine shifts arithmetically and all is OK;");
    cprint(
"\nIf positive, it shifts logically, and you must #define LOGCSHIFT");
    cprint(
"\nin msysdef.h and recompile at least mirarray,mirgraph,and mirellin");
    cprint("\n\npress any key to continue"); get1char(0);
    cprint(
"\n\nThe next test tests the mirella mprintf function, which assumes a");
cprint(
    "\ncommon stack ordering for function calls. If it does not work, try");
    cprint(
"\nchanging the definition of SDIR in mirellio.c to '-'. If that does not");
    cprint(
"\nwork, and your system supports va_args and vsprintf(), #define VARARGS in");
    cprint(
"\nmirella.h, and recompile mirellio.c. If none of the above, you are out of");
    cprint("\n luck.");    

    cprint("\n\nThe following is printed with mprintf, and should read:");
    cprint("\nn = 1 2 3; hello; pi = 3.14159 1/alpha=137"); 
    mprintf("\nn = %d %d %d; %s; pi = %7.5f 1/alpha=%g",iv1,iv2,3,string,
        floatvar,dvar);
    cprint("\n");
    fflush(stdout);
}
