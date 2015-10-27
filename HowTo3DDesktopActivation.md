# Introduction #

이페이지는 B1L ubuntu 7.10 에서  3D Compiz-Fusion 활성화하는 방법입니다.


# 적용 방법 #

  * /usr/bin/compiz 파일 vi 편집

```
sudo vi /usr/bin/compiz
```

  * compiz 파일안에 54 줄로 가서 White list에 **via** 추가하기

---

```
54 WHITELIST="via nvida intel ati radeon i810"
```

---

  * 시스템->관리->모양새->화면효과 설정을 **없음\*에서**보통**이나**Extra**등으로 변경한다.**


# 알려진 문제 #

  1. 동영상 재생시 출력화면(YUV 2D 가속기)이 안보인다.
  1. VT 전환을 2~3회 연속해서 하면, 시스템이 죽는 경우가 생긴다.
  1. 최대절전(Hibernation) 모드에서 돌아올때 화면에 아무것도 안나온다.
위 세가지 경우가 자신에게 심각한 문제라면, 3D Desktop 기능은 사용 불가 (현재 원인 및 대책 파악 중).

추가로 Hyunwoo.Park님의 Issue Report에 의하면, 3D Desktop 30분~1시간
이상 사용시 화면 출력 기능이 머추는 문제 발생