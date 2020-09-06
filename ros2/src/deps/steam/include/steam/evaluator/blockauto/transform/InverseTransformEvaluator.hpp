//////////////////////////////////////////////////////////////////////////////////////////////
/// \file InverseTransformEvaluator.hpp
///
/// \author Sean Anderson, ASRL
//////////////////////////////////////////////////////////////////////////////////////////////

#ifndef STEAM_INVERSE_TRANSFORM_EVALUATOR_HPP
#define STEAM_INVERSE_TRANSFORM_EVALUATOR_HPP

#include <Eigen/Core>

#include <steam/evaluator/blockauto/transform/TransformEvaluator.hpp>

namespace steam {
namespace se3 {

//////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Evaluator for the inverse of a transformation matrix
//////////////////////////////////////////////////////////////////////////////////////////////
class InverseTransformEvaluator : public TransformEvaluator
{
public:

  /// Convenience typedefs
  typedef boost::shared_ptr<InverseTransformEvaluator> Ptr;
  typedef boost::shared_ptr<const InverseTransformEvaluator> ConstPtr;

  //////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Constructor
  //////////////////////////////////////////////////////////////////////////////////////////////
  InverseTransformEvaluator(const TransformEvaluator::ConstPtr& transform);

  //////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Pseudo constructor - return a shared pointer to a new instance
  //////////////////////////////////////////////////////////////////////////////////////////////
  static Ptr MakeShared(const TransformEvaluator::ConstPtr& transform);

  //////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Returns whether or not an evaluator contains unlocked state variables
  //////////////////////////////////////////////////////////////////////////////////////////////
  virtual bool isActive() const;

  //////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Adds references (shared pointers) to active state variables to the map output
  //////////////////////////////////////////////////////////////////////////////////////////////
  virtual void getActiveStateVariables(
      std::map<unsigned int, steam::StateVariableBase::Ptr>* outStates) const;

  //////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Evaluate the resultant transformation matrix
  //////////////////////////////////////////////////////////////////////////////////////////////
  virtual lgmath::se3::Transformation evaluate() const;

  //////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Evaluate the transformation matrix tree
  ///
  /// ** Note that the returned pointer belongs to the memory pool EvalTreeNode<TYPE>::pool,
  ///    and should be given back to the pool, rather than being deleted.
  //////////////////////////////////////////////////////////////////////////////////////////////
  virtual EvalTreeNode<lgmath::se3::Transformation>* evaluateTree() const;

  //////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Evaluate the Jacobian tree
  //////////////////////////////////////////////////////////////////////////////////////////////
  virtual void appendBlockAutomaticJacobians(const Eigen::MatrixXd& lhs,
                               EvalTreeNode<lgmath::se3::Transformation>* evaluationTree,
                               std::vector<Jacobian<> >* outJacobians) const;

  //////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Fixed-size evaluations of the Jacobian tree
  //////////////////////////////////////////////////////////////////////////////////////////////
  virtual void appendBlockAutomaticJacobians(const Eigen::Matrix<double,1,6>& lhs,
                                EvalTreeNode<lgmath::se3::Transformation>* evaluationTree,
                                std::vector<Jacobian<1,6> >* outJacobians) const;

  virtual void appendBlockAutomaticJacobians(const Eigen::Matrix<double,2,6>& lhs,
                                EvalTreeNode<lgmath::se3::Transformation>* evaluationTree,
                                std::vector<Jacobian<2,6> >* outJacobians) const;

  virtual void appendBlockAutomaticJacobians(const Eigen::Matrix<double,3,6>& lhs,
                                EvalTreeNode<lgmath::se3::Transformation>* evaluationTree,
                                std::vector<Jacobian<3,6> >* outJacobians) const;

  virtual void appendBlockAutomaticJacobians(const Eigen::Matrix<double,4,6>& lhs,
                                EvalTreeNode<lgmath::se3::Transformation>* evaluationTree,
                                std::vector<Jacobian<4,6> >* outJacobians) const;

  virtual void appendBlockAutomaticJacobians(const Eigen::Matrix<double,6,6>& lhs,
                                EvalTreeNode<lgmath::se3::Transformation>* evaluationTree,
                                std::vector<Jacobian<6,6> >* outJacobians) const;

private:

  //////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Implementation for Block Automatic Differentiation
  //////////////////////////////////////////////////////////////////////////////////////////////
  template<int LHS_DIM, int INNER_DIM, int MAX_STATE_SIZE>
  void appendJacobiansImpl(const Eigen::Matrix<double,LHS_DIM,INNER_DIM>& lhs,
                           EvalTreeNode<lgmath::se3::Transformation>* evaluationTree,
                           std::vector<Jacobian<LHS_DIM,MAX_STATE_SIZE> >* outJacobians) const;

  //////////////////////////////////////////////////////////////////////////////////////////////
  /// \brief Transform evaluator
  //////////////////////////////////////////////////////////////////////////////////////////////
  TransformEvaluator::ConstPtr transform_;

};

} // se3
} // steam

#endif // STEAM_INVERSE_TRANSFORM_EVALUATOR_HPP
