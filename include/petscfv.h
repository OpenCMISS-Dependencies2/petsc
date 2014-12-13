/*
      Objects which encapsulate finite volume spaces and operations
*/
#if !defined(__PETSCFV_H)
#define __PETSCFV_H
#include <petscdm.h>
#include <petscdt.h>
#include <petscfvtypes.h>
#include <petscdstypes.h>

/* Assuming dim <= 3 */
typedef struct {
  PetscReal   normal[3];   /* Area-scaled normals */
  PetscReal   centroid[3]; /* Location of centroid (quadrature point) */
  PetscScalar grad[2][3];  /* Face contribution to gradient in left and right cell */
} PetscFVFaceGeom;

typedef struct {
  PetscReal centroid[3];
  PetscReal volume;
} PetscFVCellGeom;

PETSC_EXTERN PetscClassId PETSCLIMITER_CLASSID;

/*J
  PetscLimiterType - String with the name of a PETSc finite volume slope limiter

  Level: beginner

.seealso: PetscLimiterSetType(), PetscLimiter
J*/
typedef const char *PetscLimiterType;
#define PETSCLIMITERSIN       "sin"
#define PETSCLIMITERZERO      "zero"
#define PETSCLIMITERNONE      "none"
#define PETSCLIMITERMINMOD    "minmod"
#define PETSCLIMITERVANLEER   "vanleer"
#define PETSCLIMITERVANALBADA "vanalbada"
#define PETSCLIMITERSUPERBEE  "superbee"
#define PETSCLIMITERMC        "mc"

PETSC_EXTERN PetscFunctionList PetscLimiterList;
PETSC_EXTERN PetscErrorCode PetscLimiterCreate(MPI_Comm, PetscLimiter *);
PETSC_EXTERN PetscErrorCode PetscLimiterDestroy(PetscLimiter *);
PETSC_EXTERN PetscErrorCode PetscLimiterSetType(PetscLimiter, PetscLimiterType);
PETSC_EXTERN PetscErrorCode PetscLimiterGetType(PetscLimiter, PetscLimiterType *);
PETSC_EXTERN PetscErrorCode PetscLimiterSetUp(PetscLimiter);
PETSC_EXTERN PetscErrorCode PetscLimiterSetFromOptions(PetscLimiter);
PETSC_EXTERN PetscErrorCode PetscLimiterViewFromOptions(PetscLimiter, const char[], const char[]);
PETSC_EXTERN PetscErrorCode PetscLimiterView(PetscLimiter, PetscViewer);
PETSC_EXTERN PetscErrorCode PetscLimiterRegister(const char [], PetscErrorCode (*)(PetscLimiter));
PETSC_EXTERN PetscErrorCode PetscLimiterRegisterAll(void);
PETSC_EXTERN PetscErrorCode PetscLimiterRegisterDestroy(void);

PETSC_EXTERN PetscErrorCode PetscLimiterLimit(PetscLimiter, PetscReal, PetscReal *);


PETSC_EXTERN PetscErrorCode PetscFVInitializePackage(void);

PETSC_EXTERN PetscClassId PETSCFV_CLASSID;

/*J
  PetscFVType - String with the name of a PETSc finite volume discretization

  Level: beginner

.seealso: PetscFVSetType(), PetscFV
J*/
typedef const char *PetscFVType;
#define PETSCFVUPWIND       "upwind"
#define PETSCFVLEASTSQUARES "leastsquares"

PETSC_EXTERN PetscFunctionList PetscFVList;
PETSC_EXTERN PetscErrorCode PetscFVCreate(MPI_Comm, PetscFV *);
PETSC_EXTERN PetscErrorCode PetscFVDestroy(PetscFV *);
PETSC_EXTERN PetscErrorCode PetscFVSetType(PetscFV, PetscFVType);
PETSC_EXTERN PetscErrorCode PetscFVGetType(PetscFV, PetscFVType *);
PETSC_EXTERN PetscErrorCode PetscFVSetUp(PetscFV);
PETSC_EXTERN PetscErrorCode PetscFVSetFromOptions(PetscFV);
PETSC_EXTERN PetscErrorCode PetscFVViewFromOptions(PetscFV, const char[], const char[]);
PETSC_EXTERN PetscErrorCode PetscFVView(PetscFV, PetscViewer);
PETSC_EXTERN PetscErrorCode PetscFVRegister(const char [], PetscErrorCode (*)(PetscFV));
PETSC_EXTERN PetscErrorCode PetscFVRegisterAll(void);
PETSC_EXTERN PetscErrorCode PetscFVRegisterDestroy(void);

PETSC_EXTERN PetscErrorCode PetscFVSetLimiter(PetscFV, PetscLimiter);
PETSC_EXTERN PetscErrorCode PetscFVGetLimiter(PetscFV, PetscLimiter *);
PETSC_EXTERN PetscErrorCode PetscFVSetNumComponents(PetscFV, PetscInt);
PETSC_EXTERN PetscErrorCode PetscFVGetNumComponents(PetscFV, PetscInt *);
PETSC_EXTERN PetscErrorCode PetscFVSetSpatialDimension(PetscFV, PetscInt);
PETSC_EXTERN PetscErrorCode PetscFVGetSpatialDimension(PetscFV, PetscInt *);
PETSC_EXTERN PetscErrorCode PetscFVSetComputeGradients(PetscFV, PetscBool);
PETSC_EXTERN PetscErrorCode PetscFVGetComputeGradients(PetscFV, PetscBool *);
PETSC_EXTERN PetscErrorCode PetscFVSetQuadrature(PetscFV, PetscQuadrature);
PETSC_EXTERN PetscErrorCode PetscFVGetQuadrature(PetscFV, PetscQuadrature *);

PETSC_EXTERN PetscErrorCode PetscFVComputeGradient(PetscFV, PetscInt, PetscScalar[], PetscScalar[]);
PETSC_EXTERN PetscErrorCode PetscFVIntegrateRHSFunction(PetscFV, PetscDS, PetscInt, PetscInt, PetscFVFaceGeom *, PetscReal *, PetscScalar[], PetscScalar[], PetscScalar[], PetscScalar[]);

PETSC_EXTERN PetscErrorCode PetscFVLeastSquaresSetMaxFaces(PetscFV, PetscInt);

#endif
