import cv2
import numpy as np 
import matplotlib
matplotlib.use('TkAgg')  # or 'Qt5Agg' — be explicit
import matplotlib.pyplot as plt
import os

import pandas as pd

import time

import dense_flow_sonar_debugv2


def main(args=None):

    results = pd.read_csv('/home/ale/sonar_ws/src/datasets/rosbag2_2026_04_02-17_01_57/sonar/cartesian/optical_flow_results/result_magn5_points25_thresh3_iter100.csv', index_col=0)

    img_names = results["img_name"].tolist() 
    t_csv= results["time"].to_numpy()
    dx_csv = results["dx_estim"].to_numpy()
    dy_csv = results["dy_estim"].to_numpy()
    dth_csv= results["dth_estim"].to_numpy()
    n_best_inliers_csv = results["N_inliers"].to_numpy()

    # --- Path to Cartesian Sonar Images ---
    imgs_path = '/home/ale/sonar_ws/src/datasets/rosbag2_2026_04_02-17_01_57/sonar/cartesian/'
    
    rgb_path = '/home/ale/sonar_ws/src/datasets/rosbag2_2026_04_02-17_01_57/rgb/'
    
    # Read and Rescale first images 
    img_S1 = cv2.imread(os.path.join(imgs_path, img_names[0]), cv2.IMREAD_GRAYSCALE)
    img_S2 = cv2.imread(os.path.join(imgs_path, img_names[1]), cv2.IMREAD_GRAYSCALE)

    scale_factor = 0.6
    img_S1 = cv2.resize(img_S1, None, fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)
    img_S2 = cv2.resize(img_S2, None, fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)
    
    # --- Setup figure ---
    fig, axes = plt.subplots(2, 3, figsize=(15, 10))
    ax_flow       = axes[0, 0]
    ax_rgb        = axes[0, 1]
    ax_traj       = axes[0, 2]
    ax_log        = axes[1, 0]
    ax_transl     = axes[1, 1]
    ax_rot        = axes[1, 2]

    plt.tight_layout()
    plt.ion()

    cv2.namedWindow("Frame Transform", cv2.WINDOW_NORMAL)
    
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
    rgb_imshow = ax_rgb.imshow(dummy_rgb)
    ax_rgb.set_title("RGB Image")

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

    dense_flow_sonar_debugv2.draw_axes(T_t1, 'S0')

    plt.ion()
    fig.canvas.draw()      # Force canvas initialization
    plt.pause(0.5)         # Block briefly so the window actually appears

    # cv2.namedWindow("Sonar Image", cv2.WINDOW_NORMAL)
    # cv2.imshow("Sonar Image", img_S1)

    flow_pose = dense_flow_sonar_debugv2.SonarOpticalFlowRansacPose(max_range=5, max_aperture_deg=130,
                                    height=img_S1.shape[0], width=img_S1.shape[1],
                                    min_magnitude=15, n_points=30, threshold=5, max_iter=30)
    
    step_size = 1

    for i in range(0, len(img_names) - step_size, step_size):

        time.sleep(0.15)

        # Read consecutive frame images and resize 
        img_S1 = cv2.imread(os.path.join(imgs_path, img_names[i]),   cv2.IMREAD_GRAYSCALE)
        img_S2 = cv2.imread(os.path.join(imgs_path, img_names[i+step_size]), cv2.IMREAD_GRAYSCALE)
        img_S1 = cv2.resize(img_S1, None, fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)
        img_S2 = cv2.resize(img_S2, None, fx=scale_factor, fy=scale_factor, interpolation=cv2.INTER_LINEAR)
        
        img_rgb = cv2.imread(os.path.join(rgb_path, img_names[i]),   cv2.IMREAD_COLOR_RGB)
        rgb_imshow.set_data(img_rgb)

        timestamp_string = f"S: {img_names[i+step_size]} | t={t:.2f}s"
        timestamp_string_img = f"S1: {img_names[i]} -> S2: {img_names[i+step_size]}"

        # Compute optimal estimated motion from optical flow between S1 and S2
        x_optim = [dx_csv[i], dy_csv[i], dth_csv[i]]

        # S1_T_S2_estim = flow_pose.compute_S2_to_S1_transform(x_optim[0], x_optim[1], x_optim[2])
        # T_t2 = np.dot(T_t1, S1_T_S2_estim)

        dx_px = x_optim[0] / flow_pose.resolution_x
        dy_px = x_optim[1] / flow_pose.resolution_x    
        
        S1_T_S2_estim = flow_pose.compute_S2_to_S1_transform(x_optim[0], x_optim[1], x_optim[2])
        T_t2 = np.dot(T_t1, S1_T_S2_estim)

        S1_T_S2_estim_px = flow_pose.compute_S2_to_S1_transform(dx_px, dy_px, x_optim[2])
        affine = flow_pose.compute_Im2_to_Im1_transform(S1_T_S2_estim_px)
        S2_warped = cv2.warpAffine(img_S2, affine, (flow_pose.width, flow_pose.height))

        S1_color = cv2.cvtColor(img_S1, cv2.COLOR_GRAY2BGR)

        red_overlay = np.zeros_like(S1_color)
        red_overlay[:, :, 2] = S2_warped

        overlay = cv2.addWeighted(S1_color, 1.0, red_overlay, 0.95, 0)

        txtS1 = f"S1: {img_names[i]}"
        txtS2 = f"S2: {img_names[i+step_size]}"

        cv2.putText(overlay, txtS1, (10, overlay.shape[0]-50),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.85, (255, 255, 255), 2)
        cv2.putText(overlay, txtS2, (10, overlay.shape[0]-20),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.85, (0, 0, 255), 2)
        
        cv2.imshow("Frame Transform", overlay)
        cv2.waitKey(1) 

        dth_deg = x_optim[2] * 180 / np.pi

        # Accumulate time series
        times.append(t_csv[i])
        xs.append( xs[-1] + x_optim[0])
        ys.append( ys[-1] + x_optim[1])
        ths.append(ths[-1] + dth_deg)

        n_best_inliers.append(n_best_inliers_csv[i])

        # Origin of new frame in world coords
        Origin = np.array([0, 0, 1])
        orig = T_t2 @ Origin
        orig_xs.append(orig[0])
        orig_ys.append(orig[1])
        

        line_inliers.set_data(times, n_best_inliers)
        ax_log.relim()
        ax_log.autoscale_view()

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
        dense_flow_sonar_debugv2.draw_axes(T_t2, f'S{i+2}')
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
        
        T_t1 = T_t2

        plt.draw()
        plt.pause(0.05)


    plt.ioff()
    plt.show()
    cv2.destroyAllWindows()

if __name__ == '__main__':
    main()