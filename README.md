### 简要说明
* 程序使用vs code的platformio插件进行的开发

### 已知问题
* nodemcu 网络不稳定,有时候会造成未能响应,刷新下即可
* 并未给出直观的页面配置,写过一个但是不太想弄了
* 机械郁金香的制作,真的令人心力憔悴,但是程序却不是重点
* 欢迎朋友们继续完善代码
* 如果我后续不玩摩托,可能会考虑继续完善

### 连接手机
nodemcu烧录代码后
手机开启热点,设置热点名称和密码同为
>mechflower

在手机端即可看到连接的设备ip,仅限安卓


### 花开
手机上看到连接的设备的ip地址后,打开浏览器在地址栏输入

```
http://你看到的ip地址
```

### 花闭
```
http://你看到的ip地址/close
```

### 花蕊色彩设置

```
http://你看到的ip地址/hm?mode=参数

参数可以为下面的字符
bpm
fire
xhl
pk
dpk
hpk
nomode
```



