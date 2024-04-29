
bgfx的Shader使用时，注意在头文件中有一行
```
#include <generated/shaders/src/all.h>
```
bigg2里面对shader编译的结果，指定了最终的集合头文件为all.h