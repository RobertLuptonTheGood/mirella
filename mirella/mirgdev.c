/*VERSION 88/10/24, Mirella 5.10*/

/******************** MIRGDEV.C ***************************************/
/* This module has various device-dependent hardcopy graphics output routines */
/* exported functions
void wpgraph(filename)       makes bitmap file from current graph suitable
                                for sending to a printer or sends bitmap
                                directly to printer, depending on system.
                                lq1500 & PCL on DSI and tosh 351 on VMS 
                                uvax are supported now
*/                                        


#ifdef VMS
#include mirella
#else
#include "mirella.h"
#endif
#ifdef DSI
#include <doscalls.h>
#endif


extern int g_xsize, g_ysize;   /* size of bit array in pixels. */
extern int g_xbsize;           /* size of x axis in bytes:((g_xsize-1)>>3)+1*/
extern u_char *gpointer;       /* pointer to body of graph array */
extern u_char **mgraph;        /* pointer to line pointers; graph byte is 
                                mgraph[y][x/8] */

int prmag = 0;                  /* magnification; # printer dots per bitmap
                                    dot if nonzero. default varies if zero. */
#ifdef TOSH351VMS

#define NPIN 24
#define ESC 27

static u_char bmmask[8] = {1,2,4,8,16,32,64,128};
/* to use with normal bitorder Mirella bitmaps, reverse order of mask */

#define NPRE 8
static char preamble[NPRE] = { ESC, '>', ESC, 'L', '0', '7', '\r', '\n' };
/* this preamble sets unidirectional printing, sets the spacing, and does
    a CR LF to get things going */
    
#define NHEAD 6
static char thead[NHEAD]= { ESC, '>', ESC, 'L','0','7'};
/* this header is send at the beginning of each packet to set unidirectional
 * print and set the spacing.	*/

#define NSKIP 5
#define NIMAGE 6
static char ins_im[2]	= { ESC, ';' };	/* Image transfer instruction	*/
static char ins_sk[2]	= { ESC, 'H' }; /* Relative horizontal skip	*/

#define NRET 8
static char retnor[NRET] = { ESC, 'L', '0', '8', ESC, '<', '\r', '\n' };
/* this postamble is sent to return the printer to its normal state after
 * transmission of the image */
 
static char ttail[2] = { '\r' , '\n' };
/* the CR LF are appended to each graphics packet to go to the next line */


/************** WPGRAPH(), VMS VAXSTATION/TOSHIBA P351 VERSION **************/

           /* this writes to the output file, extension .gts ; the file
           can be submitted directly to the print queue with the
           PASSALL qualifier. The magnification in the output is
           set with the Mirella variable prmag */
void
wpgraph(fname)
char *fname;
{
    char filename[64];
    void bmtodisk();
    
    extchk(fname,"gts",filename);
    bmtodisk(gpointer,g_xsize,g_ysize,(prmag ? prmag : 1),filename);
}

/*#define DEBUG*/

/********************** BMTODISK()****************************************/
/* This is the basic routine; no Mirella-specific variables; must have 
 *   bufcpy(d,s,n),clearn(n,buf).*/

