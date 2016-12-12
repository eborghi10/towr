/**
 @file    range_of_motion_constraint.h
 @author  Alexander W. Winkler (winklera@ethz.ch)
 @date    Jun 6, 2016
 @brief   Declares various Range of Motion Constraint Classes
 */

#include <xpp/opt/range_of_motion_constraint.h>
#include <xpp/opt/a_robot_interface.h>
#include <xpp/opt/com_motion.h>
#include <xpp/opt/optimization_variables.h>

namespace xpp {
namespace opt {

using namespace xpp::utils;

RangeOfMotionConstraint::RangeOfMotionConstraint ()
{
  name_ = "Range of Motion";
}

RangeOfMotionConstraint::~RangeOfMotionConstraint ()
{
}

void
RangeOfMotionConstraint::Init (const ComMotion& com_motion,
                               const MotionStructure& motion_structure,
                               RobotPtrU p_robot)
{
  com_motion_       = com_motion.clone();
  motion_structure_ = motion_structure;
  robot_            = std::move(p_robot);
}

void
RangeOfMotionConstraint::UpdateVariables (const OptimizationVariables* opt_var)
{
  VectorXd x_coeff   = opt_var->GetVariables(VariableNames::kSplineCoeff);
  com_motion_->SetCoefficients(x_coeff);

  VectorXd footholds = opt_var->GetVariables(VariableNames::kFootholds);
  footholds_ = utils::ConvertEigToStd(footholds);

  // jacobians are constant, only need to be set once
  if (first_update_) {
    SetJacobianWrtContacts(jac_wrt_contacts_);
    SetJacobianWrtMotion(jac_wrt_motion_);
    first_update_ = false;
  }
}

RangeOfMotionConstraint::Jacobian
RangeOfMotionConstraint::GetJacobianWithRespectTo (std::string var_set) const
{
  if (var_set == VariableNames::kFootholds)
    return jac_wrt_contacts_;
  else if (var_set == VariableNames::kSplineCoeff)
    return jac_wrt_motion_;
  else
    return Jacobian();
}

RangeOfMotionBox::VectorXd
RangeOfMotionBox::EvaluateConstraint () const
{
  std::vector<double> g_vec;

  for (const auto& node : motion_structure_.GetPhaseStampedVec()) {
    PosXY com_W = com_motion_->GetCom(node.time_).p;

    for (const auto& f_W : node.phase_.GetAllContacts(footholds_)) {

//      PosXY contact_pos_B = f_W.p.topRows<kDim2d>() - com_W;

//      // this is actually constant, but moved here from bounds
//      // so I can make a meaningful cost out of this constraint
//      PosXY f_nom_B = robot_->GetNominalStanceInBase(f_W.leg);
//      PosXY distance_to_nom = contact_pos_B - f_nom_B;

      // contact position expressed in base frame
      PosXY g;
//      if (false)
      if (f_W.id == Contact::kFixedByStartStance)
        g = -com_W;
      else
        g = f_W.p.topRows<kDim2d>() - com_W;




      for (auto dim : {X,Y})
        g_vec.push_back(g(dim));//f_W.p(dim) - com_W(dim));

    }
  }

  return Eigen::Map<VectorXd>(&g_vec[0], g_vec.size());
}

RangeOfMotionBox::VecBound
RangeOfMotionBox::GetBounds () const
{
  auto max_deviation = robot_->GetMaxDeviationXYFromNominal();

  std::vector<Bound> bounds;
  for (auto node : motion_structure_.GetPhaseStampedVec()) {
    for (auto c : node.phase_.GetAllContacts()) {

      auto leg = static_cast<hyq::LegID>(c.ee);
      PosXY f_nom_B = robot_->GetNominalStanceInBase(leg);
      for (auto dim : {X,Y}) {
        Bound b;
//        b.upper_ = /*f_nom_B(dim)*/   + max_deviation.at(dim);
//        b.lower_ = /*f_nom_B(dim) */  - max_deviation.at(dim);
        b += f_nom_B(dim);
        b.upper_ += max_deviation.at(dim);
        b.lower_ -= max_deviation.at(dim);

//        if (false) {
        if (c.id == Contact::kFixedByStartStance) {

          for (auto f : node.phase_.fixed_contacts_) {
            if (f.ee == c.ee) {
              b -= f.p(dim);
            }
          }


//          hyq::Foothold foot = hyq::Foothold::GetLastFoothold(leg, node.phase_.fixed_contacts_);
////          std::cout << "f: " << f << std::endl;
//          b -= foot.p(dim);
        }

        bounds.push_back(b);
      }
    }
  }
  return bounds;
}

void
RangeOfMotionBox::SetJacobianWrtContacts (Jacobian& jac_wrt_contacts) const
{
  int n_contacts = footholds_.size() * kDim2d;
  int m_constraints = GetNumberOfConstraints();
  jac_wrt_contacts = Jacobian(m_constraints, n_contacts);

  int row=0;
  for (const auto& node : motion_structure_.GetPhaseStampedVec()) {
    for (auto c : node.phase_.GetAllContacts()) {
      if (c.id != Contact::kFixedByStartStance) {
        for (auto dim : {X,Y})
          jac_wrt_contacts.insert(row+dim, ContactVars::Index(c.id,dim)) = 1.0;
      }

      row += kDim2d;
    }
  }
}

void
RangeOfMotionBox::SetJacobianWrtMotion (Jacobian& jac_wrt_motion) const
{
  int n_motion   = com_motion_->GetTotalFreeCoeff();
  int m_constraints = GetNumberOfConstraints();
  jac_wrt_motion = Jacobian(m_constraints, n_motion);

  int row=0;
  for (const auto& node : motion_structure_.GetPhaseStampedVec())
    for (const auto c : node.phase_.GetAllContacts())
      for (auto dim : {X,Y})
        jac_wrt_motion.row(row++) = -1*com_motion_->GetJacobian(node.time_, kPos, dim);
}


//RangeOfMotionFixed::VectorXd
//RangeOfMotionFixed::EvaluateConstraint () const
//{
//  std::vector<double> g_vec;
//
//  auto feet = contacts_->GetFootholdsInWorld();
//  for (const auto& f : feet) {
//    g_vec.push_back(f.p.x());
//    g_vec.push_back(f.p.y());
//  }
//
//  return Eigen::Map<VectorXd>(&g_vec[0], g_vec.size());
//}
//
//RangeOfMotionFixed::VecBound
//RangeOfMotionFixed::GetBounds () const
//{
//  std::vector<Bound> bounds;
//
//  auto start_stance = contacts_->GetStartStance();
//  auto steps = contacts_->GetFootholdsInWorld();
//
//  for (const auto& s : steps) {
//    auto leg = s.leg;
//    auto start_foothold = hyq::Foothold::GetLastFoothold(leg, start_stance);
//
//    bounds.push_back(Bound(start_foothold.p.x() + kStepLength_,
//                           start_foothold.p.x() + kStepLength_));
//    bounds.push_back(Bound(start_foothold.p.y(), start_foothold.p.y()));
//  }
//
//  return bounds;
//}
//
//void
//RangeOfMotionFixed::SetJacobianWrtContacts (Jacobian& jac_wrt_contacts) const
//{
//  int n_contacts = contacts_->GetTotalFreeCoeff();
//  int m_constraints = n_contacts;
//  jac_wrt_contacts = Jacobian(m_constraints, n_contacts);
//  jac_wrt_contacts.setIdentity();
//}
//
//void
//RangeOfMotionFixed::SetJacobianWrtMotion (Jacobian& jac_wrt_motion) const
//{
//  // empty jacobian
//  int n_contacts = contacts_->GetTotalFreeCoeff();
//  int m_constraints = n_contacts;
//  jac_wrt_motion = Jacobian(m_constraints, n_contacts);
//}


} /* namespace opt */
} /* namespace xpp */

