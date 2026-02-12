import matplotlib.pyplot as plt
import numpy as np 
import cv2
import re
import os

class SonarPostProcess:
    def __init__(self, images_path, save_video = False):
        self.images_path = images_path
        self.save_video = save_video
        print("Processing sonar images in: ", images_path)

    @staticmethod
    def extract_s_number(filename):
        """Extract the number after 's' in the filename for sorting"""
        match = re.search(r's(\d+)', filename)
        return int(match.group(1)) if match else 0
       
    def mean_filtering(self, window_size):
        # Display the Filtered images

        image_window = []
        
        if self.save_video:
            first_file_path = os.path.join(self.images_path, os.listdir(self.images_path)[0])
            height, width = cv2.imread(first_file_path, cv2.IMREAD_GRAYSCALE).shape
            fourcc = cv2.VideoWriter_fourcc(*"mp4v")
            out_video_raw = cv2.VideoWriter("/home/ale/ros2_ws/src/oculus_ros2/Data/videos/sonar/2025_11_12_15_25/sonar_raw.mp4", fourcc, 30, (width, height))
            out_video_filtered = cv2.VideoWriter("/home/ale/ros2_ws/src/oculus_ros2/Data/videos/sonar/2025_11_12_15_25/sonar_mean.mp4", fourcc, 30, (width, height))

        while True:
            # for filename in sorted(os.listdir(self.images_path), key=self.extract_s_number):
            for filename in sorted(os.listdir(self.images_path)):
                file_path = os.path.join(self.images_path, filename)

                print(file_path)

                image = cv2.imread(file_path, cv2.IMREAD_GRAYSCALE)

                image_window.append(image)

                if len(image_window) == window_size:
                    image_window.pop(0)

                mean_filtered_image = np.uint8(np.array(image_window).sum(axis=0,dtype=np.uint16) / window_size)

                # Display the Image
                cv2.imshow('Original Image', image)
                # display Canny edge detection 
                # image_canny = cv2.Canny(image,100,200)
                # cv2.imshow("Original Image Canny", image_canny) 

                # Display the Filtered
                cv2.imshow("Mean Filtered Image", mean_filtered_image)
                # display Canny edge detection 
                # mean_filtered_image_canny = cv2.Canny(mean_filtered_image,100,200)
                # cv2.imshow("Mean Filtered Image Canny", mean_filtered_image_canny) 



                if self.save_video:
                    out_video_filtered.write(cv2.cvtColor(mean_filtered_image, cv2.COLOR_GRAY2BGR))
                    out_video_raw.write(cv2.cvtColor(image, cv2.COLOR_GRAY2BGR))

                if cv2.waitKey(100) & 0xFF == ord('q'):
                    break

            out_video_filtered.release()
            out_video_raw.release()
            cv2.destroyAllWindows()


def main(args=None):
    images_path = "/home/ale/sonar_ws/src/oculus-ros2-cpp-driver/OCULUS_ROS2/Data/images/sonar/2026_02_06_14_15/polar"
    # images_path = "/home/ale/ros2_ws/src/oculus_ros2/Data/images/sonar/shipwreck/"
    sonar_post_process = SonarPostProcess(images_path)

    window_size = 5
    sonar_post_process.mean_filtering(window_size)

if __name__ == '__main__':
    main()



        