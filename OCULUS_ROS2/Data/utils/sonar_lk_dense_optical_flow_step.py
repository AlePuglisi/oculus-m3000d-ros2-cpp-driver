import numpy as np 
import cv2 
import os 
import matplotlib.pyplot as plt 

def start_dense_optical_flow(images_path, save_video=False, output_path="optical_flow_dense_output.mp4"):
    # Get sorted list of files
    filenames = sorted(os.listdir(images_path))

    # Read the first frame
    old_gray = cv2.imread(os.path.join(images_path, filenames[0]), cv2.IMREAD_GRAYSCALE)

    scale_factor = 0.35
    old_gray = cv2.resize(old_gray, None, fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)

    if old_gray is None: 
        print("Error Reading the first image")
        return

    # Get dimensions after scaling
    height, width = old_gray.shape
    
    # Set up video writer if saving
    target_fps = 5
    if save_video:
        fourcc = cv2.VideoWriter_fourcc(*'mp4v')
        out = cv2.VideoWriter(output_path, fourcc, target_fps, (width, height))
        
        if not out.isOpened():
            print("Error: Could not open video writer")
            return
        
        print(f"Saving video to: {output_path} at {target_fps} fps")
    
    frame_count = 0

    while True:
        for filename in sorted(os.listdir(images_path)):
            file_path = os.path.join(images_path, filename)

            frame_gray = cv2.imread(file_path, cv2.IMREAD_GRAYSCALE)
            frame_gray = cv2.resize(frame_gray, None, fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)
            frame = cv2.cvtColor(frame_gray, cv2.COLOR_GRAY2BGR)

            # Calculate dense optical flow using Farneback method
            flow = cv2.calcOpticalFlowFarneback(
                old_gray, frame_gray, 
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

            # Set step size for the vector field (sample every N pixels)
            step = 20
            
            # Draw arrows for the flow vectors
            h, w = frame_gray.shape
            y, x = np.mgrid[step//2:h:step, step//2:w:step].reshape(2, -1).astype(int)
            
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
                                   (0, 255, 0),  # Blue color (BGR format)
                                   thickness=2, 
                                   tipLength=0.3)

            # Scale and display
            # scaled_img = cv2.resize(flow_viz, None, fx=0.4, fy=0.4, interpolation=cv2.INTER_LINEAR)
            flow_viz = cv2.putText(flow_viz, f"frame: {filename}", (10, 400), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 1)
            cv2.imshow('Dense Optical Flow', flow_viz)
            
            cv2.imshow('Previous Frame', old_gray)
            cv2.imshow('Next Frame', frame_gray)

            # Save frame if requested
            if save_video:
                out.write(flow_viz)
                frame_count += 1
                
                # Show what's being saved in a separate window
                cv2.imshow('Saved Output (Preview)', flow_viz)

            # Check for the 'Esc' key to exit the loop
            # k = cv2.waitKey(100) & 0xff
            # if k == 27:
            #     break
            k = cv2.waitKey(0) & 0xff
            if k == 27:
                break

            # Update the previous frame for the next iteration
            old_gray = frame_gray.copy()

        # Reset everything after the for loop completes (one full cycle through images)
        old_gray = cv2.imread(os.path.join(images_path, filenames[0]), cv2.IMREAD_GRAYSCALE)
        cv2.destroyAllWindows()

        if k == 27:
            break

def main(args=None):
    images_path = "/home/ale/sonar_ws/src/oculus-ros2-cpp-driver/OCULUS_ROS2/Data/images/sonar/2026_02_06_14_15/cartesian"
    video_out_path = "/home/ale/sonar_ws/src/oculus-ros2-cpp-driver/OCULUS_ROS2/Data/images/sonar/2026_02_06_14_15/cartesian_dense_of.mp4"
    start_dense_optical_flow(images_path=images_path, save_video=False, output_path=video_out_path)

    return

if __name__ == '__main__':
    main()