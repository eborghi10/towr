/**
 @file    endeffector_load.h
 @author  Alexander W. Winkler (winklera@ethz.ch)
 @date    Mar 16, 2017
 @brief   Brief description
 */

#ifndef XPP_XPP_OPT_INCLUDE_XPP_OPT_ENDEFFECTOR_LOAD_H_
#define XPP_XPP_OPT_INCLUDE_XPP_OPT_ENDEFFECTOR_LOAD_H_

#include <xpp/opt/endeffectors_motion.h>
#include <xpp/parametrization.h>

namespace xpp {
namespace opt {

/** Parametrizes the load/force each endeffector is holding during the motion.
  *
  * The are the lambda values in the paper.
  */
class EndeffectorLoad : public Parametrization {
public:
  using VectorXd = Eigen::VectorXd;
  using LoadParams = Endeffectors<double>;//std::vector<double>;

  EndeffectorLoad (const EndeffectorsMotion ee_motion, double dt, double T);
  virtual ~EndeffectorLoad ();

  void SetOptimizationParameters(const VectorXd& x) override;
  VectorXd GetOptimizationParameters() const override;
  /** @param k the number of discretized node with lambda parameters.
    * @param ee which endeffector we are interested in.
    * @returns the index in the optimization vector where this value is stored
    */
  int IndexDiscrete(int k, EndeffectorID ee) const;
  int Index(double t, EndeffectorID ee) const;


  LoadParams GetLoadValues(double t) const;
  LoadParams GetLoadValuesIdx(int k) const;
  int GetNumberOfSegments() const;

  /** Global time at beginning and end of segment */
  double GetTimeCenterSegment(int segment_id) const;
//  double GetTEnd(int segment_id) const;

private:
  int n_ee_; ///< number of endeffectors
  VectorXd lambdas_;
  double dt_; ///< disretization interval [s]
  double T_;  ///< total time [s]
  int num_segments_;

  int GetSegment(double t) const;
};

} /* namespace opt */
} /* namespace xpp */

#endif /* XPP_XPP_OPT_INCLUDE_XPP_OPT_ENDEFFECTOR_LOAD_H_ */
