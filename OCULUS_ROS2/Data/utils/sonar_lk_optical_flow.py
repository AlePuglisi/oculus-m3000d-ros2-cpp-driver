import numpy as np 
import cv2 
import os 
import matplotlib.pyplot as plt 

def start_lk_optical_flow(images_path, save_video=False, output_path="optical_flow_dense_output.mp4"):
    # Get sorted list of files
    filenames = sorted(os.listdir(images_path))
    
    if len(filenames) == 0:
        print("Error: No images found in directory")
        return

    feature_params = dict(maxCorners=100,
                         qualityLevel=0.1,
                         minDistance=10,
                         blockSize=10)

    # Set parameters for Lucas-Kanade method
    lk_params = dict(winSize=(10, 10), maxLevel=1,
                    criteria=(cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 10, 0.03))
    
    # Create some random colors for feature points
    color = np.random.randint(0, 255, (100, 3))

    # Read the first frame and find corners in it
    old_gray = cv2.imread(os.path.join(images_path, filenames[0]), cv2.IMREAD_GRAYSCALE)

    if old_gray is None: 
        print("Error Reading the first image")
        return
    
    p0 = cv2.goodFeaturesToTrack(old_gray, mask=None, **feature_params)
    
    if p0 is None:
        print("Error: No features detected in first image")
        return

    # Create a mask image for drawing purposes
    old_frame = cv2.cvtColor(old_gray, cv2.COLOR_GRAY2BGR)
    mask = np.zeros_like(old_frame)
    
    # Get dimensions
    height, width = old_gray.shape
    
    # Define scale factor
    scale_factor = 0.4
    
    # Set up video writer if saving (use SCALED dimensions)
    target_fps = 5
    if save_video:
        fourcc = cv2.VideoWriter_fourcc(*'mp4v')
        scaled_width = int(width * scale_factor)
        scaled_height = int(height * scale_factor)
        out = cv2.VideoWriter(output_path, fourcc, target_fps, (scaled_width, scaled_height))
        
        if not out.isOpened():
            print("Error: Could not open video writer")
            return
        
        print(f"Saving video to: {output_path} at {target_fps} fps")
        print(f"Output dimensions: {scaled_width}x{scaled_height}")
    
    frame_count = 0
    saved_count = 0
    clear_interval = 30  # Clear trails every 30 frames

    while True:
        for filename in sorted(os.listdir(images_path)):
            file_path = os.path.join(images_path, filename)

            frame_gray = cv2.imread(file_path, cv2.IMREAD_GRAYSCALE)
            
            if frame_gray is None:
                print(f"Warning: Could not read {filename}, skipping...")
                continue
            
            frame = cv2.cvtColor(frame_gray, cv2.COLOR_GRAY2BGR)

            # Clear the mask periodically and re-detect features
            if frame_count > 0 and frame_count % clear_interval == 0:
                mask = np.zeros_like(frame)
                # Re-detect features
                p0 = cv2.goodFeaturesToTrack(frame_gray, mask=None, **feature_params)
                if p0 is None:
                    print(f"Warning: No features detected at frame {frame_count}")
                    continue

            # Calculate optical flow using the Lucas-Kanade method
            p1, st, err = cv2.calcOpticalFlowPyrLK(old_gray, frame_gray, p0, None, **lk_params)

            if p1 is None or st is None:
                old_gray = frame_gray.copy()
                continue

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

                mask = cv2.line(mask, (a, b), (c, d), color[i % len(color)].tolist(), 2)
                frame = cv2.circle(frame, (a, b), 8, color[i % len(color)].tolist(), -1)
            
            # Combine the frame and mask to visualize the tracks
            img = cv2.add(frame, mask)
            
            # Scale for display and saving
            scaled_img = cv2.resize(img, None, fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)
            
            # Display the frame with tracks
            cv2.imshow('LK Optical Flow', scaled_img)

            # Save frame if requested
            if save_video:
                out.write(scaled_img)
                saved_count += 1
                
                # Show what's being saved in a separate window
                cv2.imshow('Saved Output (Preview)', scaled_img)
            
            # Check for the 'Esc' key to exit the loop
            k = cv2.waitKey(30) & 0xff
            if k == 27:
                break

            # Update the previous frame and points for the next iteration
            old_gray = frame_gray.copy()
            p0 = good_new.reshape(-1, 1, 2)
            
            frame_count += 1

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

    # Cleanup
    if save_video:
        out.release()
        print(f"Video saved successfully: {output_path}")
        print(f"Total frames written: {saved_count}")

def main(args=None):
    images_path = "/home/ale/sonar_ws/src/oculus-ros2-cpp-driver/OCULUS_ROS2/Data/images/sonar/2026_02_06_14_15/cartesian"
    video_out_path = "/home/ale/sonar_ws/src/oculus-ros2-cpp-driver/OCULUS_ROS2/Data/images/sonar/2026_02_06_14_15/cartesian_sparse_of.mp4"
    start_lk_optical_flow(images_path, True, video_out_path)

    return

if __name__ == '__main__':
    main()