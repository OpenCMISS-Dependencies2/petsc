#ifndef lint
static char vcid[] = "$Id: mpiov.c,v 1.26.1.29 1996/04/26 00:04:17 balay Exp $";
#endif
/*
   Routines to compute overlapping regions of a parallel MPI matrix
  and to find submatrices that were shared across processors.
*/
#include "mpiaij.h"
#include "src/inline/bitarray.h"

static int MatIncreaseOverlap_MPIAIJ_Once(Mat, int, IS *);
static int MatIncreaseOverlap_MPIAIJ_Local(Mat , int , char **,int*, int**);
static int MatIncreaseOverlap_MPIAIJ_Receive(Mat , int, int **, int**, int* );
extern int MatGetRow_MPIAIJ(Mat,int,int*,int**,Scalar**);
extern int MatRestoreRow_MPIAIJ(Mat,int,int*,int**,Scalar**);

int MatIncreaseOverlap_MPIAIJ(Mat C, int imax, IS *is, int ov)
{
  int i, ierr;
  if (ov < 0){ SETERRQ(1," MatIncreaseOverlap_MPIAIJ:Negative overlap specified\n");}
  for ( i=0; i<ov; ++i ) {
    ierr = MatIncreaseOverlap_MPIAIJ_Once(C, imax, is); CHKERRQ(ierr);
  }
  return 0;
}

