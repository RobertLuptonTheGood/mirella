/*VERSION 90/05/07: Mirella 5.50                                         */

/************************** MAPPL.C **************************************/

/*An demonstration application program module */
#ifdef VMS
#include mirella
#include mapp
#else
#include "mirella.h"
#include "mapp.h"
#endif

normal iarr[100];
short int shiarr[100];
char chararr[100];
float farr[100];
normal *iarrp;
short int *shiarrp;
char *chararrp;
float *farrp;

/*  declared in mapp.h
struct mix{
    short int x;
    short int y;
    float ff;
    char name[8];
};
*/
struct mix strucar[10];
struct mix *strucarp;
normal Sizmix = sizeof(struct mix);

normal *area ;
float floatvar = 3.14159;   /* note that external floats referenced by Mirella 
                    must be FLOATS, not DOUBLES */
int intvar = 137;
char greeting[] = "\nHello, out there";
int iv1 = 1;
int iv2 = 2;
double dvar = 137.;
normal **imatrx;


static char string[] =  "hello";

struct nonsense nstruct = { 1 , 2, 3, "nonsense" };

void applinit()
{
    int i,j;
    char buf[10];

    imatrx = (normal **)matrix(12,17,sizeof(normal)); 
    for(i=0;i<12;i++){
        for(j=0;j<17;j++)
            imatrx[i][j]=100*i + j;
    }                                         
    for(i=0; i<100; i++){
        iarr[i] = i;
        shiarr[i] = 2*i;
        chararr[i] = i/2;
        farr[i] = 0.1 * (float)i ;
    }
    iarrp = iarr;
    chararrp = chararr;
    shiarrp = shiarr;
    farrp = farr;
    for(i=0;i<10;i++){
        strucar[i].x = i;
        strucar[i].y = 2*i;
        strucar[i].ff = 0.2* (float)i;
        sprintf(buf,"i=%d",i);
        strcpy(strucar[i].name,buf);
    }
    strucarp = strucar;
        
    area = ap_env;
    cprint("\nInitializing Application Code"); 
}

void oldd_init()  /* does nothing in this trivial example; you might, for
                instance, like to initialize arrays in the application
                communication area, or reset some user variables, both of
                which are saved and read in with the dictionary image */
{
   cprint("\nInitializing the old dictionary image (null task)");
}

void ftimes()        /* PROCEDURE which multiplies two floats and 
                        pushes the product */
{
    double arg1;
    double arg2;

    arg1 = fpop;
    arg2 = fpop;
    fpush( arg1 * arg2 );

}

void ppower()          /* PROCEDURE which pushes the contents of the 
                    floating stack to the power of the contents of the
                    parameter stack on the float stack */
{
    int iarg1;
    double arg1;
    int i;
    double ret ;

    iarg1 = pop;
    arg1 = fpop;
    ret = 1.;
    for(i=0; i<iarg1; i++)
        ret *= arg1;

    fpush( ret );
}

double
power(farg,iarg)   /* function which does the same thing */
double farg;
int iarg;
{
    int i;
    double ret= 1.0 ;
    for(i=0; i<iarg; i++)
        ret *= farg;
    return ( ret );
}

void
hello(i) /* prints 'hello' i times */
int i;
{
    emit('\n');
    if(i>100) {
        cprint("Surely ");
        ndot(i);
        cprint(" is more hello's than you want");
        erret((char *)NULL);
    }
    while(i--)
        cprint(string);
}

char *
puttogether(s1,s2)    /* returns address of concatenation of s1,s2 */
char * s1, *s2;
{
    static char scat[200];

    strcpy(scat,s1);
    return strcat(scat,s2);
}

void _puttogether()  /* PROCEDURE which does the same thing */
{
    static char scat[200];
    char * s1 = cspop;
    char * s2 = cspop;

    strcpy(scat,s1);
    strcat(scat,s2);
    cspush(scat);
}

void 
test1()
{
    printf("\n%s","Hello, there");
#ifdef IBMORDER
    printf("\n%s\n"," I think the byte order is IBMORDER");
#else
    printf("\n%s\n"," I think the byte order is DEC-INTEL");
#endif
    printf("\n%s","Hello, there");
    flushit();
}

void 
test2()
{
}

void 
test3()
{
#ifdef NDP_PL
    short int mstat;
    short int deltax, deltay;
    int iter = 0;
       
    printf("\n");
    do{ 
        do{
            mouse(&mstat,&deltax,&deltay);
        }while (mstat == -1); 
        printf("\ri= %4d s=%d dx,dy= %4d %4d",iter, mstat, deltax, deltay);
        fflush(stdout);
        pause(out);
        iter++;
    }while(1);
out:
    return;    
#endif    
}

void
test4()
{
}

void 
test5()
{
}

void 
test6()
{
}

void 
test7()
{
}

void 
test8()
{
}

void 
test9()
{
}

void
test10()
{
}

