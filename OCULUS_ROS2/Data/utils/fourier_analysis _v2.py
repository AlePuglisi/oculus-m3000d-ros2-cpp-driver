import numpy as np 
import cv2 
import os 
import matplotlib.pyplot as plt 
from scipy.fft import fft, fftfreq

images_path = "/home/ale/sonar_ws/src/oculus-ros2-cpp-driver/OCULUS_ROS2/Data/images/sonar/2026_02_06_14_15/polar"

# Get sorted list of files
filenames = sorted(os.listdir(images_path))

# Extract timestamps from filenames
timestamps = []
for filename in filenames:
    # Remove .png extension
    timestamp_str = filename.replace('.png', '')
    # Split by underscore: seconds_nanoseconds
    parts = timestamp_str.split('_')
    seconds = int(parts[0])
    nanoseconds = int(parts[1])
    # Convert to seconds (float)
    timestamp = seconds + nanoseconds / 1e9
    timestamps.append(timestamp)

timestamps = np.array(timestamps)

# Calculate actual time intervals
time_intervals = np.diff(timestamps)
mean_dt = np.mean(time_intervals)  # Average time between frames
actual_sample_rate = 1.0 / mean_dt  # Frames per second

print(f"Number of frames: {len(filenames)}")
print(f"Total duration: {timestamps[-1] - timestamps[0]:.4f} seconds")
print(f"Mean time between frames: {mean_dt:.6f} seconds")
print(f"Actual sampling rate: {actual_sample_rate:.2f} Hz")
print(f"Nyquist frequency: {actual_sample_rate/2:.2f} Hz")
print(f"Min interval: {time_intervals.min():.6f} s")
print(f"Max interval: {time_intervals.max():.6f} s")
print(f"Std deviation of intervals: {time_intervals.std():.6f} s")

# Load images
first_image = cv2.imread(os.path.join(images_path, filenames[0]), cv2.IMREAD_GRAYSCALE)
pixels_signal = np.zeros((first_image.shape[0], first_image.shape[1], len(filenames)))

for t, filename in enumerate(filenames):
    file_path = os.path.join(images_path, filename)
    image = cv2.imread(file_path, cv2.IMREAD_GRAYSCALE)
    pixels_signal[:,:,t] = np.array(image)

samples = len(filenames)

# Compute FFT using actual time interval
yf = fft(pixels_signal, axis=2)

# Average the magnitude spectrum across all pixels
yf_magnitude = np.abs(yf)
yf_mean_magnitude = yf_magnitude.mean(axis=(0, 1))

# Use actual sampling interval for frequency axis
xf = fftfreq(samples, mean_dt)[:samples//2]

plt.figure(figsize=(12, 6))
plt.plot(xf, 2.0/samples * yf_mean_magnitude[:samples//2])
plt.xlabel('Frequency (Hz)')
plt.ylabel('Average Magnitude')
plt.title(f'Mean FFT Spectrum (Sampling Rate: {actual_sample_rate:.2f} Hz)')
plt.grid()
plt.xlim(0, min(50, actual_sample_rate/2))  # Limit x-axis to first 50 Hz or Nyquist
plt.show()

# Optional: Plot to see if there are specific dominant frequencies
plt.figure(figsize=(12, 6))
dominant_freq_idx = np.argmax(yf_mean_magnitude[1:samples//2]) + 1  # Skip DC component
dominant_freq = xf[dominant_freq_idx]
print(f"\nDominant frequency (excluding DC): {dominant_freq:.2f} Hz")

plt.semilogy(xf, yf_mean_magnitude[:samples//2])
plt.xlabel('Frequency (Hz)')
plt.ylabel('Average Magnitude (log scale)')
plt.title(f'Mean FFT Spectrum - Log Scale')
plt.axvline(dominant_freq, color='r', linestyle='--', label=f'Dominant: {dominant_freq:.2f} Hz')
plt.grid()
plt.legend()
plt.xlim(0, min(50, actual_sample_rate/2))
plt.show()