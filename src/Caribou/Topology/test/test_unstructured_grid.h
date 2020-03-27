#ifndef CARIBOU_TOPOLOGY_TEST_MESH_H
#define CARIBOU_TOPOLOGY_TEST_MESH_H

#include <Caribou/Topology/UnstructuredMesh.h>
#include <Caribou/Topology/Domain.h>
#include <Caribou/Geometry/Segment.h>
#include <Caribou/Geometry/Triangle.h>
#include <Caribou/Geometry/Tetrahedron.h>

#include <Eigen/Core>

using namespace caribou::topology;
using namespace caribou::geometry;

TEST(UnstructuredGrid, Mesh) {
    UnstructuredMesh<3> mesh;
    mesh.add_node({0,0,0});
    mesh.add_node({1,1,1});

    {
        auto positions = mesh.positions({1, 0});
        EXPECT_EQ(mesh.position(1), positions[0]);
        EXPECT_EQ(mesh.position(0), positions[1]);
    }

    {
        std::array<unsigned int, 2> indices = {{1, 0}};
        auto positions = mesh.positions(indices);
        EXPECT_EQ(mesh.position(indices[0]), positions[0]);
        EXPECT_EQ(mesh.position(indices[1]), positions[1]);
    }

    {
        std::vector<int> indices = {1, 0};
        auto positions = mesh.positions(indices);
        EXPECT_EQ(mesh.position(1), positions[0]);
        EXPECT_EQ(mesh.position(0), positions[1]);
    }
}

TEST(UnstructuredGrid, Segment) {
    UnstructuredMesh<3> mesh;
    mesh.add_node({0,0,0});
    mesh.add_node({1,1,1});

    auto domain = mesh.add_domain<Segment<3>>("segments");

    domain->add({0, 1});
    EXPECT_EQ(domain->number_of_elements(), 1);

    {
        const auto &indices = domain->element_indices(0);
        EXPECT_EQ(indices, Domain<Segment<3>>::Indices({0, 1}));
    }

    EXPECT_ANY_THROW(mesh.add_domain<Segment<3>>("segments"));

    {
        auto positions = mesh.positions(domain->element_indices(0));
        const Segment<3> segment(positions.data()->data());
        EXPECT_EQ(segment.center(), Segment<3>::WorldCoordinates(0.5, 0.5, 0.5));
    }
}

TEST(UnstructuredGrid, Triangle) {
    UnstructuredMesh<3> mesh;
    mesh.add_node({50,50,33});
    mesh.add_node({60, 50, 21});
    mesh.add_node({55, 55, -4});

    auto domain = mesh.add_domain<Triangle<3>>("triangles");

    domain->add({2, 1, 0});
    EXPECT_EQ(domain->number_of_elements(), 1);

    {
        const auto &indices = domain->element_indices(0);
        EXPECT_EQ(indices, Domain<Triangle<3>>::Indices({2, 1, 0}));
    }

    EXPECT_ANY_THROW(mesh.add_domain<Triangle<3>>("triangles"));


    {
        auto positions = mesh.positions(domain->element_indices(0));
        const Triangle<3> triangle (positions.data()->data());
        EXPECT_EQ(triangle.node(0), Triangle<3>::WorldCoordinates(55, 55, -4));
    }
}

TEST(UnstructuredGrid, Tetrahedron) {
    UnstructuredMesh<3> mesh;
    mesh.add_node({50,50,33});
    mesh.add_node({60, 50, 21});
    mesh.add_node({55, 55, -4});
    mesh.add_node({55, 55, -40});

    auto domain = mesh.add_domain<Tetrahedron<interpolation::Tetrahedron4>>("tetras");

    domain->add({3, 2, 1, 0});
    EXPECT_EQ(domain->number_of_elements(), 1);

    {
        const auto &indices = domain->element_indices(0);
        EXPECT_EQ(indices, Domain<Tetrahedron<interpolation::Tetrahedron4>>::Indices({3, 2, 1, 0}));
    }

    EXPECT_ANY_THROW(mesh.add_domain<Tetrahedron<interpolation::Tetrahedron4>>("tetras"));


    {
        auto positions = mesh.positions(domain->element_indices(0));
        const Tetrahedron<interpolation::Tetrahedron4> tetra (positions.data()->data());
        EXPECT_EQ(tetra.node(0), Tetrahedron<interpolation::Tetrahedron4>::WorldCoordinates(55, 55, -40));
    }
}
#endif //CARIBOU_TOPOLOGY_TEST_MESH_H