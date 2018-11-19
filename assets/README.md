# Sample VR media

The **controller** directory contains a model of the Daydream Controller.

The **6dof_controller** directory contains a model of the Daydream 6DOF
Controller.

The **panoramas** directory contains sample 360 images. See the javadoc for
**sdk-simplepanowidget**'s **SimpleVrPanoramaActivity** class for information on
how to view these videos.

* **testRoom1_2kMono.jpg** is a 2k x 1k equirectangular projection of the inside
of a sphere with heavy, medium, and light gridlines at 90, 15, and 5 degrees,
respectively. The red, green, and blue objects are aligned with the x, y, and z
axes, respectively.

* **testRoom1_2kStereo.jpg** is a 2k x 2k top-bottom stereo version of
**testRoom1_2kMono.jpg**.

The **videos** directory contains sample videos. See the javadoc for
**sdk-simplevideowidget**'s **SimpleVrVideoActivity** class for information on
how to view these videos.

* **testRoom1_1920Mono.mp4** is similar to **testRoom1_2kMono.jpg** with a
moving sphere & cylinder revolving around the camera at 1 degree per frame and
30 fps. It has 360 metadata using the python script described at
https://support.google.com/youtube/answer/6178631?hl=en and **--stereo=none**.
It is only 1920x960 because some devices don't support resolutions higher than
1920x1080.

* **testRoom1_1080Stereo.mp4** is similar to **testRoom1_1080Mono.mp4** but is
stereo with **--stereo=top-bottom**. It is 1080x1080 because some devices don't
support higher resolutions. Alternatively, a 1920x1080 video could have been
used, but this would result in 1920x540 per eye which results in a noticeable
difference in horizontal and vertical distortion when resized to a 2:1
equirectangular projection due to the sharp horizontal and vertical lines.

* There are other video files in the **assets** directory of
sdk-simplevideowidget.
