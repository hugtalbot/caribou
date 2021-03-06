#include "TractionForce.h"

#include <sofa/core/ObjectFactory.h>
#include <sofa/core/visual/VisualParams.h>
#include <sofa/simulation/AnimateBeginEvent.h>
#include <sofa/helper/AdvancedTimer.h>

#include <Caribou/config.h>
#include <Caribou/Geometry/Triangle.h>
#include <Caribou/Geometry/Quad.h>

namespace SofaCaribou::GraphComponents::forcefield {

template<class DataTypes>
TractionForce<DataTypes>::TractionForce()
    // Inputs
    : d_traction(initData(&d_traction,
            "traction",
            "Tractive force per unit area (if an incremental load is set by the slope parameter, this is the final load "
            "reached after all increments)."))
    , d_triangles(initData(&d_triangles,
            "triangles",
            "List of triangles (ex: [t1p1 t1p2 t1p3 t2p1 t2p2 t2p3 ...])."))
    , d_quads(initData(&d_quads,
            "quads",
            "List of quads (ex: [q1p1 q1p2 q1p3 q1p4 q2p1 q2p2 q2p3 ...])."))
    , d_slope(initData(&d_slope,
            (Real) 0,
            "slope",
            "Slope of load increment, the resulting tractive force will be p^t = p^{t-1} + p*slope where p is the "
            "traction force passed as a data and p^t is the traction force applied at time step t. "
            "If slope = 0, the traction will be constant."))
    , d_mechanicalState(initLink(
            "state",
            "Mechanical state that contains the positions of the surface elements."))
    , d_number_of_steps_before_increment(initData(&d_number_of_steps_before_increment,
            (unsigned int) 1,
            "number_of_steps_before_increment",
            "Number of time steps to wait before adding an increment. "
            "This can be used to simulate Newton-Raphson solving process where the time steps are the Newton iterations."))
    , d_draw_faces(initData(&d_draw_faces,
            (bool) true,
            "draw_faces",
            "Draw the faces on which the traction will be applied"))

