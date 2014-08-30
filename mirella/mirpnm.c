/****************************************************************************/
/****************************************************************************/
/* MIRPNM.C    PBM, PGM, PPM SUPPORT CODE FOR MIRELLA GRAPHICS AND IMAGES ***/
/****************************************************************************/
/****************************************************************************/

#include "mirella.h"
#include "graphs.h"
#include "images.h"

void gallocgraph();

#if 0   /* in graphs.h */
struct pnm_head_a{
    char pnm_magicnos[8];
    char pnm_xsizes[8];
    char pnm_ysizes[8];
    char pnm_maxgreys[8];
};


struct pnm_head_i{
    int pnm_magicno;
    int pnm_xsize;
    int pnm_ysize;
    int pnm_maxgrey;
    int pnm_data;
};
#endif

struct pnm_head_i pnm_ihead;
struct pnm_head_a pnm_ahead;

/* ftp at oak.oakland.edu: pub/msdos/djgpp, pub/msdos/graphics  
    pbmp191d.zip is full pbmplus distribution. */


/* jeg0003 */

#if 0
Notes on formats: A ppm, pbm, pgm header consists of a set of ascii
lines (sep by whitespace, not necc. newlines or cr/newlines:

P3  (magic number, P1 P2 P3 for pbm, pgm, ppm ascii, P4 P5 P6 for binary
# comments
78 54 (image width and height in pixels )
255   (maxgrey, or max color for pnm, but only one entry for pnm; 
            no entry for pbm)
1 character of whitespace
Data-- bytes with MSb first, and ( I think, MSb=leftmost dot in pbm)
ppm data is R-G-B

For DOS files, use spaces to be sure, bcz cf/lf pairs are 2 bytes
#endif

/************** ALLOCATION CODE ********************************************/

/* call for monochrome bitmap graph (same as allocgraph) */
void
allocpbmgraph(xs,ys,n)
int xs,ys;
int n;
{
    gallocgraph(xs,ys,n,0,1);
}

/* call for grey pgm graph */
void
allocpgmgraph(xs,ys,n)
int xs,ys;
int n;
{
    gallocgraph(xs,ys,n,0,8);
}

/* call for 24-bit ppm graph */
void
allocppmgraph(xs,ys,n)
int xs,ys;
int n;
{
    gallocgraph(xs,ys,n,0,24);
}

/************************ VERIFICATION CODE ********************************/

mirella int
gispbm()
{
    return (g_isalloc(gchan) && g_bitscol == 1);
}

mirella int 
gispgm()
{
    return(g_isalloc(gchan) && g_bitscol == 8);    
}

mirella int
gisppm()
{
    return(g_isalloc(gchan) && g_bitscol == 24);
}

/********************** HEADER STUFF ***************************************/

/************************* GETPNMTOK() *************************************/
/* This routine does all the work of reading PNM headers and ascii files.
 * it returns a count of the number of comment characters it has collected
 * in commentptr or zero if none. If it finds no token, str is set to null.
 * if commentptr is NULL, ignores comments.
 * if token pointer is null, returns position in file; this is the data offset;
 * the call is made after the last token is obtained.
 */

/* made global jeg0005*/ 

mirella int   
getpnmtok(str,commentptr)
char *str;
char *commentptr;
{
    char commentstr[512];
    char *cp = str;
    int c;
    int len=0;
    int clen;
    int tclen = 0;
    
 

    if(str == NULL){
        return mtell();
    }

    *str = '\0';        
    do{
        c = mfgetc();
        if(rdeof){
            *cp++ = '\0';
            return tclen;
        }
        if(!isspace(c) && c != '#'){
            *cp++ = c;
            len++;
            if(len == 7) erret("\nGETPNMTOK: Token too long");
        }else if(len != 0){   /* got something; end whitespace */
            *cp = '\0';
            return tclen;  /* success, not comment */
        }
            
        if(c == '#'){  /* comment */
            mfgets(commentstr,512);
            clen = strlen(commentstr);
            strcpy(commentstr + clen,EOL);
            if(commentptr && tclen + clen + LEOL < 512){
                strcpy(commentptr + tclen,commentstr);
                tclen += (clen+LEOL);
            }
            if(rdeof) return(tclen);
        }
    }while(1);
}

/********************* GETPNMHEAD() **************************************/
/* This one uses arbitrary structures */

mirella char *
ggetpnmhead(file,pa,pi)
char *file;
struct pnm_head_a *pa;
struct pnm_head_i *pi;
{
    int fchan;
    char tokstr[10];
    int m;
    /*char *cp;*/
    int cco;
    static char comment[2048];
    char *comptr = comment;
    int c;
    
    fchan = mopen(file,0);
    pushchan(fchan);
    
    cco = getpnmtok(tokstr,comptr);
    comptr += cco;

    if((((c=*tokstr) != 'P') && (c != 'p') ) || (m=atoi(tokstr+1)) < 1 || m>6){
        erret("\nGETPNMHEAD:Not a legal pnm file");
    }
    *tokstr = 'P';
    strcpy(pa->pnm_magicnos,tokstr);
    pi->pnm_magicno = m;
    

    cco = getpnmtok(tokstr,comptr);
    comptr += cco;

    if(*tokstr != '\0'){
        strcpy(pa->pnm_xsizes,tokstr);
        pi->pnm_xsize = atoi(tokstr);
    }else{
        erret("\nGETPNMHEAD: No valid x size");
    }

    cco = getpnmtok(tokstr,comptr);
    comptr += cco;

    if(*tokstr != '\0'){
        strcpy(pa->pnm_ysizes,tokstr);
        pi->pnm_ysize = atoi(tokstr);
    }else{
        erret("\nGETPNMHEAD: No valid y size");
    }

    if(m != 1 && m != 4){  /* PGM or PPM */
        cco = getpnmtok(tokstr,comptr);
        comptr += cco;
    
        if(*tokstr != '\0'){
            strcpy(pa->pnm_maxgreys,tokstr);
            pi->pnm_maxgrey = atoi(tokstr);
        }else{
            erret("\nGETPNMHEAD: No valid maxgrey");
        }
    }

    pi->pnm_data = getpnmtok(NULL,NULL);
    popchan();
    return comment;
}

/* this one uses pnm_ahead, phm_ihead */

mirella char *
getpnmhead(file)
char *file;
{
    return ggetpnmhead(file,&pnm_ahead,&pnm_ihead);
}

/******************* MAKEPBMHEAD(), MAKEPNMHEAD() **************************/
/* 
 * Makes ascii headers for pbm, pnm files; entries are sep by spaces, 
 * terminated with a newline; pbm header is exactly 16 chars long, pnm
 * header 20 chars long.
 */
 
mirella char * 
makepbmhead(xsize,ysize)
int xsize;
int ysize;
{
    static char header[16];
    int i;
    int xlen,ylen;
    
    strcpy(pnm_ahead.pnm_magicnos,"P4");
    sprintf(pnm_ahead.pnm_xsizes,"%d",g_xsize);
    sprintf(pnm_ahead.pnm_ysizes,"%d",g_ysize);
    for(i=0;i<16;i++) header[i] = ' ';
    strncpy(header,pnm_ahead.pnm_magicnos,2);
    
    xlen = strlen(pnm_ahead.pnm_xsizes);
    if(xlen > 5) erret("\nMAKEPBMHEAD:x size too big");
    strncpy(header+3,pnm_ahead.pnm_xsizes,xlen);
    
    ylen = strlen(pnm_ahead.pnm_ysizes);
    if(ylen > 5) erret("\nMAKEPBMHEAD:y size too big");    
    strncpy(header + 15 - ylen,pnm_ahead.pnm_ysizes,ylen);
    
    header[15]='\n';
    return header;
}


mirella char * 
makepnmhead(magicno,xsz,ysz,maxgrey)
int magicno;
int xsz;
int ysz;
int maxgrey;
{
    static char header[20];
    int i;
    int xlen,ylen,glen;
    
    sprintf(pnm_ahead.pnm_magicnos,"P%d",magicno);
    sprintf(pnm_ahead.pnm_xsizes,"%d",xsz);
    sprintf(pnm_ahead.pnm_ysizes,"%d",ysz);
    sprintf(pnm_ahead.pnm_maxgreys,"%d",maxgrey);
    
    for(i=0;i<20;i++) header[i] = ' ';
    
    strncpy(header,pnm_ahead.pnm_magicnos,2);

    xlen = strlen(pnm_ahead.pnm_xsizes);
    if(xlen > 5) erret("\nMAKEPNMHEAD:x size too big");
    strncpy(header + 3,pnm_ahead.pnm_xsizes,xlen);

    ylen = strlen(pnm_ahead.pnm_ysizes);
    if(ylen > 5) erret("\nMAKEPNMHEAD:y size too big");    
    strncpy(header + 9,pnm_ahead.pnm_ysizes,ylen);

    glen = strlen(pnm_ahead.pnm_maxgreys);
    if(glen > 3) erret("\nMAKEPNMHEAD:maxgrey too big");    
    strncpy(header + 19 - glen,pnm_ahead.pnm_maxgreys,glen);

    header[19]='\n';
    return header;
}

mirella char *makepgmhead(xsz,ysz,maxgrey)
int xsz;
int ysz;
int maxgrey;
{
    return makepnmhead(5,xsz,ysz,maxgrey);
}

/***************** COLOR TABLE FROM RHL **************************/
/* thanks, Robert. maps 1->0, note */
/* these arrays have 56(!) entries */

static u_char col_rtab[] = {               
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
255, 255, 240, 245, 240, 208, 184, 160, 136, 112,  88,  64,  32,  
 16,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  32,  
 48,  64,  64,   0 };
                                  
static u_char col_gtab[] = {
  0,   2,   4,   8,  16,  32,  64,  96, 128, 144, 160, 176, 192, 
208, 224, 230, 240, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 240, 224, 208, 
192, 176, 160, 144, 128,  96,  80,  64,  48,  32,  16,  32,  32,  
 32,  32,   0,   0 };

static u_char col_btab[] = {
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,  64,  90,  64,   0,   0,   0,   0,   0,   0,   0,   0,   0,   
  0,   0,  32,  64,  96, 128, 160, 192, 224, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 224, 208,
192, 176, 160,   0 };
                                                   


/****************** DISK I/O CODE FOR PBM. PGM. PPM **************/

/******************* PUTPBM(), GETPBM() **************************/
/* These routines write and read binary pbm files; the pbm spec does
 * not insist that xsize be a multiple of 8, which is the case for
 * Mirella bitmap graphs; the input code throws up its hands if
 * this is not the case. Also, the pbm origin is at the top left,
 * and ours is at the bottom left, so we have to read/write a line
 * at a time.
 */
 
mirella void
putpbm(fname) /* writes a binary PBM file for the current graph */
char *fname;
{
    int fdes;
    char *cp;
    char filename[64];
    int i;
    
    if(!g_bitmap)erret("\nGraph has no bitmap data");
    extchk(fname,"pbm",filename);
    if(!access(filename,1)) unlink(filename);
    if((fdes = creatblk(filename,16 + g_xbsize*g_ysize)) == -1){
        scrprintf("\nCannot open %s for writing",filename);
        erret((char*)1);
    }
    if(8*(g_xsize/8) != g_xsize){
        erret("\nPUTPBM:Mirella graph does not have xsize divisible by 8");
    }
    cp = makepbmhead(g_xsize,g_ysize);
    writeblk(fdes,cp,16);
    for(i=0;i<g_ysize;i++){
        writeblk(fdes,mgraph[g_ysize - i - 1],g_xbsize);
    }
    closeblk(fdes);
}

mirella void
getpbm(fname,n)   /* reads a binary pbm file into to graph n; 
                   * if there is not a graph n, allocates one; if there
                   * is, frees and overwrites it. NB!!! only handles BINARY
                   * pbm graphs--it would be simple to handle general ones,
                   * but someday. Note that pbm image is 'upside down'; its
                   * origin is at the top left, Mirella's at the bottom left.
                   */
char *fname;
int n;
{
    int fdes;
    char name[100];
    int gxs, gys; 
    int datoffs;
    int dt;
    int i;

    strcpy(name,fname);
    if(!strchr(name,'.')) strcat(name,".pbm");

    /* get header */
    getpnmhead(name);
    if(pnm_ihead.pnm_magicno != 4){
        erret("\nNOT A BINARY PBM FILE");
    }
    gxs = pnm_ihead.pnm_xsize;
    gys = pnm_ihead.pnm_ysize;
    datoffs = pnm_ihead.pnm_data;
    
    if(gxs != 8*(gxs/8)) erret("\nGETPBM:linelength is not a multiple of 8");
    
    if((fdes = openblk(name,O_RDONLY)) == -1){
        scrprintf("\nCannot open %s for reading",name);
        erret((char *)1);
    }
    irchk(n,0,NGRAPH,"No such graph");
    if(g_env[n].gmcp || g_env[n].vpptr){
        freegraph(n);
    }
    allocgraph(gxs,gys,n);
    dt = lseekblk(fdes,datoffs,0);
    if(dt != datoffs) erret("\nGETPBM: seek error for reading graph");
    for(i=0;i<g_ysize;i++){
        readblk(fdes,mgraph[g_ysize - i - 1],g_xbsize);
    }
    closeblk(fdes);
}

/******************* PUTPGM(), GETPGM() **************************/
/* These routines write and read binary pgm files; 
 * The pgm origin is at the top left,
 * and ours is at the bottom left, so we have to read/write a line
 * at a time.
 */
 
mirella void
putpgm(fname) /* writes a binary PGM file for the current graph */
char *fname;
{
    int fdes;
    char *cp;
    char filename[64];
    int i;
    
    if(!g_bitmap) erret("\nGraph has no bitmap data");
    if(g_bitscol != 8) erret("\nLo siento. Graph is not a PGM graph");
    extchk(fname,"pgm",filename);
    if(!access(filename,1)) unlink(filename);
    if((fdes = creatblk(filename,20 + g_xbsize*g_ysize)) == -1){
        scrprintf("\nCannot open %s for writing",filename);
        erret((char*)1);
    }
    cp = makepnmhead(5,g_xsize,g_ysize,255);
    writeblk(fdes,cp,20);
    for(i=0;i<g_ysize;i++){
        writeblk(fdes,mgraph[g_ysize - i - 1],g_xbsize);
    }
    closeblk(fdes);
}

mirella void
getpgm(fname,n)   /* reads a binary pgm file into to graph n; 
                   * if there is not a graph n, allocates one; if there
                   * is, frees and overwrites it. NB!!! only handles BINARY
                   * pgm graphs. Note that pgm image is 'upside down'; its
                   * origin is at the top left, Mirella's at the bottom left.
                   */
char *fname;
int n;
{
    int fdes;
    char name[100];
    int gxs, gys; 
    int datoffs;
    int dt;
    int i;

    strcpy(name,fname);
    if(!strchr(name,'.')) strcat(name,".pbm");

    /* get header */
    getpnmhead(name);
    if(pnm_ihead.pnm_magicno != 5){
        erret("\nNOT A BINARY PGM FILE");
    }
    gxs = pnm_ihead.pnm_xsize;
    gys = pnm_ihead.pnm_ysize;
    datoffs = pnm_ihead.pnm_data;
    
    if((fdes = openblk(name,O_RDONLY)) == -1){
        scrprintf("\nCannot open %s for reading",name);
        erret((char *)NULL);
    }
    irchk(n,0,NGRAPH,"No such graph");
    if(g_env[n].gmcp || g_env[n].vpptr){
        freegraph(n);
    }
    allocpgmgraph(gxs,gys,n);
    dt = lseekblk(fdes,datoffs,0);
    if(dt != datoffs) erret("\nGETPBM: seek error for reading graph");
    for(i=0;i<g_ysize;i++){
        readblk(fdes,mgraph[g_ysize - i - 1],g_xbsize);
    }
    closeblk(fdes);
}


/******************* PUTPPM(), GETPPM() **************************/
/* These routines write and read binary ppm files; 
 * The ppm origin is at the top left,
 * and ours is at the bottom left, so we have to read/write a line
 * at a time.
 */
 
mirella void
putppm(fname) /* writes a binary PPM file for the current graph */
char *fname;
{
    int fdes;
    char *cp;
    char filename[64];
    int i;
    
    if(!g_bitmap) erret("\nGraph has no bitmap data");
    if(g_bitscol != 24) erret("\nLo siento. Graph is not a PPM graph");
    /* XXXXX FIXME--should be able to write ANY graph as a PPM graph */
    extchk(fname,"ppm",filename);
    if(!access(filename,1)) unlink(filename);
    if((fdes = creatblk(filename,20 + g_xbsize*g_ysize)) == -1){
        scrprintf("\nCannot open %s for writing",filename);
        erret((char*)NULL);
    }
    cp = makepnmhead(6,g_xsize,g_ysize,255);
    writeblk(fdes,cp,20);
    for(i=0;i<g_ysize;i++){
        writeblk(fdes,mgraph[g_ysize - i - 1],g_xbsize);
    }
    closeblk(fdes);
}

mirella void
getppm(fname,n)   /* reads a binary ppm file into to graph n; 
                   * if there is not a graph n, allocates one; if there
                   * is, frees and overwrites it. NB!!! only handles BINARY
                   * ppm graphs. Note that ppm image is 'upside down'; its
                   * origin is at the top left, Mirella's at the bottom left.
                   */
char *fname;
int n;
{
    int fdes;
    char name[100];
    int gxs, gys; 
    int datoffs;
    int dt;
    int i;

    strcpy(name,fname);
    if(!strchr(name,'.')) strcat(name,".pbm");

    /* get header */
    getpnmhead(name);
    if(pnm_ihead.pnm_magicno != 6){
        erret("\nNOT A BINARY PPM FILE");
    }
    gxs = pnm_ihead.pnm_xsize;
    gys = pnm_ihead.pnm_ysize;
    datoffs = pnm_ihead.pnm_data;
    
    if((fdes = openblk(name,O_RDONLY)) == -1){
        scrprintf("\nCannot open %s for reading",name);
        erret((char *)NULL);
    }
    irchk(n,0,NGRAPH,"No such graph");
    if(g_env[n].gmcp || g_env[n].vpptr){
        freegraph(n);
    }
    allocppmgraph(gxs,gys,n);
    
    dt = lseekblk(fdes,datoffs,0); 
    
    if(dt != datoffs) erret("\nGETPBM: seek error for reading graph");
    for(i=0;i<g_ysize;i++){
        readblk(fdes,mgraph[g_ysize - i - 1],g_xbsize);
    }
    closeblk(fdes);
}

mirella void
putpnm(fname)
char *fname;
{
}

mirella void
getpnm(fname, n)
char *fname;
int n;
{
}

/********* CODE TO CONVERT *IMAGES* TO PPM FILES ****************/

/******************* PUTPGMIMG **********************************/
/* this code writes a pgm from the IMAGE in the current buffer,
 * stretched from zlo to zhi, to fname
 */
 
mirella void
putpgmimg(zlo,zhi,fname)
int zlo, zhi;
char *fname;
{
    int fdes;
    char *cp;
    char filename[64];
    int i,j;
    u_char *lbuf;
    int val;
    int span = zhi - zlo;
    
    lbuf = (u_char *)malloc(xsize);
    if(lbuf == NULL) erret("\nPUTPGMIMG:Cannot allocate line buffer");
    
    if(isacbuf(pbuffer) == 0) erret("\nNO ACTIVE PICTURE BUFFER");    
    if(h_bitpix != 16) erret("\nLo siento. Buffer is not shorts");
    extchk(fname,"pgm",filename);
    if(!access(filename,1)) unlink(filename);
    if((fdes = creatblk(filename,20 + xize*ysize)) == -1){
        scrprintf("\nCannot open %s for writing",filename);
        erret((char*)1);
    }
    cp = makepnmhead(5,xsize,ysize,255);
    writeblk(fdes,cp,20);
    for(i=0;i<ysize;i++){
        for(j=0;j<xsize;j++){
            val = pic[ysize - i - 1][j];
            if(val < zlo){
                lbuf[j] = 0;
            }else if(val > zhi){
                lbuf[j] = 0xff;
            }else{
                lbuf[j] = ((val - zlo)*255)/span;
            }
        }
        writeblk(fdes,lbuf,xsize);
    }
    closeblk(fdes);
    free(lbuf);
}


/******************* PUTPGMIMG **********************************/
/* this code writes a pgm from a subfraome of the IMAGE in the current buffer,
 * specified by a lower left corner and a size, and
 * stretched from zlo to zhi, to fname, 
 */
 
mirella void
putpartpgmimg(zlo,zhi,xlo,ylo,xs,ys,fname)
int zlo, zhi;
int xlo, ylo; /* ll corner */
int xs, ys;   /* x and y extents */
char *fname;
{
    int fdes;
    char *cp;
    char filename[64];
    int i,j;
    u_char *lbuf;
    int val;
    int span = zhi - zlo;
    
    /* check geometry */
    if ( xs > xsize ) xs = xsize;
    if ( ys > ysize ) ys = ysize;
    if ( xlo < 0 ) xlo = 0;
    if ( ylo < 0 ) ylo = 0;
    if ( xlo + xs > xsize ) xlo = xsize - xs;
    if ( ylo + ys > ysize ) ylo = ysize - ys;
  
    lbuf = (u_char *)malloc(xs);
    if(lbuf == NULL) erret("\nPUTPARTPGMIMG:Cannot allocate line buffer");
    
    if(isacbuf(pbuffer) == 0) erret("\nNO ACTIVE PICTURE BUFFER");    
    if(h_bitpix != 16) erret("\nLo siento. Buffer is not shorts");
    extchk(fname,"pgm",filename);
    if(!access(filename,1)) unlink(filename);
    if((fdes = creatblk(filename,20 + xize*ysize)) == -1){
        scrprintf("\nCannot open %s for writing",filename);
        erret((char*)1);
    }
    cp = makepnmhead(5,xs,ys,255);
    writeblk(fdes,cp,20);
    for(i=0;i<ys;i++){
        for(j=xlo;j<xlo + xs;j++){
            val = pic[ylo + ys -1 - i][j];
            if(val < zlo){
                lbuf[j - xlo] = 0;
            }else if(val > zhi){
                lbuf[j - xlo] = 0xff;
            }else{
                lbuf[j-xlo] = ((val - zlo)*255)/span;
            }
        }
        writeblk(fdes,lbuf,xs);
    }
    closeblk(fdes);
    free(lbuf);
}

/******************* PUTPPMIMG **********************************/
/* this code writes a ppm from the image in the current buffer,
 * stretched from zlo to zhi, to filename, using the RHL color map
 * (about which we need to DO something).
 */
 
mirella void
putppmimg(zlo,zhi,fname)
int zlo, zhi;
char *fname;
{
    int fdes;
    char *cp;
    char filename[64];
    int i,j;
    u_char *lbuf;
    int val;
    int span = zhi - zlo;
    int jj;
    
    lbuf = (u_char *)malloc(3*xsize);
    if(lbuf == NULL) erret("\nPUTPPMIMG:Cannot allocate line buffer");
    
    if(isacbuf(pbuffer) == 0) erret("\nNO ACTIVE PICTURE BUFFER");    
    if(h_bitpix != 16) erret("\nLo siento. Buffer is not shorts");
    extchk(fname,"ppm",filename);
    if(!access(filename,1)) unlink(filename);
    if((fdes = creatblk(filename,20 + 3*xsize*ysize)) == -1){
        scrprintf("\nCannot open %s for writing",filename);
        erret((char*)1);
    }
    cp = makepnmhead(6,xsize,ysize,255);
    writeblk(fdes,cp,20);
    for(i=0;i<ysize;i++){
        for(j=0;j<xsize;j++){
            val = pic[ysize - i - 1][j];
            if(val <= zlo){
                lbuf[3*j]   = col_rtab[55];
                lbuf[3*j+1] = col_gtab[55];
                lbuf[3*j+2] = col_btab[55];
            }else if(val >= zhi){
                lbuf[3*j]   = col_rtab[0];
                lbuf[3*j+1] = col_gtab[0];
                lbuf[3*j+2] = col_btab[0];
            }else{
                jj = ((zhi - val)*55)/span;
                lbuf[3*j]   = col_rtab[jj];
                lbuf[3*j+1] = col_gtab[jj];
                lbuf[3*j+2] = col_btab[jj];
            }
        }
        writeblk(fdes,lbuf,3*xsize);
    }
    closeblk(fdes);
    free(lbuf);
}

/********************** PNM manipulation code ******************************/
/* This code supposes that you have a matrix of values you wish to
 * represent as a continuous-tone pgm or ppm graph, either as a whole graphics
 * frame or in the plot box, as determined by whether you have executed
 * plotinall or plotinframe (we need a global flag). It is assumed that
 * the rows increase with y, the columns with x. We will need the
 * ordinates and abscissae at the ll and ur `corners' of the matrix.
 * Then the corresponding corners of the graph g_xorg, g_fxhi are used
 * to create a map from the matrix into the graph. We will first write
 * code for pgm graphs and float matrices; generalizations to other
 * types should be easy. This code does not bin in the source
 * matrix, so if it is of much higher resolution than the graph, this
 * can cause trouble. If the other case, it would probably be well to
 * interpolate, but we do not do that either. Caveat emptor.
 * (use the mirage repixel code--already written).
 * NB!!! the calling sequence of this graphics thing is very different
 * from the calling sequence of the smattopgm routine below; we should
 * DO something.
 */
 
void
fmattopgm(fxl,fyl,fxh,fyh,black,white,nx,ny,mat)
double fxl;
double fyl;
double fxh;
double fyh;    /* abscissae and ordinates at ul ([ny-1][nx-1]) and rh ([0][0]) 
                    corners of matrix */
double black;
double white;  /* values in matrix to map into 0 and 255 in pgm */
int nx;
int ny;        /* size of matrix */
float **mat;   /* float matrix of values */
{
    int i, j;
    float x, y;
    float mxspan = fxh - fxl;
    float myspan = fyh - fyl;
    float mxscl  = ((float)nx)/mxspan ;
    float myscl  = ((float)ny)/myspan ;
    int mi, mj;
    float val;
    int ival;
    int vscl = 255./(white - black);
    int miin;

    if(g_bitpix != 8) erret("\nSORRY: current graph is not an 8-bit graph");
    
    for(i = 0; i < g_ysp; i++){                     /* for each y val IN graph */
        y = ((float)i +0.5)/g_fyscl + g_fyorg;   
                                                    /* find the float y value */
        mi = round((y - fyl)*myscl);                /* and the matrix index */
        miin = (mi >= 0 && mi < ny);                /* pt in range in y ? */
        for(j = 0; j < g_xsp; j++){
            x = ((float)j + 0.5)/g_fxscl + g_fxorg; /* find the float x value */
            mj = round((x - fxl)*mxscl);            /* and the matrix index */
            if(mj >= 0 && mj < nx && miin){         /* in range in x and y ? */
                val = mat[mi][mj];                  /* matrix value */
                ival = (val - black)*vscl ;         /* graphics value */
                if (ival < 0)   ival = 0;
                if (ival > 255) ival = 255;
            }else{
                ival = 0;
            }
            mgraph[i + g_yl][j + g_xl] = ival;
        }
    }
}

/************* GEN SHORT MATRIX AND PBUF TO PGM ******************************/
/* we REALLY want to be able to extract a part of an image 
 * do it soon before this gets embedded in other code. Want to
 * change the argument list to be like fmattopgm above. Also
 * want ability to write ppm with some arbitrary val->RGB map;
 * how to specify function and args from M is the problem, and we need
 * to make a universal void pass-it-a-pointer-to-a-struct function pointer for
 * the mirella interface. This would be past cool. Maybe--maybe just a
 * map with M access to the table would be OK.
 */
 
mirella void 
smattopgm(fname,mat,xsz,ysz,blk,wht,clip)
char *fname;        /* output filename */
short int **mat;    /* short int input matrix */
int xsz;            /* x size */
int ysz;            /* y size */
int blk;            /* short int value -> zero in pgm */
int wht;            /* short int value -> 255 in pgm */
int clip;           /* flag to clip at blk and white, 0, 255); rolls if 0 */
{
    u_char **bmat = (u_char **)matrix(ysz,xsz,1);
    int i,j;
    int ii;
    int val;
    char *hdr;
    FILE* fptr;
    
    for(i=0;i<ysz;i++){
        ii = ysz -i -1;
        for(j=0;j<xsz;j++){
            val = ((mat[ii][j] - blk)*255)/(wht-blk);
            if(clip){
                val = val < 0 ? 0 : val;
                val = val > 255? 255 : val ;
            }else{
                val = val & 0xff;
            }
            bmat[i][j] = val;
        }
    }
    hdr = makepgmhead(xsz,ysz,255);
    fptr = fopen(fname,"w");
    if(fptr == (FILE *)0) erret("SMATTOPGM: cannot open output file");
    fwrite(hdr,1,20,fptr);
    fwrite((char *)(&bmat[0][0]),1,xsz*ysz,fptr);
    fclose(fptr);
    free(bmat);
}


/* creates pgm from current IMAGE buffer with current xsize and ysize 
 * and stretch. Note this functionality is same as putpgmimg() above,
 * but calling sequence is different--putpgmimg always clips, and
 * the caller provides the stretch parameters. ????? This should be
 * cleaned up.
 */

mirella void 
buftopgm(fname,clip)
char *fname;        /* output filename */
int clip;           /* flag to clip at blk and white, 0, 255); rolls if 0 */
{
    short int **mat = pic;    /* short int input matrix, current buffer */
    int xsz = xsize;
    int ysz = ysize;
    int blk = d_zorg;
    int wht = d_zorg + d_zrng;
    smattopgm(fname,mat,xsz,ysz,blk,wht,clip);
}

#if 0

/* not done--just use convert -mosaic */

/******************* PPMMOSAIC ****************************************/
/* code to put several identical-sized ppm files into a bigger one
 * to make an n x m mosaic. This one ONLY does ppm files.
 */
 
/* allocates memory for the mosaic, opens the file, and writes the header 
 * returns the pointer to the matrix */

static FILE *mosptr;
static char mosname[64];

mirella char **
ppmmosalloc (fname, n, m, xsz, ysz)
char *fname
int n;
int m;
int xsz;
int ysz;
{
    u_char **bmat = (u_char **)matrix(m*ysz,n*xsz,1);
    char *hdr;

    hdr = makeppmhead(xsz,ysz,255);
    mosptr = fopen(fname,"w");
    if(fptr == (FILE *)0) erret("PPMMOSALLOC: cannot open output file");
    if(bmat == (u_char **)0) erret("PPMMOSALLOC: cannot allocate memory");
    fwrite(hdr,1,20,mosptr);
    strcpy[mosname,fname);
}

mirella void
ppmmosaic(i, j, fname)
int i;
int j;
char *fname;
{
    char hdr[20];
    FILE *fptr;
    char *lptr[20];
    int nitem; 
   
    fptr = fopen(fname,r);
    if(fptr == (FILE *)0) erret("PPMMOSAIC: cannot open input file");    
    fread(hdr,1,20,fptr);
    
    fclose(fptr);
    free(bmat);
    nitem = parse(hdr,lptr);
}

#endif

/******************** END MODULE MIRPNM.C **********************************/
