import numpy as np 
import cv2 
import os 
import matplotlib.pyplot as plt 

def start_lk_optical_flow(images_path):
    # Get sorted list of files
    filenames = sorted(os.listdir(images_path))

    feature_params = dict( maxCorners = 100,
    qualityLevel = 0.1,
    minDistance = 10,
    blockSize = 10 )

    # Set parameters for Lucas-Kanade method
    lk_params = dict(winSize=(10, 10), maxLevel=1,
                    criteria=(cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 10, 0.03))
    # Create some random colors for feature points
    color = np.random.randint(0, 255, (100, 3))

    # Read the first frame and find corners in it
    # Use the Shi-Tomasi method to detect good features to track
    old_gray = cv2.imread(os.path.join(images_path, filenames[0]), cv2.IMREAD_GRAYSCALE)

    if old_gray is not None: 
        p0 = cv2.goodFeaturesToTrack(old_gray, mask=None, **feature_params)

        # Create a mask image for drawing purposes
        old_frame = cv2.cvtColor(old_gray, cv2.COLOR_GRAY2BGR)
        mask = np.zeros_like(old_frame)
    else: 
        print("Error Reading the first image")
        return
    
    frame_count = 0
    clear_interval = 30  # Clear trails every 10 frames


    while True:
        # for filename in sorted(os.listdir(self.images_path), key=self.extract_s_number):
        for filename in sorted(os.listdir(images_path)):
            file_path = os.path.join(images_path, filename)

            frame_gray = cv2.imread(file_path, cv2.IMREAD_GRAYSCALE)
            frame = cv2.cvtColor(frame_gray, cv2.COLOR_GRAY2BGR)

            # Clear the mask periodically
            frame_count += 1
            if frame_count % clear_interval == 0:
                mask = np.zeros_like(frame)

            # Calculate optical flow using the Lucas-Kanade method
            p1, st, err = cv2.calcOpticalFlowPyrLK(old_gray, frame_gray, p0, None, **lk_params)

            # Select good points
            good_new = p1[st == 1]
            good_old = p0[st == 1]

            # Draw the tracks on the frame
            for i, (new, old) in enumerate(zip(good_new, good_old)):
                a, b = new.ravel()
                c, d = old.ravel()

                # Convert to integers
                a, b = int(a), int(b)
                c, d = int(c), int(d)

                mask = cv2.line(mask, (a, b), (c, d), color[i].tolist(), 2)
                frame = cv2.circle(frame, (a, b), 8, color[i].tolist(), -1)
            # Combine the frame and mask to visualize the tracks
            img = cv2.add(frame, mask)
            scaled_img = cv2.resize(img, None, fx=0.4, fy=0.4, interpolation=cv2.INTER_LINEAR)
            # Display the frame with tracks
            cv2.imshow('LK Optical Flow', scaled_img)

            # Check for the 'Esc' key to exit the loop
            k = cv2.waitKey(30) & 0xff
            if k == 27:
                break

            # Update the previous frame and points for the next iteration
            old_gray = frame_gray.copy()
            p0 = good_new.reshape(-1, 1, 2)

        # Reset everything after the for loop completes (one full cycle through images)
        # Re-read the first frame
        old_gray = cv2.imread(os.path.join(images_path, filenames[0]), cv2.IMREAD_GRAYSCALE)
        # Re-detect features
        p0 = cv2.goodFeaturesToTrack(old_gray, mask=None, **feature_params)
        # Clear the mask
        mask = np.zeros_like(old_frame)
        # Reset frame counter
        frame_count = 0
        cv2.destroyAllWindows()

        if k == 27:
            break

def main(args=None):
    images_path = "/home/ale/sonar_ws/src/oculus-ros2-cpp-driver/OCULUS_ROS2/Data/images/sonar/2026_02_06_14_15/cartesian"
    start_lk_optical_flow(images_path)

    return

if __name__ == '__main__':
    main()