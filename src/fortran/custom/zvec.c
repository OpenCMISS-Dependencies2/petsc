#ifdef PETSC_RCS_HEADER
static char vcid[] = "$Id: zvec.c,v 1.39 1998/06/11 19:53:29 bsmith Exp bsmith $";
#endif

#include "src/fortran/custom/zpetsc.h"
#include "vec.h"
#ifdef HAVE_FORTRAN_CAPS
#define vecsetvalue_           VECSETVALUE
#define vecmaxpy_              VECMAXPY
#define vecmdot_               VECMDOT
#define veccreateseq_          VECCREATESEQ
#define veccreateseqwitharray_ VECCREATESEQWITHARRAY
#define veccreatempiwitharray_ VECCREATEMPIWITHARRAY
#define veccreate_             VECCREATE
#define vecduplicate_          VECDUPLICATE
#define veccreatempi_          VECCREATEMPI
#define veccreateshared_       VECCREATESHARED
#define vecscattercreate_      VECSCATTERCREATE
#define vecscattercopy_        VECSCATTERCOPY
#define vecdestroy_            VECDESTROY
#define vecdestroyvecs_        VECDESTROYVECS
#define vecscatterdestroy_     VECSCATTERDESTROY
#define vecrestorearray_       VECRESTOREARRAY
#define vecgetarray_           VECGETARRAY
#define vecload_               VECLOAD
#define vecgettype_            VECGETTYPE
#define vecduplicatevecs_      VECDUPLICATEVECS
#define vecview_               VECVIEW
#define mapgetlocalsize_       MAPGETLOCALSIZE
#define mapgetglobalsize_      MAPGETGLOBALSIZE
#define mapgetlocalrange_      MAPGETLOCALRANGE
#define mapgetglobalrange_     MAPGETGLOBALRANGE
#define mapdestroy_            MAPDESTROY
#define mapcreatempi_          MAPCREATEMPI
#define vecgetmap_             VECGETMAP
#define vecghostgetlocalrepresentation_     VECGHOSTGETLOCALREPRESENTATION
#define vecghostrestorelocalrepresentation_ VECGHOSTRESTORELOCALREPRESENTATION
#define veccreateghostwitharray_            VECCREATEGHOSTWITHARRAY
#define veccreateghost_                     VECCREATEGHOST
#elif !defined(HAVE_FORTRAN_UNDERSCORE)
#define vecghostrestorelocalrepresentation_ vecghostrestorelocalrepresentation
#define vecghostgetlocalrepresentation_     vecghostgetlocalrepresentation
#define veccreateghostwitharray_            veccreateghostwitharray
#define veccreateghost_                     veccreateghost
#define vecgetmap_             vecgetmap
#define mapcreatempi_          mapcreatempi
#define mapgetglobalrange_     mapgetglobalrange
#define mapgetglobalsize_      mapgetglobalsize
#define mapgetlocalsize_       mapgetlocalsize
#define mapgetlocalrange_      mapgetlocalrange
#define mapdestroy_            mapdestroy
#define vecsetvalue_           vecsetvalue
#define vecview_               vecview
#define vecmaxpy_              vecmaxpy
#define vecmdot_               vecmdot
#define veccreateseq_          veccreateseq
#define veccreateseqwitharray_ veccreateseqwitharray
#define veccreatempiwitharray_ veccreatempiwitharray
#define veccreate_             veccreate
#define vecduplicate_          vecduplicate
#define veccreatempi_          veccreatempi
#define veccreateshared_       veccreateshared
#define vecscattercreate_      vecscattercreate
#define vecscattercopy_        vecscattercopy
#define vecdestroy_            vecdestroy
#define vecdestroyvecs_        vecdestroyvecs
#define vecscatterdestroy_     vecscatterdestroy
#define vecrestorearray_       vecrestorearray
#define vecgetarray_           vecgetarray
#define vecload_               vecload
#define vecgettype_            vecgettype
#define vecduplicatevecs_      vecduplicatevecs
#endif

