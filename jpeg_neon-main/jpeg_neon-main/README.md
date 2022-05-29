## 修改说明
**移植说明：**
图像增强部分在image_enhanced.c中，加入了两行代码已标出.

需要在jpeg_encoder_neon.中使用，需要将其移植过去,并在jpeg_encoder_neon.c的开头加入`extern int image_enhanced(jpeg_data* data);`

**修改说明：**
1.为了便于在不同代码之间进行一些结构体的引用，我将jpeg_encoder_neon.c和jpeg_encoder.c的结构体声明改到了.h文件中
2.这一版取消了c++的opencv库的引用，故不需要安装opencv
3.makefile文件进行了修改，区分了C和C++不同的编译器(虽然没用到g++)

## 使用说明

编译：
```sh
make clean
make
# 若需要编译不带neo版本的 
# 查看Makefile文件中修改提示

```


运行：
可以在bin/下找到gcode,运行就可以拍摄一张图片并编码输出为out.jpg文件

```sh
./bin/gcode
```

