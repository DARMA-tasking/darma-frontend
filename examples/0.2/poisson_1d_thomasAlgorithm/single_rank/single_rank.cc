#include "../common_poisson1d.h"
#include "../constants.h"
#include <darma.h>
using namespace darma_runtime; //here because headers below need this too
#include "initialize.h"
#include "solveTridiag.h"
#include "checkError.h"

int darma_main(int argc, char** argv)
{
  darma_init(argc, argv);
  size_t me = darma_spmd_rank();
  size_t n_spmd = darma_spmd_size();

  // supposed to be run with 1 rank
  if (n_spmd>1){
    std::cerr << "# of ranks != 1, not supported!" << std::endl;
    std::cerr << " " __FILE__ << ":" << __LINE__ << '\n';        
    exit( EXIT_FAILURE );
  }

  typedef std::vector<double> vecDbl;
	// handles for data needed for matrix
	auto subD = initial_access<vecDbl>("a",me); // subdiagonal 
	auto diag = initial_access<vecDbl>("b",me); // diagonal 
	auto supD = initial_access<vecDbl>("c",me); // superdiagonal
	auto rhs  = initial_access<vecDbl>("d",me); // rhs and solution

  // initialize the handles
	initialize(subD, diag, supD, rhs);

  // solve tridiagonal system 
	solveTridiagonalSystem(subD, diag, supD, rhs);

  // check solution L1 error
	checkFinalL1Error(rhs);

  darma_finalize();
  return 0;
}