## 使用说明
开发板：FT2000/4

编译方法：
(不带neon版本的根据makefile注释更改)
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