void
bmtodisk(bmbase,xs,ys,mag,filename)
char *filename;
u_char *bmbase;    /* address of base of bitmap */
int xs, ys;               /* x and y sizes. xs MUST be a multiple of 8 */
int mag;                  /* magnification to printer */
{
    register int mgc; 
    register int j;
    register char *bmline, *tptr;
    int i,a,b,c,d;
    char *fline, *blkline, *bmlend, *bfline, *ftop;
    int lstripe;                 
    int k,l,s,bl,el,y; 
    int fdes;
    int dB,db,tmask; 
    int xsb = xs>>3;
    int xsmag= xs*mag;
    int ysmag = ys*mag;
    int nbyte,nblk;
    int nbtot;
    int nbold;
    int skip, blank, nload, ntrip;
    char *tailptr;    /* number of bytes and pointer to tail of last line */

    if( mag == 0) mag=1;   /* default */    
    if(xs != (xsb<<3)){
        scrprintf("\nX size MUST be a multiple of 8");
        return;
    }
#ifdef DEBUG
    scrprintf("\ncreating %s, xs,ys= %d %d, bmbase= %d",filename,
        xs,ys,bmbase); flushit();
#endif       
    /* open file */
    fdes = creat(filename,0775,"bls=512","mrs=512","rfm=fix");
    if(fdes == -1){
        scrprintf("\nCannot open %s for writing",filename);
        return;
    }

    /* allocate and init buffers */
    bfline = (char *)malloc(4*xsmag+1124);
    blkline = (char *)malloc(xsmag);
    if(fline==0 || blkline==0) /* erret("Cannot allocate stripe memory") */;
#ifdef DEBUG
    scrprintf("\nbfline,blkline=%d %d; clearing",bfline,blkline); flushit();
#endif
    clearn(xsmag,blkline);
    fline = bfline;
    strncpy(fline,preamble,NPRE);   /* copy preamble to beginning of buf */
    fline	+= NPRE;
            
    /* write header in fline*/
    strncpy(fline,thead,NHEAD);
    fline	+= NHEAD;
    
    /* diddle the bits */
    lstripe = (ysmag-1)/NPIN +1;  /* number of 24-pin stripes thru magn. pic*/
    for(s=0;s<lstripe;s++){
        clearn(4*xsmag,fline+NIMAGE);         /* clear buffer */
        bl = s*NPIN;                         /* beginning line */
        el = bl + NPIN ;                     /* ending line in grp of 24 */
#ifdef DEBUG
        scrprintf("\nstripe %d, lines %d to %d",s,bl,el);
        _pause();
#endif
        for(l=bl;l<el;l++){   /* l is line # in magn. graph, down from top */
            bmline = bmbase + xsb*((l)/mag);  /* source line in bitmap */
                /* to use with normal lineorder Mirella bitmaps, use
                   with (ysmag-1-l) instead of (l) in above line */
            bmlend = bmline + xsb;
            if(l>=ysmag) bmline = blkline;   /* point runover at null line */
            dB = (l%NPIN)/6;      /* byte in group of 4;first byte at top */
            db = 5 - (l%NPIN)%6;  /* bit in byte; msb at top */
            tmask = (1<<db)+64;   /* always set 6th bit */
            tptr = fline+dB+NIMAGE;	 /*target byte for first point
            					   in bmap line*/
            do{
                for(j=0;j<8;j++){
                    for(mgc=0;mgc<mag;mgc++){
                        if((~(*bmline))&(bmmask[j])) (*tptr) |= tmask;
                        else (*tptr) |= 64;
                        tptr += 4; /* point to targ byte for next point in bm*/
                    }
                }
                bmline++;
            }while(bmline < bmlend);
        }

/*	Filter the output.  At this point a full raster stripe has been
	converted to a printer stripe; however, it is likely that the
	stripe consists mainly of blank pixels.  Detect the blanks,
	skip over them and replace with horizontal head movement command.
	Repack the nonblank image pixels.  Pixels are checked in triplets
	of 24pin - 4 byte groups, as this is the smallest number of
	print locations that can be skipped over while maintaining
	image alignment.						*/

	skip	= 1;                    /* Now in blank region flag	*/
	nload	= 1;                    /* Number of triplets in region	*/
	ntrip	= (4 * xsmag)/12;	/* Number of complete triplets	*/
	tptr	= fline + NIMAGE;
	ftop	= fline;		/* Location in filtered line	*/
	for (j = 0; j < 12; j++)	/* Look at first triplet to	*/
		if (*tptr++ != 64)	/* initialize mode.		*/
			skip	= 0;	/* Image detected	*/
			
	for (i = 1; i < ntrip; i++) {
		blank	= 1;			/* Blank triplet flag	*/
		for (j = 0; j < 12; j++)
			if (*tptr++ != 64)
				blank	= 0;	/* Image detected	*/

/*	Check to see if a region as ended. If so, set up a head skip or
	image transfer command and pack the data, otherwise keep going.	*/
	
		if ((skip & blank) | (!skip & !blank))
			nload++;
		else if (skip) {
			skip	= 0;
			nload	*= 2;		/* 2 stops per triplet	*/
		        strncpy(ftop,ins_sk,2);
		        ftop	+= 2;
			*ftop++	= (nload%4096) /256  + '@';
			*ftop++	= (nload%256)  /16   + '@';
			*ftop++	= (nload%16)         + '@';
			nload	= 1;
		} else {
        		skip	= 1;
        		nload	*= 3;		/* 3 columns per triplet */ 
		        strncpy(ftop,ins_im,2);
		        ftop	+= 2;
			*ftop++	= (nload%10000)/1000 + '0';
			*ftop++	= (nload%1000) /100  + '0';
			*ftop++	= (nload%100)  /10   + '0';
			*ftop++	= (nload%10)         + '0';
			nload	*= 4;		/* 4 bytes per column	*/
			bufcpy(ftop,tptr-12-nload,nload);
			ftop	+= nload;
			nload	= 1;
		}
	}

/*	Finish up the line.  Pack up the final image data or move the
	head.  Send out any partial triplets as image data.		*/

	if (skip) {
		nload	*= 2;		/* 2 stops per triplet	*/
	        strncpy(ftop,ins_sk,2);
	        ftop	+= 2;
		*ftop++	= (nload%4096) /256  + '@';
		*ftop++	= (nload%256)  /16   + '@';
		*ftop++	= (nload%16)         + '@';
		nload	= 0;
	}
     	nload	*= 3;			/* 3 columns per triplet	*/ 
	if (!skip)
		tptr	-= 4 * nload;
	nload	+= (4 * xsmag - 12 * ntrip)/4;	/* Incomplete triplet	*/
	if (nload) {
        	strncpy(ftop,ins_im,2);
	       	ftop	+= 2;
		*ftop++	= (nload%10000)/1000 + '0';
		*ftop++	= (nload%1000) /100  + '0';
		*ftop++	= (nload%100)  /10   + '0';
		*ftop++	= (nload%10)         + '0';
		nload	*= 4;
		bufcpy(ftop,tptr,nload);
		ftop	+= nload;
	}

        strncpy(ftop,ttail,2);        /* add CR LF */
        ftop	+= 2;
	
	nbyte	= ftop - bfline;
        nblk = nbyte/512;             /* how many filled 512-byte blocks ? */
#ifdef DEBUG
        scrprintf("...writing %d bytes",nblk*512);
#endif
        for(i=0;i<nblk;i++) 
            write(fdes,bfline+i*512,512);
        tailptr = bfline + nblk*512;      /* pointer to leftovers */
        nbold = nbyte - (nblk * 512);   /* how many left ? */
        bufcpy(bfline,tailptr,nbold);   /* move to before buffer origin */
	fline	= bfline + nbold;
    }
    /* done; now finish writing last tail and reset printer  */
    clearn(1024,fline);
    strncpy(fline,retnor,NRET);
    if((nbold+NRET)<=512) write(fdes,fline-nbold,512);
    else{
        write(fdes,fline-nbold,512);
        write(fdes,fline-nbold+512,512);
    }
    /* clean house */
    close(fdes);
    free(bfline);    /* fixed feb 91 */
    free(blkline);
}

