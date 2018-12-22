/*
 * humanoid_state_estimation - a complete state estimation scheme for humanoid robots
 *
 * Copyright 2017-2018 Stylianos Piperakis, Foundation for Research and Technology Hellas (FORTH)
 * License: BSD
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Foundation for Research and Technology Hellas (FORTH)
 *	 nor the names of its contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <serow/IMUEKF.h>


IMUEKF::IMUEKF()
{
    //Gravity Vector
    g = Vector3d::Zero();
    g(2) = -9.80665;
}

void IMUEKF::init() {
    
    
    firstrun = true;
    useEuler = true;
    If = Matrix<double, 15, 15>::Identity();
    P = Matrix<double, 15, 15>::Zero();
    //vel
    P(0,0) = 1e-4;
    P(1,1) = 1e-4;
    P(2,2) = 1e-4;
    //Rot error
    P(3,3) = 1e-3;
    P(4,4) = 1e-3;
    P(5,5) = 1e-3;
    //Pos
    P(6,6)  = 1e-6;
    P(7,7)  = 1e-6;
    P(8,8)  = 1e-6;
    //Biases
    P(9, 9) = 1e-4;
    P(10, 10) = 1e-4;
    P(11, 11) = 1e-4;
    P(12, 12) = 1e-4;
    P(13, 13) = 1e-4;
    P(14, 14) = 1e-4;
    
    
    
    //Construct C
    Hf = Matrix<double, 6, 15>::Zero();
    Hf.block<3,3>(0,6) = Matrix3d::Identity();
    Hf.block<3,3>(3,3) = Matrix3d::Identity();

    Hv = Matrix<double, 3, 15>::Zero();
    
    /*Initialize the state **/
    
    //Rotation Matrix from Inertial to body frame
    Rib = Matrix3d::Identity();
    
    x = Matrix<double,15,1>::Zero();
        
    //Innovation Vector
    z = Matrix<double, 6, 1>::Zero(); //For Odometry
    zv = Vector3d::Zero(); //For Twist
    
    
    
    //Initializing vectors and matrices
    v = Vector3d::Zero();
    dxf = Matrix<double, 15, 1>::Zero();
    
    
    
    temp = Vector3d::Zero();
    Kf = Matrix<double, 15, 6>::Zero();
    Kv = Matrix<double, 15, 3>::Zero();
    
    s = Matrix<double, 6, 6>::Zero();
    sv = Matrix<double, 3, 3>::Zero();
    
    R = Matrix<double, 6, 6>::Zero();
    Rv = Matrix<double, 3, 3>::Zero();
    
    Acf = Matrix<double, 15, 15>::Zero();
    Qff = Matrix<double, 15, 15>::Zero();
    Qf = Matrix<double, 12, 12>::Zero();
    Af = Matrix<double, 15, 15>::Zero();
    
    bgyr = Vector3d::Zero();
    bacc = Vector3d::Zero();
    gyro = Vector3d::Zero();
    acc = Vector3d::Zero();
    angle = Vector3d::Zero();


    //bias removed acceleration and gyro rate
    fhat = Vector3d::Zero();
    omegahat = Vector3d::Zero();
    f_p = Vector3d::Zero();
    omega_p = Vector3d::Zero();
    Tib = Affine3d::Identity();



    //Compute some parts of the Input-Noise Jacobian once since they are constants 
    //gyro (0),acc (3),gyro_bias (6),acc_bias (9)
    Lcf = Matrix<double, 15, 12>::Zero();
    Lcf.block<3,3>(0,3) = Matrix3d::Identity();
    Lcf.block<3,3>(3,0) = Matrix3d::Identity();
    Lcf.block<3,3>(9,6) = Matrix3d::Identity();
    Lcf.block<3,3>(12,9) = Matrix3d::Identity();


    //Output Variables
    angleX = 0.000;
    angleY = 0.000;
    angleZ = 0.000;
    gyroX = 0.000;
    gyroY = 0.000;
    gyroZ = 0.000;
    accX = 0.000;
    accY = 0.000;
    accZ = 0.000;
    rX = 0.000;
    rY = 0.000;
    rZ = 0.000;
    velX = 0.000;
    velY = 0.000;
    velZ = 0.000;
    
    std::cout << "IMU EKF Initialized Successfully" << std::endl;
}

