#include <string>
#include <filesystem>
#include <gtest/gtest.h>
#include <Eigen/Core>

static std::string executable_directory_path;

#ifdef CARIBOU_WITH_VTK
#include "test_vtkreader.h"
#endif

template <class Derived>
struct is_eigen : public std::is_base_of<Eigen::DenseBase<Derived>, Derived> {
};
template <class Derived,
    class = typename std::enable_if<is_eigen<Derived>::value>::type>
::std::ostream &operator<<(::std::ostream &o, const Derived &m) {
    o << "\n" << static_cast<const Eigen::DenseBase<Derived> &>(m);
    return o;
}

#include "Grid/Grid.h"
#include "test_unstructured_grid.h"


int main(int argc, char **argv) {
    executable_directory_path = weakly_canonical(std::filesystem::path(argv[0])).parent_path();
    testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    return ret;
}
