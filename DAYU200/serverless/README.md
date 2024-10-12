使用搭载了 Rockchip RK3568 芯片的润和 HH-SCDAYU200 开发板作为前端开发板。在此开发板上部署鸿蒙系统，开发一个前端显示程序。使用 DevEco Studio 进行前端开发，采用基于 api9 的 ArkTS 和 Native C++ Stage 模型。主要arkTS页面代码：index.ets，Native C++代码：hello.cpp。
```
 📁DAYU200
  └─ 📁serverless
     ├─ 📁APPScore
     ├─ 📁entry
     │  ├─ 📁libs                         // 存放OpenCV库文件
     │  ├─ 📁oh_modules                   // c++与ArkTS代码的绑定库
     │  └─ 📁src
     │     └─ 📁main
     │        ├─ 📁cpp
     │        │  ├─ 📁common
     │        │  │  └─ 📄common.cpp       // OpenCV相关代码
     │        │  ├─ 📁include             // OpenCV相关库文件
     │        │  ├─ 📁types
     │        │  ├─ 📄CmakeLists.txt
     │        │  └─ 📄hello.cpp           // 实现HTTP请求和响应结果处理
     │        ├─ 📁ets
     │        │  ├─ 📁enttyability
     │        │  ├─ 📁pages
     │        │  │  └─ 📄index.ets        // 前端主要页面代码
     │        │  └─ 📁util                // 工具类
     │        └─ 📁resources 
     │           └─ 📄module.json         // 更改权限代码
     ├─ 📁hvigor
     └─ 📁oh_modules
```
帧图像数据和推理结果由 HTTP 响应的 body 传输，定义格式为：推理结果 +!+ 帧图像数据。通过!对响应进行分割，叹号前的推理结果转成字符串保存，将字符串内的属性进行拆分得到图像框的坐标、概率和标签的数据，叹号后的帧图像数据经过格式转换后直接生成图片，由 Canvas 进行绘制，连续的 HTTP 请求带来的连续图像则显示成视频。通过 @ohos.request 和 @ohos.net.http 模块中的接口实现 HTTP 传输，通过 @ohos.multimedia.image 实现图片的生成和绘制。

具体前端部分流程如下图所示：

<img src="/image/前端开发板流程.png" alt="前端开发板流程" width="50%" />

前端程序函数调用关系如下图所示：

<img src="/image/前端函数调用.png" alt="前端函数调用" width="50%" />

前端界面实现效果如下：
<div style="display: flex; gap: 10px;">
  <img src="/image/前端界面实现效果.png" alt="前端界面" width="20%" style="margin-right: 100px;" />
  <img src="/image/前端界面全屏实现效果.png" alt="前端界面" width="20%" />
</div>
