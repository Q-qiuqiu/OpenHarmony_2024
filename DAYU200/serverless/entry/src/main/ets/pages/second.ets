import testNapi from 'libentry.so';
import image from '@ohos.multimedia.image';
import http from '@ohos.net.http';
@Entry
@Component
struct Index {
  @State result1: string = `{
  "result": [
    [
      269.28125,
      197.75,
      392.71875,
      359.25,
      0.7147121429443359,
      41
    ],
    [
      596.15625,
      246.5625,
      639.84375,
      357.9375,
      0.49903178215026855,
      24
    ],
    [
      15.8125,
      295.171875,
      88.0625,
      358.328125,
      0.27820515632629395,
      41
    ],
    [
      444.40625,
      299.125,
      531.59375,
      360.375,
      0.27074623107910156,
      64
    ]
  ],
  "width": 640,
  "height": 360,
  "status": "success"
}
`
  @State url:Array<string>=["rtsp://192.168.58.1:8554/camera_test1","rtsp://192.168.58.1:8554/camera_test2","rtsp://192.168.58.1:8554/camera_test1","rtsp://192.168.58.1:8554/camera_test2"];
  @State msgHistory: string = '日志:\r\n'

  @State imgWidth: Array<number> = [640,640,640,640];
  @State imgHeight: Array<number> = [360,360,360,360];
  //是否放大
  @State look:Array<boolean>=[false,false,false,false];
  //是否绘制完成
  @State finished:Array<boolean>=[true,true,true,true];
  //定义画板大小
  @State canvasWidth: number =200;
  @State canvasHeight: number = 200;
  @State bizhi:number=1//略缩比
  @State services: Array<string> = ['窗口0', '窗口1']
  @State newimages: Array<PixelMap> = [];//储存图像
  //canvas
  private settings: RenderingContextSettings = new RenderingContextSettings(true);
  @State contexts: Array<CanvasRenderingContext2D> = [];
  @State count: number = 0;
  //初始化画板
  @State canvasborder:Array<number>=[2,2];
  @State inital:boolean=true;
  @State  frameCount:number = 0; // 记录刷新次数
  @State  frame:number = 0; // 记录刷新次数
  timerHandle: number[] = []; // 存储每个窗口的计时器句柄
  @State lock:boolean=true;
  scroller: Scroller = new Scroller()
  startTime = new Date().getTime();
  pixelInfo: Array<testNapi.PixelInfo> = [{ byteBuffer: null,status:"failed"},
    { byteBuffer: null,status:"failed" }];
  @State stop:Array<boolean>=[true,true,true,true];

  //发送停止解码的请求
  HttpStop(index:number){
    const startTime = new Date().getTime();
    this.msgHistory = "停止中，请等待";
    let httpRequest = http.createHttp();
    const url='http://192.168.58.3:8081/stop?url='+this.url[index]
    let promise = httpRequest.request(url, {
      method: http.RequestMethod.POST,
    });
    this.stop[index]=true;
    promise.then((resp:http.HttpResponse) => {
      let responseData = JSON.parse(resp.result.toString());
      if (responseData.status === 'success') {
        this.msgHistory = "停止成功\r\n";
        this.stopFetching(index)
        const endTime = new Date().getTime();
        this.msgHistory+='停止用时：'+(endTime-startTime)+"ms\r\n"
      } else {
        this.msgHistory = "停止错误,系统已停止!\r\n"+responseData.reason+responseData.status+"\r\n";
        const endTime = new Date().getTime();
        this.msgHistory+='停止用时：'+(endTime-startTime)+"ms\r\n"
      }
    }).catch((err:Error) => {
      //this.msgHistory ='停止成功，未知错误:' + JSON.stringify(err)+"\r\n";
      this.msgHistory = "停止成功\r\n";
      const endTime = new Date().getTime();
      this.msgHistory+='停止用时：'+(endTime-startTime)+"ms\r\n"
    });
    httpRequest.destroy()
  }

  //发送开始解码的请求
  HttpStart(index){
    const startTime = new Date().getTime();
    this.msgHistory = "开启中，请等待";
    const url='http://192.168.58.3:8081/start?url='+this.url[index]
    let httpRequest = http.createHttp();
    let promise = httpRequest.request(url, {
      method: http.RequestMethod.POST,
    });
    this.stop[index]=false;
    promise.then((resp:http.HttpResponse) => {
      let responseData = JSON.parse(resp.result.toString());
      if (responseData.status === 'success') {
        this.msgHistory = "开启成功\r\n";
        const endTime = new Date().getTime();
        this.msgHistory+='开启用时：'+(endTime-startTime)+"ms\r\n"
        //this.Httpget(index)
        this.startFetching(index)
      } else {
        this.msgHistory = "开启错误,系统已启动!\r\n"+responseData.reason+responseData.status+"\r\n";
        const endTime = new Date().getTime();
        this.msgHistory+='开启用时：'+(endTime-startTime)+"ms\r\n"
      }
      httpRequest.destroy()
    }).catch((err:Error) => {
      this.msgHistory ='开启错误:' + JSON.stringify(err)+"\r\n";
      const endTime = new Date().getTime();
      this.msgHistory+='开启用时：'+(endTime-startTime)+"ms\r\n"
      httpRequest.destroy()
    });
  }

