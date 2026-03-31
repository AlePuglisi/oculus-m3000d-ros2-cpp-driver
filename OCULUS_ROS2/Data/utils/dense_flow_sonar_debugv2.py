import cv2
import numpy as np 
import matplotlib
matplotlib.use('TkAgg')  # or 'Qt5Agg' — be explicit
import matplotlib.pyplot as plt
import os

import pandas as pd

class SonarOpticalFlowRansacPose: 

    def __init__(self, debug=False, max_range=5, max_aperture_deg=130, height=100, width=100, min_magnitude=10, n_points=10, threshold=100, max_iter=50):
        # Logging debugging info 
        self.debug = debug

        # --- SONAR IMAGE parameters ---   
        # Current sonar configuration (maximum range and aperture)
        # While the range is configurable (0.1m-30m), 
        # The aperture is fixed between high frequency mode (40 deg) and low frequency 130 deg)
        self.max_range = max_range                       
        self.max_aperture_deg = max_aperture_deg
        # Sonar image dimensions
        self.height = height
        self.width = width

        # Resolution on the sonar x axis [m/pixels] (origin-to-forward)
        self.resolution_x = self.max_range/self.height              
        # Resolution on the sonar y axis [m/pixels] (origin-to-right)
        self.max_aperture_rad = self.max_aperture_deg * np.pi / 180
        self.resolution_y = 2*(self.max_range*np.sin(self.max_aperture_rad/2))/self.width    
        
        # --- OPTICAL FLOW ---    
        # Initialize optical flow matrix in the image coordinates
        self.cv_flow = np.zeros(shape=(self.height, self.width, 2))  # (y,x,flow) optical flow matrix, OpenCV convention
        # Minimal flow magnitude to be considered
        self.min_magnitude = min_magnitude

        # --- FRAME Transformation parameters ---         
        # Fixed transformation from Im frame to Sonar frame (S_p = S_T_Im * Im_p)
        # Im frame (opencv convention): top left corner of the image, y downward, x right-oriented
        # To Sonar frame (centered in the sonar = middle bottom): x upward, y right-oriented 
        self.S_R_Im = np.array([[0, -1], [1, 0]])              # Rotation    from Im (OpenCV) to Sonar
        self.S_t_Im = np.array([self.height, -self.width//2])  # Translation from Im (OpenCV) to Sonar 
        self.S_T_Im = np.vstack([
            np.column_stack([self.S_R_Im, self.S_t_Im]),
            [0, 0, 1]
        ])

        # Fixed transformation from Sonar frame to Im frame (Im_p = Im_T_S * S_p)
        self.Im_R_S = np.array([[0, 1], [-1, 0]])              # Rotation    from Im (OpenCV) to Sonar
        self.Im_t_S = np.array([self.width /2, self.height])  # Translation from Im (OpenCV) to Sonar 
        self.Im_T_S = np.vstack([
            np.column_stack([self.Im_R_S, self.Im_t_S]),
            [0, 0, 1]
        ])


        # --- RANSAC parameters --- 
        # Parameters for Ransac Estimation
        # Best model unkowns (translation in x, y and rotation)
        self.tx_best = 0
        self.ty_best = 0
        self.dtheta_best = 0
        # Best RANSAC inliers 
        self.inliers_best = None
        self.n_best_inliers = 0
        # Number of points used for estimation 
        self.n_points = n_points 
        # List of inliers [[x1,y1], [x2, y2], ..]
        self.inliers = np.zeros(shape=(n_points, 2))
        # Inliers-outliers threshold 
        self.threshold = threshold
        # Max RANSAC iteration 
        self.max_iter = max_iter

        print(f'''SonarOpticalFlowRansacPose initialized:
                    range_max={self.max_range:.2f} m  
                    aperture_max={self.max_aperture_deg:.2f} deg
                    x_res={self.resolution_x:.4f} m/pixels
                    y_res={self.resolution_y:.4f} m/pixels''')


    def update_resolution(self):
        # Resolution on the sonar x axis [m/pixels] (origin-to-forward)
        self.resolution_x = self.max_range/self.height              
        # Resolution on the sonar y axis [m/pixels] (origin-to-right)
        self.max_aperture_rad = self.max_aperture_deg * np.pi / 180
        self.resolution_y = 2*(self.max_range*np.sin(self.max_aperture_rad/2))/self.width    

    def update_Im_to_S_Transform(self):
        # In case the image shape chages due to sonar reconfiguration 
        self.S_t_Im = np.array([self.height, -self.width//2])  # Translation from Im (OpenCV) to Sonar 
        # The rotation matrix remain the same 
        self.S_T_Im = np.vstack([
            np.column_stack([self.S_R_Im, self.S_t_Im]),
            [0, 0, 1]
        ])

    # In case the sonar image frame size changes (e.g. different range or aperture - freq mode)
    def set_image_shape(self, frame):
        self.height = frame.shape[0]
        self.width = frame.shape[1]
        self.update_resolution()
        self.update_Im_to_S_Transform()

        print(f'''SonarOpticalFlowRansacPose Updated:
                    range_max={self.max_range:.2f} m  
                    aperture_max={self.max_aperture_deg:.2f} deg
                    x_res={self.resolution_x:.4f} m/pixels
                    y_res={self.resolution_y:.4f} m/pixels''')

    # In case the sonar configuration changes (e.g. different range or aperture - freq mode)
    def set_sonar_config(self, new_max_range, new_max_aperture):
        self.max_range = new_max_range                       
        self.max_aperture_deg = new_max_aperture
        self.update_resolution()
        self.update_Im_to_S_Transform()

        print(f'''SonarOpticalFlowRansacPose Updated:
                    range_max={self.max_range:.2f} m  
                    aperture_max={self.max_aperture_deg:.2f} deg
                    x_res={self.resolution_x:.4f} m/pixels
                    y_res={self.resolution_y:.4f} m/pixels''')

    def compute_image_flow(self, s1_frame, s2_frame, min_flow_magnitude=5):
        # Computing sonar image optical flow (with OpenCV convention)
        # It return a vector field describing the pixel gradient as image motion 
        # from frame S1 to S2, therefore it is anchored to frame S1 and describe the difference 
        # as p_s2 - p_s1 pixel motion in the image frame 
        self.cv_flow = cv2.calcOpticalFlowFarneback(
            s1_frame, 
            s2_frame, 
            None,
            pyr_scale=0.5,       # Pyramid scale
            levels=3,            # Number of pyramid layers
            winsize=10,          # Averaging window size
            iterations=3,        # Number of iterations at each pyramid level
            poly_n=5,            # Size of pixel neighborhood
            poly_sigma=1.2,      # Standard deviation for Gaussian
            flags=0
        )
        
        # Consider only flow above a given threshold
        magnitude = np.sqrt(self.cv_flow[:,:,0]**2 + self.cv_flow[:,:,1]**2)
        mask = magnitude <= min_flow_magnitude
        self.cv_flow[mask] = [0,0]  # Zero out low-magnitude vectors in place

        return self.cv_flow

    def compute_masked_image_flow(self, s1_frame, s2_frame, min_flow_magnitude=5):
        # Computing sonar image optical flow (with OpenCV convention)
        # It return a vector field describing the pixel gradient as image motion 
        # from frame S1 to S2, therefore it is anchored to frame S1 and describe the difference 
        # as p_s2 - p_s1 pixel motion in the image frame 
        self.cv_flow = cv2.calcOpticalFlowFarneback(
            s1_frame, 
            s2_frame, 
            None,
            pyr_scale=0.5,       # Pyramid scale
            levels=3,            # Number of pyramid layers
            winsize=10,          # Averaging window size
            iterations=3,        # Number of iterations at each pyramid level
            poly_n=5,            # Size of pixel neighborhood
            poly_sigma=1.2,      # Standard deviation for Gaussian
            flags=0
        )
        
        # Consider only flow above a given threshold
        magnitude = np.sqrt(self.cv_flow[:,:,0]**2 + self.cv_flow[:,:,1]**2)

        kernel_blur = (15, 15)
        blur = cv2.GaussianBlur(s1_frame,kernel_blur,0)
        ret3, s1_frame_masked = cv2.threshold(blur,150,255,cv2.THRESH_BINARY+cv2.THRESH_OTSU)    
        kernel_dilate = np.ones((15, 15), np.uint8)  
        s1_frame_masked_dilate = cv2.dilate(s1_frame_masked, kernel_dilate, iterations=1)

        mask = ((magnitude <= min_flow_magnitude) | (s1_frame_masked_dilate == 0)).astype(bool)
        mask_inv = np.logical_not(mask)
        cv2.imshow("Masked Sonar Frame", mask_inv.astype(np.uint8) * 255)

        self.cv_flow[mask] = [0,0]  # Zero out low-magnitude vectors in place

        return self.cv_flow
    
    def apply_flow(self, p_s1):
        # Project points based on sonar image flow 
        # p_s1 measured Image coordinates 
        x = p_s1[:, 0].astype(np.int32)
        y = p_s1[:, 1].astype(np.int32)
        
        # Convert optical flow in Sonar coordinates
        flow_Im = self.cv_flow[y, x]        
        flow_S = (self.S_R_Im @ flow_Im.T).T

        # Invert y and x that are in the opposite order 
        # p_s1 = np.column_stack([p_s1[:,1], p_s1[:,0]])
        # Create homogeneus coordinates
        p_s1_tilde = np.hstack([p_s1.astype(np.float32), np.ones((len(p_s1), 1))])
        
        # Transform p_s1 in Sonar coordinates 
        p_s1_tilde = (self.S_T_Im @ p_s1_tilde.T).T                
        p_s1 = p_s1_tilde[:, :2]   

        # Compute new pixel positions in Sonar coordinates, based on real flow  
        p_s2_flow = p_s1 + flow_S         
        # result: pixel coordinates in S1 and the expected new position
        return p_s1, p_s2_flow
    
    def compute_S1_to_S2_transform(self, dx, dy, dth):
        # Convert 3DOF motion to transformation matrix S2_T_S1 
        # (to move from frame S1 to S2 = frame S1 measured in frame S2) 
        # p_S2 = S2_T_S1 * p_S1
        #return np.linalg.inv(np.array([[np.cos(dth), -np.sin(dth), dx], [np.sin(dth), np.cos(dth), dy], [0, 0, 1]]))
        # Explicit inverse matrix S2_T_S1 (avoid inversion)
        return np.array([[np.cos(dth), np.sin(dth), -dx*np.cos(dth)-dy*np.sin(dth)], 
                         [-np.sin(dth), np.cos(dth), dx*np.sin(dth)-dy*np.cos(dth)], 
                         [0,0,1]])
    
    def compute_S2_to_S1_transform(self, dx, dy, dth):
        # Convert 3DOF motion to transformation matrix S1_T_S2 
        # (to move from frame S2 to S1 = frame S2 measured in frame S1) 
        # p_S1 = S1_T_S2 * p_S2
        return np.array([[np.cos(dth), -np.sin(dth), dx], 
                         [np.sin(dth), np.cos(dth), dy], 
                         [0, 0, 1]])

    def compute_Im2_to_Im1_transform(self, S1_T_S2):
        Im1_T_Im2 = self.Im_T_S @ S1_T_S2 @ self.S_T_Im
        return Im1_T_Im2[:2, :]

    def compute_equation_matrix(self, p_s1, p_s2):
        # Compute the matrices associated to the system of equations from flow correspondances
        # It expects p_s1 and p_s2 to be measured in local coordinates S1, S2 and stored as [[x1,y1],[x2,y2],...]
        alpha = p_s2[:,1] / p_s2[:,0]
        beta  = p_s2[:,0] + np.square(p_s2[:,1])/p_s2[:,0]
        gamma = p_s1[:,1] - p_s2[:,1]/p_s2[:,0] * p_s1[:,0]

        A = np.column_stack([-alpha, np.ones(len(alpha)), beta])
        b = np.array(gamma)

        return A, b

    def solve_pose(self, A : np.ndarray , b: np.ndarray):
        # Solve the system of equations from flow correspondances
        # Result: estimated sensor motion as [dx,dy,dth]
        if A.shape[0] ==3 and A.shape[1] ==3: # inversion 
            x = np.linalg.inv(A) @ b
            x[2] = np.arcsin(x[2])
            return x
        else: # Least-squares (pseudo-inverse)
            x = (np.linalg.inv(A.T @ A) @ A.T) @ b
            x[2] = np.arcsin(x[2])
            return x

    def solve_pose_robust(self, A: np.ndarray, b: np.ndarray):
        b = b.ravel()
        if A.shape[0] == 3 and A.shape[1] == 3:
            try:
                x = np.linalg.inv(A) @ b
            except np.linalg.LinAlgError:
                x, _, _, _ = np.linalg.lstsq(A, b, rcond=None)
            x[2] = np.arcsin(np.clip(x[2], -1.0, 1.0))
            return x
        else:
            # lstsq handles singular/near-singular A.T@A automatically
            x, _, _, _ = np.linalg.lstsq(A, b, rcond=None)
            x[2] = np.arcsin(np.clip(x[2], -1.0, 1.0))
            return x
      
    def measure_consensus(self, p_s1, S2_T_S1_estim):
        # Check which vectors are coherent with the computed motion 
        # invert y and x that are in the opposite order 
        # p_s1 = np.column_stack([p_s1[:,1], p_s1[:,0]])
        p_s1_tilde = np.hstack([p_s1.astype(np.float32), np.ones((len(p_s1), 1))])
        # convert p_s1 from Im to Sonar coordinate frame 
        p_s1_tilde = (self.S_T_Im @ p_s1_tilde.T).T  
        # get the 2D vector of the homogeneus one 
        p_s1_sonar = p_s1_tilde[:, :2]  

        # Compute p_s2 in S2 frame by projecting p_s1 with estimated transform 
        p_s2_tilde = (S2_T_S1_estim @ p_s1_tilde.T).T                
        p_s2_estim = p_s2_tilde[:, :2]    

        # Get the OF in Sonar frame (using original p_s1 in image frame)    
        flow_Im = self.cv_flow[p_s1[:,1].astype(np.int32), p_s1[:,0].astype(np.int32)] 
        flow_S = (self.S_R_Im @ flow_Im.T).T

        # Initialize flow 
        estim_flow = p_s2_estim - p_s1_sonar
        # Search for inliers based on real flow with respect to estimated flow 
        # for idx, p in enumerate(p_s1_sonar):
        #     error = np.sqrt((flow_S[idx, 0] - estim_flow[idx, 0])**2 + 
        #                     (flow_S[idx, 1] - estim_flow[idx, 1])**2)
        #     # print(f"Point: {p_s1[idx]}\nCV Flow: {flow_S[idx]}\nEstim Flow: {estim_flow[idx]}\nError: {error}")
        #     if error < self.threshold and p not in self.inliers: 
        #         # Store inliers in the image frame 
        #         self.inliers = np.append(self.inliers, [[p_s1[idx,0], p_s1[idx,1]]], axis=0)
        
        errors = np.sqrt((flow_S[:, 0] - estim_flow[:, 0])**2 + 
                         (flow_S[:, 1] - estim_flow[:, 1])**2)
        mask_consensus = errors <= self.threshold
        new_inliers = p_s1[mask_consensus]

        # Extend inliers set without duplicates
        if len(self.inliers) > 0 and len(new_inliers) > 0:
            combined = np.vstack([self.inliers, new_inliers])
            # np.unique on rows: axis=0 removes duplicate [x,y] pairs
            self.inliers = np.unique(combined, axis=0)
        elif len(new_inliers) > 0:
            self.inliers = new_inliers

        # Convert inliers set in the Sonar frame 
        inliers_tilde = np.hstack([self.inliers, np.ones((len(self.inliers), 1))]) 
        inliers_tilde_sonar = (self.S_T_Im @ inliers_tilde.T).T     
        inliers_sonar = inliers_tilde_sonar[:, :2]  

        # Result inliers in Image frame and in sonar frame         
        return self.inliers, inliers_sonar

    def estimate_2d_flow_motion_debug(self, s1_frame, s2_frame, flow_debug_imshow, timestamp_string_img):
        # --- RANSAC sonar 3 DOF 2D motion estimation ---
        # --- 1) Initialize Image Flow and filter out useless flow vectors ---
        # The flow is stored in cv_flow attribute
        self.compute_masked_image_flow(s1_frame, s2_frame, min_flow_magnitude = self.min_magnitude)
        
        # Get indices where flow is higher than min magnitude 
        nonzero_y, nonzero_x = np.where(
            np.sqrt(self.cv_flow[:,:,0]**2 + self.cv_flow[:,:,1]**2) >= self.min_magnitude
        )
        # Get useful flow coordinates to sample the image space 
        nonzero_coords = np.column_stack((nonzero_x, nonzero_y))

        # Remove points where x == 0 or y == 0
        # valid_mask = (nonzero_coords[:, 0] != 0) & (nonzero_coords[:, 1] != 0)
        # nonzero_coords = nonzero_coords[valid_mask]

        # Guard: not enough flow points to sample from - return zero motion estimation 
        if len(nonzero_coords) < self.n_points:
            print(f"[WARN] Not enough flow points ({len(nonzero_coords)}) to sample {self.n_points}, skipping frame.")
            return np.array([0.0, 0.0, 0.0])
        
        # --- 2) Start RANSAC Iterations ---       
        n_iter = 0
        n_best_inliers = self.n_points
        # Initialize set of best inliers and current inliers 
        self.inliers_best = None
        self.inliers = np.zeros(shape=(self.n_points, 2), dtype=np.int32)

        # CV image for debugging 
        flow_img = cv2.cvtColor(s2_frame, cv2.COLOR_GRAY2BGR)
        
        # Plot all vector of OF where the flow vector is above the desired min_magnitude 
        x_all, y_all = nonzero_x, nonzero_y
        for xi, yi in zip(x_all[::10], y_all[::10]):
            fdx, fdy = self.cv_flow[yi, xi]
            cv2.arrowedLine(flow_img, (xi, yi), ((xi+fdx).astype(np.int32), (yi+fdy).astype(np.int32)),
                            (0, 0, 255), thickness=1, tipLength=0.3)
                
        while n_iter < self.max_iter:
            print("START ITERATION #", n_iter)

            # --- 2.1) Initialize empty inliers set --- 
            # Re-initialize inlier set
            self.inliers = np.zeros(shape=(self.n_points, 2), dtype=np.int32)
            
            # --- 2.2) Select n_points random points in the non-zero flow image ---
            sampled_idx = np.random.choice(len(nonzero_coords), size=self.n_points, replace=False)
            self.inliers = nonzero_coords[sampled_idx]  # (n_points, 2) of [x, y]
            
            # Initialize best inliers as the initial set of inliers 
            if self.inliers_best is None: 
                self.inliers_best = self.inliers.copy()

            # --- 2.3) Apply flow on the randomly selected points ---
            # Now p_s1 and p_s2 are in the sonar frame 
            p_s1, p_s2 = self.apply_flow(self.inliers)

            # --- 2.4) Solve the pose estimation problem based on flow constraints ---
            A, b = self.compute_equation_matrix(p_s1, p_s2)
            x = self.solve_pose(A, b)

            # Re-scale based on metric/pixels resolution
            # x[0] = x[0] * self.resolution_x 
            # x[1] = x[1] * self.resolution_y

            # print(f"Estimate: tx=", x[0], "m | ty=", x[1], " m | theta=", x[2], " rad")
            
            # Compute tranformation matrix 
            S2_T_S1_estim = self.compute_S1_to_S2_transform(x[0], x[1], x[2])

            # --- 2.5) Measure Consensus and update inliers ---
            self.inliers, inliers_sonar = self.measure_consensus(nonzero_coords, S2_T_S1_estim)
            n_inliers = len(self.inliers)
            print("Inliers Number: ", n_inliers)
            
            # --- Debugging Visualization ---   
            flow_img_iter = flow_img.copy()
                 
            x_flow = self.inliers[:, 0].astype(np.int32)
            y_flow = self.inliers[:, 1].astype(np.int32)
            for xi, yi in zip(x_flow, y_flow):
                fdx, fdy = self.cv_flow[yi, xi]
                cv2.arrowedLine(flow_img_iter, (xi, yi), ((xi+fdx).astype(np.int32), (yi+fdy).astype(np.int32)),
                                (0, 255, 0), thickness=2, tipLength=0.3)
                                    
            cv2.putText(flow_img_iter, timestamp_string_img, (10, flow_img_iter.shape[0]-10),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.85, (0, 255, 0), 2)
            flow_debug_imshow.set_data(cv2.cvtColor(flow_img_iter, cv2.COLOR_BGR2RGB))

            cv2.putText(flow_img_iter, f"#Iteration: {n_iter}", (10, 30),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.85, (0, 255, 0), 2)
            cv2.putText(flow_img_iter, f"#Inliers: {n_inliers}", (10, 60),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.85, (0, 255, 0), 2)
            cv2.putText(flow_img_iter, f"#BestInliers: {n_best_inliers}", (10, 90),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.85, (0, 255, 0), 2) 
            cv2.imshow("RANSAC Debug", flow_img_iter) 
            cv2.waitKey(1)
            plt.pause(0.001)  # Let matplotlib process events too        

            # Update best model if higher number of inliers are found 
            if n_inliers > n_best_inliers: 
                self.tx_best = x[0]
                self.ty_best = x[1]
                self.dtheta_best = x[2]
                print("Update optimal model")

                self.inliers_best = self.inliers.copy()
                n_best_inliers = n_inliers
                self.n_best_inliers = n_best_inliers
                print(f" \n--------------------------------------\n ")

            n_iter += 1
            # input("Press Enter for Next Iteration")

        print(f"\nUsing all ({n_best_inliers}) best inliers over best model")

        # --- 3) Recompute best pose estimation using all inliers of the best model ---
        # Use p_s1,p_s2 in Sonar frame 
        p_s1, p_s2_optim = self.apply_flow(self.inliers_best)
        A_optim,b_optim = self.compute_equation_matrix(p_s1, p_s2_optim)
        x_optim = self.solve_pose(A_optim, b_optim)
        
        # Re-scale based on metric/pixels resolution
        x_optim[0] = x_optim[0] * self.resolution_x 
        x_optim[1] = x_optim[1] * self.resolution_y

        print("Optimal Estimate: tx=", x_optim[0], "m | ty=", x_optim[1], " m | theta=", x_optim[2], " rad")

        # Guard condition in case of unexpected failures and nan values 
        if(np.isnan(x_optim).sum() > 0):
            return np.array([0.0, 0.0, 0.0], dtype=np.int32)

        return x_optim
    
def draw_axes(T, frame):
    # Draw frame T axis in a x,y cartesian frame 
    x = np.array([1, 0, 1])
    y = np.array([0, 1, 1])

    Origin = np.array([0, 0, 1])
    
    Origin_new = T @ Origin.T

    x_new = T @ x.T - Origin_new
    y_new = T @ y.T - Origin_new

    # plt.text(Origin_new[0]-0.5, Origin_new[1]-0.5, frame)
    plt.quiver(Origin_new[0], Origin_new[1], x_new[0], x_new[1], scale=10, color='red')
    plt.quiver(Origin_new[0], Origin_new[1], y_new[0], y_new[1], scale=10, color='green')
    plt.scatter(Origin_new[0], Origin_new[1], c='black')


def main(args=None):

    save_data_csv = False
    results = pd.DataFrame(columns=[
        'img_name', 'time', 'dx_estim', 'dy_estim', 'dth_estim', 'N_inliers'
    ])

    # --- Path to Cartesian Sonar Images ---
    imgs_path = '/home/ale/sonar_ws/src/oculus-ros2-cpp-driver/OCULUS_ROS2/Data/images/sonar/2026_02_06_14_15/cartesian/'
    image_files = sorted([
        f for f in os.listdir(imgs_path)
        if f.lower().endswith(('.png', '.jpg', '.jpeg'))
    ])

    results_row = [image_files[0], 0.0, 0.0, 0.0, 0.0, 0]
    results.loc[len(results)] = results_row

    # Read and Rescale first images 
    img_S1 = cv2.imread(os.path.join(imgs_path, image_files[0]), cv2.IMREAD_GRAYSCALE)
    img_S2 = cv2.imread(os.path.join(imgs_path, image_files[1]), cv2.IMREAD_GRAYSCALE)

    scale_factor = 0.6
    img_S1 = cv2.resize(img_S1, None, fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)
    img_S2 = cv2.resize(img_S2, None, fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)
    
    # --- Initialize RANSAC-Sonar-Flow Pose estimation from first couple of images ---
    min_magnitude=10
    n_points=20
    threshold=3
    max_iter=50

    flow_pose = SonarOpticalFlowRansacPose(max_range=5, max_aperture_deg=130,
                                           height=img_S1.shape[0], width=img_S1.shape[1],
                                           min_magnitude=min_magnitude, 
                                           n_points=n_points, 
                                           threshold=threshold, 
                                           max_iter=max_iter)

    # --- Setup figure ---
    fig, axes = plt.subplots(2, 3, figsize=(15, 10))
    ax_flow_debug = axes[0, 0]
    ax_log        = axes[1, 0]
    ax_flow       = axes[0, 1]
    ax_traj       = axes[0, 2]
    ax_transl     = axes[1, 1]
    ax_rot        = axes[1, 2]

    plt.tight_layout()
    plt.ion()

    cv2.namedWindow("RANSAC Debug", cv2.WINDOW_NORMAL)
    cv2.namedWindow("Masked Sonar Frame", cv2.WINDOW_NORMAL)
    cv2.namedWindow("Frame Transform", cv2.WINDOW_NORMAL)

    # ax_flow_debug: initialize imshow with a dummy image, update with set_data each frame
    dummy_rgb = np.zeros((img_S1.shape[0], img_S1.shape[1], 3), dtype=np.uint8)
    flow_debug_imshow = ax_flow_debug.imshow(dummy_rgb)
    ax_flow_debug.set_title("RANSAC OF Debug")
    
    # ax_log: persistent lines updated with set_data
    ax_log.set_title("#Inliers and Debug")
    ax_log.grid()
    ax_log.set_xlabel("t [s]")
    ax_log.set_ylabel("#Inliers")
    line_inliers, = ax_log.plot([], [], 'b-', label='#inliers')
    ax_log.legend()

    # ax_flow: initialize imshow with a dummy image, update with set_data each frame
    dummy_rgb = np.zeros((img_S1.shape[0], img_S1.shape[1], 3), dtype=np.uint8)
    flow_imshow = ax_flow.imshow(dummy_rgb)
    ax_flow.set_title("Optical Flow")

    # ax_traj: persistent line for origin trajectory (with current xy frame) + text
    ax_traj.set_title("Trajectory")
    ax_traj.set_xlim(-4, 4)
    ax_traj.set_ylim(-4, 10)
    ax_traj.grid()
    ax_traj.set_xlabel("x [m]")
    ax_traj.set_ylabel("y [m]")
    traj_line, = ax_traj.plot([], [], 'k--', linewidth=1, zorder=1)  # connecting origins
    plt_text = ax_traj.text(0, -3.5, '', ha='center', fontsize=7, color='black')

    # ax_transl / ax_rot: persistent lines updated with set_data
    ax_transl.set_title("x(t), y(t) [m]")
    ax_transl.grid()
    ax_transl.set_xlabel("t [s]")
    ax_transl.set_ylabel("tx,ty [m]")
    line_x, = ax_transl.plot([], [], 'r-', label='x')
    line_y, = ax_transl.plot([], [], 'g-', label='y')
    ax_transl.legend()

    ax_rot.set_title("theta(t) [deg]")
    ax_rot.grid()
    ax_rot.set_xlabel("t [s]")
    ax_rot.set_ylabel("theta [deg]")
    line_th, = ax_rot.plot([], [], 'b-', label='theta')
    ax_rot.legend()

    # Time series accumulators
    t = 0.0
    times = [0.0]
    xs    = [0.0]
    ys    = [0.0]
    ths   = [0.0]
    # Origin positions in trajectory frame for the connecting line
    orig_xs = [0.0]
    orig_ys = [0.0]
        
    n_best_inliers = [0]

    # Initialize Sonar Frame S0
    T_t1 = np.eye(3)
    T_t1[0:2, 0:2] = np.array([[0, 1], [1,0]]) @ T_t1[0:2, 0:2]
    plt.sca(ax_traj)
    draw_axes(T_t1, 'S0')

    plt.ion()
    fig.canvas.draw()      # Force canvas initialization
    plt.pause(0.5)         # Block briefly so the window actually appears

    # cv2.namedWindow("Sonar Image", cv2.WINDOW_NORMAL)
    # cv2.imshow("Sonar Image", img_S1)

    step_size = 1

    try:
        for i in range(0, len(image_files) - step_size, step_size):
            # Read consecutive frame images and resize 
            img_S1 = cv2.imread(os.path.join(imgs_path, image_files[i]),   cv2.IMREAD_GRAYSCALE)
            img_S2 = cv2.imread(os.path.join(imgs_path, image_files[i+step_size]), cv2.IMREAD_GRAYSCALE)
            img_S1 = cv2.resize(img_S1, None, fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)
            img_S2 = cv2.resize(img_S2, None, fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)
            
            t1 = float(os.path.splitext(image_files[i]  )[0].replace('_', '.'))
            t2 = float(os.path.splitext(image_files[i+step_size])[0].replace('_', '.'))
            dt = t2 - t1
            t += dt

            timestamp_string = f"S: {image_files[i+step_size]} | t={t:.2f}s"
            timestamp_string_img = f"S1: {image_files[i]} -> S2: {image_files[i+step_size]}"

            # Compute optimal estimated motion from optical flow between S1 and S2
            x_optim = flow_pose.estimate_2d_flow_motion_debug(img_S1, img_S2, flow_debug_imshow, timestamp_string_img)

            # S1_T_S2_estim = flow_pose.compute_S2_to_S1_transform(x_optim[0], x_optim[1], x_optim[2])
            # T_t2 = np.dot(T_t1, S1_T_S2_estim)
            
            S1_T_S2_estim = flow_pose.compute_S2_to_S1_transform(x_optim[0], x_optim[1], x_optim[2])
            T_t2 = np.dot(T_t1, S1_T_S2_estim)

            dx_px = x_optim[0] / flow_pose.resolution_x
            dy_px = x_optim[1] / flow_pose.resolution_y   
            S1_T_S2_estim_px = flow_pose.compute_S2_to_S1_transform(dx_px, dy_px, x_optim[2]) 
            affine = flow_pose.compute_Im2_to_Im1_transform(S1_T_S2_estim_px)
            S2_warped = cv2.warpAffine(img_S2, affine, (flow_pose.width, flow_pose.height))

            S1_color = cv2.cvtColor(img_S1, cv2.COLOR_GRAY2BGR)

            red_overlay = np.zeros_like(S1_color)
            red_overlay[:, :, 2] = S2_warped

            overlay = cv2.addWeighted(S1_color, 1.0, red_overlay, 0.7, 0)

            txtS1 = f"S1: {image_files[i]}"
            txtS2 = f"S2: {image_files[i+step_size]}"
            cv2.putText(overlay, txtS1, (10, overlay.shape[0]-50),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.85, (255, 255, 255), 2)
            cv2.putText(overlay, txtS2, (10, overlay.shape[0]-20),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.85, (0, 0, 255), 2)
            
            cv2.imshow("Frame Transform", overlay)


            dth_deg = x_optim[2] * 180 / np.pi

            # Accumulate time series
            times.append(t)
            xs.append(xs[-1] + x_optim[0])
            ys.append(ys[-1] + x_optim[1])
            ths.append(ths[-1] + dth_deg)

            # Origin of new frame in world coords
            Origin = np.array([0, 0, 1])
            orig = T_t2 @ Origin
            orig_xs.append(orig[0])
            orig_ys.append(orig[1])
            
            # --- Update Log Plot with inliers count ---   
            if flow_pose.inliers_best is None:  
                n_best_inliers.append(0)
            else: 
                n_best_inliers.append(len(flow_pose.inliers_best))

            line_inliers.set_data(times, n_best_inliers)
            ax_log.relim()
            ax_log.autoscale_view()

            # --- Update optical flow subplot with set_data (no cla needed) ---
            flow = flow_pose.cv_flow
            flow_img = cv2.cvtColor(img_S2, cv2.COLOR_GRAY2BGR)
            step = 20
            h, w = img_S2.shape
            y_flow, x_flow = np.mgrid[step//2:h:step, step//2:w:step].reshape(2, -1).astype(np.int32)
            for xi, yi in zip(x_flow, y_flow):
                fdx, fdy = flow[yi, xi]
                if np.sqrt(fdx**2 + fdy**2) > 5:
                    cv2.arrowedLine(flow_img, (xi, yi), ((xi+fdx).astype(np.int32), (yi+fdy).astype(np.int32)),
                                    (0, 255, 0), thickness=2, tipLength=0.3)
                                    
            cv2.putText(flow_img, timestamp_string_img, (10, flow_img.shape[0]-10),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.85, (0, 255, 0), 2)
            
            flow_imshow.set_data(cv2.cvtColor(flow_img, cv2.COLOR_BGR2RGB))

            # --- Clear trajectory subplot, redraw axes + trajectory line ---
            ax_traj.cla()                           
            ax_traj.set_title("Trajectory")
            ax_traj.set_xlim(-4, 4)
            ax_traj.set_ylim(-4, 10)
            ax_traj.grid()
            ax_traj.plot(orig_xs, orig_ys, 'k--', linewidth=1, zorder=1)  
            plt.sca(ax_traj)
            draw_axes(T_t2, f'S{i+2}')
            ax_traj.text(0, 6.5, timestamp_string, ha='center', fontsize=10, color='black', backgroundcolor='white')

            # --- Update time series in-place ---
            line_x.set_data(times, xs)
            line_y.set_data(times, ys)
            ax_transl.relim()
            ax_transl.autoscale_view()

            line_th.set_data(times, ths)
            ax_rot.relim()
            ax_rot.autoscale_view()

            # --- OpenCV window ---
            # img_S2_bgr = cv2.cvtColor(img_S2, cv2.COLOR_GRAY2BGR)
            # cv2.putText(img_S2_bgr, timestamp_string, (10, img_S2_bgr.shape[0]-10),
            #             cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 255, 0), 1)
            
            # cv2.imshow("Sonar Image", img_s2_bgr)
            # cv2.waitKey(1)

            plt.draw()
            plt.pause(0.05)

            # Store current frame as previous 
            T_t1 = T_t2

            results_row = [image_files[i+step_size], t , x_optim[0], x_optim[1], x_optim[2], flow_pose.n_best_inliers]
            results.loc[len(results)] = results_row

            # input("Press Enter for Next Sonar Frame")

        plt.ioff()
        plt.show()
        # cv2.destroyAllWindows()
        cv2.destroyWindow("RANSAC Debug")
        cv2.destroyWindow("Masked Sonar Frame")
        cv2.destroyWindow("Frame Transform")

    finally: 
        save_dir = '/home/ale/sonar_ws/src/oculus-ros2-cpp-driver/OCULUS_ROS2/Data/utils/results/'
        csv_name = 'result_magn' + str(min_magnitude) + '_points' + str(n_points) + '_thresh' + str(threshold) + '_iter' + str(max_iter) + '.csv'
        save_path = os.path.join(save_dir, csv_name)
        if save_data_csv == True:
            results.to_csv(save_path)
        
        return 


if __name__ == '__main__':
    main()