#endif


# ifdef PRINTRONIX   /* Printronix 300, for Palomar */


/************** WPGRAPH(), VMS /PRINTRONIX VERSION **********************/


void
wpgraph(fname)
char *fname;
{
    char filename[64];
    void bmtodisk();
    
    extchk(fname,"gts",filename);
    bmtodisk(gpointer,g_xsize,g_ysize,(prmag ? prmag : 1),filename);
}

           /* this writes to the output file, extension .gts ; the file
           can be submitted directly to the print queue with the
           PASSALL qualifier, and MUST BE. The magnification in the output is
           set with the Mirella variable prmag */

/*#define DEBUG*/

/********************** BMTODISK()****************************************/
/* This is the basic routine; no Mirella-specific variables; must have 
 *   bufcpy(d,s,n),clearn(n,buf).*/
static unsigned char mmask[8] = {1,2,4,8,16,32,64,128};
static unsigned char pmask[6] = {1,2,4,8,16,32};

void
bmtodisk(bmbase,xs,ys,mag,filename)
        char *filename;
    unsigned char *bmbase;    /* address of base of bitmap */
    int xs, ys;               /* x and y sizes. */
    int mag;                  /* magnification to printer */
{
    int i, j, k, l;
    int fdes;
    char *buf1;   
    char *buf2;
    int xsb = (xs-1)/8 + 1;
    int pxbsiz =  (xs*mag - 1)/6 +3;   /* # of bytes per line to send to 
                                        printer;  xs data + lf + grinit */
    int pysiz = ys*mag;                /* lines in printer graph */
    int yfill = (720-pysiz)/2;         /* lines fill to center graph, each
                                            3 bytes */
    int pxsiz = xs*mag;
    int fsize = ((ys*mag*pxbsiz + yfill*3)/512 + 1)*512;   
                                    /* file size in bytes; allows
                                                for final formfeed */
    char *lptr;                     /* pointer to current line in buffer */
    char *gptr;                     /* pointer in graph */
    
    int ig,jg,jc;
    char gdot,c,mby;
    char *pby, *pptr;
    int pbit;
    
    if(pysiz > 720) erret("\nBMTODISK: graph too big vertically");
    /* open file */
    fdes = creatblk(filename,fsize);
    if(fdes == -1){
        printf("\nCannot open %s for writing",filename);
        return;
    }

    /* allocate and init buffers */
    buf1 = (char *)malloc(fsize+2048);
    buf2 = (char *)malloc(pxbsiz+512);
    if(buf1 == 0 || buf2 == 0) erret("\nCannot allocate printer buffer memory");
    
    clearn(pxbsiz,buf2);
    buf2[0] = 0x05;
    buf2[2] = 0x0a;   /* blank line */
    pptr = buf1;
    for(i=0;i<yfill;i++){
        bufcpy(pptr,buf2,3);        
        pptr += 3;
    }
    printf("\nBuilding file:");
    printf("\n     lines remaining");
    for(ig=ys-1; ig>= 0; ig--){
        if(!(ig%5))printf("\r%4d",ig);
        gptr = bmbase + xsb*ig;    /* ptr to line in mirgraph */
        clearn(pxbsiz,buf2);
        buf2[0] = 0x05;               /* grinit */
        jc = 0;
        for(j=0;j<pxsiz;j++,jc++){
            if(jc == mag) jc=0;
            if(jc == 0){
                jg = j/mag;                 /* dot in mgraph */
                mby = *(gptr + (jg>>3));    /* byte in graph */
                gdot = mby & mmask[jg & 7]; /* nonzero if dot is on */
            }
            pby = buf2 + j/6 + 1;
            pbit = j%6;
            if(!pbit) *pby |= 0x40;   /* data byte */
            if(gdot){
                *pby |= pmask[pbit];
            }
        }                
        buf2[pxbsiz -1] = 0x0a;    /* lf to terminate */
        for(i=0;i<mag;i++){
            bufcpy(pptr,buf2,pxbsiz);
            pptr += pxbsiz;
        }
    }
    *pptr++ = 0x0c;                       /* formfeed to eject */
    clearn(512,pptr);                   /* clear the tail */
    /* write it */
    writeblk(fdes,buf1,pxbsiz*pysiz + 1);
    /* clean up */
    closeblk(fdes);
    free(buf1);
    free(buf2);
}

