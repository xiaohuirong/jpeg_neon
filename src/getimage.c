#include "getimage.h"

int initcamera(unsigned char *device , unsigned char *mptr[4], unsigned int size[4], __u32 format, unsigned int width, unsigned int height){
  //1.打开设备
    int fd = open(device, O_RDWR);
    if(fd < 0){
        perror("打开设备失败\n");
        return -1;
    }

    else{
      printf("1. 打开设备成功\n");
    }

    //2.获取摄像头支持的格式ioctl(文件描述符，命令，与命令相应的结构体)可查看/usr/include/linux/videodev2.h
    struct v4l2_fmtdesc v4fmt;
    v4fmt.type =  V4L2_BUF_TYPE_VIDEO_CAPTURE;

    printf("2. 打印支持格式\n");
    int i;
    for(i=0; ; i++){
      v4fmt.index = i; //查询特定格式，可让index一直加直到ret等于-1，就可以读取所有支持的格式，这是一种穷尽方式读取支持类型的方法
      int ret = ioctl(fd, VIDIOC_ENUM_FMT, &v4fmt);//获取摄像头支持的格式
      if(ret < 0){
          break;
      }
      printf("---------%d---------\n", i);
      printf("index=%d\n", v4fmt.index);
      printf("flag=%d\n", v4fmt.flags);
      printf("description=%s\n", v4fmt.description);
      unsigned char *p = (unsigned char *)&v4fmt.pixelformat;
      printf("pixelformat=%c%c%c%c\n", p[0], p[1], p[2], p[3]);
      printf("reserved=%d\n", v4fmt.reserved[0]);
      printf("\n");
    }

    //3.设置摄像头采集格式
    struct v4l2_format vfmat;
    vfmat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//摄像头采集
    //不能设置任意大小
    vfmat.fmt.pix.width = width;//设置宽
    vfmat.fmt.pix.height = height;//设置宽
    vfmat.fmt.pix.pixelformat = format;//设置视频采集格式
    int ret = ioctl(fd, VIDIOC_S_FMT, &vfmat);
    if(ret < 0){
        perror("设置格式失败\n");
    }
    //查看设置是否成功
    memset(&vfmat, 0, sizeof(vfmat));//清空结构体
    vfmat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//重新设置type
    ret = ioctl(fd, VIDIOC_G_FMT, &vfmat);//获取格式
    if(ret < 0){
        perror("获取格式失败");
    }
    if(vfmat.fmt.pix.width == width && vfmat.fmt.pix.height == height && vfmat.fmt.pix.pixelformat == format){
        printf("3. 设置采集格式成功\n");
    }
    else{
        printf("设置失败\n");
    }

    //4. 申请内核空间
    struct v4l2_requestbuffers reqbuffer;
    reqbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuffer.count = 4; //最多四个缓冲区
    reqbuffer.memory = V4L2_MEMORY_MMAP; //映射方式
    
    ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuffer);
    if(ret < 0){
        perror("申请队列空间失败\n");
    }
    else{
      printf("4. 申请队列空间成功\n");
    }


    //5. 映射内存空间
    //unsigned char *mptr[4]; //保存映射空间的首地址
    //unsigned int size[4]; //长度
    struct v4l2_buffer mapbuffer;
    //结构体初始化type, index
    mapbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    for(int i=0; i<4; i++){
      mapbuffer.index = i;
      ret = ioctl(fd, VIDIOC_QUERYBUF, &mapbuffer);//从内核空间查询一个空间作映射
      if(ret < 0){
        perror("查询失败\n");
      }
      mptr[i] = (unsigned char *)mmap(NULL, mapbuffer.length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, mapbuffer.m.offset);
      size[i] = mapbuffer.length;
      //通知内核使用完毕，放回
      ret = ioctl(fd, VIDIOC_QBUF, &mapbuffer);
      if(ret < 0){
        perror("放回失败\n");
      }
    }

    return fd;

}

int getimage(int fd, struct v4l2_buffer* readbuffer){
    //6. 开始采集
    //启动
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;
    ret  = ioctl(fd, VIDIOC_STREAMON, &type);
    if(ret < 0){
      perror("开启失败\n");
    }
    //从队列中提取一帧数据
    (*readbuffer).type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd, VIDIOC_DQBUF, readbuffer);
    if(ret < 0){
      perror("读取数据失败\n");
    }

    return 0;
}

int getfinish(int fd, struct v4l2_buffer* readbuffer){

    //通知内核使用完毕
    int ret;
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("index=%d size=%d\n", (*readbuffer).index, (*readbuffer).length);
    ret = ioctl(fd, VIDIOC_QBUF, readbuffer);
    if(ret < 0){
      perror("通知失败\n");
    }
    //停止采集
    ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
    return 0;
}