  //获取图像请求
  Httpget(index){
    this.msgHistory = '日志:\r\n'
    this.msgHistory += '---------------------窗口' + index + '--------------------\r\n'
    //this.msgHistory+='当前帧率:' +this.frame+'\r\n'
    const startTime = new Date().getTime();
    this.pixelInfo[index]=testNapi.HttpGet_png('http://192.168.58.3:8081',this.url[index])
    const endTime = new Date().getTime();
    this.msgHistory+='HTTP用时：'+(endTime-startTime)+"\r\n"

    if(this.pixelInfo[index].status=="failed"){
      this.msgHistory+="视频结束"
      this.Httpget(index)
    }else {
      let combinedArrayBuffer
      let combinedHeight= this.imgHeight[0];
      let combinedWidth= this.imgWidth[0];
      // 根据pixelInfo中的状态确定如何合并并显示图像
      if(this.pixelInfo[0].byteBuffer != null && this.pixelInfo[1].byteBuffer != null) {
        combinedArrayBuffer = this.mergeArrayBuffers(this.pixelInfo[0].byteBuffer, this.pixelInfo[1].byteBuffer);
        combinedHeight = this.imgHeight[0] + this.imgHeight[1];
      }
      else if ( this.pixelInfo[1].byteBuffer != null && this.pixelInfo[1].byteBuffer == null) {
        combinedArrayBuffer = this.pixelInfo[0].byteBuffer;
      } else if (this.pixelInfo[0].byteBuffer == null && this.pixelInfo[1].byteBuffer != null) {
        combinedArrayBuffer = this.pixelInfo[1].byteBuffer;
      }
      let opts: image.InitializationOptions = {
        editable: true,
        pixelFormat: 3,
        size: { height: combinedHeight, width: combinedWidth }
      }

      image.createPixelMap(combinedArrayBuffer, opts, (error, pixelmap) => {
        if (error) {
          console.info(`Failed to create combined pixelmap error_code ${error.code}`);
        } else {
          this.newimages[index] = pixelmap;
           // 绘制拼接后的图片
          if(!this.stop[index]) {
            this.drawimg(index)
          }
        }
      })
    }
  }
  mergeArrayBuffers(buffer1: ArrayBuffer, buffer2: ArrayBuffer): ArrayBuffer {
    let tmp = new Uint8Array(buffer1.byteLength + buffer2.byteLength);
    tmp.set(new Uint8Array(buffer1), 0);
    tmp.set(new Uint8Array(buffer2), buffer1.byteLength);
    return tmp.buffer;
  }

  getfile(index){
    let context = getContext(this);
    let resourceManager = context.resourceManager;
    resourceManager.getRawFileContent("frame_newsize.bin", (error, value) => {
      if (error != null) {
      } else {
        // 将字符串转换为 Uint8Array
        const stringToUint8Array = (str: string): Uint8Array => {
          const charCodes = new Array(str.length);
          for (let i = 0; i < str.length; i++) {
            charCodes[i] = str.charCodeAt(i);
          }
          return new Uint8Array(charCodes);
        };

        const result1Bytes = stringToUint8Array(this.result1);
        const separator = stringToUint8Array('!');

        // 创建一个新的 Uint8Array 用于存储拼接结果
        const combinedLength = result1Bytes.length + separator.length + value.byteLength;
        const combinedResult = new Uint8Array(combinedLength);
        // 设置各部分的数据到 combinedResult
        combinedResult.set(result1Bytes, 0);
        combinedResult.set(separator, result1Bytes.length);
        combinedResult.set(new Uint8Array(value), result1Bytes.length + separator.length);

        this.pixelInfo[index]=testNapi.Img2Gray(combinedResult)

        let combinedArrayBuffer
        let combinedHeight= this.imgHeight[0];
        let combinedWidth= this.imgWidth[0];
        // 根据pixelInfo中的状态确定如何合并并显示图像
        if (this.pixelInfo[0].byteBuffer == null) {
          combinedArrayBuffer = this.pixelInfo[1].byteBuffer;

        } else if (this.pixelInfo[1].byteBuffer == null) {
          combinedArrayBuffer = this.pixelInfo[0].byteBuffer;

        } else if(this.pixelInfo[0].byteBuffer != null && this.pixelInfo[1].byteBuffer != null) {
          combinedArrayBuffer = this.mergeArrayBuffers(this.pixelInfo[0].byteBuffer, this.pixelInfo[1].byteBuffer);
          combinedHeight = this.imgHeight[0] + this.imgHeight[1];
        }
        let opts: image.InitializationOptions = {
          editable: true,
          pixelFormat: 3,
          size: { height: combinedHeight, width: combinedWidth }
        }

        image.createPixelMap(combinedArrayBuffer, opts, (error, pixelmap) => {
          if (error) {
            console.info(`Failed to create combined pixelmap error_code ${error.code}`);
          } else {
            this.newimages[index] = pixelmap;
            this.drawimg(index); // 绘制拼接后的图片
          }
        })
      }
    })
  }

