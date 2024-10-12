export interface PixelInfo {
  byteBuffer: ArrayBuffer;
  status:string;
  height:number;
  weight:number;
}
export const HttpGet: (url1:string,url2:string) => PixelInfo;
export const HttpGet_png: (url1:string,url2:string) => PixelInfo;
export const Img2Gray: (buf:ArrayBuffer) => PixelInfo;
export const Img2Gray_png: (buf:ArrayBuffer) => PixelInfo;