/** ------------------------------------------------------------- **/
Matrix<double,15,1> IMUEKF::computeDyn(Matrix<double,15,1> x_, Matrix<double,3,3> Rib_, Vector3d omega_, Vector3d f_)
{
    Matrix<double,15,1> res = Matrix<double,15,1>::Zero();
    
    //Inputs without bias
    omega_ -= x_.segment<3>(9);
    f_ -= x_.segment<3>(12);
    
    //Nonlinear Process Model
    v = x_.segment<3>(0);
    res.segment<3>(0).noalias() = v.cross(omega_);
    res.segment<3>(0).noalias() += Rib_.transpose()*g;
    res.segment<3>(0) += f_;
    res.segment<3>(6).noalias() = Rib_ * v;
    
    return res;
}

void IMUEKF::RK4(Vector3d omega_, Vector3d f_, Vector3d omega0, Vector3d f0)
{
    
    Matrix<double,15,1> k, k1, k2, k3, k4, x_mid, x0;
    Matrix<double,15,15> K1, K2, K3, K4, K0;
    Vector3d f_mid, omega_mid;
    Matrix3d Rib_mid, Rib0;
    

    Rib_mid = Matrix3d::Identity();
    k1 = Matrix<double,15,1>::Zero();
    k2 = Matrix<double,15,1>::Zero();
    k3 = Matrix<double,15,1>::Zero();
    k4 = Matrix<double,15,1>::Zero();
    K1 = Matrix<double,15,15>::Zero();
    K2 = Matrix<double,15,15>::Zero();
    K3 = Matrix<double,15,15>::Zero();
    K4 = Matrix<double,15,15>::Zero();
    
    x0 = x;
    Rib0 = Rib;
    //compute first coefficient
    k1 = computeDyn(x0,Rib0, omega0, f0);
    
    //Compute mid point with k1
    x_mid.noalias() = x0 + k1 * dt/2.00;
    omega_mid.noalias() = (omega_ + omega0)/2.00;
    Rib_mid.noalias() = Rib0 * expMap(omega_mid);
    f_mid.noalias() = (f_ + f0)/2.00;
    
    //Compute second coefficient
    k2 = computeDyn(x_mid,Rib_mid, omega_mid, f_mid);
    
    //Compute mid point with k2
    x_mid.noalias() = x0 + k2 * dt/2.00;
    //Compute third coefficient
    k3 = computeDyn(x_mid, Rib_mid, omega_mid, f_mid);
    
    //Compute point with k3
    x_mid.noalias() = x0 + k3 * dt;
    Rib_mid.noalias() = Rib0 * expMap(omega_);
    //Compute fourth coefficient
    k4 = computeDyn(x_mid,Rib_mid, omega_, f_);
    

    //RK4 approximation of x
    k.noalias() =  (k1 + 2*k2 +2*k3 + k4)/6.00;

    //Compute the RK4 approximated mid point
    x_mid = x0;
    x_mid.noalias() += dt/2.00 * k;
    Rib_mid.noalias() = Rib0 * expMap(omega_mid);
    
    //Next state
    x.noalias() += dt * k;

    

    K1 = computeTrans(x0,Rib0, omega0, f0);
    K2 = computeTrans(x_mid,Rib_mid, omega_mid, f_mid);
    K3 = K2;
    
    K0  = If;
    K0.noalias() += dt/2.00 * K1;
    K2 = K2 * K0;
    
    K0 = If;
    K0.noalias() += dt/2.00 * K2;
    K3 = K3 * K0;
    
    //Update Rotation
    temp.noalias() = omega_- x.segment<3>(9);
    temp *= dt;
    if(temp(0)!=0 && temp(1) !=0 && temp(2)!=0)
        Rib_mid *=  expMap(temp);

    //Compute the 4th Coefficient
    K4 =  computeTrans(x, Rib_mid, omega_, f_);
    K0 = If;
    K0.noalias() += dt * K3;
    K4 = K4 *  K0;
    
    //RK4 approximation of Transition Matrix
    Af =  If;
    Af.noalias() += (K1 + 2*K2 + 2*K3 + K4) * dt/6.00;
}

