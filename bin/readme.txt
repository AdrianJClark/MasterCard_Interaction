Config file Settings:

colour_thresh: The threshold for difference between two colours to be considered the small. A smaller number will get less false positive matches, but have less variance to changes in lighting conditions, and vice versa.

angle_thresh: The threshold for angles. For example a angle_thresh of 15 indicates a value from 75 to 105 degrees is the equivalent to 90 degrees - ie: pointing right.

use_camera: A boolean (0 = false, 1 = true) value indicating whether to use a camera or load from a video. When playing a video, the program will close automatically once the video has finished playing.

camera_index: The index of the camera to use, starting at 0.

camera_width, camera_height: The resolution to use, a higher resolution will be slower.

debug_level: The amount of debugging information to show. 0 = nothing. 1 = window showing unedited video input. 2 = annotated video with rotation amount and location of logo. 3 = full debug output showing colour segmentation etc.

video_filename: The filename of the video file to load if use_camera is false. 

screensaver_timeout: The minimum number of frames in which the card wasn't visible before the invalid card animation will be played

key_delay: The number of frames between sending the same keypress, ie. If the card is facing right, it will send the right key, then after key_delay frames it will send it again. If the card changes to facing up before this time elapses, the up key will be pressed and then key_delay frames will be waited before sending up again

min_dist: The minimum distance between the yellow and red circle of the master card. Anything smaller than this will be rejected as a false match.

max_dist: The maximum distance between the yellow and red circle of the master card. Anything larger than this will be rejected as a false match.

movement_thresh: The threshold for level of movement before we assume thing has moved in front of the camera. Higher numbers indicate a larger movement has occured

min_contour_size: The minimum size of the logo

colour_yellow_(red/green/blue): The red/green/blue components of the yellow colour to search for. These should be obtained from video captured in the exact operating conditions to maximise robustness.

colour_red_(red/green/blue): The red/green/blue components of the red colour to search for. These should be obtained from video captured in the exact operating conditions to maximise robustness.