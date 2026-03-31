import cv2
import numpy as np 
# import matplotlib.pyplot as plt

class SonarOpticalFlowRansacPose: 

    def __init__(self, range=5, bins=500, height=100, width=100, min_magnitude=10, n_points=10, threshold=100, max_iter=50):

        self.range = range                        # [m]
        self.bins = bins                          # [bins] number of discretization, along range
        self.height = height
        self.width = width

        self.min_magnitude = min_magnitude
        self.resolution = range/height              # [m/pixel]

        self.cv_flow = np.zeros(shape=(height, width, 2))  # (y,x,flow) optical flow matrix, OpenCV convention

        self.R_S_Im = np.array([[0, -1], [1, 0]])              # Rotation from Im (OpenCV) to Sonar
        self.t_S_Im = np.array([self.height, -self.width//2])  # Translation from Im (OpenCV) to Sonar 

        # parameters for Ransac Estimation
        # best model unkowns (translation in x, y and rotation)
        self.tx_best = 0
        self.ty_best = 0
        self.dtheta_best = 0

        self.inliers_best = None

        # number of points used for estimation 
        self.n_points = n_points 
        # list of inliers [[x1,y1], [x2, y2], ..]
        self.inliers = np.zeros(shape=(n_points, 2))
        # inliers-outliers threshold 
        self.threshold = threshold
        # max RANSAC iteration 
        self.max_iter = max_iter


    # In case the sonar image frame size changes (e.g. different range or aperture - freq mode)
    def set_image_shape(self, frame):
        self.height = frame.shape[0]
        self.width = frame.shape[1]
        self.resolution = self.range/self.height  

    def compute_image_flow(self, s1_frame, s2_frame, min_flow_magnitude=0.5):

        self.cv_flow = cv2.calcOpticalFlowFarneback(
            s1_frame, 
            s2_frame, 
            None,
            pyr_scale=0.5,      # Pyramid scale
            levels=3,            # Number of pyramid layers
            winsize=10,          # Averaging window size
            iterations=3,        # Number of iterations at each pyramid level
            poly_n=5,            # Size of pixel neighborhood
            poly_sigma=1.2,      # Standard deviation for Gaussian
            flags=0
        )

        magnitude = np.sqrt(self.cv_flow[:,:,0]**2 + self.cv_flow[:,:,1]**2)
        mask = magnitude <= min_flow_magnitude
        self.cv_flow[mask] = 0  # zero out low-magnitude vectors in place

        return self.cv_flow

    # def apply_flow_sonar(self, p_s1):
    #     p_s2_flow = np.zeros(shape=p_s1.shape)

    #     flow_vec = self.cv_flow.reshape(-1, 2).T           # (2, h*w)
    #     flow_rotated = self.R_S_Im @ flow_vec              # (2, h*w)
    #     flow_rotated = flow_rotated.T.reshape(self.height, self.width, 2)  # (h, w, 2)

    #     p_s2_flow[:,:,0] = p_s1[:,:,0] + self.resolution * flow_rotated[:,:,0]
    #     p_s2_flow[:,:,1] = p_s1[:,:,1] + self.resolution * flow_rotated[:,:,1]

    #     return p_s2_flow
    
    def apply_cv_flow(self, p_s1):
        y = p_s1[:, 0].astype(int)
        x = p_s1[:, 1].astype(int)

        flow = self.cv_flow[y, x]        # shape (N, 2)

        p_s2_flow = p_s1 + flow          # shape (N, 2)

        return p_s2_flow
    
    def compute_transform(self, dx, dy, dth):
        return np.linalg.inv(np.array([[np.cos(dth), -np.sin(dth), dx], [np.sin(dth), np.cos(dth), dy], [0, 0, 1]]))
  
    def compute_matrix(self, p_s1, p_s2):
        alpha = p_s2[:,0] / p_s2[:,1]
        beta  = p_s2[:,1] + np.square(p_s2[:,0])/p_s2[:,1]
        gamma = p_s1[:,0] - p_s2[:,0]/p_s2[:,1] * p_s1[:,1]

        A = np.array([[-alpha[i], 1, beta[i]] for i in range(len(alpha))])
        b = np.array(gamma)

        return A, b

    def solve_pose(self, A : np.ndarray , b: np.ndarray):
        if A.shape[0] ==3 and A.shape[1] ==3: # inversion 
            x = np.linalg.inv(A) @ b
            x[2] = np.arcsin(x[2])
            return x
        else: # least squares (pseudo-inverse)
            x = np.linalg.inv(A.T @ A) @ A.T @ b
            x[2] = np.arcsin(x[2])
            return x
    
    def measure_consensus(self, p_s1, T_s1_s2_estim):

        p_s1_tilde = np.hstack([p_s1, np.ones((len(p_s1), 1))])  # (10, 3)
        p_s2_tilde = (T_s1_s2_estim @ p_s1_tilde.T).T                  # (10, 3)
        p_s2_estim = p_s2_tilde[:, :2]    
                                      
        estim_flow = np.zeros_like(self.cv_flow)
        estim_flow[p_s1[:,0], p_s1[:,1],:] = p_s2_estim - p_s1

        for p in p_s1:
            error = np.sqrt((self.cv_flow[p[0],p[1],0] - estim_flow[p[0],p[1],0])**2 + (self.cv_flow[p[0], p[1],1] - estim_flow[p[0],p[1],1])**2)
            if error < self.threshold: 
                self.inliers = np.append(self.inliers, [[p[0], p[1]]], axis=0)
                
        return self.inliers
    

    def estimate_2d_flow_motion(self, s1_frame, s2_frame):

        self.compute_image_flow(s1_frame, s2_frame, min_flow_magnitude = self.min_magnitude)
        
        # Get indices where flow magnitude is above threshold
        nonzero_y, nonzero_x = np.where(
            np.sqrt(self.cv_flow[:,:,0]**2 + self.cv_flow[:,:,1]**2) > self.min_magnitude
        )
        nonzero_coords = np.column_stack((nonzero_y, nonzero_x))

        # Remove points where x == 0 or y == 0
        valid_mask = (nonzero_coords[:, 0] != 0) & (nonzero_coords[:, 1] != 0)
        nonzero_coords = nonzero_coords[valid_mask]

        print("Vectors above magnitude (after filtering): ", len(nonzero_coords))

        # Now check AFTER filtering
        if len(nonzero_coords) < self.n_points:
            return np.array([0.0, 0.0, 0.0])

        # Remove points where x == 0 or y == 0
        valid_mask = (nonzero_coords[:, 0] != 0) & (nonzero_coords[:, 1] != 0)
        nonzero_coords = nonzero_coords[valid_mask]

        n_iter = 0
        n_best_inliers = self.n_points
        while n_iter < self.max_iter:

            # print("Iteration number: ", n_iter)

            # re-initialize inlier set
            self.inliers = np.zeros(shape=(self.n_points, 2))
            
            # initialize inliers 
            if self.inliers_best is None: 
                self.inliers_best = self.inliers

            sampled_idx = np.random.choice(len(nonzero_coords), size=self.n_points, replace=False)
            self.inliers = nonzero_coords[sampled_idx]  # (n_points, 2) of [y, x]
                
            p_s2 = self.apply_cv_flow(self.inliers)
            A, b = self.compute_matrix(self.inliers, p_s2)
            x = self.solve_pose(A, b)
            # print("Current Estimate: tx = ", x[0], " ty = ", x[1], "theta = ", x[2])

            T_s1_s2_estim = self.compute_transform(x[0], x[1], x[2])
            self.inliers = self.measure_consensus(nonzero_coords, T_s1_s2_estim)
            n_inliers = len(self.inliers)
            # print("Inliers Number: ", n_inliers)
            if n_inliers > n_best_inliers: 
                self.tx_best = x[0]
                self.ty_best = x[1]
                self.dtheta_best = x[2]

                # print("Inliers Number: ", n_inliers)

                self.inliers_best = self.inliers
                n_best_inliers = n_inliers

            n_iter += 1

        # print(f"\nUsing all {n_best_inliers} inliers over best model")
        p_s2_optim = self.apply_cv_flow(self.inliers_best)
        A_optim,b_optim = self.compute_matrix(self.inliers_best, p_s2_optim)
        x_optim = self.solve_pose(A_optim, b_optim)

        x_optim[0] = x_optim[0] * self.resolution
        x_optim[1] = x_optim[1] * self.resolution
        print("Optimal Estimate: dx = ", x_optim[0], " dy = ", x_optim[1], "dtheta = ", x_optim[2])
        
        return x_optim
