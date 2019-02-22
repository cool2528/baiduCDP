# 简介
BaiduCDP 是工作业余时间使用C/C++写的一个百度网盘高速下载工具,通过分析百度网盘Web接口API结合aria2实现多线程高速下载,目前只能在Windows平台下使用。
## 构建
BaiduCDP 依赖第三方库如下
1.boost</br>
2.openssl </br>
3.zlib </br>
4.rapidjson </br>
5.libcurl </br>
6.miniblink </br>
7.glog </br>
##### 1.获取源代码
> git clone https://github.com/cool2528/baiduCDP.git
##### 2.下载依赖第三方库
> 到这里下载 除boost以外的第三方库
https://github.com/cool2528/baiduCDP/releases/tag/1.0.0
至于Node.dll可以去https://github.com/cool2528/baiduCDP/releases/tag/1.0.1
成品文件中提取也可以直接下载miniblin编译
##### 3.构建环境
> 使用Visual Studio 2015 因为第三库我自己构建的都是 vs2015编译构建的
##### 4.下载UI界面源代码
> git clone https://github.com/cool2528/BaiduCdpUi.git
下载下来后新建一个文件夹重命名为`ui`把Ui界面的源码文件拷贝到`ui`文件夹下
然后把`ui`文件夹拷贝到`BaiduCDP`项目下`Debug`目录下编译生成即可
##### 5.使用效果
>1.先启动BaiduCDP.exe 点击登录百度网盘按钮登录成功后即可使用
![执行结果](https://i.loli.net/2019/02/21/5c6e1a5e9bba5.gif)