Matrix<double,15,15> IMUEKF::computeTrans(Matrix<double,15,1> x_, Matrix<double,3,3> Rib_, Vector3d omega_, Vector3d f_)
{
    omega_.noalias() -= x_.segment<3>(9);
    f_.noalias() -= x_.segment<3>(12);
    v = x_.segment<3>(0);
    Matrix<double,15,15> res = Matrix<double,15,15>::Zero();
    
    res.block<3,3>(0,0).noalias() = -wedge(omega_);
    res.block<3,3>(0,3).noalias() = wedge(Rib_.transpose() * g);
    res.block<3,3>(3,3).noalias() = -wedge(omega_);
    res.block<3,3>(6,0) = Rib_;
    res.block<3,3>(6,3).noalias() = -Rib_ * wedge(v);
    res.block<3,3>(0,9).noalias() = -wedge(v);
    res.block<3,3>(0,12).noalias() = -Matrix3d::Identity();
    res.block<3,3>(3,9).noalias() = -Matrix3d::Identity();
    
    return res;
}


void IMUEKF::euler(Vector3d omega_, Vector3d f_)
{
    Acf=computeTrans(x,Rib,omega_,f_);
      
    //Euler Discretization - First order Truncation
    Af = If;
    Af.noalias() += Acf * dt;  
    
    /** Predict Step : Propagate the Mean estimate **/   
    dxf = computeDyn(x,Rib,omega_,f_);
    x.noalias() += (dxf*dt);
}




/** IMU EKF filter to  deal with the Noise **/
void IMUEKF::predict(Vector3d omega_, Vector3d f_)
{

    omega = omega_;
    f = f_;
    //Used in updati     ng Rib with the Rodriquez formula
    omegahat.noalias() = omega - x.segment<3>(9);
    v = x.segment<3>(0);
    
    //Update the Input-noise Jacobian
    Lcf.block<3,3>(0,0).noalias() = wedge(v);


    if(useEuler)
        euler(omega_,f_);
    else
    {
        RK4(omega_,f_,omega_p,f_p);
        //Store the IMU input for the next integration 
        f_p = f;
        omega_p = omega;
    }

  

    // Covariance Q with full state + biases
    Qf(0, 0) = gyr_qx * gyr_qx * dt ;
    Qf(1, 1) = gyr_qy * gyr_qy * dt ;
    Qf(2, 2) = gyr_qz * gyr_qz * dt ;
    Qf(3, 3) = acc_qx * acc_qx * dt ;
    Qf(4, 4) = acc_qy * acc_qy * dt ;
    Qf(5, 5) = acc_qz * acc_qz * dt ;
    Qf(6, 6) = gyrb_qx * gyrb_qx ;
    Qf(7, 7) = gyrb_qy * gyrb_qy ;
    Qf(8, 8) = gyrb_qz * gyrb_qz  ;
    Qf(9, 9) = accb_qx * accb_qx  ;
    Qf(10, 10) = accb_qy * accb_qy ;
    Qf(11, 11) = accb_qz * accb_qz ;
    

    Qff.noalias() =  Lcf * Qf * Lcf.transpose() * dt;
    //Qff.noalias() =  Af * Lcf * Qf * Lcf.transpose() * Af.transpose() * dt ;
    
    /** Predict Step: Propagate the Error Covariance  **/
    P = Af * P * Af.transpose();
    P.noalias() += Qff;
    
    //Propagate only if non-zero input

    if (omegahat(0) != 0 && omegahat(1) != 0 && omegahat(2) != 0)
    {
        Rib  *=  expMap(omegahat*dt);
    }

    x.segment<3>(3) = Vector3d::Zero();
    updateVars();
}


