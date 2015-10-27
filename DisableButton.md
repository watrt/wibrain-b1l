# Logout 화면에 나타나는 Suspend/Hibernate 버튼 삭제하는 방법 #

**자주 사용하지 않는 버튼을 삭제하여 실수로 인한 오동작을 막을 수 있습니다.**

[터미널 상에서 command입력을 통하여 버튼을 삭제하는 방법 ](.md)
```
1. [프로그램]->[보조프로그램]->[터미널]을 실행합니다.
2. gconftool-2 -t bool -s /apps/gnome-power-manager/general/can_hibernate false
3. gconftool-2 -t bool -s /apps/gnome-power-manager/general/can_suspend false
```

[gconf-editer를 통하여 버튼을 삭제하는 방법 ](.md)
```
1. [프로그램]->[보조프로그램]->[터미널]을 실행합니다.
2. gconf-editer를 실행합니다.
3. 왼쪽 창에서 apps->gnome-power-manager->general을 선택합니다.
4. 오른쪽의 can_suspend, can_hibernate 아이템 옆 Check Box의 Check상태를 해제합니다.
```