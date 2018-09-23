static char help[] = "Create and view a forest mesh\n\n";

#include <petscdmforest.h>
#include <petscoptions.h>

int main(int argc, char **argv)
{
  DM             dm;
  char           typeString[256] = {'\0'};
  PetscViewer    viewer          = NULL;
  PetscBool      flg;
  PetscErrorCode ierr;

  ierr = PetscInitialize(&argc, &argv, NULL,help);if (ierr) return ierr;
  ierr = DMCreate(PETSC_COMM_WORLD, &dm);CHKERRQ(ierr);
  ierr = PetscStrncpy(typeString,DMFOREST,256);CHKERRQ(ierr);
  ierr = PetscOptionsBegin(PETSC_COMM_WORLD,NULL,"DM Forest example options",NULL);CHKERRQ(ierr);
  ierr = PetscOptionsString("-dm_type","The type of the dm",NULL,DMFOREST,typeString,sizeof(typeString),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsEnd();CHKERRQ(ierr);
  ierr = DMSetType(dm,(DMType) typeString);CHKERRQ(ierr);
  ierr = DMSetFromOptions(dm);CHKERRQ(ierr);
  ierr = DMSetUp(dm);CHKERRQ(ierr);
  ierr = PetscOptionsGetViewer(PETSC_COMM_WORLD,NULL,"-dm_view",&viewer,NULL,&flg);CHKERRQ(ierr);
  if (flg) {
    ierr = DMView(dm,viewer);CHKERRQ(ierr);
  }
  ierr = PetscViewerDestroy(&viewer);CHKERRQ(ierr);
  ierr = DMDestroy(&dm);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return ierr;
}

/*TEST

      test:
        suffix: moebius
        nsize: 3
        args: -dm_type p4est -dm_forest_topology moebius -dm_view vtk:moebius.vtu
        requires: p4est

      test:
        suffix: shell
        nsize: 3
        args: -dm_type p8est -dm_forest_topology shell -dm_view vtk:shell.vtu
        requires: p4est

      test:
        suffix: brick
        nsize: 3
        args: -dm_type p8est -dm_forest_topology brick -dm_p4est_brick_size 2,3,5 -dm_view vtk:brick.vtu
        requires: p4est

TEST*/
