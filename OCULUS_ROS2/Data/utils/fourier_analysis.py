import numpy as np 
import cv2 
import os 
import matplotlib.pyplot as plt 

from scipy.fft import fft, fftfreq

images_path = "/home/ale/sonar_ws/src/oculus-ros2-cpp-driver/OCULUS_ROS2/Data/images/sonar/2026_02_06_14_15/polar"

image_window = []
window_len = 10

image = None

first_image = cv2.imread(os.path.join(images_path, os.listdir(images_path)[0]), cv2.IMREAD_GRAYSCALE)
pixels_signal = np.zeros((first_image.shape[0],first_image.shape[1], len(os.listdir(images_path))))

for t, filename in enumerate(os.listdir(images_path)):
    file_path = os.path.join(images_path, filename)
    
    image = cv2.imread(file_path, cv2.IMREAD_GRAYSCALE)

    pixels_signal[:,:,t] = np.array(image)


samples = len(os.listdir(images_path))
T = 1

yf = fft(pixels_signal[0,0,:])
# xf = fftfreq(samples, T)[:samples//2]
# import matplotlib.pyplot as plt

# plt.subplot(1,2,1)
# plt.plot(xf, 2.0/samples * np.abs(yf[0:samples//2]))
# plt.grid()

# plt.subplot(1,2,2)
# plt.plot(range(samples), pixels_signal[0,0,:])

# plt.show()

# M = np.reshape(pixels_signal, pixels_signal.size)

# yf_mean = fft(M)
# xf_mean = fftfreq(samples, T)[:samples//2]
# import matplotlib.pyplot as plt

# plt.plot(xf_mean, 2.0/samples * np.abs(yf_mean[0:samples//2]))
# plt.grid()
# plt.show()

# Compute FFT for all pixels
yf = np.zeros((first_image.shape[0], first_image.shape[1], samples), dtype=complex)
for i in range(first_image.shape[0]):
    for j in range(first_image.shape[1]):
        yf[i,j,:] = fft(pixels_signal[i,j,:])

# Average the magnitude spectrum across all pixels
yf_magnitude = np.abs(yf)
yf_mean_magnitude = yf_magnitude.mean(axis=(0, 1))  # Average over both spatial dimensions

# Plot
xf = fftfreq(samples, T)[:samples//2]
plt.plot(xf, 2.0/samples * yf_mean_magnitude[:samples//2])
plt.xlabel('Frequency (Hz)')
plt.ylabel('Average Magnitude')
plt.grid()
plt.show()