  //绘制图像
  drawimg(index){
    if (this.imgWidth[index] > this.imgHeight[index]) {
      this.bizhi = this.canvasWidth / this.imgWidth[index];
    } else {
      this.bizhi = this.canvasHeight / this.imgHeight[index];
    }
    this.canvasborder[index]=0
    this.contexts[index].drawImage(this.newimages[index], 0, 0, this.imgWidth[index]* this.bizhi, 2*this.imgHeight[index] * this.bizhi)
    this.finished[index]=true
    this.lock=true
  }

  //启动计时器
  async startFetching(index: number) {
    if (this.timerHandle[index]) {
      clearInterval(this.timerHandle[index]);
    }
    this.timerHandle[index] = setInterval(() => {
      if(this.finished[index]) {
        this.finished[index]=false
        this.Httpget(index)
        //this.getfile(index)
      }
    },20);
  }

  //停止计时器
  stopFetching(index: number) {
    if (this.timerHandle[index]) {
      this.msgHistory=""
      this.timerHandle[index] = null;
      clearInterval(this.timerHandle[index]);
      this.contexts[index].clearRect(0, 0, this.canvasWidth, this.canvasHeight); // 清理画布内容
    }
  }


  build() {
    Row() {
      Column() {
        if(!this.look[this.count]) {
          Text("new窗口模式")
            .fontSize(20)
            .width(200)
            .height("30")
            .textAlign(TextAlign.Center)
            .fontWeight(FontWeight.Bold)
          Grid() {
            ForEach(this.services, (service, index) => {
              GridItem() {
                Column() {
                  Text(service)
                    .textAlign(TextAlign.Start)
                    .width(250)
                  Canvas(this.contexts[index])
                    .width(this.canvasWidth)
                    .height(this.canvasHeight)
                    .border({ width: this.canvasborder[index] })
                    .onReady(() => {
                      if (this.inital) {
                        this.services.forEach(() => {
                          this.contexts.push(new CanvasRenderingContext2D(this.settings));
                        });
                        this.inital = false

                        this.contexts[0].strokeRect(10,10,200,200)
                      }
                    });
                  Text("")
                  Row() {
                    Button('启动', { type: ButtonType.Capsule, stateEffect: true })//开始
                      .fontSize(20)
                      .backgroundColor('#317aff')
                      .width(80)
                      .height(40)
                      .onClick(() => {
                        //this.getfile(index)
                        this.HttpStart(index)
                      })
                    Button('停止', { type: ButtonType.Capsule, stateEffect: true })//停止
                      .fontSize(20)
                      .backgroundColor('#317aff')
                      .width(80)
                      .height(40)
                      .onClick(() => {
                        this.HttpStop(index)
                      })
                    Button('全屏', { type: ButtonType.Capsule, stateEffect: true })//全屏
                      .fontSize(20)
                      .backgroundColor('#317aff')
                      .width(80)
                      .height(40)
                      .onClick(() => {
                        this.count=index
                        this.canvasHeight = 400
                        this.canvasWidth = 400
                        this.look[index]=true
                      })
                  }
                }
              }
            }, service => service)
          }.maxCount(2)
          .layoutDirection(GridDirection.Row)
          .columnsGap(10)
          .rowsGap(10)
        }else{
          Column() {
            Text("全屏模式")
              .fontSize(20)
              .width(200)
              .height("30")
              .textAlign(TextAlign.Center)
              .fontWeight(FontWeight.Bold)
            Canvas(this.contexts[this.count])
              .width(this.canvasWidth)
              .height(this.canvasHeight)
              .border({ width: this.canvasborder[this.count] })
              .onReady(() => {
                this.contexts[this.count] = new CanvasRenderingContext2D(this.settings);
              });
            Text("")
            Button('返回主页', { type: ButtonType.Capsule, stateEffect: true })
              .fontSize(20)
              .backgroundColor('#317aff')
              .width(120)
              .height(40)
              .onClick(() => {
                this.canvasHeight = 200
                this.canvasWidth = 200
                this.look[this.count] = false
              })
          }
        }
        Scroll(this.scroller) {
          Text(this.msgHistory)
            .textAlign(TextAlign.Start)
            .padding(10)
            .width('100%')
            .backgroundColor("#D6D6D6")
        }
        .align(Alignment.Top)
        .backgroundColor("#D6D6D6")
        .height(300)
        .flexGrow(1)
        .scrollable(ScrollDirection.Vertical)
        .scrollBar(BarState.On)
        .scrollBarWidth(20)
      }
      .height('100%')
    }
  }
}