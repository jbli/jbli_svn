raspberrypi

----------

#初始化
树莓派默认用户名: pi <br>
密码: raspberry <br>
sudo raspi-config <br>


无线网络
-------
使用磊科的usb接口的无线wifi， /etc/network/interfaces配置如下

```
auto lo

iface lo inet loopback
auto eth0
iface eth0 inet dhcp

auto wlan0
allow-hotplug wlan0
iface wlan0 inet static
address 192.168.1.112
netmask 255.255.255.0
network 192.168.1.0
broadcast 192.168.1.255
wpa-ssid "xxLINK"
wpa-psk "xxxxxx"
```

视频监控
------
##安装mjpg-streamer
参考: [使用树莓派制作近距离遥控摄像机](http://blog.catscarlet.com/201311131578.html) <br>

安装

```
apt-get install libv4l-dev libjpeg8-dev imagemagick
svn co https://svn.code.sf.net/p/mjpg-streamer/code/mjpg-streamer
sudo ln -s /usr/include/libv4l1-videodev.h /usr/include/linux/videodev.h
cd mjpg-streamer
make USE_LIBV4L2=true clean all
```
测试

```
sudo ./mjpg_streamer -i “./input_uvc.so -d /dev/video0  -r 1280×720 -f 12″ -o “./output_http.so -p 8090 -w ./www”
（-r后是分辨率参数，-f后面是帧率，请根据您的摄像头参数进行调整）
```
PC上打开浏览器，在地址栏填写树莓派的地址+:8090，如果成功的话就会打开mjpg-streamer提供的页面

后台开启视频采集并进行实时监控，在做记录的同时可以开浏览器进行监控。记录的文件存放在 /home/pi/tmp/ 中

```
cd ~/src/mjpg-streamer/ 
sudo nohup ./mjpg_streamer -i “./input_uvc.so -d /dev/video0  -r 1280×720 -f 12″ -o “./output_file.so -f /home/pi/tmp/” &
sudo nohup ./mjpg_streamer -i “./input_file.so -f /home/pi/tmp/” -o “./output_http.so -p 8090 -w ./www” &
```