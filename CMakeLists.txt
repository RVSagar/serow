cmake_minimum_required(VERSION 2.8.3)
project(serow)

add_definitions("-DBOOST_MPL_LIMIT_LIST_SIZE=30")
add_compile_options(-std=c++11)

find_package(catkin REQUIRED COMPONENTS
  geometry_msgs
  roscpp
  rospy
  std_msgs
  dynamic_reconfigure
)
find_package(Eigen3 REQUIRED)

#find_package(PkgConfig REQUIRED)
#pkg_check_modules(PINOCCHIO pinocchio REQUIRED)

find_package(pinocchio REQUIRED)
find_package(hpp-fcl REQUIRED)

generate_dynamic_reconfigure_options(
   cfg/VarianceControl.cfg
)

catkin_package(
  INCLUDE_DIRS include
  LIBRARIES serow
  CATKIN_DEPENDS geometry_msgs roscpp rospy std_msgs dynamic_reconfigure
  DEPENDS EIGEN3 PINOCCHIO 
)

include_directories(
  include
  ${catkin_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIRS}
  ${PINOCCHIO_INCLUDE_DIRS}
)

link_directories(  
${PINOCCHIO_LIBRARY_DIRS})

add_executable(serow src/serow_driver.cpp src/humanoid_ekf.cpp  src/quadruped_ekf.cpp src/IMUEKF.cpp  src/MovingAverageFilter.cpp src/CoMEKF.cpp src/differentiator.cpp src/butterworthLPF.cpp src/JointDF.cpp src/butterworthHPF.cpp src/IMUinEKFQuad.cpp src/IMUinEKF.cpp src/ContactEKF.cpp)
target_link_libraries(serow ${catkin_LIBRARIES} ${EIGEN3_LIBRARIES} ${PINOCCHIO_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${PINOCCHIO_CFLAGS_OTHER})

add_dependencies(serow ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS} ${PROJECT_NAME}_gencfg)
