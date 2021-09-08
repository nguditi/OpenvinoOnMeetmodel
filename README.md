# openvino_meet_model
Google meet model run in C++ project with open vino to optimize performance intel x64 machine

# This project simulates Zoom Meeting virtual background flow:
- Process meet_camera get input from camera to do meeting stub standalone.(x86,x64...)
- Process meet_segment support virtual background for meet_camera.(only x64 support by openvino)
- These 2 process using share memory to comunicate.

**So this project solves the problem of architectural differences from x86 application and openvino toolkit support x64 app.**

# Demo
<video width="320" height="240" controls>
  <source src="/demo/demo.mp4" type="video/mp4">
</video>
<video width="320" height="240" controls>
  <source src="/demo/cpu_using.mp4" type="video/mp4">
</video>
