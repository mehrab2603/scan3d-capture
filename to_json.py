"""
Converts calibration.yml file output by the Moreno-Taubin calibration software
into a calibration_result.json format compatible with Lux.
"""

import cv2
import argparse
import numpy as np


def convert(yml_path, json_path):
    """
    Parses the calibration results from the .yml file and converts it into a
    JSON file compatible with Lux.
    """
    reader = cv2.FileStorage(yml_path, cv2.FILE_STORAGE_READ)
    writer = cv2.FileStorage(json_path, cv2.FILE_STORAGE_WRITE)
    writer.write("cam_int", reader.getNode("cam_K").mat())
    writer.write("cam_dist", reader.getNode("cam_kc").mat())
    writer.write("proj_int", reader.getNode("proj_K").mat())
    writer.write("proj_dist", reader.getNode("proj_kc").mat())
    writer.write("rotation", reader.getNode("R").mat())
    writer.write("translation", reader.getNode("T").mat() / 1000)
    writer.write("img_shape", np.array([1080.0, 1920.0]))


def main():
    # Handle command-line arguments
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument("path_to_yml", type=str)
    parser.add_argument("path_to_json", type=str)

    args = parser.parse_args()
    yml_path = args.path_to_yml
    json_path = args.path_to_json

    convert(yml_path, json_path)

if __name__ == '__main__':
    main()
