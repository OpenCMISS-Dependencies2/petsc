#!/usr/bin/env python
""" Configure PETSc and build and place still-required classic docs"""

import os
import errno
import subprocess
import shutil
import argparse

CLASSIC_DOCS_LOC = os.path.join(os.getcwd(), '_build_classic')

def main():
    """ Operations to provide data from the 'classic' PETSc docs system. """
    petsc_dir = os.path.abspath('..')
    petsc_arch = _configure_minimal_petsc(petsc_dir)
    _build_classic_docs_subset(petsc_dir, petsc_arch)


def clean():
    """ Clean up data generated by main()

        Does not remove the configuration of PETSc.
    """
    for directory in [CLASSIC_DOCS_LOC]:
        print('Removing %s' % directory)
        if os.path.isdir(directory):
            shutil.rmtree(directory)

def _mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc:
        if exc.errno == errno.EEXIST:
            pass
        else: raise


def _configure_minimal_petsc(petsc_dir, petsc_arch='arch-classic-docs') -> None:
    configure = [
        './configure',
        '--with-mpi=0',
        '--with-blaslapack=0',
        '--with-fortran=0',
        '--with-cxx=0',
        '--with-x=0',
        '--with-cmake=0',
        '--with-pthread=0',
        '--with-regexp=0',
        '--download-sowing',
        '--download-c2html',
        '--with-mkl_sparse_optimize=0',
        '--with-mkl_sparse=0',
        'PETSC_ARCH=' + petsc_arch,
    ]
    print('============================================')
    print('Performing a minimal PETSc (re-)configuration')
    print('PETSC_DIR=%s' % petsc_dir)
    print('PETSC_ARCH=%s' % petsc_arch)
    print('============================================')
    subprocess.run(configure, cwd=petsc_dir, check=True)
    return petsc_arch


def _build_classic_docs_subset(petsc_dir, petsc_arch):
    # Use htmlmap file as a sentinel
    htmlmap_filename = os.path.join(CLASSIC_DOCS_LOC, 'docs', 'manualpages', 'htmlmap')
    if os.path.isfile(htmlmap_filename):
        print('============================================')
        print('Assuming that the classic docs in %s are current' % CLASSIC_DOCS_LOC)
        print('To rebuild, manually run the following before re-making:\n  rm -rf %s' % CLASSIC_DOCS_LOC)
        print('============================================')
    else:
        command = ['make', 'alldoc12',
                   'PETSC_DIR=%s' % petsc_dir,
                   'PETSC_ARCH=%s' % petsc_arch,
                   'LOC=%s' % CLASSIC_DOCS_LOC]
        print('============================================')
        print('Building a subset of PETSc classic docs')
        print('PETSC_DIR=%s' % petsc_dir)
        print('PETSC_ARCH=%s' % petsc_arch)
        print(command)
        print('============================================')
        subprocess.run(command, cwd=petsc_dir, check=True)


def copy_classic_docs(outdir):
    subdirs = ['docs', 'include', 'src']
    for subdir in subdirs:
        target = os.path.join(outdir, subdir)
        if os.path.isdir(target):
            shutil.rmtree(target)
        source = os.path.join(CLASSIC_DOCS_LOC, subdir)
        print('============================================')
        print('Copying directory %s from %s to %s' % (subdir, source, target))
        print('============================================')
        shutil.copytree(source, target)


def _get_classic_build_dir():
    return os.path.join(os.getcwd(), '_build_classic')

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--clean', '-c', action='store_true')
    args = parser.parse_args()

    if args.clean:
        clean()
    else:
        main()
