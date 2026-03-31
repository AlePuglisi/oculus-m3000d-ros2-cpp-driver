#!/usr/bin/env python3
import rclpy
from rclpy.node import Node

from sensor_msgs.msg import Image
from sensor_msgs.msg import CameraInfo

import numpy as np 
import os

import cv2
from cv_bridge import CvBridge

class OpticalFlowSonar(Node):
    def __init__(self):
        super().__init__('oculus_optical_flow')

        self.sonar_flow_img_publisher = self.create_publisher(Image, 'fan_image/optical_flow', 10)
        
        # Subscriptions
        self.cartesian_img_sb = self.create_subscription(
            Image,
            'fan_image',
            self.cartesian_image_callback,
            10
        )

        self._cv_bridge = CvBridge()

        self.scale_factor = 0.3
        self.step = 20

        self.old_gray = None

        self.get_logger().info('Oculus Optical Flow Node Initialized')
    


    def cartesian_image_callback(self, sonar_fan_img_msg:Image):

        cartesian_img_msg = Image()

        try: 
            cv_image = self._cv_bridge.imgmsg_to_cv2(sonar_fan_img_msg, desired_encoding='mono8')
            cv_image = cv2.resize(cv_image, None, fx=self.scale_factor, fy=self.scale_factor, interpolation=cv2.INTER_LINEAR)

        except Exception as e: 
            self.get_logger().error(f"Error converting image: {e}")
            return
        
        if self.old_gray is None: 
            self.old_gray = cv_image.copy()
            return
        
        # Downsample for detection
        # gray_img = cv2.cvtColor(cv_image, cv2.COLOR_BGR2GRAY)
        frame_gray = cv_image.copy()
        frame = cv2.cvtColor(frame_gray, cv2.COLOR_GRAY2BGR)


        # Calculate dense optical flow using Farneback method
        flow = cv2.calcOpticalFlowFarneback(
            self.old_gray, frame_gray, 
            None,
            pyr_scale=0.5,      # Pyramid scale
            levels=3,            # Number of pyramid layers
            winsize=10,          # Averaging window size
            iterations=3,        # Number of iterations at each pyramid level
            poly_n=5,            # Size of pixel neighborhood
            poly_sigma=1.2,      # Standard deviation for Gaussian
            flags=0
        )

        # Create a copy of the frame to draw arrows on
        flow_viz = frame.copy()

        # Draw arrows for the flow vectors
        h, w = frame_gray.shape
        y, x = np.mgrid[self.step//2:h:self.step, self.step//2:w:self.step].reshape(2, -1).astype(int)
        
        for i in range(len(x)):
            xi, yi = x[i], y[i]
            # Get flow at this point
            dx, dy = flow[yi, xi]
            
            # Only draw if motion is significant (threshold to reduce clutter)
            magnitude = np.sqrt(dx**2 + dy**2)
            if magnitude > 0.5:  # Adjust threshold as needed
                # Calculate end point of arrow
                x_end = int(xi + dx)
                y_end = int(yi + dy)
                
                # Draw blue arrow
                cv2.arrowedLine(flow_viz, (xi, yi), (x_end, y_end), 
                                (200, 0, 0),  # Blue color (BGR format)
                                thickness=1, 
                                tipLength=0.2)

            # Scale and display
            # scaled_img = cv2.resize(flow_viz, None, fx=0.4, fy=0.4, interpolation=cv2.INTER_LINEAR)
            time = str(sonar_fan_img_msg.header.stamp.sec) + "_" + str(sonar_fan_img_msg.header.stamp.nanosec)
            flow_viz = cv2.putText(flow_viz, f"frame: {time}", (10, 340), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (200, 0, 0), 1)

        self.old_gray = frame_gray.copy()
        # Convert the OpenCV image back into a sensor_msgs/Image
        cartesian_img_msg = self._cv_bridge.cv2_to_imgmsg(flow_viz, encoding="bgr8")
        cartesian_img_msg.header = sonar_fan_img_msg.header

        self.sonar_flow_img_publisher.publish(cartesian_img_msg)

    def destroy_node(self):
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = OpticalFlowSonar()
    
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