    // Outputs
    , d_nodal_forces(initData(&d_nodal_forces,
            "nodal_forces",
            "Current nodal forces from the applied traction", true, true))
    , d_total_load(initData(&d_total_load,
            (Real) 0,
            "total_load",
            "Accumulated load applied on all the surface area."))
{
    this->f_listening.setValue(true);
}

template<class DataTypes>
void TractionForce<DataTypes>::init()
{
    sofa::core::behavior::ForceField<DataTypes>::init();

    // if no faces are provided (and no links are specified for these)
    if (d_triangles.getValue().empty() and ! d_triangles.getParent() and
        d_quads.getValue().empty() and ! d_quads.getParent()) {
        // Try to find a triangle container
        auto triangle_container = this->getContext()->template get<sofa::component::topology::TriangleSetTopologyContainer>();
        if (triangle_container) {
            d_triangles.setParent(&triangle_container->getTriangleDataArray());
            msg_info() << "Using the triangles of the container '" << triangle_container->getPathName() << "'";
        }

        // Try to find a quad container
        auto quad_container = this->getContext()->template get<sofa::component::topology::QuadSetTopologyContainer>();
        if (quad_container) {
            d_quads.setParent(&quad_container->getQuadDataArray());
            msg_info() << "Using the quads of the container '" << quad_container->getPathName() << "'";
        }
    }

    // If no mechanical state specified, try to find one in the context
    if (! d_mechanicalState.get()) {
        auto state = this->getContext()->template get<sofa::core::behavior::MechanicalState<DataTypes>>();
        if (state)
            d_mechanicalState.set(state);
        else
            msg_warning() << "No mechanical state provided or found in the current context.";
    }


    // Initialization from the data inputs
    // @todo(jnbrunet2000@gmail.com) : this part should be moved to a real data update function
    const auto rest_positions = d_mechanicalState.get()->readRestPositions();
    sofa::helper::WriteOnlyAccessor<Data<VecDeriv>> nodal_forces = d_nodal_forces;

    if (d_slope.getValue() > -EPSILON && d_slope.getValue() < EPSILON)
        m_traction_is_constant = true;

    nodal_forces.resize(rest_positions.size());

    // Do an increment on the first step of the simulation
    m_number_of_steps_since_last_increment = d_number_of_steps_before_increment.getValue();

    // If all the load is applied at once, increment now the load since it will be used for the computation of the first time step
    if (m_traction_is_constant)
        increment_load(d_traction.getValue());
}

template<class DataTypes>
void TractionForce<DataTypes>::reset()
{
    // Do an increment on the first step of the simulation
    m_number_of_steps_since_last_increment = d_number_of_steps_before_increment.getValue();

    sofa::helper::WriteAccessor<Data<VecDeriv>> nodal_forces = d_nodal_forces;

    nodal_forces.clear();
    d_total_load.setValue(0.);
    m_current_traction = Deriv();
}

template<class DataTypes>
void TractionForce<DataTypes>::handleEvent(sofa::core::objectmodel::Event* event)
{
    if (!sofa::simulation::AnimateBeginEvent::checkEventType(event))
        return;

    if (m_traction_is_constant)
        return;

    const Deriv & maximum_traction_to_apply = d_traction.getValue();
    const Real &  slope = d_slope.getValue();
    const unsigned int number_of_steps_before_increment = d_number_of_steps_before_increment.getValue();
    unsigned int & number_of_steps_since_last_increment = m_number_of_steps_since_last_increment;

    // If we've reach the total traction threshold, do not increment the load again
    if (m_current_traction.norm() >= maximum_traction_to_apply.norm())
        return;

    // Update the counter of steps since the last increment was done
    number_of_steps_since_last_increment++;

    // We did not reach the amount of steps required, skip the increment
    if (number_of_steps_since_last_increment < number_of_steps_before_increment)
        return;

    // We reached the amount of steps required, reset the counter as we will do an increment right now
    number_of_steps_since_last_increment = 0;

    // Compute the increment value from the given slope
    Deriv increment = maximum_traction_to_apply * slope;

    // Add the increment to the current applied load to get an idea of the traction we would apply
    Deriv traction_to_apply = m_current_traction + increment;

    // If this traction is greater than the goal traction, then only increment with the remaining load to get exactly the
    // prescribed goal load
    if (traction_to_apply.norm() > maximum_traction_to_apply.norm())
        increment = maximum_traction_to_apply - m_current_traction;

    m_current_traction += increment;

    // Finally, apply the increment
    increment_load(increment);
}

template<class DataTypes>
void TractionForce<DataTypes>::increment_load(Deriv traction_increment_per_unit_area)
{
    using QuadElement = caribou::geometry::Quad<3>;
    sofa::helper::WriteAccessor<Data<VecDeriv>> nodal_forces = d_nodal_forces;
    sofa::helper::WriteAccessor<Data<Real>> current_load = d_total_load;

    const auto & triangles = d_triangles.getValue();
    const auto & quads = d_quads.getValue();
    const auto rest_positions = d_mechanicalState.get()->readRestPositions();

    msg_info() << "Incrementing the load by " << traction_increment_per_unit_area << " (tractive force per unit area).";

    Deriv load;
    for (size_t i = 0; i < triangles.size(); ++i) {
        const auto & triangle_node_indices = triangles[i];
        const auto p1 = MapVector<3>(&(rest_positions[triangle_node_indices[0]][0]));
        const auto p2 = MapVector<3>(&(rest_positions[triangle_node_indices[1]][0]));
        const auto p3 = MapVector<3>(&(rest_positions[triangle_node_indices[2]][0]));

        const auto triangle = caribou::geometry::Triangle<3>(p1, p2, p3);

        // Integration of the traction increment over the element.
        const auto area = triangle.area();
        const Deriv integrated_force_increment = area * traction_increment_per_unit_area;

        nodal_forces[triangle_node_indices[0]] += integrated_force_increment / (Real) 3;
        nodal_forces[triangle_node_indices[1]] += integrated_force_increment / (Real) 3;
        nodal_forces[triangle_node_indices[2]] += integrated_force_increment / (Real) 3;

        load += integrated_force_increment;
    }

    for (size_t i = 0; i < quads.size(); ++i) {
        const auto & quad_node_indices = quads[i];
        const auto p1 = MapVector<3>(&(rest_positions[quad_node_indices[0]][0]));
        const auto p2 = MapVector<3>(&(rest_positions[quad_node_indices[1]][0]));
        const auto p3 = MapVector<3>(&(rest_positions[quad_node_indices[2]][0]));
        const auto p4 = MapVector<3>(&(rest_positions[quad_node_indices[3]][0]));

        const auto quad = QuadElement(p1, p2, p3, p4);

        // Integration of the traction increment over the element.
        for (size_t gauss_node_id = 0; gauss_node_id < QuadElement::number_of_gauss_nodes; ++gauss_node_id) {
            const auto & g = Vector<2>(QuadElement::gauss_nodes[gauss_node_id]);
            const auto & w = QuadElement::gauss_weights[gauss_node_id];

            const auto J = quad.jacobian(g);
            const auto detJ = J.col(0).cross(J.col(1)).norm();

            // Traction evaluated at the gauss point position
            const auto F = traction_increment_per_unit_area * w * detJ;

            // Shape values at each nodes evaluated at the gauss point position
            const auto S = QuadElement::L(g);

            for (size_t j = 0; j < QuadElement::NumberOfNodes; ++j) {
                nodal_forces[quad_node_indices[j]] += F*S[j];
                load += F*S[j];;
            }

            nodal_forces[quad_node_indices[1]] += F*S[1];
            nodal_forces[quad_node_indices[2]] += F*S[2];
            nodal_forces[quad_node_indices[3]] += F*S[3];
        }
    }

    current_load += load.norm();
}

template<class DataTypes>
void TractionForce<DataTypes>::addForce(const sofa::core::MechanicalParams* mparams, Data<VecDeriv>& d_f, const Data<VecCoord>& d_x, const Data<VecDeriv>& /*d_v*/)
{
    SOFA_UNUSED(mparams);
    SOFA_UNUSED(d_x);
    sofa::helper::AdvancedTimer::stepBegin("TractionForce::addForce");
    sofa::helper::ReadAccessor<Data<Real>> current_load = d_total_load;
    sofa::helper::ReadAccessor<Data<VecDeriv>> nodal_forces = d_nodal_forces;
    sofa::helper::WriteAccessor<Data<VecDeriv>> f = d_f;

    for (size_t i = 0; i < f.size(); ++i)
        f[i] += nodal_forces[i];

    sofa::helper::AdvancedTimer::valSet("load", current_load);
    sofa::helper::AdvancedTimer::stepEnd("TractionForce::addForce");
}

template<class DataTypes>
void TractionForce<DataTypes>::draw(const sofa::core::visual::VisualParams* vparams)
{
    using Color = sofa::core::visual::DrawTool::RGBAColor;
    using Vector3 = sofa::core::visual::DrawTool::Vector3;

    const auto & draw_faces = d_draw_faces.getValue();

    if (! vparams->displayFlags().getShowForceFields() )
        return;

    const auto & triangles = d_triangles.getValue();
    const auto & quads = d_quads.getValue();
    const auto positions = d_mechanicalState.get()->readPositions();

    sofa::helper::vector<Vector3> triangles_points;
    sofa::helper::vector<Vector3> quads_points;
    sofa::helper::vector<Vector3> line_points;
    if (draw_faces) {
        triangles_points.resize(triangles.size() * 3);
        quads_points.resize(quads.size() * 4);
    }

    line_points.resize((triangles.size() + quads.size()) * 2);
    size_t line_index = 0;
    for (size_t i = 0; i < triangles.size(); ++i) {
        const auto & triangle_node_indices = triangles[i];
        const auto p1 = MapVector<3>(&(positions[triangle_node_indices[0]][0]));
        const auto p2 = MapVector<3>(&(positions[triangle_node_indices[1]][0]));
        const auto p3 = MapVector<3>(&(positions[triangle_node_indices[2]][0]));

        const auto triangle = caribou::geometry::Triangle<3>(p1, p2, p3);

        const auto c = triangle.center();
        const Vector3 center(c[0], c[1], c[2]);

        const auto n = triangle.normal();
        const Vector3 normal(n[0], n[1], n[2]);

        if (draw_faces) {
            const auto pp1 = c + (p1-c)*0.666667 + n*0.001;
            const auto pp2 = c + (p2-c)*0.666667 + n*0.001;
            const auto pp3 = c + (p3-c)*0.666667 + n*0.001;
            triangles_points[3 * i + 0] = {pp1[0], pp1[1], pp1[2]};
            triangles_points[3 * i + 1] = {pp2[0], pp2[1], pp2[2]};
            triangles_points[3 * i + 2] = {pp3[0], pp3[1], pp3[2]};
        }

        line_points[line_index++] = center;
        line_points[line_index++] = center + normal;
    }

    for (size_t i = 0; i < quads.size(); ++i) {
        const auto & quad_node_indices = quads[i];
        const auto p1 = MapVector<3>(&(positions[quad_node_indices[0]][0]));
        const auto p2 = MapVector<3>(&(positions[quad_node_indices[1]][0]));
        const auto p3 = MapVector<3>(&(positions[quad_node_indices[2]][0]));
        const auto p4 = MapVector<3>(&(positions[quad_node_indices[3]][0]));

        const auto quad = caribou::geometry::Quad<3>(p1, p2, p3, p4);

        const auto c = quad.center();
        const Vector3 center(c[0], c[1], c[2]);

        const auto t1 = caribou::geometry::Triangle<3>(p1, p2, p3);
        const auto t2 = caribou::geometry::Triangle<3>(p2, p3, p4);
        const auto n = ((t1.normal()+t2.normal())/2).normalized().eval();
        const Vector3 normal(n[0], n[1], n[2]);

        if (draw_faces) {
            const auto pp1 = c + (p1-c)*0.666667 + n*0.001;
            const auto pp2 = c + (p2-c)*0.666667 + n*0.001;
            const auto pp3 = c + (p3-c)*0.666667 + n*0.001;
            const auto pp4 = c + (p4-c)*0.666667 + n*0.001;
            quads_points[4 * i + 0] = {pp1[0], pp1[1], pp1[2]};
            quads_points[4 * i + 1] = {pp2[0], pp2[1], pp2[2]};
            quads_points[4 * i + 2] = {pp3[0], pp3[1], pp3[2]};
            quads_points[4 * i + 3] = {pp4[0], pp4[1], pp4[2]};
        }

        line_points[line_index++] = center;
        line_points[line_index++] = center + normal;
    }

    if (draw_faces) {
        vparams->drawTool()->drawTriangles(triangles_points, Color(1, 0, 0, 0.5));
        vparams->drawTool()->drawQuads(quads_points, Color(1, 0, 0, 0.5));
    }

    vparams->drawTool()->drawLines(line_points, 1.f, Color(0, 1, 0, 1));
}


SOFA_DECL_CLASS(TractionForce)
static int TractionForceClass = sofa::core::RegisterObject("Traction forcefield.")
                                          .add< TractionForce<sofa::defaulttype::Vec3dTypes> >(true)
;

} // namespace SofaCaribou::GraphComponents::forcefield