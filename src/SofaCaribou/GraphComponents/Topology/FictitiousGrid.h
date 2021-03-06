#ifndef SOFACARIBOU_GRAPHCOMPONENTS_TOPOLOGY_FICTITIOUSGRID_H
#define SOFACARIBOU_GRAPHCOMPONENTS_TOPOLOGY_FICTITIOUSGRID_H

#include <sofa/core/objectmodel/BaseObject.h>
#include <sofa/defaulttype/Vec.h>
#include <sofa/defaulttype/VecTypes.h>
#include <sofa/core/topology/BaseMeshTopology.h>

#include <Caribou/Geometry/RectangularQuad.h>
#include <Caribou/Geometry/RectangularHexahedron.h>
#include <Caribou/Topology/Grid/Grid.h>
#include <Caribou/config.h>

#include <memory>
#include <exception>
#include <bitset>
#include <functional>
#include <sstream>

// Forward declarations
namespace caribou::topology::engine {
template <size_t Dim>
struct Grid;
}

namespace sofa::helper::io {
struct Mesh;
}

namespace SofaCaribou::GraphComponents::topology {

using namespace sofa::core::objectmodel;

using sofa::defaulttype::Vec2Types;
using sofa::defaulttype::Vec3Types;

template <typename DataTypes>
class FictitiousGrid : public virtual BaseObject
{
public:

    SOFA_CLASS(SOFA_TEMPLATE(FictitiousGrid, DataTypes), BaseObject);

    static constexpr unsigned char Dimension = DataTypes::spatial_dimensions;

    // --------------------
    // Caribou data aliases
    // --------------------
    using Index = std::size_t;
    using Int   = INTEGER_TYPE;
    using Float = FLOATING_POINT_TYPE;

    // -----------------
    // Sofa data aliases
    // -----------------
    using SofaFloat = typename DataTypes::Real;
    using SofaVecInt = sofa::defaulttype::Vec<Dimension, UNSIGNED_INTEGER_TYPE>;
    using SofaVecFloat = sofa::defaulttype::Vec<Dimension, SofaFloat>;
    using Coord = typename DataTypes::Coord;
    using SofaVecCoord = sofa::helper::vector<Coord>;
    using ElementId = sofa::core::topology::Topology::index_type;
    using VecElementId = sofa::helper::vector<ElementId>;
    using SofaHexahedron = sofa::core::topology::BaseMeshTopology::Hexahedron;
    using SofaQuad = sofa::core::topology::BaseMeshTopology::Quad;
    using SofaTriangle = sofa::core::topology::BaseMeshTopology::Triangle;
    using SofaEdge = sofa::core::topology::BaseMeshTopology::Edge;

    // -----------------
    // Grid data aliases
    // -----------------
    using GridType = caribou::topology::Grid<Dimension>;
    using NodeIndex = typename GridType::NodeIndex;
    using CellIndex = typename GridType::CellIndex;
    using Dimensions = typename GridType::Dimensions;
    using Subdivisions = typename GridType::Subdivisions;
    using LocalCoordinates = typename GridType::LocalCoordinates;
    using WorldCoordinates = typename GridType::WorldCoordinates;
    using GridCoordinates = typename GridType::GridCoordinates;
    using CellSet = typename GridType::CellSet;
    using CellElement = typename GridType::Element;

    // -----------------
    // Structures
    // -----------------
    enum class Type : INTEGER_TYPE {
        Undefined = std::numeric_limits<INTEGER_TYPE>::lowest(),
        Inside =   (unsigned) 1 << (unsigned) 0,
        Outside =  (unsigned) 1 << (unsigned) 1,
        Boundary = (unsigned) 1 << (unsigned) 2
    };

    ///< The Cell structure contains the quadtree (resp. octree) data of a given cell or subcell.
    struct CellData {
        CellData(const Type & t, const Float& w, const int & r)
        : type(t), weight(w), region_id(r) {}
        Type type = Type::Undefined;
        Float weight = 0.; // 1 for full cell, 1/4 (resp. 1/8) for the first level of subdivision in 2D (resp. 3D), etc.
        int region_id = -1;
    };

