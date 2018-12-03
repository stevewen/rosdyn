#ifndef __ITIA_DYNAMICS_CORE__FRICTION_POLY1__
#define __ITIA_DYNAMICS_CORE__FRICTION_POLY1__

# include <rosdyn_core/base_component.h>


namespace rosdyn
{


class FirstOrderPolynomialFriction: public ComponentBase
{
protected:
  double m_Dq_threshold;
  double m_Dq_max;
  
  void computeRegressor(const Eigen::Ref<Eigen::VectorXd>& Dq )
  {
    double omega = std::min(std::max(Dq(m_component_joint_number), -m_Dq_max), m_Dq_max);
    double sign_Dq = std::min(std::max(omega/m_Dq_threshold,-1.0),1.0);
    m_regressor.setZero();
    m_regressor(m_component_joint_number, 0) = sign_Dq;
    m_regressor(m_component_joint_number, 1) = omega;
    
  }
  
public:
  FirstOrderPolynomialFriction(const std::string& joint_name, const std::string& robot_name, const ros::NodeHandle& nh): ComponentBase(joint_name, robot_name, nh)
  {
    m_type="friction";
    loadParametersAndConstants();
    
    if (m_constants_map.size()!=2)
      throw std::invalid_argument(robot_name+"/" + joint_name + "/friction/constants has wrong dimensions, " + std::to_string(m_constants_map.size()));
    
    if (m_parameters_map.size()!=2)
      throw std::invalid_argument(robot_name+"/" + joint_name + "/friction/coefficients has wrong dimensions, " + std::to_string(m_parameters_map.size()));
    
    m_nominal_parameters.resize(2);
    m_nominal_parameters(0)=m_parameters_map.at("coloumb");
    m_nominal_parameters(1)=m_parameters_map.at("viscous");
    
    m_Dq_threshold = m_constants_map.at("min_velocity");
    if (m_Dq_threshold  <1e-6)
    {
      ROS_WARN("friction/min_velocity must be greater than 1.0e-6, set default value = 1.0e-6");
      m_Dq_threshold = 1.0e-6;
      m_nh.setParam(robot_name+"/"+joint_name+"/friction/constants/min_velocity",m_Dq_threshold);
    }
    
    m_Dq_max=m_constants_map.at("max_velocity");
    if ((m_Dq_max<=0))
    {
      ROS_WARN("friction/max_velocity must be positive, set default value = 1.0e6");
      m_Dq_max = 1.0e6;
      m_nh.setParam(robot_name+"/"+joint_name+"/friction/constants/max_velocity",m_Dq_max);
    }
    m_regressor.resize(m_joints_number, 2);
    m_regressor.setZero();
    

    
    
  };
    
  virtual Eigen::VectorXd getTorque(const Eigen::Ref<Eigen::VectorXd>& q,  const Eigen::Ref<Eigen::VectorXd>& Dq, const Eigen::Ref<Eigen::VectorXd>& DDq)
  {
    computeRegressor(Dq);
    m_torques(m_component_joint_number) = m_regressor.row(m_component_joint_number)*m_nominal_parameters;
    return m_torques;
  };
  virtual Eigen::MatrixXd getRegressor(const Eigen::Ref<Eigen::VectorXd>& q,  const Eigen::Ref<Eigen::VectorXd>& Dq, const Eigen::Ref<Eigen::VectorXd>& DDq)
  {
    computeRegressor(Dq);
    return m_regressor;
  }
  
  bool setParameters(const Eigen::Ref<Eigen::VectorXd>& parameters)
  {
    if (parameters.rows() != m_nominal_parameters.rows() )
    {
      ROS_WARN("dimensions mismatch between new parameters and the nominal one");
      return false;
    }
    m_nominal_parameters=parameters;
    ROS_INFO_STREAM(m_component_joint_name << ": " << (parameters.transpose()));
    m_parameters_map.at("coloumb")=m_nominal_parameters(0);
    m_parameters_map.at("viscous")=m_nominal_parameters(1);
    return true;
  }
  
};

}




#endif