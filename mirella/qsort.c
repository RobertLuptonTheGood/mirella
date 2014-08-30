#include "mirella.h"
#ifdef DOS32

static int (*__comp)(), __size;
static u_char *__base, *__temp;
void _quick P_ARGS(( unsigned int, unsigned int ));
    

void 
qsort(base,number,size,compare)
Void *base;
unsigned int number,size;
int (*compare)();
{
    __comp = compare;
    __base = base;
    __size = size;
    if (size > 0) {
	if ((__temp = malloc(size)) == 0) {
	    printf("qsort: Out of memory.");
	    exit(1);
	}
	_quick(0,number-1);
	free(__temp);
    }
}

static void
_quick(lb,ub)
register unsigned int lb, ub;
{
    register unsigned int j, _rearr();
    
    if (lb < ub) {
	if ((j = _rearr(lb,ub)) != 0)
	    _quick(lb, j - 1);
	_quick(j + 1, ub);
    }
}

static unsigned int
_rearr(lb, ub)
register unsigned int lb, ub;
{
    do {
	while (ub > lb && (*__comp)(__base + __size * ub,
				    __base + __size * lb) >=0)
	    ub--;
	if (ub != lb) {
	    _swap(ub,lb);
	    while (lb < ub && (*__comp)(__base + __size * lb,
					__base + __size * ub) <=0)
		lb++;
	    if (lb != ub)
		_swap(lb,ub);
	}
    } while(lb != ub);
    return(lb);
}

static int
_swap(lb,ub)
register unsigned int lb, ub;
{
    register int lsize, x;
    register unsigned char *temp, *t1, *t2, *t3, *t4;
    register unsigned long temp4;
    register unsigned short int temp2;
    register unsigned char temp1;
    
    lsize = __size;

    t1 = __base + lsize * lb;
    t2 = __base + lsize * ub;
    
    switch (lsize) {
    
	case 1:
	    temp1 = *t1;
	    *t1 = *t2;
	    *t2 = temp1;
	    break;
	
	case 2:
	    temp2 = *(unsigned short int *)t1;
	    *(unsigned short int *)t1 = *(unsigned short int *)t2;
	    *(unsigned short int *)t2 = temp2;
	    break;
	    
	case 4:
	    temp4 = *(unsigned long *)t1;
	    *(unsigned long *)t1 = *(unsigned long *)t2;
	    *(unsigned long *)t2 = temp4;
	    break;
	    
	default:
	    t4 = temp = __temp;
	    t3 = t1;
	    for (x=lsize; x > 0; x--) *t4++ = *t1++;
	    
	    t4 = t3;
	    t1 = t2;
	    for (x=lsize; x > 0; x--) *t4++ = *t1++;
    
	    for (x=lsize; x > 0; x--) *t2++ = *temp++;
	    break;
    }
}
#endif
