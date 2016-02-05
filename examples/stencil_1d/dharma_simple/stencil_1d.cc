
#include <dharma.h>

#include "../common.h" // do_stencil()

using namespace dharma_runtime;

constexpr size_t n_data_total = 5;
constexpr size_t n_iter = 10;
constexpr bool print_data = true;

////////////////////////////////////////////////////////////////////////////////
// A user data type for holding mesh data

template <typename T>
struct DataArray
{
  public:

    typedef abstract::frontend::SerializationManager::zero_copy_slot_t zero_copy_slot_t;

    // Some user methods
    void allocate(size_t n_data) {
      assert(data_ == nullptr)
      if(n_data) {
        data_ = malloc(n_data * sizeof(T));
        data_size_ = n_data;
      }
    }

    T* get() { return data_; }
    const T* get() const { return data_; }

    size_t size() const { return data_size_; }

    ~DataArray() {
      if(data_size_) free(data_);
    }

  protected:

    void*& get_zero_copy_slot(zero_copy_slot_t slot) {
      return data_;
    }

    size_t zero_copy_slot_size(zero_copy_slot_t slot) const {
      return data_size_;
    }

  private:

    T* data_ = nullptr;

    size_t data_size_ = 0;

};

////////////////////////////////////////////////////////////////////////////////
// Two functions that copy ghost data in and out of a local contiguous buffer

typedef AccessHandle<DataArray<double>> dep_t;

void copy_in_ghost_data(double* dest,
  const dep_t left_ghost, const dep_t main, const dep_t right_ghost
) {
  memcpy(dest, left_ghost->get(), left_ghost->size()*sizeof(double));
  dest += left_ghost->size();
  memcpy(dest, main->get(), main->size()*sizeof(double));
  dest += main->size();
  memcpy(dest, right_ghost->get(), right_ghost->size()*sizeof(double));
}

void copy_out_ghost_data(const double* src,
  dep_t send_left, dep_t main, dep_t send_right
) {
  memcpy(main->get(), src, main->size()*sizeof(double));
  memcpy(send_left->get(), main->get(), send_left->size());
  memcpy(send_right->get(), main->get() + main->size() - 1, send_right->size()*sizeof(double));
}

////////////////////////////////////////////////////////////////////////////////
// main() function

int main(int argc, char** argv)
{
  dharma_init(argc, argv);

  using namespace dharma_runtime::keyword_arguments_for_publication;

  size_t me = dharma_spmd_rank();
  size_t n_spmd = dharma_spmd_size();

  // Figure out how much local data we have
  size_t my_n_data = n_data_total / n_spmd;
  if(me < n_data_total % n_spmd) ++my_n_data;

  // figure out the total amount of data that will be passed
  // to do_stencil(), including ghosts
  size_t my_total_data = my_n_data;
  if(me != 0) ++my_total_data;
  if(me != n_spmd - 1) ++my_total_data;

  // Figure out my neighbors.  If I'm 0 or n_spmd-1, I am my own
  // neighbor on the left and right
  const bool is_leftmost = me == 0;
  const size_t left_neighbor = is_leftmost ? me : me - 1;
  const bool is_rightmost = me == n_spmd - 1;
  const size_t right_neighbor = me == is_rightmost ? me : me + 1;

  typedef DataArray<double> data_t;
  auto data = initial_access<data_t>("data", me);
  auto sent_to_left = initial_access<data_t>("sent_to_left", me, 0);
  auto sent_to_right = initial_access<data_t>("sent_to_right", me, 0);

  create_work([=]{
    data->allocate(my_n_data);
    double* data_ptr = data->get();
    memset(data_ptr, 0, my_n_data*sizeof(double));

    if(me == 0) data_ptr[0] = 1.0;
    if(me == n_spmd - 1) data_ptr[my_n_data - 1] = 1.0;

    if(!is_leftmost) {
      sent_to_left->allocate(1);
      sent_to_left->get()[0] = data_ptr[0];
    }
    if(!is_rightmost) {
      sent_to_right->allocate(1);
      sent_to_right->get()[0] = data_ptr[my_n_data - 1];
    }
  });

  sent_to_left.publish(n_readers=1);
  sent_to_right.publish(n_readers=1);

  for(int iter = 0; iter < n_iter; ++iter) {

    auto left_ghost = read_access<data_t>("sent_to_right", left_neighbor, iter);
    auto right_ghost = read_access<data_t>("sent_to_left", right_neighbor, iter);

    sent_to_left = initial_access<data_t>("sent_to_left", me, iter);
    sent_to_right = initial_access<data_t>("sent_to_right", me, iter);

    create_work([=]{

      double data_with_ghosts[my_total_data];
      copy_in_ghost_data(data_with_ghosts, left_ghost, data, right_ghost);

      do_stencil(data_with_ghosts, my_n_data - is_leftmost - is_rightmost);

      sent_to_left->allocate((int)!is_leftmost);
      sent_to_right->allocate((int)!is_rightmost);
      copy_out_ghost_data(data_with_ghosts, sent_to_left, data, sent_to_right);

    });

    sent_to_left.publish(n_readers=1);
    sent_to_right.publish(n_readers=1);

  } // end of loop over n_iter

  if(print_data) {

    auto prev_node_finished_writing = read_access<void>("write_done", me-1);

    // The `waits()` tag is equivalent to calling prev_node_finished_writing.wait() inside the lambda
    create_work(
      waits(prev_node_finished_writing),
      [=]{
        std::cout << "On worker " << me << ": ";
        do_print_data(data->get(), my_n_data);

        initial_access<void>("write_done", me).publish(n_readers=1);
      }
    );

    // If we're the first node, start the chain in motion
    if(me == 0) {
      initial_access<void>("write_done", me-1).publish(n_readers=1);
    }

  }

  dharma_finalize();
}

