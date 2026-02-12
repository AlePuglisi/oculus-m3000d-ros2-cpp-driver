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
    timestamp_str = filename.replace('.png', '')
    parts = timestamp_str.split('_')
    seconds = int(parts[0])
    nanoseconds = int(parts[1])
    timestamp = seconds + nanoseconds / 1e9
    timestamps.append(timestamp)

timestamps = np.array(timestamps)

# Calculate actual time intervals
time_intervals = np.diff(timestamps)
mean_dt = np.mean(time_intervals)
actual_sample_rate = 1.0 / mean_dt

print(f"Number of frames: {len(filenames)}")
print(f"Actual sampling rate: {actual_sample_rate:.2f} Hz")
print(f"Nyquist frequency: {actual_sample_rate/2:.2f} Hz")

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

# Create multiple visualizations
fig, axes = plt.subplots(2, 2, figsize=(15, 10))

# 1. Linear scale
axes[0, 0].plot(xf, 2.0/samples * yf_mean_magnitude[:samples//2])
axes[0, 0].set_xlabel('Frequency (Hz)')
axes[0, 0].set_ylabel('Average Magnitude')
axes[0, 0].set_title('Linear Scale')
axes[0, 0].grid(True)

# 2. Log-log scale (best for 1/f noise)
# Skip DC component (index 0) and zero values
valid_idx = (xf > 0) & (yf_mean_magnitude[:samples//2] > 0)
axes[0, 1].loglog(xf[valid_idx], yf_mean_magnitude[:samples//2][valid_idx])
axes[0, 1].set_xlabel('Frequency (Hz)')
axes[0, 1].set_ylabel('Average Magnitude')
axes[0, 1].set_title('Log-Log Scale (1/f noise appears linear)')
axes[0, 1].grid(True, which="both", ls="-", alpha=0.2)

# 3. Semi-log (log y-axis)
axes[1, 0].semilogy(xf[valid_idx], yf_mean_magnitude[:samples//2][valid_idx])
axes[1, 0].set_xlabel('Frequency (Hz)')
axes[1, 0].set_ylabel('Average Magnitude (log)')
axes[1, 0].set_title('Semi-log Scale')
axes[1, 0].grid(True)

# 4. Power Spectral Density
power_spectrum = yf_magnitude**2
mean_power = power_spectrum.mean(axis=(0, 1))
axes[1, 1].loglog(xf[valid_idx], mean_power[:samples//2][valid_idx])
axes[1, 1].set_xlabel('Frequency (Hz)')
axes[1, 1].set_ylabel('Power Spectral Density')
axes[1, 1].set_title('Power Spectrum (log-log)')
axes[1, 1].grid(True, which="both", ls="-", alpha=0.2)

plt.tight_layout()
plt.show()

# Analyze the spectral slope (for 1/f^alpha noise)
# Fit a line in log-log space to estimate alpha
from scipy import stats

# Use mid-frequency range to avoid DC and high-frequency noise
freq_range = (xf > actual_sample_rate * 0.01) & (xf < actual_sample_rate * 0.3)
log_freq = np.log10(xf[freq_range])
log_mag = np.log10(yf_mean_magnitude[:samples//2][freq_range])

slope, intercept, r_value, p_value, std_err = stats.linregress(log_freq, log_mag)

print(f"\n=== Spectral Analysis ===")
print(f"Spectral slope (alpha): {-slope:.2f}")
print(f"R² (goodness of fit): {r_value**2:.4f}")
print(f"\nInterpretation:")
if -slope < 0.5:
    print("  → White noise dominant (flat spectrum)")
elif 0.5 <= -slope < 1.5:
    print("  → Pink noise / 1/f noise (typical for natural phenomena)")
elif 1.5 <= -slope < 2.5:
    print("  → Brownian motion / random walk")
else:
    print("  → Strong low-frequency components (drift, slow variations)")

# Find dominant frequencies (excluding DC)
peak_indices = []
threshold = np.percentile(yf_mean_magnitude[1:samples//2], 95)
for i in range(1, samples//2):
    if yf_mean_magnitude[i] > threshold:
        peak_indices.append(i)

if peak_indices:
    print(f"\n=== Significant Frequencies (>95th percentile) ===")
    for idx in peak_indices[:10]:  # Show top 10
        print(f"  {xf[idx]:.3f} Hz (magnitude: {yf_mean_magnitude[idx]:.2e})")