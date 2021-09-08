# openvino_meet_model
Google meet model run in C++ project with open vino to optimize performance intel x64 machine

# This project simulates Zoom Meeting virtual background flow:
- Process meet_camera get input from camera to do meeting stub standalone.(x86,x64...)
- Process meet_segment support virtual background for meet_camera.(only x64 support by openvino)
- These 2 process using share memory to comunicate.

**So this project solves the problem of architectural differences from x86 application and openvino toolkit support x64 app.**

# Demo
https://user-images.githubusercontent.com/22221442/132448362-2cf5f0a8-4e2a-4716-9df1-f777b775df0e.mp4

https://user-images.githubusercontent.com/22221442/132448375-7a7eb5ac-bfa0-4d86-95fb-a66a6b722e6c.mp4