#if defined(__cplusplus)
extern "C" {
#endif

void vecgetmap_(Vec x,Map *map, int *__ierr )
{
  Map m;
  *__ierr = VecGetMap((Vec)PetscToPointer( (x) ),&m);
  *(PetscFortranAddr*) map = PetscFromPointer(m);
}

void mapgetlocalsize_(Map *m,int *n, int *__ierr )
{
  *__ierr = MapGetLocalSize((Map)PetscToPointer(m),n);
}

void mapgetglobalsize_(Map *m,int *N, int *__ierr )
{
  *__ierr = MapGetGlobalSize((Map)PetscToPointer(m),N);
}

void mapgetlocalrange_(Map *m,int *rstart,int *rend, int *__ierr )
{
  *__ierr = MapGetLocalRange((Map)PetscToPointer(m),rstart,rend);
}

void mapgetglobalrange_(Map *m,int **range, int *__ierr )
{
  *__ierr = MapGetGlobalRange((Map)PetscToPointer(m),range);
}

void mapdestroy_(Map *m, int *__ierr )
{
  *__ierr = MapDestroy((Map)PetscToPointer(m));
}

void vecsetvalue_(Vec v,int *i,Scalar *va,InsertMode *mode)
{
  /* cannot use VecSetValue() here since that uses CHKERRQ() which has a return in it */
  VecSetValues((Vec)PetscToPointer(v),1,i,va,*mode);
}

void vecview_(Vec v,Viewer viewer, int *__ierr )
{
  PetscPatchDefaultViewers_Fortran(viewer);
  *__ierr = VecView((Vec)PetscToPointer(v),viewer);
}

void vecgettype_(Vec vv,CHAR name,int *__ierr,int len)
{
  char *tname;
  *__ierr = VecGetType((Vec)PetscToPointer(vv),&tname);
#if defined(USES_CPTOFCD)
  {
  char *t = _fcdtocp(name); int len1 = _fcdlen(name);
  PetscStrncpy(t,tname,len1);
  }
#else
  PetscStrncpy(name,tname,len);
#endif

}

void vecload_(Viewer viewer,Vec *newvec, int *__ierr )
{ 
  Vec vv;
  *__ierr = VecLoad((Viewer)PetscToPointer(viewer),&vv);
  *(PetscFortranAddr*) newvec = PetscFromPointer(vv);
}

/* Be to keep vec/examples/ex21.F and snes/examples/ex12.F up to date */
void vecrestorearray_(Vec x,Scalar *fa,long *ia,int *__ierr)
{
  Vec    xin = (Vec)PetscToPointer(x);
  int    m;
  Scalar *lx;

  *__ierr = VecGetLocalSize(xin,&m);if (*__ierr) return;
  *__ierr = PetscScalarAddressFromFortran((PetscObject)xin,fa,*ia,m,&lx);if (*__ierr) return;
  *__ierr = VecRestoreArray(xin,&lx);if (*__ierr) return;
}

void vecgetarray_(Vec x,Scalar *fa,long *ia,int *__ierr)
{
  Vec    xin = (Vec)PetscToPointer(x);
  Scalar *lx;
  int    m;

  *__ierr = VecGetArray(xin,&lx); if (*__ierr) return;
  *__ierr = VecGetLocalSize(xin,&m);if (*__ierr) return;
  *__ierr = PetscScalarAddressToFortran((PetscObject) xin,fa,lx,m,ia);if (*__ierr) return;
}

void vecscatterdestroy_(VecScatter ctx, int *__ierr )
{
  *__ierr = VecScatterDestroy((VecScatter)PetscToPointer(ctx));
   PetscRmPointer(ctx); 
}

void vecdestroy_(Vec v, int *__ierr )
{
  *__ierr = VecDestroy((Vec)PetscToPointer(v));
   PetscRmPointer(v); 
}

void vecscattercreate_(Vec xin,IS ix,Vec yin,IS iy,VecScatter *newctx, int *__ierr )
{
  VecScatter lV;
  *__ierr = VecScatterCreate((Vec)PetscToPointer(xin),
                             (IS)PetscToPointer(ix),
                             (Vec)PetscToPointer(yin),
                             (IS)PetscToPointer(iy),&lV);
  *(PetscFortranAddr*) newctx = PetscFromPointer(lV);
}

void vecscattercopy_(VecScatter sctx,VecScatter *ctx, int *__ierr )
{
  VecScatter lV;
  *__ierr = VecScatterCopy((VecScatter)PetscToPointer(sctx),&lV);
   *(PetscFortranAddr*) ctx = PetscFromPointer(lV); 
}

void mapcreatempi_(MPI_Comm *comm,int *n,int *N,Map *vv, int *__ierr )
{
  Map lV;
  *__ierr = MapCreateMPI((MPI_Comm)PetscToPointerComm( *comm ),*n,*N,&lV);
  *(PetscFortranAddr*)vv = PetscFromPointer(lV);
}

void veccreatempi_(MPI_Comm *comm,int *n,int *N,Vec *vv, int *__ierr )
{
  Vec lV;
  *__ierr = VecCreateMPI((MPI_Comm)PetscToPointerComm( *comm ),*n,*N,&lV);
  *(PetscFortranAddr*)vv = PetscFromPointer(lV);
}

void veccreateshared_(MPI_Comm *comm,int *n,int *N,Vec *vv, int *__ierr )
{
  Vec lV;
  *__ierr = VecCreateShared((MPI_Comm)PetscToPointerComm( *comm ),*n,*N,&lV);
  *(PetscFortranAddr*)vv = PetscFromPointer(lV);
}

void veccreateseq_(MPI_Comm *comm,int *n,Vec *V, int *__ierr )
{
  Vec lV;
  *__ierr = VecCreateSeq((MPI_Comm)PetscToPointerComm( *comm),*n,&lV);
  *(PetscFortranAddr*)V = PetscFromPointer(lV);
}

void veccreateseqwitharray_(MPI_Comm *comm,int *n,Scalar *s,Vec *V, int *__ierr )
{
  Vec lV;
  *__ierr = VecCreateSeqWithArray((MPI_Comm)PetscToPointerComm( *comm),*n,s,&lV);
  *(PetscFortranAddr*)V = PetscFromPointer(lV);
}

void veccreatempiwitharray_(MPI_Comm *comm,int *n,int *N,Scalar *s,Vec *V, int *__ierr )
{
  Vec lV;
  *__ierr = VecCreateMPIWithArray((MPI_Comm)PetscToPointerComm( *comm),*n,*N,s,&lV);
  *(PetscFortranAddr*)V = PetscFromPointer(lV);
}

void veccreate_(MPI_Comm *comm,int *n,int *N,Vec *V, int *__ierr )
{
  Vec lV;
  *__ierr = VecCreate((MPI_Comm)PetscToPointerComm( *comm ),*n,*N,&lV);
  *(PetscFortranAddr*)V = PetscFromPointer(lV);
}

void vecduplicate_(Vec v,Vec *newv, int *__ierr )
{
  Vec lV;
  *__ierr = VecDuplicate((Vec)PetscToPointer(v),&lV);
  *(PetscFortranAddr*)newv = PetscFromPointer(lV);
}

void vecduplicatevecs_(Vec v,int *m,PetscFortranAddr *newv, int *__ierr )
{
  Vec *lV;
  int i;
  *__ierr = VecDuplicateVecs((Vec)PetscToPointer(v),*m,&lV);
  for (i=0; i<*m; i++) {
    newv[i] = PetscFromPointer(lV[i]);
  }
  PetscFree(lV); 
}

void vecdestroyvecs_(PetscFortranAddr *vecs,int *m,int *__ierr )
{
  int i;
  for (i=0; i<*m; i++) {
    *__ierr = VecDestroy((Vec)PetscToPointer(&vecs[i]));
    PetscRmPointer(&vecs[i]); 
  }
}

void vecmtdot_(int *nv,Vec x,PetscFortranAddr *y,Scalar *val, int *__ierr )
{
  int i;
  Vec *yV = (Vec *) PetscMalloc( *nv * sizeof(Vec *));
  if (!(yV)) {
     *__ierr = PetscError(__LINE__,"VecMTDot_Fortran",__FILE__,__SDIR__,PETSC_ERR_MEM,0,(char*)0);
     return;
  }
  for (i=0; i<*nv; i++) yV[i] = ((Vec)PetscToPointer(&y[i]));
  *__ierr = VecMTDot(*nv,(Vec)PetscToPointer(x),yV,val);
  PetscFree(yV);
}

void vecmdot_(int *nv,Vec x,PetscFortranAddr *y,Scalar *val, int *__ierr )
{
  int i;
  Vec *yV = (Vec *) PetscMalloc( *nv * sizeof(Vec *));
  if (!(yV)) {
     *__ierr = PetscError(__LINE__,"VecMDot_Fortran",__FILE__,__SDIR__,PETSC_ERR_MEM,0,(char*)0);
     return;
  }
  for (i=0; i<*nv; i++) yV[i] = ((Vec)PetscToPointer(&y[i]));
  *__ierr = VecMDot(*nv,(Vec)PetscToPointer(x),yV,val);
  PetscFree(yV);
}

void vecmaxpy_(int *nv,Scalar *alpha,Vec x,PetscFortranAddr *y, int *__ierr )
{
  int i;
  Vec *yV = (Vec *) PetscMalloc( *nv * sizeof(Vec *));
  if (!(yV)) {
     *__ierr = PetscError(__LINE__,"VecMAXPY_Fortran",__FILE__,__SDIR__,PETSC_ERR_MEM,0,(char*)0);
     return;
  }
  for (i=0; i<*nv; i++) yV[i] = ((Vec)PetscToPointer(&y[i]));
  *__ierr = VecMAXPY(*nv,alpha,(Vec)PetscToPointer(x),yV);
  PetscFree(yV);
}

/* ----------------------------------------------------------------------------------------------*/
void veccreateghostwitharray_(MPI_Comm *comm,int *n,int *N,int *nghost,int *ghosts,Scalar *array,
                              Vec *vv, int *__ierr )
{
  Vec vec;
  *__ierr = VecCreateGhostWithArray((MPI_Comm)PetscToPointerComm( *comm) ,*n,*N,*nghost,
                                    ghosts,array,&vec);
  *(PetscFortranAddr*) vv = PetscFromPointer(vec);
}

void veccreateghost_(MPI_Comm *comm,int *n,int *N,int *nghost,int *ghosts,Vec *vv, int *__ierr )
{
  Vec vec;
  *__ierr = VecCreateGhost((MPI_Comm)PetscToPointerComm( *comm),*n,*N,*nghost,ghosts,&vec);
  *(PetscFortranAddr*) vv = PetscFromPointer(vec);
}

void vecghostgetlocalrepresentation_(Vec g,Vec *l, int *__ierr )
{
  Vec vec;
  *__ierr = VecGhostGetLocalRepresentation(
	(Vec)PetscToPointer( g),&vec);
  *(PetscFortranAddr*) l = PetscFromPointer(vec);
}

void vecghostrestorelocalrepresentation_(Vec g,Vec *l, int *__ierr )
{
  *__ierr = VecGhostRestoreLocalRepresentation((Vec)PetscToPointer( g ),l);
}

#if defined(__cplusplus)
}
#endif