    struct Cell {
        Cell * parent = nullptr;
        CellIndex index = 0; // Index relative to the parent cell
        std::unique_ptr<CellData> data; // Data is only stored on leaf cells
        std::unique_ptr<std::array<Cell,(unsigned) 1 << Dimension>> childs;

        inline bool is_leaf() const {return childs.get() == nullptr;}
    };

    ///< A region is a cluster of cells sharing the same type and surrounded by either a boundary region or the outside
    ///< of the grid
    struct Region {
        Type type = Type::Undefined;
        std::vector<Cell*> cells;
    };

    // -------
    // Aliases
    // -------
    using f_implicit_test_callback_t = std::function<float(const WorldCoordinates &)>;

    template <typename ObjectType>
    using Link = SingleLink<FictitiousGrid<DataTypes>, ObjectType, BaseLink::FLAG_STRONGLINK>;

    // ----------------
    // Public functions
    // ----------------
    FictitiousGrid();
    void init() override;
    void draw(const sofa::core::visual::VisualParams* vparams) override;

    /** Initialization of the grid. This must be called before anything else. */
    virtual void create_grid();

    /** Get the number of sparse cells in the grid */
    inline
    UNSIGNED_INTEGER_TYPE number_of_cells() const {
        return p_cell_index_in_grid.size();
    }

    /** Get the number of sparse nodes in the grid */
    inline
    UNSIGNED_INTEGER_TYPE number_of_nodes() const {
        return p_node_index_in_grid.size();
    }

    /** Get the number of subdivisions in the grid */
    inline
    UNSIGNED_INTEGER_TYPE number_of_subdivisions() const {
        return d_number_of_subdivision.getValue();
    }

    /**
     * Get neighbors cells around a given cell. A cell is neighbor to another one if they both have a face in common,
     * or if a face contains one of the face of the other. Neighbors outside of the surface boundary are excluded.
     */
    std::vector<Cell *> get_neighbors(Cell * cell);

    /**
     * Get the list of gauss nodes coordinates and their respective weight inside a cell. Here, all the gauss nodes of
     * the leafs cells that are within (or onto) the boundary are given. The coordinates are given with respect of the
     * local frame of the cell (local coordinates).
     *
     * * @param sparse_cell_index The index of the cell in the sparse grid
     */
    std::vector<std::pair<LocalCoordinates, FLOATING_POINT_TYPE>> get_gauss_nodes_of_cell(const CellIndex & sparse_cell_index) const;

    /**
     * Similar to `get_gauss_nodes_of_cell(const CellIndex & index)`, but here only the gauss nodes of inner cells up to
     * the subdivision level given are returned. Leafs cells bellow the given level are only used to compute the weight
     * of a gauss node.
     *
     * For example, if the grid's subdivision level is 3, calling this function with level = 0 will give the standard
     * 4 gauss nodes in 2D (8 gauss nodes in 3D), but where each gauss nodes will use their underlying quad tree
     * (resp. octree in 3D) to compute their weight.
     *
     * @param sparse_cell_index The index of the cell in the sparse grid
     */
    std::vector<std::pair<LocalCoordinates, FLOATING_POINT_TYPE>>
    get_gauss_nodes_of_cell(const CellIndex & sparse_cell_index, const UNSIGNED_INTEGER_TYPE level) const;

    /**
     * Get the element of a cell from its index in the sparse grid.
     */
    inline
    CellElement get_cell_element(const CellIndex & sparse_cell_index) const {
        const auto cell_index = p_cell_index_in_grid[sparse_cell_index];
        return std::move(p_grid->cell_at(cell_index));
    }

    /**
     * Get the node indices of a cell from its index in the sparse grid.
     */
    inline
    const SofaHexahedron & get_node_indices_of(const CellIndex & sparse_cell_index) const {
        return d_hexahedrons.getValue().at(sparse_cell_index);
    }

