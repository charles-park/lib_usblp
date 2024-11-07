# lib_usblp
Zebra USB Label Printer Control Library(ZD230D, GC420D)

### Network Printer Server(ODROID-C4)
* Linux OS Image https://dn.odroid.com/S905X3/ODROID-C4/Ubuntu/22.04/ubuntu-22.04-4.9-minimal-odroid-c4-hc4-20220705.img.xz
* Available printer : Zebra GC420D(Discontinued product), Zebra ZD230D
* Printer connect : Direct USB Port connection.

### USB Label Pirinter Direct Control. ('lpr' linux command,https://www.computerhope.com/unix/ulpr.htm)
* GC420D(EPL Code) control example file (ref lib_usblp/manual/EPL2_GC420D_Manual.pdf file)
* ZD230D(ZPL code) control example file (ref lib_usblp/manual/ZPL_ZD230D_Manual.pdf file)
```
; GC420D Form
; Example Mac address print
I8,0,001
Q78,16
q240
rN
S4
D15
ZB
JF
O
R304,10
f100
N
A10,0,0,2,1,1,N,"forum.odroid.com >"
A16,32,0,2,1,1,N,"00:1E:06:11:22:33"
P1

; Example Error message print
I8,0,001
Q85,16
q240
rN
S4
D15
ZB
JF
O
R304,10
f100
N
A4,0,0,2,1,1,N,"1234567890123456789"
A4,22,0,2,1,1,N,"abcdefghijklmnopqrs"
A4,44,0,2,1,1,N,"ABCDEFGHIJKLMNOPQRS"
P1
```
```
; ZD230D Form
; Example Mac address print
^XA
^CFC
^LH0,0
^FO310,25
^FDforum.odroid.com >^FS
^FO316,55
^FD00:1E:06:11:22:33^FS
^XZ

; Example Error message print
^XA
^CFC
^LH0,0
^FO304,20
^FD1234567890123456789^FS
^FO304,40
^FDabcdefghijklmnopqrs^FS
^FO304,60
^FDABCDEFGHIJKLMNOPQRS^FS
^XZ
```

### Install package
```
root@odroid:~# uname -a
Linux odroid 4.9.312-6 #1 SMP PREEMPT Wed Jun 29 17:01:17 UTC 2022 aarch64 aarch64 aarch64 GNU/Linux

root@odroid:~# apt update --fix-missing
...
root@odroid:~# apt update && apt upgrade -y
...
root@odroid:~# apt install build-essential vim ssh git cups cups-bsd
...

root@odroid:~# reboot
...

root@odroid:~# uname -a
Linux odroid 4.9.337-17 #1 SMP PREEMPT Mon Sep 2 05:42:54 UTC 2024 aarch64 aarch64 aarch64 GNU/Linux

```

### Github setting
```
root@odroid:~# git config --global user.email "charles.park@hardkernel.com"
root@odroid:~# git config --global user.name "charles-park"
```

* Send data to label printer
```
root@odroid: lpr {printer control example file} -P zebra
```

### Label Printer setup & test
* Print device info
```
root@odroid:~# lpinfo -v
network beh
file cups-brf:/
network socket
network https
network ipps
network lpd
network http
direct usb://Zebra%20Technologies/ZTC%20ZD230-203dpi%20ZPL?serial=D4J222603053
network ipp
serial serial:/dev/ttyS0?baud=115200
serial serial:/dev/ttyS1?baud=115200
root@odroid:~# 
```

* Printer setting state
```
root@odroid: lpstat -v
lpstat: No destinations added.
```

* Label printer setup (GC420d)
```
root@odroid: lpadmin -p zebra -E -v usb://Zebra%20Technologies/ZTC%20GC420d%20\(EPL\)
root@odroid: lpstat -v
device for zebra: usb://Zebra%20Technologies/ZTC%20GC420d%20\(EPL\)
```
* Label printer setup (ZD230D)
```
root@odroid: lpadmin -p zebra -E -v usb://Zebra%20Technologies/ZTC%20ZD230-203dpi%20ZPL?serial=D4J222603053
root@odroid: lpstat -v
device for zebra: usb://Zebra%20Technologies/ZTC%20ZD230-203dpi%20ZPL?serial=D4J222603053
```

* Label printer setup (Network : ZD230D)
```
root@odroid: lpadmin -p zebra -E -v socket://192.168.20.36
root@odroid: lpstat -v
device for zebra: socket://192.168.20.36
```

* Label printer test
```
root@odroid:# git clone https://github.com/charles-park/lib_usblp
root@odroid:# cd lib_usblp
root@odroid:~/lib_usblp# lpr ./example/gc420d_form.txt -P zebra
root@odroid:~/lib_usblp# lpr ./example/zd230d_form.txt -P zebra
```

