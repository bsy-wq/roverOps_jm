项目架构：

├── advanced_param.xml
├── CMakeLists.txt
├── FSM_DESIGN.md
├── hikvision_ptz.py
├── include
│   ├── CommandExecutor.h
│   ├── common
│   ├── config
│   ├── http_map_uploader.h
│   ├── manager.h
│   └── RobotFsm.h
├── LIGHT_CONTROL.md
├── LOCAL_WEBSOCKET.md
├── MOTION_WEB_CONTROL_PROTOCOL.md
├── params22.yaml
├── params.yaml
├── readme.md
├── robot.yaml
├── src
│   ├── aiRecognition  // AI 识别， 用来存储和转发图片到web端
│   ├── bms            // 电池数据读取
│   ├── broadcast      // 喊话器， 用来下载远程服务器上文件，并复制到喊话器内部
│   ├── CommandExecutor.cc
│   ├── gateway        // 网关
│   ├── hikvision      // 海康相机控制
│   ├── localWebsocket // 本地 websocket 
│   ├── main.cc
│   ├── mapping        // 建图
│   ├── modifyValue    // 修改 xml 和 yaml 文件
│   ├── motionControl  // 机器狗运控
│   ├── robotDto.h
│   └── robotState     // 对接 web 端机器狗状态
