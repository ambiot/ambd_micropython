# MicroPython Ameba RTL8722
Realtek的RTL8722是基于ARM Cortex-M33的双频WiFi和BLE 5.0的微型控制器，非常适合各种各样的物联网应用。

这是用于Ameba RTL8722平台的MicroPython移植的Alpha版本，该平台的详细信息可以在这里找到 https://www.amebaiot.com/cn/amebad/

##1.如何建立固件？
目前，该SDK仅支持在Cygwin或Linux上构建。

在继续之前，请确保您已经安装了GNU make

打开Cygwin终端/ Ubuntu终端，并导航到“ \ micropython_amebaD \ MicroPython_RTL8722 \ ports \ rtl8722”，然后键入，

$make
## 2.如何上传？
有两种方法可以将Ameba D MicroPython固件上传到您的Ameba。

###2.1发布文件夹
在Release文件夹中，有一个Double-Click-Me-to-Upload.cmd文件。

首先，我们右键单击它并选择“编辑”，然后将打开一个记事本，现在检查PC上Ameba的串行COM端口，并将正确的COM更新到文件的最后三行，然后保存文件并关闭它。

现在，在按住UART下载按钮的同时按RESET按钮，进入Download Mode，您Double-Click-Me-to-Upload.cmd现在可以双击并立即开始上传。

### 2.2 port / rtl8722文件夹
1，检查您的ameba串行/ COM端口，确保在makefile中的UPLOAD_PATH变量已更新了正确的ameba D的端口名；

2，在按住UART下载按钮的同时按RESET按钮进入Download Mode，然后键入以下命令，

$make upload
##3.如何使用MicroPython？
###3.1 MicroPython RTL8722移植简介
MicroPython通过强大的内置功能与单片机进行实时交互的REPL，使其与其他基于编译的平台（Arduino等）区分开来 。

REPL代表Read-Evaluation-Print-Loop，它是一个交互式提示，可用于访问和控制微控制器。

REPL还具有其他强大的功能，例如制表符补全，行编辑，自动缩进，输入历史记录等。它的基本功能类似于经典的Python IDLE，但在微控制器上运行。

要使用REPL，只需打开PC上的任何串行终端软件（最常用的软件是teraterm，putty等），然后连接到微控制器的串行端口，然后将波特率设置为115200再按reset即可重置开发板，然后您将看到>>>MicroPython提示符出现在串行终端软件上。现在，只要是MicroPython支持的Python模块，您都可以在REPL上任意的使用他们来进行开发。

###3.2 REPL热键
Ctrl + d：软重启->MicroPython将执行软件重启，这在微控制器行为异常时很有用。这还将再次在“ boot.py”中运行脚本。

Ctrl + e：粘贴模式->粘贴模式使您无需立即执行代码就可以立即将一大堆代码粘贴到REPL中。当您找到MicroPython库并希望通过复制和粘贴立即对其进行测试时，这很有用

Ctrl + b：普通模式->此热键会将REPL设置回普通模式。如果您陷于某些模式而无法脱身，这很有用。

Ctrl + c：快速取消->此热键可帮助您取消任何输入或中断当前正在运行的代码

##4.外围控制-machine模块
MicroPython Ameba D端口通过使用umachine模块支持丰富的外围功能

###GPIO
要控制GPIO，将Pin模块通过模块umachine导入。此处以PB_18引脚为例，输出逻辑电平0和1并闪烁3次

from umachine import Pin
a = Pin("PB_18", Pin.OUT)
a.value(1)
time.sleep_ms(500)
a.value(0)
time.sleep_ms(500)
a.on()
time.sleep_ms(500)
a.off()
time.sleep_ms(500)
a.toggle()
time.sleep_ms(500)
a.toggle()
###脉宽调制
要使用PWM（脉冲宽度调制），将PWM通过umachine导入。这里以引脚PA_26为例，以使LED缓慢点亮

from umachine import Pin, PWM
import time as t
p = PWM(pin = "PA_26")
for i in range(1000):
p.pulsewidth(i) # this and following line will be auto indented on REPL
t.sleep_ms(2)
###延迟和时间
使用time模块

import time
time.sleep(1)           # sleep for 1 second
time.sleep_ms(500)      # sleep for 500 milliseconds
time.sleep_us(10)       # sleep for 10 microseconds
start = time.ticks_ms() # get millisecond counter
###计时器
Timer通过umachine模块来导入

有4组32KHz通用定时器可供用户使用，分别是定时器0/1/2/3

from umachine import Timer
t = Timer(0)   ＃仅使用计时器0/1/2/3 
t.start（2000000，t.PERIODICAL）   ＃设置GTimer以2秒为周期定期触发，在终端上打印文本
###实时时钟
RTC通过umachine模块使用（实时时钟）模块

from umachine import RTC
rtc = RTC（）
rtc.datetime （（2020 ， 12 ， 31 ， 4 ， 23 ， 58 ， 59 ， 0 ））  ＃设置一个特定的日期和时间（年，月，日，星期（0星期一），小时，分钟，秒，总秒）
rtc.datetime（）＃获取日期和时间
###串口
UART通过umachine模块来导入模块

from umachine import UART
uart = UART（tx = “ PA_21 ”，rx = “ PA_22 ”）
uart.write（' hello '）
uart.read（5）＃最多读取5个字节
###I2C
I2C通过umachine模块来导入模块

注意：I2C仅在master模式下工作。

from umachine import I2C
i2c = I2C（scl = “ PA_25 ”，sda = “ PA_26 ”，freq = 100000）＃使用引脚和频率配置I2C。100KHz 
i2c.scan（）
i2c.writeto（8，' hello '）＃向地址为8的从站发送5个字节
i2c.readfrom（8，6）＃从从站接收5个字节
###SPI
SPI通过umachine模块使用串行外围接口模块，当前仅支持Master模式，默认SPI波特率是2MHz。

from umachine import SPI
spi = SPI（0）		 ＃仅支持2组SPI-0和1个
spi 				 ＃类型实例名称以检查SPI集的详细信息
spi.write（123）		 ＃写入编号123 
spi.read（）