    /**
     * Get the type (inside, outside, boundary or undefined) of a given point in space.
     */
    inline
    Type get_type_at(const WorldCoordinates & p) const {
        if (p_grid->contains(p)) {
            const auto cells = p_grid->cells_around(p);

            if (cells.size() == 1) {
                return p_cells_types[cells[0]];
            }

            // The position p is on the boundary of multiple cells, gather the different cells types
            INTEGER_TYPE types = 0;
            for (const auto & cell_index : cells) {
                types |= static_cast<INTEGER_TYPE>(p_cells_types[cell_index]);
            }

            if (types & static_cast<INTEGER_TYPE>(Type::Boundary)) {
                // If one of the cells around p is of type boundary, return the type boundary
                return Type::Boundary;
            } else if((types & static_cast<INTEGER_TYPE>(Type::Inside)) and (types & static_cast<INTEGER_TYPE>(Type::Outside))) {
                // If one of the cells around p is of type inside, and another one is of type outside, return the type boundary
                return Type::Boundary;
            } else {
                // We cannot decide which type it is...this should never happen. Report it.
                std::ostringstream error;
                if (cells.empty()) {
                    // Normally should have been caught by the grid->contains test
                    error << "The position " << p << " was found within the boundaries of the grid, but is not part of any grid cells.";
                } else {
                    error << "The position " << p << " is contained inside multiple cells of different types, and"
                                                     " the type of the position cannot be determined.";
                }

                throw std::runtime_error(error.str());
            }
        } else {
            return Type::Outside;
        }
    }

    /**
     * Set the implicit test callback function.
     * @param callback This should point to a function that takes one world position as argument and return 0 if the
     * given position is directly on the surface, < 0 if it is inside the surface, > 1 otherwise.
     *
     * float implicit_test(const WorldCoordinates & query_position);
     */
    inline void
    set_implicit_test_function(const f_implicit_test_callback_t & callback)
    {
        p_implicit_test_callback = callback;
    }

    void computeBBox(const sofa::core::ExecParams* params, bool onlyVisible) override
    {
        if( !onlyVisible )
            return;

        if (Dimension == 2) {
            const Float min[3] = {
                d_min.getValue()[0], d_min.getValue()[1], -1
            };
            const Float max[3] = {
                d_max.getValue()[0], d_max.getValue()[1], +1
            };
            this->f_bbox.setValue(params,sofa::defaulttype::TBoundingBox<Float>(min, max));
        } else {
            this->f_bbox.setValue(params,sofa::defaulttype::TBoundingBox<Float>(
                d_min.getValue().array(),d_max.getValue().array()));
        }
    }

    virtual std::string getTemplateName() const override
    {
        return templateName(this);
    }

    static std::string templateName(const FictitiousGrid<DataTypes>* = nullptr)
    {
        return DataTypes::Name();
    }

private:
    virtual void tag_intersected_cells_from_implicit_surface();
    virtual void tag_intersected_cells();
    virtual void tag_outside_cells();
    virtual void tag_inside_cells();
    virtual void subdivide_intersected_cells();
    virtual void create_regions_from_same_type_cells();
    virtual void create_sparse_grid();
    virtual void populate_drawing_vectors();

    std::array<CellElement, (unsigned) 1 << Dimension> get_subcells_elements(const CellElement & e) const;
    std::vector<Cell *> get_leaf_cells(const Cell & c) const {return std::move(get_leaf_cells(&c));}
    std::vector<Cell *> get_leaf_cells(const Cell * c) const;
    inline FLOATING_POINT_TYPE get_cell_weight(const Cell & c) const;

private:
    // ------------------
    // Input data members
    // ------------------
    Data<SofaVecInt> d_n;
    Data<SofaVecFloat> d_min;
    Data<SofaVecFloat> d_max;
    Data<UNSIGNED_INTEGER_TYPE> d_number_of_subdivision;
    Data<Float> d_volume_threshold;
    Data<bool> d_use_implicit_surface;
    Data<bool> d_draw_boundary_cells;
    Data<bool> d_draw_outside_cells;
    Data<bool> d_draw_inside_cells;

    Data< SofaVecCoord > d_surface_positions;

    ///< List of edges (ex: [e1p1 e1p2 e2p1 e2p2 ...]).
    Data<sofa::helper::vector<SofaEdge> > d_surface_edges;