/*
  Sample message format:
  If a processor A wants processor B to process some elements corresponding
  to index sets 1s[1], is[5]
  mesg [0] = 2   ( no of index sets in the mesg)
  -----------  
  mesg [1] = 1 => is[1]
  mesg [2] = sizeof(is[1]);
  -----------  
  mesg [5] = 5  => is[5]
  mesg [6] = sizeof(is[5]);
  -----------
  mesg [7] 
  mesg [n]  datas[1]
  -----------  
  mesg[n+1]
  mesg[m]  data(is[5])
  -----------  
  
  Notes:
  nrqs - no of requests sent (or to be sent out)
  nrqr - no of requests recieved (which have to be or which have been processed
*/
static int MatIncreaseOverlap_MPIAIJ_Once(Mat C, int imax, IS *is)
{
  Mat_MPIAIJ  *c = (Mat_MPIAIJ *) C->data;
  int         **idx, *n, *w1, *w2, *w3, *w4, *rtable,**data,len,*idx_i;
  int         size,rank,m,i,j,k,ierr,**rbuf,row,proc,nrqs,msz,**outdat,**ptr;
  int         *ctr,*pa,tag,*tmp,bsz,nrqr,*isz,*isz1,**xdata,bsz1,**rbuf2;
  char        **table;
  MPI_Comm    comm;
  MPI_Request *s_waits1,*r_waits1,*s_waits2,*r_waits2;
  MPI_Status  *s_status,*recv_status;

  comm   = C->comm;
  tag    = C->tag;
  size   = c->size;
  rank   = c->rank;
  m      = c->M;

  len    = (imax+1)*sizeof(int *) + (imax + m)*sizeof(int);
  idx    = (int **) PetscMalloc(len); CHKPTRQ(idx);
  n      = (int *) (idx + imax);
  rtable = n + imax;
   
  for ( i=0; i<imax; i++ ) {
    ierr = ISGetIndices(is[i],&idx[i]);  CHKERRQ(ierr);
    ierr = ISGetSize(is[i],&n[i]);  CHKERRQ(ierr);
  }
  
  /* Create hash table for the mapping :row -> proc*/
  for ( i=0,j=0; i<size; i++ ) {
    len = c->rowners[i+1];  
    for ( ; j<len; j++ ) {
      rtable[j] = i;
    }
  }

  /* evaluate communication - mesg to who, length of mesg, and buffer space
     required. Based on this, buffers are allocated, and data copied into them*/
  w1   = (int *)PetscMalloc(size*4*sizeof(int));CHKPTRQ(w1);/*  mesg size */
  w2   = w1 + size;       /* if w2[i] marked, then a message to proc i*/
  w3   = w2 + size;       /* no of IS that needs to be sent to proc i */
  w4   = w3 + size;       /* temp work space used in determining w1, w2, w3 */
  PetscMemzero(w1,size*3*sizeof(int)); /* initialise work vector*/
  for ( i=0; i<imax; i++ ) { 
    PetscMemzero(w4,size*sizeof(int)); /* initialise work vector*/
    idx_i = idx[i];
    len   = n[i];
    for ( j=0; j<len; j++ ) {
      row  = idx_i[j];
      proc = rtable[row];
      w4[proc]++;
    }
    for ( j=0; j<size; j++ ){ 
      if (w4[j]) { w1[j] += w4[j]; w3[j]++;} 
    }
  }

  nrqs     = 0;              /* no of outgoing messages */
  msz      = 0;              /* total mesg length (for all proc */
  w1[rank] = 0;              /* no mesg sent to intself */
  w3[rank] = 0;
  for ( i=0; i<size; i++ ) {
    if (w1[i])  {w2[i] = 1; nrqs++;} /* there exists a message to proc i */
  }
  /* pa - is list of processors to communicate with */
  pa = (int *)PetscMalloc((nrqs+1)*sizeof(int));CHKPTRQ(pa);
  for ( i=0,j=0; i<size; i++ ) {
    if (w1[i]) {pa[j] = i; j++;}
  } 

  /* Each message would have a header = 1 + 2*(no of IS) + data */
  for ( i=0; i<nrqs; i++ ) {
    j      = pa[i];
    w1[j] += w2[j] + 2*w3[j];   
    msz   += w1[j];  
  }
  
  
  /* Do a global reduction to determine how many messages to expect*/
  {
    int *rw1, *rw2;
    rw1   = (int *) PetscMalloc(2*size*sizeof(int)); CHKPTRQ(rw1);
    rw2   = rw1+size;
    MPI_Allreduce(w1, rw1, size, MPI_INT, MPI_MAX, comm);
    bsz   = rw1[rank];
    MPI_Allreduce(w2, rw2, size, MPI_INT, MPI_SUM, comm);
    nrqr  = rw2[rank];
    PetscFree(rw1);
  }

  /* Allocate memory for recv buffers . Prob none if nrqr = 0 ???? */ 
  len     = (nrqr+1)*sizeof(int*) + nrqr*bsz*sizeof(int);
  rbuf    = (int**) PetscMalloc(len);  CHKPTRQ(rbuf);
  rbuf[0] = (int *) (rbuf + nrqr);
  for ( i=1; i<nrqr; ++i ) rbuf[i] = rbuf[i-1] + bsz;
  
  /* Post the receives */
  r_waits1 = (MPI_Request *) PetscMalloc((nrqr+1)*sizeof(MPI_Request)); 
  CHKPTRQ(r_waits1);
  for ( i=0; i<nrqr; ++i ){
    MPI_Irecv(rbuf[i],bsz,MPI_INT,MPI_ANY_SOURCE,tag,comm,r_waits1+i);
  }

  /* Allocate Memory for outgoing messages */
  len    = 2*size*sizeof(int*) + (size+msz)*sizeof(int);
  outdat = (int **)PetscMalloc(len); CHKPTRQ(outdat);
  ptr    = outdat + size;     /* Pointers to the data in outgoing buffers */
  PetscMemzero(outdat,2*size*sizeof(int*));
  tmp    = (int *) (outdat + 2*size);
  ctr    = tmp + msz;

  {
    int *iptr = tmp,ict  = 0;
    for ( i=0; i<nrqs; i++ ) {
      j         = pa[i];
      iptr     +=  ict;
      outdat[j] = iptr;
      ict       = w1[j];
    }
  }

  /* Form the outgoing messages */
  /*plug in the headers*/
  for ( i=0; i<nrqs; i++ ) {
    j            = pa[i];
    outdat[j][0] = 0;
    PetscMemzero(outdat[j]+1,2*w3[j]*sizeof(int));
    ptr[j]       = outdat[j] + 2*w3[j] + 1;
  }
 
  /* Memory for doing local proc's work*/
  { 
    int  *d_p;
    char *t_p;

    len      = (imax+1)*(sizeof(char *) + sizeof(int *) + sizeof(int)) + 
               (m+1)*imax*sizeof(int)  + (m/BITSPERBYTE+1)*imax*sizeof(char);
    table    = (char **)PetscMalloc(len);  CHKPTRQ(table);
    data     = (int **)(table + imax);
    data[0]  = (int *)(data + imax);
    isz      = data[0] + (m+1)*imax;
    table[0] = (char *)(isz + imax);
    d_p = data[0]; t_p = table[0]; 
    for ( i=1; i<imax; i++ ) {
      table[i] = t_p + (m/BITSPERBYTE+1)*i;
      data[i]  = d_p + (m+1)*i;
    }
  }
  PetscMemzero(*table,(m/BITSPERBYTE+1)*imax); 
  PetscMemzero(isz,imax*sizeof(int));

  /* Parse the IS and update local tables and the outgoing buf with the data*/
  {
    int  n_i,*data_i,isz_i,*outdat_j,ctr_j;
    char *table_i;

    for ( i=0; i<imax; i++ ) {
      PetscMemzero(ctr,size*sizeof(int));
      n_i     = n[i];
      table_i = table[i];
      idx_i   = idx[i];
      data_i  = data[i];
      isz_i   = isz[i];
      for ( j=0;  j<n_i; j++ ) {  /* parse the indices of each IS */
        row  = idx_i[j];
        proc = rtable[row];
        if (proc != rank) { /* copy to the outgoing buffer */
          ctr[proc]++;
          *ptr[proc] = row;
          ptr[proc]++;
        }
        else { /* Update the local table */
          if (!BT_LOOKUP(table_i,row)) { data_i[isz_i++] = row;}
        }
      }
      /* Update the headers for the current IS */
      for ( j=0; j<size; j++ ) { /* Can Optimise this loop by using pa[] */
        if ((ctr_j = ctr[j])) {
          outdat_j        = outdat[j];
          k               = ++outdat_j[0];
          outdat_j[2*k]   = ctr_j;
          outdat_j[2*k-1] = i;
        }
      }
      isz[i] = isz_i;
    }
  }
  


  /*  Now  post the sends */
  s_waits1 = (MPI_Request *) PetscMalloc((nrqs+1)*sizeof(MPI_Request));
  CHKPTRQ(s_waits1);
  for ( i=0; i<nrqs; ++i ) {
    j = pa[i];
    MPI_Isend(outdat[j], w1[j], MPI_INT, j, tag, comm, s_waits1+i);
  }
    
  /* No longer need the original indices*/
  for ( i=0; i<imax; ++i ) {
    ierr = ISRestoreIndices(is[i], idx+i); CHKERRQ(ierr);
  }
  PetscFree(idx);

  for ( i=0; i<imax; ++i ) {
    ierr = ISDestroy(is[i]); CHKERRQ(ierr);
  }
  
  /* Do Local work*/
  ierr = MatIncreaseOverlap_MPIAIJ_Local(C,imax,table,isz,data);CHKERRQ(ierr);

  /* Receive messages*/
  {
    int        index;
    
    recv_status = (MPI_Status *) PetscMalloc( (nrqr+1)*sizeof(MPI_Status) );
    CHKPTRQ(recv_status);
    for ( i=0; i<nrqr; ++i ) {
      MPI_Waitany(nrqr, r_waits1, &index, recv_status+i);
    }
    
    s_status = (MPI_Status *) PetscMalloc( (nrqs+1)*sizeof(MPI_Status) );
    CHKPTRQ(s_status);
    MPI_Waitall(nrqs,s_waits1,s_status);
  }

  /* Phase 1 sends are complete - deallocate buffers */
  PetscFree(outdat);
  PetscFree(w1);

  xdata = (int **)PetscMalloc((nrqr+1)*sizeof(int *)); CHKPTRQ(xdata);
  isz1  = (int *)PetscMalloc((nrqr+1)*sizeof(int)); CHKPTRQ(isz1);
  ierr  = MatIncreaseOverlap_MPIAIJ_Receive(C,nrqr,rbuf,xdata,isz1);CHKERRQ(ierr);
  PetscFree(rbuf);

  /* Send the data back*/
  /* Do a global reduction to know the buffer space req for incoming messages*/
  {
    int *rw1, *rw2;
    
    rw1 = (int *)PetscMalloc(2*size*sizeof(int)); CHKPTRQ(rw1);
    PetscMemzero(rw1,2*size*sizeof(int));
    rw2 = rw1+size;
    for ( i=0; i<nrqr; ++i ) {
      proc      = recv_status[i].MPI_SOURCE;
      rw1[proc] = isz1[i];
    }
      
    MPI_Allreduce(rw1, rw2, size, MPI_INT, MPI_MAX, comm);
    bsz1   = rw2[rank];
    PetscFree(rw1);
  }

  /* Allocate buffers*/
  
  /* Allocate memory for recv buffers. Prob none if nrqr = 0 ???? */ 
  len      = (nrqs+1)*sizeof(int*) + nrqs*bsz1*sizeof(int);
  rbuf2    = (int**) PetscMalloc(len);  CHKPTRQ(rbuf2);
  rbuf2[0] = (int *) (rbuf2 + nrqs);
  for ( i=1; i<nrqs; ++i ) rbuf2[i] = rbuf2[i-1] + bsz1;
  
  /* Post the receives */
  r_waits2 = (MPI_Request *)PetscMalloc((nrqs+1)*sizeof(MPI_Request));
  CHKPTRQ(r_waits2);
  for ( i=0; i<nrqs; ++i ) {
    MPI_Irecv(rbuf2[i], bsz1, MPI_INT, MPI_ANY_SOURCE, tag, comm, r_waits2+i);
  }
  
  /*  Now  post the sends */
  s_waits2 = (MPI_Request *) PetscMalloc((nrqr+1)*sizeof(MPI_Request));
  CHKPTRQ(s_waits2);
  for ( i=0; i<nrqr; ++i ) {
    j = recv_status[i].MPI_SOURCE;
    MPI_Isend( xdata[i], isz1[i], MPI_INT, j, tag, comm, s_waits2+i);
  }

  /* receive work done on other processors*/
  {
    int         index, is_no, ct1, max,*rbuf2_i,isz_i,*data_i,jmax;
    char        *table_i;
    MPI_Status  *status2;
     
    status2 = (MPI_Status *) PetscMalloc((nrqs+1)*sizeof(MPI_Status));CHKPTRQ(status2);

    for ( i=0; i<nrqs; ++i ) {
      MPI_Waitany(nrqs, r_waits2, &index, status2+i);
      /* Process the message*/
      rbuf2_i = rbuf2[index];
      ct1     = 2*rbuf2_i[0]+1;
      jmax    = rbuf2[index][0];
      for ( j=1; j<=jmax; j++ ) {
        max     = rbuf2_i[2*j];
        is_no   = rbuf2_i[2*j-1];
        isz_i   = isz[is_no];
        data_i  = data[is_no];
        table_i = table[is_no];
        for ( k=0; k<max; k++,ct1++ ) {
          row = rbuf2_i[ct1];
          if (!BT_LOOKUP(table_i,row)) { data_i[isz_i++] = row;}   
        }
        isz[is_no] = isz_i;
      }
    }
    MPI_Waitall(nrqr,s_waits2,status2);
    PetscFree(status2); 
  }
  
  for ( i=0; i<imax; ++i ) {
    ierr = ISCreateSeq(MPI_COMM_SELF, isz[i], data[i], is+i); CHKERRQ(ierr);
  }
  
  PetscFree(pa);
  PetscFree(rbuf2); 
  PetscFree(s_waits1);
  PetscFree(r_waits1);
  PetscFree(s_waits2);
  PetscFree(r_waits2);
  PetscFree(table);
  PetscFree(s_status);
  PetscFree(recv_status);
  PetscFree(xdata[0]); 
  PetscFree(xdata);
  PetscFree(isz1);
  return 0;
}