/** Update **/
void IMUEKF::updateWithTwist(Vector3d y)
{
    Rv(0, 0) = vel_px * vel_px;
    Rv(1, 1) = vel_py * vel_py;
    Rv(2, 2) = vel_pz * vel_pz;
    //Rv = Rv*dt;
    v = x.segment<3>(0);
    
    //Innovetion vector
    zv = y;
    zv.noalias() -= Rib * v;
    
    Hv.block<3,3>(0,0) = Rib;
    Hv.block<3,3>(0,3).noalias() = -Rib * wedge(v);
    sv = Rv;
    sv.noalias() += Hv * P * Hv.transpose();
    Kv.noalias() = P * Hv.transpose() * sv.inverse();
    
    dxf.noalias() = Kv * zv;
    
    //Update the mean estimate
    x += dxf;
    
    //Update the error covariance
    P = (If - Kv * Hv) * P * (If - Hv.transpose()*Kv.transpose());
    P.noalias()+= Kv * Rv * Kv.transpose();
    
    
    if (dxf(3) != 0 && dxf(4) != 0 && dxf(5) != 0)
    {
        Rib *=  expMap(dxf.segment<3>(3));
    }
    x.segment<3>(3) = Vector3d::Zero();
    
    updateVars();
    
    
}

void IMUEKF::updateWithOdom(Vector3d y, Quaterniond qy)
{
    R(0, 0) = odom_px * odom_px;
    R(1, 1) = R(0, 0);
    R(2, 2) = R(0, 0);
    
    R(3, 3) = odom_ax * odom_ax;
    R(4, 4) =  R(3, 3);
    R(5, 5) =  R(3, 3);
    //R = R*dt;
    r = x.segment<3>(6);
    
    //Innovetion vector
    z.segment<3>(0) = y - r;    
    z.segment<3>(3) = logMap((Rib.transpose() * qy.toRotationMatrix()));
    //z.segment<3>(3) = logMap(qy.toRotationMatrix() * Rib.transpose());

    
    //Compute the Kalman Gain
    s = R;
    s.noalias() += Hf * P * Hf.transpose();
    Kf.noalias() = P * Hf.transpose() * s.inverse();
    
    dxf.noalias() = Kf * z;
    
    //Update the mean estimate
    x += dxf;
    
    
    //Update the error covariance
    P = (If - Kf * Hf) * P * (If - Kf * Hf).transpose();
    P.noalias() += Kf * R * Kf.transpose();
    
    
    if (dxf(3) != 0 && dxf(4) != 0 && dxf(5) != 0)
    {
        Rib *=  expMap(dxf.segment<3>(3));
    }
    x.segment<3>(3) = Vector3d::Zero();
    
    updateVars();
    
}



void IMUEKF::updateVars()
{
    
    
    pos = x.segment<3>(6);
    rX = pos(0);
    rY = pos(1);
    rZ = pos(2);
    Tib.linear() = Rib;
    Tib.translation() = pos;
    qib = Quaterniond(Tib.linear());

    //Update the biases
    bgyr = x.segment<3>(9);
    bacc = x.segment<3>(12);
    std::cout<<"Biasgx "<<bgyr<<std::endl;
    std::cout<<"Biasax "<<bacc<<std::endl;

    bias_gx = x(9);
    bias_gy = x(10);
    bias_gz = x(11);
    bias_ax = x(12);
    bias_ay = x(13);
    bias_az = x(14);
    
    
    omegahat = omega - bgyr;
    fhat = f - bacc;
    
    gyro  = Rib * omegahat;
    gyroX = gyro(0);
    gyroY = gyro(1);
    gyroZ = gyro(2);
    
    acc =  Rib * fhat;
    accX = acc(0);
    accY = acc(1);
    accZ = acc(2);
    
    v = x.segment<3>(0);
    vel = Rib * v;
    velX = vel(0);
    velY = vel(1);
    velZ = vel(2);
    
    
    //ROLL - PITCH - YAW
    angle = getEulerAngles(Rib);

    angleX = angle(0);
    angleY = angle(1);
    angleZ = angle(2);
}
