import numpy as np 
import cv2 
import os 
import matplotlib.pyplot as plt 

images_path = "/home/ale/ros2_ws/src/oculus_ros2/Data/images/sonar/2025_11_12_11_56/cartesian"

image_window = []
window_len = 10

image = None

for filename in os.listdir(images_path):
    file_path = os.path.join(images_path, filename)
    
    image = cv2.imread(file_path, cv2.IMREAD_GRAYSCALE)

    image_window.append(image)

    if len(image_window) == window_len:
        break

mean_filtered_image = np.floor(np.array(image_window).sum(axis=0) / window_len)

print(mean_filtered_image)

print(image)

print(image_window[0])

print(np.array(image_window).sum(axis=0))

image_sum = np.uint8(np.array(image_window).sum(axis=0) / window_len)
print(image_sum)

#cv2.imshow("Image Filter Window",image_sum) # Title of diplay window to show, and argument of image

plt.figure()
plt.imshow(image_sum,cmap='grey')
plt.show()

cv2.imshow("Image Window", image) # Title of diplay window to show, and argument of image

cv2.waitKey(0)                    # pauses program until a key is pressed (wait open until input)
cv2.destroyAllWindows()           # destroy all when key is pressed 

print(image_sum.max())
print(image_sum.min())
print(image_sum.mean())

print((image_sum == image_sum.max()).sum())