/*  
   MatIncreaseOverlap_MPIAIJ_Local - Called by MatincreaseOverlap, to do 
       the work on the local processor.

     Inputs:
      C      - MAT_MPIAIJ;
      imax - total no of index sets processed at a time;
      table  - an array of char - size = m bits.
      
     Output:
      isz    - array containing the count of the solution elements correspondign
               to each index set;
      data   - pointer to the solutions
*/
static int MatIncreaseOverlap_MPIAIJ_Local(Mat C,int imax,char **table,int *isz,
                                           int **data)
{
  Mat_MPIAIJ *c = (Mat_MPIAIJ *) C->data;
  Mat        A = c->A, B = c->B;
  Mat_SeqAIJ *a = (Mat_SeqAIJ*)A->data,*b = (Mat_SeqAIJ*)B->data;
  int        start, end, val, max, rstart,cstart, ashift, bshift,*ai, *aj;
  int        *bi, *bj, *garray, i, j, k, row,*data_i,isz_i;
  char       *table_i;

  rstart = c->rstart;
  cstart = c->cstart;
  ashift = a->indexshift;
  ai     = a->i;
  aj     = a->j +ashift;
  bshift = b->indexshift;
  bi     = b->i;
  bj     = b->j +bshift;
  garray = c->garray;

  
  for ( i=0; i<imax; i++ ) {
    data_i  = data[i];
    table_i = table[i];
    isz_i   = isz[i];
    for ( j=0, max=isz[i]; j<max; j++ ) {
      row   = data_i[j] - rstart;
      start = ai[row];
      end   = ai[row+1];
      for ( k=start; k<end; k++ ) { /* Amat */
        val = aj[k] + ashift + cstart;
        if (!BT_LOOKUP(table_i,val)) { data_i[isz_i++] = val;}  
      }
      start = bi[row];
      end   = bi[row+1];
      for ( k=start; k<end; k++ ) { /* Bmat */
        val = garray[bj[k]+bshift]; 
        if (!BT_LOOKUP(table_i,val)) { data_i[isz_i++] = val;}  
      } 
    }
    isz[i] = isz_i;
  }
  return 0;
}
/*     
      MatIncreaseOverlap_MPIAIJ_Receive - Process the recieved messages,
         and return the output

         Input:
           C    - the matrix
           nrqr - no of messages being processed.
           rbuf - an array of pointers to the recieved requests
           
         Output:
           xdata - array of messages to be sent back
           isz1  - size of each message

  For better efficiency perhaps we should malloc seperately each xdata[i],
then if a remalloc is required we need only copy the data for that one row
rather then all previous rows as it is now where a single large chunck of 
memory is used.

*/
static int MatIncreaseOverlap_MPIAIJ_Receive(Mat C,int nrqr,int **rbuf,
                                            int **xdata, int * isz1)
{
  Mat_MPIAIJ  *c = (Mat_MPIAIJ *) C->data;
  Mat         A = c->A, B = c->B;
  Mat_SeqAIJ  *a = (Mat_SeqAIJ*)A->data,*b = (Mat_SeqAIJ*)B->data;
  int         rstart,cstart, ashift, bshift,*ai, *aj, *bi, *bj, *garray, i, j, k;
  int         row,total_sz,ct, ct1, ct2, ct3,mem_estimate, oct2, l, start, end;
  int         val, max1, max2, rank, m, no_malloc =0, *tmp, new_estimate, ctr;
  int         *rbuf_i,kmax,rbuf_0;
  char        *xtable;

  rank   = c->rank;
  m      = c->M;
  rstart = c->rstart;
  cstart = c->cstart;
  ashift = a->indexshift;
  ai     = a->i;
  aj     = a->j +ashift;
  bshift = b->indexshift;
  bi     = b->i;
  bj     = b->j +bshift;
  garray = c->garray;
  
  
  for ( i=0,ct=0,total_sz=0; i<nrqr; ++i ) {
    rbuf_i =  rbuf[i]; 
    rbuf_0 =  rbuf_i[0];
    ct     += rbuf_0;
    for ( j=1; j<=rbuf_0; j++ ) { total_sz += rbuf_i[2*j]; }
  }
  
  max1         = ct*(a->nz +b->nz)/c->m;
  mem_estimate =  3*((total_sz > max1 ? total_sz : max1)+1);
  xdata[0]     = (int *)PetscMalloc(mem_estimate*sizeof(int)); CHKPTRQ(xdata[0]);
  ++no_malloc;
  xtable       = (char *)PetscMalloc((m/BITSPERBYTE+1)*sizeof(char)); CHKPTRQ(xtable);
  PetscMemzero(isz1,nrqr*sizeof(int));
  
  ct3 = 0;
  for ( i=0; i<nrqr; i++ ) { /* for easch mesg from proc i */
    rbuf_i =  rbuf[i]; 
    rbuf_0 =  rbuf_i[0];
    ct1    =  2*rbuf_0+1;
    ct2    =  ct1;
    ct3    += ct1;
    for ( j=1; j<=rbuf_0; j++ ) { /* for each IS from proc i*/
      PetscMemzero(xtable,(m/BITSPERBYTE+1)*sizeof(char));
      oct2 = ct2;
      kmax = rbuf_i[2*j];
      for ( k=0; k<kmax; k++, ct1++ ) { 
        row = rbuf_i[ct1];
        if (!BT_LOOKUP(xtable,row)) { 
          if (!(ct3 < mem_estimate)) {
            new_estimate = (int)(1.5*mem_estimate)+1;
            tmp          = (int*) PetscMalloc(new_estimate * sizeof(int));CHKPTRQ(tmp);
            PetscMemcpy(tmp,xdata[0],mem_estimate*sizeof(int));
            PetscFree(xdata[0]);
            xdata[0]     = tmp;
            mem_estimate = new_estimate; ++no_malloc;
            for ( ctr=1; ctr<=i; ctr++ ) { xdata[ctr] = xdata[ctr-1] + isz1[ctr-1];}
          }
          xdata[i][ct2++] = row;
          ct3++;
        }
      }
      for ( k=oct2,max2=ct2; k<max2; k++ ) {
        row   = xdata[i][k] - rstart;
        start = ai[row];
        end   = ai[row+1];
        for ( l=start; l<end; l++ ) {
          val = aj[l] + ashift + cstart;
          if (!BT_LOOKUP(xtable,val)) {
            if (!(ct3 < mem_estimate)) {
              new_estimate = (int)(1.5*mem_estimate)+1;
              tmp          = (int*) PetscMalloc(new_estimate * sizeof(int));CHKPTRQ(tmp);
              PetscMemcpy(tmp,xdata[0],mem_estimate*sizeof(int));
              PetscFree(xdata[0]);
              xdata[0]     = tmp;
              mem_estimate = new_estimate; ++no_malloc;
              for ( ctr=1; ctr<=i; ctr++ ) { xdata[ctr] = xdata[ctr-1] + isz1[ctr-1];}
            }
            xdata[i][ct2++] = val;
            ct3++;
          }
        }
        start = bi[row];
        end   = bi[row+1];
        for ( l=start; l<end; l++ ) {
          val = garray[bj[l]+bshift];
          if (!BT_LOOKUP(xtable,val)) { 
            if (!(ct3 < mem_estimate)) { 
              new_estimate = (int)(1.5*mem_estimate)+1;
              tmp          = (int*) PetscMalloc(new_estimate * sizeof(int));CHKPTRQ(tmp);
              PetscMemcpy(tmp,xdata[0],mem_estimate*sizeof(int));
              PetscFree(xdata[0]);
              xdata[0]     = tmp;
              mem_estimate = new_estimate; ++no_malloc;
              for ( ctr =1; ctr <=i; ctr++ ) { xdata[ctr] = xdata[ctr-1] + isz1[ctr-1];}
            }
            xdata[i][ct2++] = val;
            ct3++;
          }  
        } 
      }
      /* Update the header*/
      xdata[i][2*j]   = ct2 - oct2; /* Undo the vector isz1 and use only a var*/
      xdata[i][2*j-1] = rbuf_i[2*j-1];
    }
    xdata[i][0] = rbuf_0;
    xdata[i+1]  = xdata[i] + ct2;
    isz1[i]     = ct2; /* size of each message */
  }
  PetscFree(xtable);
  PLogInfo(0,"MatIncreaseOverlap_MPIAIJ:[%d] Allocated %d bytes, required %d bytes, no of mallocs = %d\n",rank,mem_estimate, ct3,no_malloc);    
  return 0;
}  