#endif


/* The following two drivers are for MSDOS machines, but it should not be too
much work to export to other systems */

#ifdef LQ1500
/***************** WPGRAPH(), DSI/LQ1500 VERSION ***************************/
/* output routine for LQ1500 and its ilk, normal Mirella bitmaps. 
 * Writes to file; 80286 outputs with host program 
 *
 *   printgraph filename (1)    1 if want double-strike printing
 *   
 * 1680x1260 is largest (4:3) size easily fitting on 8.5x11 page;
 * 1440x1044 is a little better (not exactly 4:3) and is matched to Herc(2h,3v).
 * g_xsize should be divisible by 8 and preferably by 24. Graph is vertical
 * on page, since the bitshuffling is negligible that way. No magnification
 * options, for the same reason. 
 */



# define ESC 27

void wpgraph(fname)   /* writes the graph to disk in a file with the
                                extension .glq, in which the bytes are
                                written in the y direction; ie each column
                                contains g_ysize bytes each placing 8 dots
                                in the x direction */
char *fname;
{
    register int i,j;
    int ix, n1,n2,ix3;
    int ofdes;
    FILE *ofp ;
    u_char *pbuffer;    
    u_char fhead[6];
    char filename[40];

    extchk(fname,"glq",filename); 
#if 0    
    if(!access(filename,1)){
        mprintf("\nFile exists. Overwrite? ");
        if(_yesno()) unlink(filename);
        else erret((char *)NULL);
    }
#endif
    if((ofdes = creatb(filename,0666)) == -1){
        mprintf("\nCannot create %s",filename);
        erret((char *)NULL);
    }
    if(!(pbuffer = (u_char *)malloc(g_ysize)))
    erret("\nCannot allocate buffer") ;
    /* write these in DEC/Intel order; will be read by IBMPC */
    fhead[0] = g_xsize & 255;
    fhead[1] = g_xsize >> 8;
    fhead[2] = g_ysize & 255;
    fhead[3] = g_ysize >> 8;
    fhead[4] = g_xbsize & 255;
    fhead[5] = g_xbsize >> 8;
    write(ofdes,fhead,6);
    write(ofdes,"gmagic",4);
    
    for(ix=0;ix<g_xbsize; ix++){
        for(j=0;j<g_ysize;j++){
            pbuffer[j] = mgraph[j][ix];
        }
        write(ofdes,pbuffer,g_ysize);
    }
out:
    close(ofdes);
    free(pbuffer);
}

