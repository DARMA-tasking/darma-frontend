
#include <darma.h>

int darma_main(int argc, char** argv)
{
  using namespace darma_runtime;

  darma_init(argc, argv);

  // get my rank
  const int myRank = darma_spmd_rank();
  // get size 
  const int size = darma_spmd_size();

  // this just creates different handles for different types
  // NOTE: data does not exist yet, only handles!
  auto my_handle1 = initial_access<double>("data_key_1");
  auto my_handle2 = initial_access<int>("data_key_2");
  auto my_handle3 = initial_access<std::string>("data_key_3");
  // etc...

  darma_finalize();

  return 0;
}
