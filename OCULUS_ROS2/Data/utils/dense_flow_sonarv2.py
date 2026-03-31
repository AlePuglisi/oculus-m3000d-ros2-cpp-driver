import cv2
import numpy as np 
import matplotlib.pyplot as plt
import os

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
        self.resolution_y = (self.max_range*np.sin(self.max_aperture_rad/2))/self.width    
        
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

        # --- RANSAC parameters --- 
        # Parameters for Ransac Estimation
        # Best model unkowns (translation in x, y and rotation)
        self.tx_best = 0
        self.ty_best = 0
        self.dtheta_best = 0
        # Best RANSAC inliers 
        self.inliers_best = None
        # Number of points used for estimation 
        self.n_points = n_points 
        # List of inliers [[x1,y1], [x2, y2], ..]
        self.inliers = np.zeros(shape=(n_points, 2))
        # Inliers-outliers threshold 
        self.threshold = threshold
        # Max RANSAC iteration 
        self.max_iter = max_iter
        
        if self.debug:
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
        self.resolution_y = (self.max_range*np.sin(self.max_aperture_rad/2))/self.width    

    def update_Im_to_S_Transform(self):
        # In case the image shape chages due to sonar reconfiguration 
        self.S_t_Im = np.array([self.height, -self.width//2])  # Translation from Im (OpenCV) to Sonar 
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

        if self.debug:
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

        if self.debug:
            print(f'''SonarOpticalFlowRansacPose Updated:
                        range_max={self.max_range:.2f} m  
                        aperture_max={self.max_aperture_deg:.2f} deg
                        x_res={self.resolution_x:.4f} m/pixels
                        y_res={self.resolution_y:.4f} m/pixels''')

    def compute_image_flow(self, s1_frame, s2_frame, min_flow_magnitude=5):
        # Computing sonar image optical flow (with OpenCV convention)
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
        self.cv_flow[mask] = 0  # Zero out low-magnitude vectors in place

        return self.cv_flow
    
    def apply_flow(self, p_s1):
        # Project points based on sonar image flow 
        y = p_s1[:, 1].astype(int)
        x = p_s1[:, 0].astype(int)

        flow_Im = self.cv_flow[y, x]        
        flow_S = (self.S_R_Im @ flow_Im.T).T

        # invert y and x that are in the opposite order 
        # p_s1 = np.column_stack([p_s1[:,1], p_s1[:,0]])
        p_s1_tilde = np.hstack([p_s1,np.ones((len(p_s1), 1))]) 

        p_s1_tilde = (self.S_T_Im @ p_s1_tilde.T).T                
        p_s1 = p_s1_tilde[:, :2]    
        p_s2_flow = p_s1 + flow_S         

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

    def compute_equation_matrix(self, p_s1, p_s2):
        # Compute the matrices associated to the system of equations from flow correspondances
        alpha = p_s2[:,1] / p_s2[:,0]
        beta  = p_s2[:,0] + np.square(p_s2[:,1])/p_s2[:,0]
        gamma = p_s1[:,1] - p_s2[:,1]/p_s2[:,0] * p_s1[:,0]

        A = np.column_stack([[-alpha, np.ones(len(alpha)), beta]])
        b = np.array(gamma)

        return A, b

    def solve_pose(self, A : np.ndarray , b: np.ndarray):
        # Solve the system of equations from flow correspondances
        if A.shape[0] ==3 and A.shape[1] ==3: # inversion 
            x = np.linalg.inv(A) @ b
            x[2] = np.arcsin(x[2])
            return x
        else: # Least-squares (pseudo-inverse)
            x = np.linalg.inv(A.T @ A) @ A.T @ b
            x[2] = np.arcsin(x[2])
            return x
    
    def measure_consensus(self, p_s1, S2_T_S1_estim):
        # Check which vectors are coherent with the computed motion 
        # invert y and x that are in the opposite order 
        # p_s1 = np.column_stack([p_s1[:,1], p_s1[:,0]])
        p_s1_tilde = np.hstack([p_s1, np.ones((len(p_s1), 1))]) 
        p_s1_tilde = (self.S_T_Im @ p_s1_tilde.T).T    

        p_s2_tilde = (S2_T_S1_estim @ p_s1_tilde.T).T                
        p_s2_estim = p_s2_tilde[:, :2]    
        
        flow_Im = self.cv_flow[p_s1[:,1], p_s1[:,0]]       
        flow_S = (self.S_R_Im @ flow_Im.T).T
        
        estim_flow = np.zeros_like(flow_S)
        # Search for inliers based on real flow with respect to estimated flow 
        for idx, p in enumerate(p_s1):
            estim_flow[idx, :] = p_s2_estim[idx] - p_s1[idx]
            error = np.sqrt((flow_S[idx, 0] - estim_flow[idx, 0])**2 + 
                            (flow_S[idx, 1] - estim_flow[idx, 1])**2)
            if error < self.threshold and p not in self.inliers: 
                self.inliers = np.append(self.inliers, [[p[0], p[1]]], axis=0)
                
        return self.inliers
    

    def estimate_2d_flow_motion(self, s1_frame, s2_frame):
        # --- RANSAC sonar 3 DOF 2D motion estimation ---
        # --- 1) Initialize Image Flow and filter out useless flow vectors ---
        self.compute_image_flow(s1_frame, s2_frame, min_flow_magnitude = self.min_magnitude)
        
        # Get indices where flow is higher than min magnitude 
        nonzero_y, nonzero_x = np.where(
            np.sqrt(self.cv_flow[:,:,0]**2 + self.cv_flow[:,:,1]**2) > self.min_magnitude
        )
        nonzero_coords = np.column_stack((nonzero_x, nonzero_y))

        # Remove points where x == 0 or y == 0
        valid_mask = (nonzero_coords[:, 0] != 0) & (nonzero_coords[:, 1] != 0)
        nonzero_coords = nonzero_coords[valid_mask]

        # Guard: not enough flow points to sample from
        if len(nonzero_coords) < self.n_points:
            print(f"[WARN] Not enough flow points ({len(nonzero_coords)}) to sample {self.n_points}, skipping frame.")
            return np.array([0.0, 0.0, 0.0])
        
        # --- 2) Start RANSAC Iterations ---       
        n_iter = 0
        n_best_inliers = self.n_points

        while n_iter < self.max_iter:
            if self.debug:
                print("START ITERATION ", n_iter)

            if self.inliers_best is None: 
                self.inliers_best = self.inliers

            # --- 2.1) Initialize empty inliers set --- 
            # Re-initialize inlier set
            self.inliers = np.zeros(shape=(self.n_points, 2))
            
            # --- 2.2) Select n_points random points in the non-zero flow image ---
            sampled_idx = np.random.choice(len(nonzero_coords), size=self.n_points, replace=False)
            self.inliers = nonzero_coords[sampled_idx]  # (n_points, 2) of [y, x]
            
            # --- 2.3) Apply flow on the randomly selected points ---
            p_s1, p_s2 = self.apply_flow(self.inliers)

            # --- 2.4) Solve the pose estimation problem based on flow constraints ---
            A, b = self.compute_equation_matrix(p_s1, p_s2)
            x = self.solve_pose(A, b)

            # Re-scale based on metrix/pixels resolution
            x[1] = x[1] * self.resolution_y
            x[0] = x[0] * self.resolution_x 
            if self.debug:
                print(f"Estimate: tx=", x[0], "m | ty=", x[1], " m | theta=", x[2], " rad")
            
            # Compute tranformation matrix 
            S2_T_S1_estim = self.compute_S1_to_S2_transform(x[0], x[1], x[2])

            # --- 2.5) Measure Consensus and update inliers ---
            self.inliers = self.measure_consensus(nonzero_coords, S2_T_S1_estim)
            n_inliers = len(self.inliers)
            if self.debug:
                print("Inliers Number: ", n_inliers)
            # Update best model if higher number of inliers are found 
            if n_inliers > n_best_inliers: 
                self.tx_best = x[0]
                self.ty_best = x[1]
                self.dtheta_best = x[2]
                if self.debug:
                    print("Update optimal model")

                self.inliers_best = self.inliers
                n_best_inliers = n_inliers
            if self.debug:
                print(f" \n--------------------------------------\n ")

            n_iter += 1

        if self.debug:
            print(f"\nUsing all ({n_best_inliers}) best inliers over best model")

        # --- 3) Recompute best pose estimation using all inliers of the best model ---
        p_s1, p_s2_optim = self.apply_flow(self.inliers_best)
        A_optim,b_optim = self.compute_equation_matrix(p_s1, p_s2_optim)
        x_optim = self.solve_pose(A_optim, b_optim)
        
        # Re-scale based on metrix/pixels resolution
        x_optim[1] = x_optim[1] * self.resolution_y
        x_optim[0] = x_optim[0] * self.resolution_x 

        if self.debug:
            print("Optimal Estimate: tx=", x_optim[0], "m | ty=", x_optim[1], " m | theta=", x_optim[2], " rad")
        
        # Guard condition in case of unexpected failures and nan values 
        if(np.isnan(x_optim).sum() > 0):
            return np.array([0.0, 0.0, 0.0])

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
    # --- Path to Cartesian Sonar Images ---
    imgs_path = '/home/ale/sonar_ws/src/oculus-ros2-cpp-driver/OCULUS_ROS2/Data/images/sonar/2026_02_06_14_15/cartesian/'
    image_files = sorted([
        f for f in os.listdir(imgs_path)
        if f.lower().endswith(('.png', '.jpg', '.jpeg'))
    ])

    # Read and Rescale first images 
    img_S1 = cv2.imread(os.path.join(imgs_path, image_files[0]), cv2.IMREAD_GRAYSCALE)
    img_S2 = cv2.imread(os.path.join(imgs_path, image_files[1]), cv2.IMREAD_GRAYSCALE)

    scale_factor = 0.6
    img_S1 = cv2.resize(img_S1, None, fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)
    img_S2 = cv2.resize(img_S2, None, fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)
    
    # --- Initialize RANSAC-Sonar-Flow Pose estimation from first couple of images ---
    flow_pose = SonarOpticalFlowRansacPose(max_range=5, max_aperture_deg=130,
                                           height=img_S1.shape[0], width=img_S1.shape[1],
                                           min_magnitude=5, n_points=15, threshold=5, max_iter=10)

    # --- Setup figure ---
    fig, axes = plt.subplots(2, 2, figsize=(12, 10))
    ax_flow   = axes[0, 0]
    ax_traj   = axes[0, 1]
    ax_transl = axes[1, 0]
    ax_rot    = axes[1, 1]

    plt.tight_layout()
    plt.ion()

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

    # Initialize Sonar Frame S0
    T_t1 = np.eye(3)
    T_t1[0:2, 0:2] = np.array([[0, 1], [1,0]]) @ T_t1[0:2, 0:2]
    plt.sca(ax_traj)
    draw_axes(T_t1, 'S0')

    # cv2.namedWindow("Sonar Image", cv2.WINDOW_NORMAL)
    # cv2.imshow("Sonar Image", img_S1)

    for i in range(len(image_files) - 1):
        # Read consecutive frame images and resize 
        img_S1 = cv2.imread(os.path.join(imgs_path, image_files[i]),   cv2.IMREAD_GRAYSCALE)
        img_S2 = cv2.imread(os.path.join(imgs_path, image_files[i+1]), cv2.IMREAD_GRAYSCALE)
        img_S1 = cv2.resize(img_S1, None, fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)
        img_S2 = cv2.resize(img_S2, None, fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)
        
        # Compute optimal estimated motion from optical flow between S1 and S2
        x_optim = flow_pose.estimate_2d_flow_motion(img_S1, img_S2)

        S1_T_S2_estim = flow_pose.compute_S2_to_S1_transform(x_optim[0], x_optim[1], x_optim[2])
        T_t2 = np.dot(T_t1, S1_T_S2_estim)

        t1 = float(os.path.splitext(image_files[i]  )[0].replace('_', '.'))
        t2 = float(os.path.splitext(image_files[i+1])[0].replace('_', '.'))
        dt = t2 - t1
        t += dt

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

        timestamp_string = f"S: {image_files[i+1]} | t={t:.2f}s"
        timestamp_string_img = f"S1: {image_files[i]} -> S2: {image_files[i+1]}"

        # --- Update optical flow subplot with set_data (no cla needed) ---
        flow = flow_pose.cv_flow
        flow_img = cv2.cvtColor(img_S2, cv2.COLOR_GRAY2BGR)
        step = 20
        h, w = img_S2.shape
        y_flow, x_flow = np.mgrid[step//2:h:step, step//2:w:step].reshape(2, -1).astype(int)
        for xi, yi in zip(x_flow, y_flow):
            fdx, fdy = flow[yi, xi]
            if np.sqrt(fdx**2 + fdy**2) > 5:
                cv2.arrowedLine(flow_img, (xi, yi), (int(xi+fdx), int(yi+fdy)),
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
        plt.pause(0.01)

        # Store current frame as previous 
        T_t1 = T_t2

    plt.ioff()
    plt.show()
    # cv2.destroyAllWindows()

if __name__ == '__main__':
    main()