#endif

/************************** PCL  **************************************/

#ifdef PCL
/* 300-dpi H-P PCL printers, Laserjet and Deskjet */
/* This code writes directly to printer and is for DOS-based systems */
#define ESC 27
#define BS   8
#define TAB  9
#define LF  10
#define FF  12
#define CR  13

static char *portrait  =    "\033&l0O";   /* select portrait orientation    */
static char *landscape =    "\033&l1O";   /*        landscape    "          */
static char *eject     =    "\033&l0H";   /* eject page                     */ 
static char *endraster =    "\033*rB";    /* end raster graphics            */
static char *setgres   =    "\033*tR";    /* set res; 75,100,150,300 goes   */
                                          /* before R                       */
static char *htabdots  =    "\033*pX";    /* tab n dots (300dpi) n after 'p'*/
static char *vtabdots  =    "\033*pY";    /* tab n dots (300dpi) n after 'p'*/
static char *setgwid   =    "\033*rS";    /* set width; #pts goes after 'r' */
static char *fullgmode =    "\033*b0M";   /* sel. full mode (all pts)       */
static char *cg1mode   =    "\033*b1M";   /* sel. Cpct Graphics mode 1      */
static char *cg2mode   =    "\033*b2M";   /* sel. Cpct Graphics mode 2      */
static char *graphleft =    "\033*r0A";   /* start raster graphics full left*/
static char *graphcur  =    "\033*r1A";   /* start raster graphics at curpos*/
static char *hoffset   =    "\033*bX";    /* hor offset n dots, n after 'b' */
static char *sendgdata =    "\033*bW";    /* hdr for bitmp line, n after 'b'*/


/*
 * this routine sets the i/o device represented by handle  'fd'
 * to binary mode. Required to make binary i/o useful, including 
 * graphics for printers. Does completely crazy things with ^Z
 * otherwise.
 */
 
static int 
bin_mode(fd)
int fd;
{
    R86 regs;                           /* defined in <doscalls.h> */

    regs.ax = 0x4400;                   /* read ioctl for binary */
    regs.bx = fd;                       /* handle to make binary */
    INT86(0x21,&regs);                  /* read ioctl status */
    regs.dx &= 0x00ff;                  /* set high order byte to 0 */
    regs.dx |= 0x20;                    /* set bit indicating binary mode */
    regs.ax = 0x4401;                   /* set ioctl status command */
    INT86(0x21,&regs);
}

static int pfdes=-1;  /* printer descriptor */

/*#define PDEBUG*/


#ifdef PDEBUG

static int cmdcnt=0;    /* debugging */
static char cmdstr[64] = {0,0}; /* " */

static void
wgstring(s,n)
char *s;
int n;
{
    int i;
    scrprintf("\nSending  :");
    for(i=0;i<n;i++) scrprintf("%02x:",s[i]);
    scrprintf(" = :%s:",s);
}

#endif

static int 
sendstr(str)
char *str;
{
    int len = strlen(str);
#ifdef PDEBUG
    wgstring(str,len);
#endif
    return write(pfdes,str,len);
}

static int 
sendbstr(str,n)
char *str;
int n;
{
    return write(pfdes,str,n);
}

static int 
sendnestr(str,n)   /* sends an esc seq, inserting ascii rep of n in
                        proper place (char 3)  */
