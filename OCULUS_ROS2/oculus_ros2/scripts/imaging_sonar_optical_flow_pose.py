#!/usr/bin/env python3
import rclpy
from rclpy.node import Node

from sensor_msgs.msg import Image
from tf2_ros import TransformBroadcaster
from nav_msgs.msg import Odometry
from rclpy.qos import QoSProfile
from tf_transformations import euler_from_quaternion, quaternion_from_euler

import math

from geometry_msgs.msg import TransformStamped, Quaternion

import numpy as np 
import os

import cv2
from oculus_ros2 import sonar_optical_flow_pose

from cv_bridge import CvBridge

class ImageProcessing(Node):
    def __init__(self):
        super().__init__('sonar_optical_flow_pose')

        self.flow_image_publisher = self.create_publisher(Image, 'sonar/fan_image/optical_flow', 10)
        
        # Subscriptions
        self.subscription_sonar_image = self.create_subscription(
            Image,
            'sonar/fan_image/now',
            self.fls_image_callback,
            10
        )

        self._cv_bridge = CvBridge()

        self.old_gray = None
        self.frame_gray = None
        self.rescale_factor = 0.20

        self.flow_pose = sonar_optical_flow_pose.SonarOpticalFlowRansacPose(
            range=5,
            bins=500,
            n_points=8,
            threshold=20,
            min_magnitude=10,
            max_iter=25
        )

        # Initialize the TransformBroadcaster
        self.flow_pose_tf_broadcaster = TransformBroadcaster(self)
        self.x = 0.0
        self.y = 0.0
        self.th = 0.0
        
        self.get_logger().info('FLS Optical Flow Ego-Motion Estimation Node Initialized')


    def fls_image_callback(self, fls_img_msg:Image):

        # flow_fls_img_msg = Image()

        try: 
            cv_image = self._cv_bridge.imgmsg_to_cv2(fls_img_msg, desired_encoding='bgr8')
        except Exception as e: 
            self.get_logger().error(f"Error converting image: {e}")
            return
        
        # Downsample for detection
        # scale_factor = 0.5
        gray_img = cv2.cvtColor(cv_image, cv2.COLOR_BGR2GRAY)

        if self.old_gray is None: 
            self.old_gray = gray_img.copy()
            self.old_gray = cv2.resize(self.old_gray, None, fx=self.rescale_factor, fy=self.rescale_factor, interpolation=cv2.INTER_AREA)
            return
        
        self.frame_gray = gray_img.copy()
        if self.flow_pose.width != self.frame_gray.shape[1]:
            self.flow_pose.set_image_shape(self.frame_gray)
            self.flow_pose.resolution *= self.rescale_factor
            print(f"Current Sonar Image: Range = {self.flow_pose.range:.4f}")
            print(f"                     Height = {self.flow_pose.height:.4f}")
            print(f"Current Pose Estim:  Resolution = {self.flow_pose.resolution:.4f}")

        self.frame_gray = cv2.resize(self.frame_gray, None, fx=self.rescale_factor, fy=self.rescale_factor, interpolation=cv2.INTER_AREA)
        
        frame_t1 = self.old_gray.copy()
        frame_t2 = self.frame_gray.copy()

        x_optim = self.flow_pose.estimate_2d_flow_motion(frame_t1, frame_t2)

        # Broadcasting pose 

        if np.isnan(x_optim[0]) or np.isnan(x_optim[1]) or np.isnan(x_optim[2]):
            self.old_gray = self.frame_gray.copy()
            return
        
        self.x += x_optim[0]
        self.y += x_optim[1]
        # self.x = 0.0
        # self.y = 0.0
        self.th -= x_optim[2]

        sonar_flow_transform = TransformStamped()
        sonar_flow_transform.header.stamp = fls_img_msg.header.stamp
        sonar_flow_transform.header.frame_id = 'map'
        sonar_flow_transform.child_frame_id = 'sonar_2d_flow'
        # translation (dx,dy)
        sonar_flow_transform.transform.translation.x = self.x 
        sonar_flow_transform.transform.translation.y = self.y
        sonar_flow_transform.transform.translation.z = 0.0
        # rotation (dth + yaw)
        qx, qy, qz, qw = quaternion_from_euler(0.0, 0.0, self.th)

        sonar_flow_transform.transform.rotation = Quaternion(
            x=qx,
            y=qy,
            z=qz,
            w=qw
        )
        self.flow_pose_tf_broadcaster.sendTransform(sonar_flow_transform)

        # # Create a copy of the frame to draw arrows on
        # flow_img = cv2.cvtColor(self.frame_gray, cv2.COLOR_GRAY2BGR)

        # # Set step size for the vector field (sample every N pixels)
        # step = 20
        # h, w = self.frame_gray.shape
        # y, x = np.mgrid[step//2:h:step, step//2:w:step].reshape(2, -1).astype(int)

        # for i in range(len(x)):
        #     xi, yi = x[i], y[i]
        #     # Get flow at this point
        #     dx, dy = self.flow_pose.cv_flow[yi, xi]
            
        #     # Only draw if motion is significant (threshold to reduce clutter)
        #     magnitude = np.sqrt(dx**2 + dy**2)
        #     if magnitude > 5:  # Adjust threshold as needed
        #         # Calculate end point of arrow
        #         x_end = int(xi + dx)
        #         y_end = int(yi + dy)
                
        #         # Draw blue arrow
        #         cv2.arrowedLine(flow_img, (xi, yi), (x_end, y_end), 
        #                         (0, 255, 0),  # Blue color (BGR format)
        #                         thickness=2, 
        #                         tipLength=0.3)
                    
        self.old_gray = self.frame_gray.copy()

        # # Convert the OpenCV image back into a sensor_msgs/Image
        # flow_fls_img_msg = self._cv_bridge.cv2_to_imgmsg(flow_img, encoding="bgr8")
        # flow_fls_img_msg.header = fls_img_msg.header

        # self.flow_image_publisher.publish(flow_fls_img_msg)

    def destroy_node(self):
        super().destroy_node()
    

def main(args=None):
    rclpy.init(args=args)
    node = ImageProcessing()
    
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