/* -------------------------------------------------------------------------*/
int MatGetSubMatrices_MPIAIJ(Mat C,int ismax,IS *isrow,IS *iscol,
                             MatGetSubMatrixCall scall,Mat **submat)
{ 
  Mat_MPIAIJ  *c = (Mat_MPIAIJ *) C->data;
  Mat         A = c->A,*submats = *submat;
  Mat_SeqAIJ  *a = (Mat_SeqAIJ*)A->data, *b = (Mat_SeqAIJ*)c->B->data, *mat;
  int         **irow,**icol,*nrow,*ncol,*w1,*w2,*w3,*w4,*rtable,start,end,size;
  int         **sbuf1,**sbuf2, rank, m,i,j,k,l,ct1,ct2,ierr, **rbuf1,row,proc;
  int         nrqs, msz, **ptr,index,*req_size,*ctr,*pa,tag,*tmp,tcol,bsz,nrqr;
  int         **rbuf3,*req_source,**sbuf_aj, ashift, **rbuf2, max1,max2,**rmap;
  int         **cmap,**lens,is_no,ncols,*cols,mat_i,*mat_j,tmp2,jmax,*irow_i;
  int         len,ctr_j,*sbuf1_j,*sbuf_aj_i,*rbuf1_i,kmax,*cmap_i,*lens_i;
  int         *rmap_i;
  MPI_Request *s_waits1,*r_waits1,*s_waits2,*r_waits2,*r_waits3;
  MPI_Request *r_waits4,*s_waits3,*s_waits4;
  MPI_Status  *r_status1,*r_status2,*s_status1,*s_status3,*s_status2;
  MPI_Status  *r_status3,*r_status4,*s_status4;
  MPI_Comm    comm;
  Scalar      **rbuf4, **sbuf_aa, *vals, *mat_a, *sbuf_aa_i;

  comm   = C->comm;
  tag    = C->tag;
  size   = c->size;
  rank   = c->rank;
  m      = c->M;
  ashift = a->indexshift;

    /* Check if the col indices are sorted */
  for ( i=0; i<ismax; i++ ) {
    ierr = ISSorted(iscol[i],(PetscTruth*)&j);
    if (!j) SETERRQ(1,"MatGetSubmatrices_MPIAIJ:IS is not sorted");
  }

  len    = (2*ismax+1)*(sizeof(int *) + sizeof(int)) + (m+1)*sizeof(int);
  irow   = (int **)PetscMalloc(len); CHKPTRQ(irow);
  icol   = irow + ismax;
  nrow   = (int *) (icol + ismax);
  ncol   = nrow + ismax;
  rtable = ncol + ismax;

  for ( i=0; i<ismax; i++ ) { 
    ierr = ISGetIndices(isrow[i],&irow[i]);  CHKERRQ(ierr);
    ierr = ISGetIndices(iscol[i],&icol[i]);  CHKERRQ(ierr);
    ierr = ISGetSize(isrow[i],&nrow[i]);  CHKERRQ(ierr);
    ierr = ISGetSize(iscol[i],&ncol[i]);  CHKERRQ(ierr);
  }

  /* Create hash table for the mapping :row -> proc*/
  for ( i=0,j=0; i<size; i++ ) {
    jmax = c->rowners[i+1];
    for ( ; j<jmax; j++ ) {
      rtable[j] = i;
    }
  }

  /* evaluate communication - mesg to who, length of mesg, and buffer space
     required. Based on this, buffers are allocated, and data copied into them*/
  w1     = (int *)PetscMalloc(size*4*sizeof(int));CHKPTRQ(w1); /* mesg size */
  w2     = w1 + size;      /* if w2[i] marked, then a message to proc i*/
  w3     = w2 + size;      /* no of IS that needs to be sent to proc i */
  w4     = w3 + size;      /* temp work space used in determining w1, w2, w3 */
  PetscMemzero(w1,size*3*sizeof(int)); /* initialise work vector*/
  for ( i=0; i<ismax; i++ ) { 
    PetscMemzero(w4,size*sizeof(int)); /* initialise work vector*/
    jmax   = nrow[i];
    irow_i = irow[i];
    for ( j=0; j<jmax; j++ ) {
      row  = irow_i[j];
      proc = rtable[row];
      w4[proc]++;
    }
    for ( j=0; j<size; j++ ) { 
      if (w4[j]) { w1[j] += w4[j];  w3[j]++;} 
    }
  }
  
  nrqs     = 0;              /* no of outgoing messages */
  msz      = 0;              /* total mesg length (for all proc */
  w1[rank] = 0;              /* no mesg sent to intself */
  w3[rank] = 0;
  for ( i=0; i<size; i++ ) {
    if (w1[i])  { w2[i] = 1; nrqs++;} /* there exists a message to proc i */
  }
  pa = (int *)PetscMalloc((nrqs+1)*sizeof(int));CHKPTRQ(pa); /*(proc -array)*/
  for ( i=0, j=0; i<size; i++ ) {
    if (w1[i]) { pa[j] = i; j++; }
  } 

  /* Each message would have a header = 1 + 2*(no of IS) + data */
  for ( i=0; i<nrqs; i++ ) {
    j     = pa[i];
    w1[j] += w2[j] + 2* w3[j];   
    msz   += w1[j];  
  }
  /* Do a global reduction to determine how many messages to expect*/
  {
    int *rw1, *rw2;
    rw1 = (int *)PetscMalloc(2*size*sizeof(int)); CHKPTRQ(rw1);
    rw2 = rw1+size;
    MPI_Allreduce(w1, rw1, size, MPI_INT, MPI_MAX, comm);
    bsz   = rw1[rank];
    MPI_Allreduce(w2, rw2, size, MPI_INT, MPI_SUM, comm);
    nrqr  = rw2[rank];
    PetscFree(rw1);
  }

  /* Allocate memory for recv buffers . Prob none if nrqr = 0 ???? */ 
  len      = (nrqr+1)*sizeof(int*) + nrqr*bsz*sizeof(int);
  rbuf1    = (int**) PetscMalloc(len);  CHKPTRQ(rbuf1);
  rbuf1[0] = (int *) (rbuf1 + nrqr);
  for ( i=1; i<nrqr; ++i ) rbuf1[i] = rbuf1[i-1] + bsz;
  
  /* Post the receives */
  r_waits1 = (MPI_Request *) PetscMalloc((nrqr+1)*sizeof(MPI_Request));
  CHKPTRQ(r_waits1);
  for ( i=0; i<nrqr; ++i ) {
    MPI_Irecv(rbuf1[i],bsz,MPI_INT,MPI_ANY_SOURCE,tag,comm,r_waits1+i);
  }

  /* Allocate Memory for outgoing messages */
  len      = 2*size*sizeof(int*) + 2*msz*sizeof(int) + size*sizeof(int);
  sbuf1    = (int **)PetscMalloc(len); CHKPTRQ(sbuf1);
  ptr      = sbuf1 + size;   /* Pointers to the data in outgoing buffers */
  PetscMemzero(sbuf1,2*size*sizeof(int*));
  /* allocate memory for outgoing data + buf to receive the first reply */
  tmp      = (int *) (ptr + size);
  ctr      = tmp + 2*msz;

  {
    int *iptr = tmp,ict = 0;
    for ( i=0; i<nrqs; i++ ) {
      j         = pa[i];
      iptr     += ict;
      sbuf1[j]  = iptr;
      ict       = w1[j];
    }
  }

  /* Form the outgoing messages */
  /* Initialise the header space */
  for ( i=0; i<nrqs; i++ ) {
    j           = pa[i];
    sbuf1[j][0] = 0;
    PetscMemzero(sbuf1[j]+1, 2*w3[j]*sizeof(int));
    ptr[j]      = sbuf1[j] + 2*w3[j] + 1;
  }
  
  /* Parse the isrow and copy data into outbuf */
  for ( i=0; i<ismax; i++ ) {
    PetscMemzero(ctr,size*sizeof(int));
    irow_i = irow[i];
    jmax   = nrow[i];
    for ( j=0; j<jmax; j++ ) {  /* parse the indices of each IS */
      row  = irow_i[j];
      proc = rtable[row];
      if (proc != rank) { /* copy to the outgoing buf*/
        ctr[proc]++;
        *ptr[proc] = row;
        ptr[proc]++;
      }
    }
    /* Update the headers for the current IS */
    for ( j=0; j<size; j++ ) { /* Can Optimise this loop too */
      if ((ctr_j = ctr[j])) {
        sbuf1_j        = sbuf1[j];
        k              = ++sbuf1_j[0];
        sbuf1_j[2*k]   = ctr_j;
        sbuf1_j[2*k-1] = i;
      }
    }
  }

  /*  Now  post the sends */
  s_waits1 = (MPI_Request *) PetscMalloc((nrqs+1)*sizeof(MPI_Request));
  CHKPTRQ(s_waits1);
  for ( i=0; i<nrqs; ++i ) {
    j = pa[i];
    /* printf("[%d] Send Req to %d: size %d \n", rank,j, w1[j]); */
    MPI_Isend( sbuf1[j], w1[j], MPI_INT, j, tag, comm, s_waits1+i);
  }

  /* Post Recieves to capture the buffer size */
  r_waits2 = (MPI_Request *) PetscMalloc((nrqs+1)*sizeof(MPI_Request)); 
  CHKPTRQ(r_waits2);
  rbuf2    = (int**)PetscMalloc((nrqs+1)*sizeof(int *));CHKPTRQ(rbuf2);
  rbuf2[0] = tmp + msz;
  for ( i=1; i<nrqs; ++i ) {
    j        = pa[i];
    rbuf2[i] = rbuf2[i-1]+w1[pa[i-1]];
  }
  for ( i=0; i<nrqs; ++i ) {
    j = pa[i];
    MPI_Irecv( rbuf2[i], w1[j], MPI_INT, j, tag+1, comm, r_waits2+i);
  }

  /* Send to other procs the buf size they should allocate */
 

  /* Receive messages*/
  s_waits2  = (MPI_Request *) PetscMalloc((nrqr+1)*sizeof(MPI_Request)); 
  CHKPTRQ(s_waits2);
  r_status1 = (MPI_Status *) PetscMalloc((nrqr+1)*sizeof(MPI_Status));
  CHKPTRQ(r_status1);
  len         = 2*nrqr*sizeof(int) + (nrqr+1)*sizeof(int*);
  sbuf2       = (int**) PetscMalloc(len);CHKPTRQ(sbuf2);
  req_size    = (int *) (sbuf2 + nrqr);
  req_source  = req_size + nrqr;
 
  {
    Mat_SeqAIJ *sA = (Mat_SeqAIJ*) c->A->data, *sB = (Mat_SeqAIJ*) c->B->data;
    int        *sAi = sA->i, *sBi = sB->i, id, rstart = c->rstart;
    int        *sbuf2_i;

    for ( i=0; i<nrqr; ++i ) {
      MPI_Waitany(nrqr, r_waits1, &index, r_status1+i);
      req_size[index] = 0;
      rbuf1_i         = rbuf1[index];
      start           = 2*rbuf1_i[0] + 1;
      MPI_Get_count(r_status1+i,MPI_INT, &end);
      sbuf2[index] = (int *)PetscMalloc(end*sizeof(int));CHKPTRQ(sbuf2[index]);
      sbuf2_i      = sbuf2[index];
      for ( j=start; j<end; j++ ) {
        id               = rbuf1_i[j] - rstart;
        ncols            = sAi[id+1] - sAi[id] + sBi[id+1] - sBi[id];
        sbuf2_i[j]       = ncols;
        req_size[index] += ncols;
      }
      req_source[index] = r_status1[i].MPI_SOURCE;
      /* form the header */
      sbuf2_i[0]   = req_size[index];
      for ( j=1; j<start; j++ ) { sbuf2_i[j] = rbuf1_i[j]; }
      MPI_Isend(sbuf2_i,end,MPI_INT,req_source[index],tag+1,comm,s_waits2+i); 
    }
  }
  PetscFree(r_status1); PetscFree(r_waits1);

  /*  recv buffer sizes */
  /* Receive messages*/
  
  rbuf3    = (int**)PetscMalloc((nrqs+1)*sizeof(int*)); CHKPTRQ(rbuf3);
  rbuf4    = (Scalar**)PetscMalloc((nrqs+1)*sizeof(Scalar*));CHKPTRQ(rbuf4);
  r_waits3 = (MPI_Request *) PetscMalloc((nrqs+1)*sizeof(MPI_Request)); 
  CHKPTRQ(r_waits3);
  r_waits4 = (MPI_Request *) PetscMalloc((nrqs+1)*sizeof(MPI_Request)); 
  CHKPTRQ(r_waits4);
  r_status2 = (MPI_Status *) PetscMalloc( (nrqs+1)*sizeof(MPI_Status) );
  CHKPTRQ(r_status2);

  for ( i=0; i<nrqs; ++i ) {
    MPI_Waitany(nrqs, r_waits2, &index, r_status2+i);
    rbuf3[index] = (int *)PetscMalloc(rbuf2[index][0]*sizeof(int)); 
    CHKPTRQ(rbuf3[index]);
    rbuf4[index] = (Scalar *)PetscMalloc(rbuf2[index][0]*sizeof(Scalar));
    CHKPTRQ(rbuf4[index]);
    MPI_Irecv(rbuf3[index],rbuf2[index][0], MPI_INT, 
              r_status2[i].MPI_SOURCE, tag+2, comm, r_waits3+index); 
    MPI_Irecv(rbuf4[index],rbuf2[index][0], MPIU_SCALAR, 
              r_status2[i].MPI_SOURCE, tag+3, comm, r_waits4+index); 
  } 
  PetscFree(r_status2); PetscFree(r_waits2);
  
  /* Wait on sends1 and sends2 */
  s_status1 = (MPI_Status *) PetscMalloc((nrqs+1)*sizeof(MPI_Status));
  CHKPTRQ(s_status1);
  s_status2 = (MPI_Status *) PetscMalloc((nrqr+1)*sizeof(MPI_Status));
  CHKPTRQ(s_status2);

  MPI_Waitall(nrqs,s_waits1,s_status1);
  MPI_Waitall(nrqr,s_waits2,s_status2);
  PetscFree(s_status1); PetscFree(s_status2);
  PetscFree(s_waits1); PetscFree(s_waits2);

  /* Now allocate buffers for a->j, and send them off */
  sbuf_aj = (int **)PetscMalloc((nrqr+1)*sizeof(int *));CHKPTRQ(sbuf_aj);
  for ( i=0,j=0; i<nrqr; i++ ) j += req_size[i];
  sbuf_aj[0] = (int*) PetscMalloc((j+1)*sizeof(int)); CHKPTRQ(sbuf_aj[0]);
  for ( i=1; i<nrqr; i++ )  sbuf_aj[i] = sbuf_aj[i-1] + req_size[i-1];
  
  s_waits3 = (MPI_Request *) PetscMalloc((nrqr+1)*sizeof(MPI_Request)); 
  CHKPTRQ(s_waits3);
  {
    int nzA, nzB, *a_i = a->i, *b_i = b->i, imark;
    int *cworkA, *cworkB, cstart = c->cstart, *bmap = c->garray;
    int *a_j = a->j, *b_j = b->j, shift = a->indexshift,ctmp, *t_cols;

    for ( i=0; i<nrqr; i++ ) {
      rbuf1_i   = rbuf1[i]; 
      sbuf_aj_i = sbuf_aj[i];
      ct1       = 2*rbuf1_i[0] + 1;
      ct2       = 0;
      for ( j=1,max1=rbuf1_i[0]; j<=max1; j++ ) { 
        kmax = rbuf1[i][2*j];
        for ( k=0; k<kmax; k++,ct1++ ) {
          row    = rbuf1_i[ct1] - cstart;
          nzA    = a_i[row+1] - a_i[row];     nzB = b_i[row+1] - b_i[row];
          ncols  = nzA + nzB;
          cworkA = a_j + a_i[row]; cworkB = b_j + b_i[row];

          /* load the column indices for this row into cols*/
          cols  = sbuf_aj_i + ct2;
          if (!shift) {
            for ( l=0; l<nzB; l++ ) {
              if ((ctmp = bmap[cworkB[l]]) < cstart)  cols[l] = ctmp;
              else break;
            }
            imark = l;
            for ( l=0; l<nzA; l++ )   cols[imark+l] = cstart + cworkA[l];
            for ( l=imark; l<nzB; l++ ) cols[nzA+l] = bmap[cworkB[l]];
          }
          else {
            ierr = MatGetRow_MPIAIJ(C,rbuf1_i[ct1],&ncols,&t_cols,0); CHKERRQ(ierr);
            PetscMemcpy(cols, t_cols, ncols*sizeof(int));
            ierr = MatRestoreRow_MPIAIJ(C,rbuf1_i[ct1],&ncols,&t_cols,0); CHKERRQ(ierr);

          }

          ct2 += ncols;
        }
      }
      MPI_Isend(sbuf_aj_i,req_size[i],MPI_INT,req_source[i],tag+2,comm,s_waits3+i);
    }
  } 
  r_status3 = (MPI_Status *) PetscMalloc((nrqs+1)*sizeof(MPI_Status));
  CHKPTRQ(r_status3);
  s_status3 = (MPI_Status *) PetscMalloc((nrqr+1)*sizeof(MPI_Status));
  CHKPTRQ(s_status3);

  /* Allocate buffers for a->a, and send them off */
  sbuf_aa = (Scalar **)PetscMalloc((nrqr+1)*sizeof(Scalar *));CHKPTRQ(sbuf_aa);
  for ( i=0,j=0; i<nrqr; i++ ) j += req_size[i];
  sbuf_aa[0] = (Scalar*) PetscMalloc((j+1)*sizeof(Scalar));CHKPTRQ(sbuf_aa[0]);
  for ( i=1; i<nrqr; i++ )  sbuf_aa[i] = sbuf_aa[i-1] + req_size[i-1];
  
  s_waits4 = (MPI_Request *) PetscMalloc((nrqr+1)*sizeof(MPI_Request)); 
  CHKPTRQ(s_waits4);
  {
    int    nzA, nzB, *a_i = a->i, *b_i = b->i, imark;
    int    *cworkA, *cworkB, cstart = c->cstart, *bmap = c->garray;
    int    *a_j = a->j, *b_j = b->j,shift = a->indexshift;
    Scalar *vworkA, *vworkB, *a_a = a->a, *b_a = b->a,*t_vals;
    
    for ( i=0; i<nrqr; i++ ) {
      rbuf1_i   = rbuf1[i];
      sbuf_aa_i = sbuf_aa[i];
      ct1       = 2*rbuf1_i[0]+1;
      ct2       = 0;
      for ( j=1,max1=rbuf1_i[0]; j<=max1; j++ ) {
        kmax = rbuf1_i[2*j];
        for ( k=0; k<kmax; k++,ct1++ ) {
          row    = rbuf1_i[ct1] - cstart;
          nzA    = a_i[row+1] - a_i[row];     nzB = b_i[row+1] - b_i[row];
          ncols  = nzA + nzB;
          cworkA = a_j + a_i[row]; cworkB = b_j + b_i[row];
          vworkA = a_a + a_i[row]; vworkB = b_a + b_i[row];

          /* load the column values for this row into vals*/
          vals  = sbuf_aa_i+ct2;
          if (!shift) {
            for ( l=0; l<nzB; l++ ) {
              if ((bmap[cworkB[l]]) < cstart)  vals[l] = vworkB[l];
              else break;
            }
            imark = l;
            for ( l=0; l<nzA; l++ )   vals[imark+l] = vworkA[l];
            for ( l=imark; l<nzB; l++ ) vals[nzA+l] = vworkB[l];
          }
          else {
            ierr = MatGetRow_MPIAIJ(C,rbuf1_i[ct1],&ncols,0,&t_vals); CHKERRQ(ierr);
            PetscMemcpy(vals, t_vals, ncols*sizeof(Scalar));
            ierr = MatRestoreRow_MPIAIJ(C,rbuf1_i[ct1],&ncols,0,&t_vals); CHKERRQ(ierr);
          }
          ct2 += ncols;
        }
      }
      MPI_Isend(sbuf_aa_i,req_size[i],MPIU_SCALAR,req_source[i],tag+3,comm,s_waits4+i);
    }
  } 
  r_status4 = (MPI_Status *) PetscMalloc((nrqs+1)*sizeof(MPI_Status));
  CHKPTRQ(r_status4);
  s_status4 = (MPI_Status *) PetscMalloc((nrqr+1)*sizeof(MPI_Status));
  CHKPTRQ(s_status4);
  PetscFree(rbuf1);

  /* Form the matrix */
  /* create col map */
  {
    int *icol_i;
    
    len     = (1+ismax)*sizeof(int *) + ismax*c->N*sizeof(int);
    cmap    = (int **)PetscMalloc(len); CHKPTRQ(cmap);
    cmap[0] = (int *)(cmap + ismax);
    PetscMemzero(cmap[0],(1+ismax*c->N)*sizeof(int));
    for ( i=1; i<ismax; i++ ) { cmap[i] = cmap[i-1] + c->N; }
    for ( i=0; i<ismax; i++ ) {
      jmax   = ncol[i];
      icol_i = icol[i];
      cmap_i = cmap[i];
      for ( j=0; j<jmax; j++ ) { 
        cmap_i[icol_i[j]] = j+1; 
      }
    }
  }
  

  /* Create lens which is required for MatCreate... */
  for ( i=0,j=0; i<ismax; i++ ) { j += nrow[i]; }
  len     = (1+ismax)*sizeof(int *) + j*sizeof(int);
  lens    = (int **)PetscMalloc(len); CHKPTRQ(lens);
  lens[0] = (int *)(lens + ismax);
  PetscMemzero(lens[0], j*sizeof(int));
  for ( i=1; i<ismax; i++ ) { lens[i] = lens[i-1] + nrow[i-1]; }
  
  /* Update lens from local data */
  for ( i=0; i<ismax; i++ ) {
    jmax   = nrow[i];
    cmap_i = cmap[i];
    irow_i = irow[i];
    lens_i = lens[i];
    for ( j=0; j<jmax; j++ ) {
      row  = irow_i[j];
      proc = rtable[row];
      if (proc == rank) {
        ierr = MatGetRow_MPIAIJ(C,row,&ncols,&cols,0); CHKERRQ(ierr);
        for ( k=0; k<ncols; k++ ) {
          if (cmap_i[cols[k]]) { lens_i[j]++;}
        }
        ierr = MatRestoreRow_MPIAIJ(C,row,&ncols,&cols,0); CHKERRQ(ierr);
      }
    }
  }
  
  /* Create row map*/
  len     = (1+ismax)*sizeof(int *) + ismax*c->M*sizeof(int);
  rmap    = (int **)PetscMalloc(len); CHKPTRQ(rmap);
  rmap[0] = (int *)(rmap + ismax);
  PetscMemzero(rmap[0],ismax*c->M*sizeof(int));
  for ( i=1; i<ismax; i++ ) { rmap[i] = rmap[i-1] + c->M;}
  for ( i=0; i<ismax; i++ ) {
    rmap_i = rmap[i];
    irow_i = irow[i];
    jmax   = nrow[i];
    for ( j=0; j<jmax; j++ ) { 
      rmap_i[irow_i[j]] = j; 
    }
  }
 
  /* Update lens from offproc data */
  {
    int *rbuf2_i, *rbuf3_i, *sbuf1_i;

    for ( tmp2=0; tmp2<nrqs; tmp2++ ) {
      MPI_Waitany(nrqs, r_waits3, &i, r_status3+tmp2);
      index   = pa[i];
      sbuf1_i = sbuf1[index];
      jmax    = sbuf1_i[0];
      ct1     = 2*jmax+1; 
      ct2     = 0;               
      rbuf2_i = rbuf2[i];
      rbuf3_i = rbuf3[i];
      for ( j=1; j<=jmax; j++ ) {
        is_no   = sbuf1_i[2*j-1];
        max1    = sbuf1_i[2*j];
        lens_i  = lens[is_no];
        cmap_i  = cmap[is_no];
        rmap_i  = rmap[is_no];
        for ( k=0; k<max1; k++,ct1++ ) {
          row  = rmap_i[sbuf1_i[ct1]]; /* the val in the new matrix to be */
          max2 = rbuf2_i[ct1];
          for ( l=0; l<max2; l++,ct2++ ) {
            if (cmap_i[rbuf3_i[ct2]]) {
              lens_i[row]++;
            }
          }
        }
      }
    }
  }    
  PetscFree(r_status3); PetscFree(r_waits3);
  MPI_Waitall(nrqr,s_waits3,s_status3); 
  PetscFree(s_status3); PetscFree(s_waits3);

  /* Create the submatrices */
  if (scall == MAT_REUSE_MATRIX) {
    /*
        Assumes new rows are same length as the old rows, hence bug!
    */
    for ( i=0; i<ismax; i++ ) {
      mat = (Mat_SeqAIJ *)(submats[i]->data);
      if ((mat->m != nrow[i]) || (mat->n != ncol[i])) {
        SETERRQ(1,"MatGetSubmatrices_MPIAIJ:Cannot reuse matrix. wrong size");
      }
      if (PetscMemcmp(mat->ilen,lens[i], mat->m *sizeof(int))) {
        SETERRQ(1,"MatGetSubmatrices_MPIAIJ:Cannot reuse matrix. wrong no of nonzeros");
      }
      /* Initial matrix as if empty */
      PetscMemzero(mat->ilen,mat->m*sizeof(int));
    }
  }
  else {
    *submat = submats = (Mat *)PetscMalloc(ismax*sizeof(Mat)); CHKPTRQ(submats);
    for ( i=0; i<ismax; i++ ) {
      ierr = MatCreateSeqAIJ(comm,nrow[i],ncol[i],0,lens[i],submats+i);CHKERRQ(ierr);
    }
  }

  /* Assemble the matrices */
  /* First assemble the local rows */
  {
    int    ilen_row,*imat_ilen, *imat_j, *imat_i,old_row;
    Scalar *imat_a;
  
    for ( i=0; i<ismax; i++ ) {
      mat       = (Mat_SeqAIJ *) submats[i]->data;
      imat_ilen = mat->ilen;
      imat_j    = mat->j;
      imat_i    = mat->i;
      imat_a    = mat->a;
      cmap_i    = cmap[i];
      rmap_i    = rmap[i];
      irow_i    = irow[i];
      jmax      = nrow[i];
      for ( j=0; j<jmax; j++ ) {
        row      = irow_i[j];
        proc     = rtable[row];
        if (proc == rank) {
          old_row  = row;
          row      = rmap_i[row];
          ilen_row = imat_ilen[row];
          ierr     = MatGetRow_MPIAIJ(C,old_row,&ncols,&cols,&vals); CHKERRQ(ierr);
          mat_i    = imat_i[row] + ashift;
          mat_a    = imat_a + mat_i;
          mat_j    = imat_j + mat_i;
          for ( k=0; k<ncols; k++ ) {
            if ((tcol = cmap_i[cols[k]])) { 
              *mat_j++ = tcol - (!ashift);
              *mat_a++ = vals[k];
              ilen_row++;
            }
          }
          ierr = MatRestoreRow_MPIAIJ(C,old_row,&ncols,&cols,&vals); CHKERRQ(ierr);
          imat_ilen[row] = ilen_row; 
        }

      }
    }
  }

  /*   Now assemble the off proc rows*/
  {
    int    *sbuf1_i,*rbuf2_i,*rbuf3_i,*imat_ilen,ilen;
    int    *imat_j,*imat_i;
    Scalar *imat_a,*rbuf4_i;

    for ( tmp2=0; tmp2<nrqs; tmp2++ ) {
      MPI_Waitany(nrqs, r_waits4, &i, r_status4+tmp2);
      index   = pa[i];
      sbuf1_i = sbuf1[index];
      jmax    = sbuf1_i[0];           
      ct1     = 2*jmax + 1; 
      ct2     = 0;    
      rbuf2_i = rbuf2[i];
      rbuf3_i = rbuf3[i];
      rbuf4_i = rbuf4[i];
      for ( j=1; j<=jmax; j++ ) {
        is_no     = sbuf1_i[2*j-1];
        rmap_i    = rmap[is_no];
        cmap_i    = cmap[is_no];
        mat       = (Mat_SeqAIJ *) submats[is_no]->data;
        imat_ilen = mat->ilen;
        imat_j    = mat->j;
        imat_i    = mat->i;
        imat_a    = mat->a;
        max1      = sbuf1_i[2*j];
        for ( k=0; k<max1; k++, ct1++ ) {
          row   = sbuf1_i[ct1];
          row   = rmap_i[row]; 
          ilen  = imat_ilen[row];
          mat_i = imat_i[row] + ashift;
          mat_a = imat_a + mat_i;
          mat_j = imat_j + mat_i;
          max2 = rbuf2_i[ct1];
          for ( l=0; l<max2; l++,ct2++ ) {
            if ((tcol = cmap_i[rbuf3_i[ct2]])) {
              *mat_j++ = tcol - (!ashift);
              *mat_a++ = rbuf4_i[ct2];
              ilen++;
            }
          }
          imat_ilen[row] = ilen;
        }
      }
    }
  }    
  PetscFree(r_status4); PetscFree(r_waits4);
  MPI_Waitall(nrqr,s_waits4,s_status4); 
  PetscFree(s_waits4); PetscFree(s_status4);

  /* Restore the indices */
  for ( i=0; i<ismax; i++ ) {
    ierr = ISRestoreIndices(isrow[i], irow+i); CHKERRQ(ierr);
    ierr = ISRestoreIndices(iscol[i], icol+i); CHKERRQ(ierr);
  }

  /* Destroy allocated memory */
  PetscFree(irow);
  PetscFree(w1);
  PetscFree(pa);

  PetscFree(sbuf1);
  PetscFree(rbuf2);
  for ( i=0; i<nrqr; ++i ) {
    PetscFree(sbuf2[i]);
  }
  for ( i=0; i<nrqs; ++i ) {
    PetscFree(rbuf3[i]);
    PetscFree(rbuf4[i]);
  }

  PetscFree(sbuf2);
  PetscFree(rbuf3);
  PetscFree(rbuf4 );
  PetscFree(sbuf_aj[0]);
  PetscFree(sbuf_aj);
  PetscFree(sbuf_aa[0]);
  PetscFree(sbuf_aa);
  
  PetscFree(cmap);
  PetscFree(rmap);
  PetscFree(lens);

  for ( i=0; i<ismax; i++ ) {
    ierr = MatAssemblyBegin(submats[i], FINAL_ASSEMBLY); CHKERRQ(ierr);
    ierr = MatAssemblyEnd(submats[i], FINAL_ASSEMBLY); CHKERRQ(ierr);
  }

  return 0;
}




