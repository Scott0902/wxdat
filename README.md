### wxdat
### 解密微信电脑版的 dat 格式文件还原为图片格式。

#### 使用方法：
>wxdat.exe <dat文件路径> [输出路径]
>
如果不指定 “输出路径”，则默认输出到 dat 文件路径。


1、纯 C 语言代码编写，编译后的 exe 程序只有 28KB。

2、使用 SSE2 指令进行异或运算转换，比传统的 xor 运算更快。

3、处理速度：机械硬盘 60MB/s 左右，SATA 固态硬盘：200MB/s 以上，NVME固态硬盘：未测试。


#### *不足之处：*

1、命令行操作。

2、每次只处理一个目录。


