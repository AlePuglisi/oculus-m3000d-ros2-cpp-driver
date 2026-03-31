import numpy as np 
import cv2 
import os 
import matplotlib.pyplot as plt 
from sklearn.linear_model import RANSACRegressor

def estimate_sonar_motion_from_flow_ransac(flow, step=20, threshold=1.0, pixels_per_meter=250, fps=10):
    h, w = flow.shape[:2]
    
    # Sample flow field
    v_img, u_img = np.mgrid[step//2:h:step, step//2:w:step].reshape(2, -1).astype(int)
    
    # Get flow vectors (in pixels/frame)
    flow_u = flow[v_img, u_img, 0]  # Horizontal flow
    flow_v = flow[v_img, u_img, 1]  # Vertical flow
    
    # Calculate magnitudes
    magnitudes = np.sqrt(flow_u**2 + flow_v**2)
    
    # Filter by threshold
    valid_mask = magnitudes > threshold
    
    if np.sum(valid_mask) < 10:
        return 0, 0, 0, 0
    
    u_img_valid = u_img[valid_mask]
    v_img_valid = v_img[valid_mask]
    flow_u_valid = flow_u[valid_mask]
    flow_v_valid = flow_v[valid_mask]
    
    # SONAR POSITION: Bottom-center of image
    u_sonar = w / 2.0
    v_sonar = h
    
    # Position relative to sonar (in pixels)
    u_rel = u_img_valid - u_sonar  # Horizontal offset
    v_rel = v_img_valid - v_sonar  # Vertical offset (negative, sonar at bottom)
    
    # Convert to meters - these are the actual (X, Y) positions in sonar frame
    Y = u_rel / pixels_per_meter  # Right distance from sonar
    X = -v_rel / pixels_per_meter  # Forward distance from sonar (negative v_rel means forward)
    
    n_points = len(u_img_valid)
    
    # BUILD LINEAR SYSTEM
    # Equations (in meters/frame):
    #   flow_v [m] = -vₓ + ω · Y
    #   flow_u [m] = -vᵧ - ω · X
    #
    # Rearranging for least squares:
    #   flow_v = -vₓ + 0·vᵧ + ω·Y
    #   flow_u = 0·vₓ - vᵧ - ω·X
    
    A = np.zeros((2 * n_points, 3))
    b = np.zeros(2 * n_points)
    
    # Convert flow from pixels/frame to meters/frame
    flow_u_m = flow_u_valid / pixels_per_meter
    flow_v_m = flow_v_valid / pixels_per_meter
    
    for i in range(n_points):
        # Equation 1: flow_v = -vₓ + ω·Y
        A[2*i, 0] = -1.0          # coefficient for vₓ
        A[2*i, 1] = 0.0           # coefficient for vᵧ
        A[2*i, 2] = Y[i]          # coefficient for ω
        b[2*i] = flow_v_m[i]
        
        # Equation 2: flow_u = -vᵧ - ω·X
        A[2*i+1, 0] = 0.0         # coefficient for vₓ
        A[2*i+1, 1] = -1.0        # coefficient for vᵧ
        A[2*i+1, 2] = X[i]       # coefficient for ω
        b[2*i+1] = flow_u_m[i]
    
    # SOLVE USING RANSAC
    try:
        ransac = RANSACRegressor(
            min_samples=0.5,
            residual_threshold=0.1,  # 10 cm threshold
            max_trials=100,
            random_state=42
        )
        
        ransac.fit(A, b)
        motion = ransac.estimator_.coef_
        
        inlier_mask = ransac.inlier_mask_
        n_inliers = np.sum(inlier_mask)
        
    except Exception as e:
        print(f"RANSAC failed: {e}, falling back to least squares")
        motion, residuals, rank, s = np.linalg.lstsq(A, b, rcond=None)
        n_inliers = n_points
    
    vx, vy, omega = motion
    
    # Convert from meters/frame and rad/frame to m/s and rad/s
    vx_ms = vx * fps
    vy_ms = vy * fps
    omega_rs = omega * fps  # Positive = clockwise
    
    return vx_ms, vy_ms, omega_rs, n_inliers

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

            # Save frame if requested
            if save_video:
                out.write(flow_viz)
                frame_count += 1
                
                # Show what's being saved in a separate window
                cv2.imshow('Saved Output (Preview)', flow_viz)

            # Check for the 'Esc' key to exit the loop
            k = cv2.waitKey(100) & 0xff
            if k == 27:
                break

            # Update the previous frame for the next iteration
            old_gray = frame_gray.copy()

        # Reset everything after the for loop completes (one full cycle through images)
        old_gray = cv2.imread(os.path.join(images_path, filenames[0]), cv2.IMREAD_GRAYSCALE)
        cv2.destroyAllWindows()

        if k == 27:
            break

def start_dense_optical_flow_with_motion(images_path, pixels_per_meter=250, fps=5):
    """
    Estimate sonar motion from dense optical flow
    """
    filenames = sorted(os.listdir(images_path))
    old_gray = cv2.imread(os.path.join(images_path, filenames[0]), cv2.IMREAD_GRAYSCALE)

    if old_gray is None: 
        print("Error Reading the first image")
        return
    
    scale_factor = 0.4
    old_gray = cv2.resize(old_gray, None, fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)
    scaled_pixels_per_meter = pixels_per_meter * scale_factor
    
    velocity_history = {'vx': [], 'vy': [], 'yaw': []}
    max_history = 50
    
    while True:
        for filename in sorted(os.listdir(images_path)):
            file_path = os.path.join(images_path, filename)

            frame_gray = cv2.imread(file_path, cv2.IMREAD_GRAYSCALE)
            frame_gray = cv2.resize(frame_gray, None, fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)
            frame = cv2.cvtColor(frame_gray, cv2.COLOR_GRAY2BGR)

            # Calculate dense optical flow
            flow = cv2.calcOpticalFlowFarneback(
                old_gray, frame_gray, 
                None,
                pyr_scale=0.5,      
                levels=2,
                winsize=10,
                iterations=2,
                poly_n=5,
                poly_sigma=1.1,
                flags=0
            )

            # ESTIMATE SONAR MOTION
            v_x, v_y, yaw, n_inliers = estimate_sonar_motion_from_flow_ransac(
                flow, step=20, threshold=1.0, 
                pixels_per_meter=scaled_pixels_per_meter, 
                fps=fps
            )
            
            # Store history
            velocity_history['vx'].append(v_x)
            velocity_history['vy'].append(v_y)
            velocity_history['yaw'].append(yaw)
            
            if len(velocity_history['vx']) > max_history:
                velocity_history['vx'].pop(0)
                velocity_history['vy'].pop(0)
                velocity_history['yaw'].pop(0)
            
            # Calculate averages
            avg_vx = np.mean(velocity_history['vx'])
            avg_vy = np.mean(velocity_history['vy'])
            avg_yaw = np.mean(velocity_history['yaw'])
            
            linear_speed = np.sqrt(v_x**2 + v_y**2)
            avg_linear_speed = np.sqrt(avg_vx**2 + avg_vy**2)
            
            # Visualization
            flow_viz = frame.copy()
            step = 20
            h, w = frame_gray.shape
            y, x = np.mgrid[step//2:h:step, step//2:w:step].reshape(2, -1).astype(int)
            
            fx = flow[y, x, 0]
            fy = flow[y, x, 1]
            magnitudes = np.sqrt(fx**2 + fy**2)
            valid_mask = magnitudes > 1.0
            
            x_valid = x[valid_mask]
            y_valid = y[valid_mask]
            fx_valid = fx[valid_mask]
            fy_valid = fy[valid_mask]
            
            # Draw flow arrows
            for i in range(len(x_valid)):
                xi, yi = x_valid[i], y_valid[i]
                dx, dy = fx_valid[i], fy_valid[i]
                x_end = int(xi + dx)
                y_end = int(yi + dy)
                cv2.arrowedLine(flow_viz, (xi, yi), (x_end, y_end), 
                               (255, 0, 0), thickness=1, tipLength=0.4)
            
            # Draw sonar frame
            sonar_u = int(w / 2)
            sonar_v = int(h - 10)
            
            axis_length = 40
            # x_sonar (forward/up)
            cv2.arrowedLine(flow_viz, (sonar_u, sonar_v), 
                           (sonar_u, sonar_v - axis_length), 
                           (0, 0, 255), 1, tipLength=0.2)
            cv2.putText(flow_viz, "x", (sonar_u + 5, sonar_v - axis_length), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 0, 255), 1)
            
            # y_sonar (right)
            cv2.arrowedLine(flow_viz, (sonar_u, sonar_v), 
                           (sonar_u + axis_length, sonar_v), 
                           (0, 255, 0), 1, tipLength=0.2)
            cv2.putText(flow_viz, "y", (sonar_u + axis_length + 5, sonar_v + 5), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 255, 0), 1)
            
            cv2.circle(flow_viz, (sonar_u, sonar_v), 5, (0, 255, 255), -1)
            cv2.putText(flow_viz, "Sonar", (sonar_u - 30, sonar_v + 25), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 255, 255), 1)

            # Display info
            y_offset = 30
            line_height = 28
            
            cv2.putText(flow_viz, f"Frame: {filename}", (10, y_offset), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.55, (255, 255, 255), 2)
            
            y_offset += line_height
            cv2.putText(flow_viz, f"Instantaneous:", (10, y_offset), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 255, 255), 2)
            
            y_offset += line_height
            cv2.putText(flow_viz, f"  v_x (forward): {v_x:.3f} m/s", (10, y_offset), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 255, 0), 2)
            
            y_offset += line_height
            cv2.putText(flow_viz, f"  v_y (right): {v_y:.3f} m/s", (10, y_offset), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 255, 0), 2)
            
            y_offset += line_height
            cv2.putText(flow_viz, f"  |v|: {linear_speed:.3f} m/s", (10, y_offset), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 255, 0), 2)
            
            y_offset += line_height
            cv2.putText(flow_viz, f"  yaw (CW+): {yaw:.4f} rad/s ({np.rad2deg(yaw):.2f} deg/s)", (10, y_offset), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 255, 0), 2)
            
            y_offset += line_height
            cv2.putText(flow_viz, f"Averaged ({len(velocity_history['vx'])} frames):", (10, y_offset), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 255, 255), 2)
            
            y_offset += line_height
            cv2.putText(flow_viz, f"  v_x: {avg_vx:.3f} m/s", (10, y_offset), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 200, 0), 2)
            
            y_offset += line_height
            cv2.putText(flow_viz, f"  v_y: {avg_vy:.3f} m/s", (10, y_offset), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 200, 0), 2)
            
            y_offset += line_height
            cv2.putText(flow_viz, f"  |v|: {avg_linear_speed:.3f} m/s", (10, y_offset), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 200, 0), 2)
            
            y_offset += line_height
            cv2.putText(flow_viz, f"RANSAC: {n_inliers}/{2*len(x_valid)} inliers", (10, y_offset), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.55, (255, 0, 255), 2)
            
            cv2.imshow('Sonar Motion Estimation', flow_viz)

            k = cv2.waitKey(30) & 0xff
            if k == 27:
                break

            old_gray = frame_gray.copy()

        old_gray = cv2.imread(os.path.join(images_path, filenames[0]), cv2.IMREAD_GRAYSCALE)
        old_gray = cv2.resize(old_gray, None, fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)
        
        velocity_history = {'vx': [], 'vy': [], 'yaw': []}
        
        cv2.destroyAllWindows()

        if k == 27:
            break



def main(args=None):
    images_path = "/home/ale/sonar_ws/src/oculus-ros2-cpp-driver/OCULUS_ROS2/Data/images/sonar/2026_02_06_14_15/cartesian"
    video_out_path = "/home/ale/sonar_ws/src/oculus-ros2-cpp-driver/OCULUS_ROS2/Data/images/sonar/2026_02_06_14_15/cartesian_dense_of.mp4"
    start_dense_optical_flow(images_path, True, video_out_path)

    return

if __name__ == '__main__':
    main()