    ///< List of triangles (ex: [t1p1 t1p2 t1p3 t2p1 t2p2 t2p3 ...]).
    Data<sofa::helper::vector<SofaTriangle> > d_surface_triangles;

    // -------------------
    // Output data members
    // -------------------
    ///< Position vector of nodes contained in the sparse grid
    Data< SofaVecCoord > d_positions;

    ///< List of quads contained in the sparse grid (ex: [q1p1 q1p2 q1p3 q1p4 q2p1 ... qnp3 qnp4]).
    Data < sofa::helper::vector<SofaQuad> > d_quads;

    ///< List of hexahedrons contained in the sparse grid (ex: [h1p1 h1p2 h1p3 h1p4 h1p5 ... hnp6 hnp7]).
    Data < sofa::helper::vector<SofaHexahedron> > d_hexahedrons;


    // ---------------
    // Private members
    // ---------------
    ///< The underground grid object. This object do not store any values beside the dimensions and size of the grid.
    ///< Most of the grid algorithms are defined there.
    std::unique_ptr<GridType> p_grid;

    ///< This is a pointer to a callback function that determines if a position is inside, outside or on the boundary.
    ///< It is used when an implicit surface definition is avaible.
    f_implicit_test_callback_t p_implicit_test_callback;

    ///< Types of the complete regular grid's cells
    std::vector<Type> p_cells_types;

    ///< List of boundary elements that intersect a given cell.
    std::vector<std::vector<Index>> p_triangles_of_cell;

    ///< Quadtree (resp. Octree) representation of the 2D (resp 3D) cell.
    std::vector<Cell> p_cells;

    ///< Distinct regions of cells.
    std::vector<Region> p_regions;

    ///< Contains the index of a node in the sparse grid from its index in the full grid, or -1 if the node isn't
    ///< present in the sparse grid.
    std::vector<INTEGER_TYPE> p_node_index_in_sparse_grid;

    ///< Contains the index of a node in the full grid from its index in the sparse grid.
    std::vector<UNSIGNED_INTEGER_TYPE> p_node_index_in_grid;

    ///< Contains the index of a cell in the sparse grid from its index in the full grid, or -1 if the cell isn't
    ///< present in the sparse grid.
    std::vector<INTEGER_TYPE> p_cell_index_in_sparse_grid;

    ///< Contains the index of a cell in the full grid from its index in the sparse grid.
    std::vector<UNSIGNED_INTEGER_TYPE> p_cell_index_in_grid;

    ///< Contains the grid's nodes to be draw
    std::vector<sofa::defaulttype::Vector3> p_drawing_nodes_vector;

    ///< Contains the grid's edges to be draw
    std::vector<sofa::defaulttype::Vector3> p_drawing_edges_vector;

    ///< Contains the edges of subdivided cells for each region to be draw
    std::vector<std::vector<sofa::defaulttype::Vector3>> p_drawing_subdivided_edges_vector;

    ///< Contains the cells for each region to be draw
    std::vector<std::vector<sofa::defaulttype::Vector3>> p_drawing_cells_vector;

    // ----------------------
    // Private static members
    // ----------------------
    ///< Contains the coordinates of each subcells of a quad (resp hexa) in 2D resp(3D)
    static const GridCoordinates subcell_coordinates[(unsigned) 1 << Dimension];

};

template<> const FictitiousGrid<Vec2Types>::GridCoordinates FictitiousGrid<Vec2Types>::subcell_coordinates[4];
template<>  const FictitiousGrid<Vec3Types>::GridCoordinates FictitiousGrid<Vec3Types>::subcell_coordinates[8];

template<> void FictitiousGrid<Vec2Types>::tag_intersected_cells ();
template<> void FictitiousGrid<Vec3Types>::tag_intersected_cells ();

template<> void FictitiousGrid<Vec2Types>::subdivide_intersected_cells ();
template<> void FictitiousGrid<Vec3Types>::subdivide_intersected_cells ();

extern template class FictitiousGrid<Vec2Types>;
extern template class FictitiousGrid<Vec3Types>;

} // namespace SofaCaribou::GraphComponents::topology

#endif //SOFACARIBOU_GRAPHCOMPONENTS_TOPOLOGY_FICTITIOUSGRID_H
