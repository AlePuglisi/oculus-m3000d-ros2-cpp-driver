# Simple Utils python code to delete all the saved sonar images 


import os
import shutil

images_polar_path = "./../images/sonar/2025_11_12_15_25/polar/"
images_cartesian_path = "./../images/sonar/2025_11_12_15_25/cartesian/"

for filename in os.listdir(images_polar_path):
    file_path = os.path.join(images_polar_path, filename)
    try:
        if os.path.isfile(file_path) or os.path.islink(file_path):
            os.unlink(file_path)
        elif os.path.isdir(file_path):
            shutil.rmtree(file_path)
    except Exception as e:
        print('Failed to delete %s. Reason: %s' % (file_path, e))

print(f'(Polar): Deleted all files from: {images_polar_path}')

for filename in os.listdir(images_cartesian_path):
    file_path = os.path.join(images_cartesian_path, filename)
    try:
        if os.path.isfile(file_path) or os.path.islink(file_path):
            os.unlink(file_path)
        elif os.path.isdir(file_path):
            shutil.rmtree(file_path)
    except Exception as e:
        print('Failed to delete %s. Reason: %s' % (file_path, e))

print(f'(Cartesian): Deleted all files from: {images_cartesian_path}')