char *str;
int n;
{
    char s[64];
    int len;
    
    strcpy(s,str);
    sprintf(s+3,"%d%c",n,str[3]);
    len = strlen(s);

#ifdef PDEBUG   
    if(strcmp(s,cmdstr)){
        wgstring(s,len);
        Debug("\n   1 times",s);
        cmdcnt=0;    
        strcpy(cmdstr,s);
    }else{
        if(!((++cmdcnt + 1)%8)){
            scrprintf("\r%4d",cmdcnt + 1);
        }
    }
#endif    

    return write(pfdes,s,len);
}

static void
sendchar(c)
int c;
{
#ifdef PDEBUG
    Debug("\nsending %d=:%c:",c,c);    
#endif
    write(pfdes,&c,1);
}
    
/* use these so changing to file output is easier, OK ? */

/********************* WPGRAPH() ***********************************/    

void gwpgraph(name)   /* does not eject at end */
char *name;        /* dummy */
{
    int i,j,k;
    int res;
    int margin;
    int topmargin;
    char restr[40];
    char gspec[40];
    int sline;
    int gxsz= g_xsize;
    int gysz= g_ysize;
    int gxbsz = g_xbsize;

    pfdes = open("prn",O_WRONLY);
    bin_mode(pfdes); 
    /* decide on resolution and offset..this logic is a little strange,
    since a forced magnification should take precedence, but in fact is OK;
    the ordering has the effect that a forced resolution which would
    result in a graph too big for the page is ignored. */
    
    if(gxsz>2400){
        mprintf("\nReducing x size to 2400");
        gxsz = 2400;
        gxbsz = 300;
    }else if(gxsz > 1200 || prmag == 1){
        res = 300;    
        margin = (2400 - gxsz)/2;
        topmargin = (2400 - gysz)/3;
    }else if(gxsz > 800 || prmag == 2){
        res = 150;
        margin = (2400 - 2*gxsz)/2;
        topmargin = (2400 - 2*gysz)/3;
    }else if(gxsz > 600 || prmag == 3){
        res = 100;
        margin = (2400 - 3*gxsz)/2;
        topmargin = (2400 - 3*gysz)/3;
    }else{
        res = 75;
        margin = (2400 - 4*gxsz)/2;
        topmargin = (3000 - 4*gysz)/3;
    }

    mprintf("\nResolution chosen =%d Margin = %d/300 in.",res,margin);

    /* send setup */
    sendstr("\n");      /* new line, keeps alignment if more than one graph
                         * on a page */
    sendstr(endraster); 
    sendstr(portrait); 
    sendnestr(setgres,res);
/*    sendnestr(setgwid,8*g_xbsize);  */
    sendstr(fullgmode);  /* for now */
    sendnestr(htabdots,margin);
    sendnestr(vtabdots,topmargin);
    sendstr(graphcur);
/*    sendstr(graphleft); */
    /* GO! */
    mprintf("\nSending graph\n       lines");
    for(i=g_ysize-1;i>=0;i--){
#ifndef PDEBUG        
        sline = g_ysize - i;
        if(!(sline %8)) scrprintf("\r%4d",sline);
#endif        
        sendnestr(sendgdata,gxbsz);
        sendbstr(mgraph[i],gxbsz);
    }
    /* stop */
    sendstr(endraster);
    close(pfdes);
}

void wpgraph(name)
char *name;
{
    pfdes = open("prn",O_WRONLY);
    gwpgraph(name);
    sendchar('\r');
    sendchar('\n');
    sendstr(eject);
    close(pfdes);
}



void wpstring(s)  /* send string to printer; appends newline if not there */
char *s;
{
    char sstr[256];
    int len;
    
    len = strlen(s);
    strncpy(sstr,s,255);
    sstr[255] = '\0';
    if(len > 253) len=253;
    if(sstr[len-1] != '\n' && sstr[len-1] != '\f'){
        sstr[len] = '\r';
        sstr[len+1] = '\n';
        sstr[len+2] = '\0';
    }
    pfdes = open("prn",O_WRONLY);
    sendstr(sstr);
    close(pfdes);
}
    
#endif

#ifdef NOGPRINT

void
wpgraph(filename)
char *filename;
{
    mprintf("\nwpgraph is not implemented on this system as yet. sorry");
}

void
gwpgraph(filename)
char *filename;
{
    mprintf("\ngwpgraph is not implemented on this system as yet. sorry");
}

void
wpstring(s)
char *s;
{
    mprintf("\nwpstring is not implemented on this system as yet. sorry");
}

#endif



