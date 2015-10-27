# Introduction #

B1L 에 제공된 liveUSB 로 인스톨후, USB 저장장치를 연결했을 때 자동으로 마운트 안되는 경우
해결책(작은 팁) 입니다.

이 부분은 제공된 메뉴얼에 그림과 함께 자세히 설명되어 있기도 합니다.

# Details #

  * vi 혹은 gedit 툴을 이용하여 /etc/fstab 를 불러들인 후, 아래 해당하는 줄을 삭제한 후 저장하시면 됩니다.

```
/dev/sda1        /media/cdrom0    udf,iso9660 user,noauto,exec 0     0
```