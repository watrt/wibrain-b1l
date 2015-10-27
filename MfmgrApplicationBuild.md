# Mfmgr application 데비안 패키지 생성방법 #

## 미리 설치되어 있어야 하는 꾸러미 ##
  * subversion, dpkg-dev, autoconf, automake, libgtk2.0-dev

## 꾸러미 설치 방법 ##
  * 미리 설치되어 있어야 하는 꾸러미 들이 없는 경우 source build시 error가 나타날 수 있습니다.
  * B1L에 설치되어있는 우분투 타스크바의 [프로그램]->[보조프로그램]->[터미널]을 실행합니다.
  * 터미널에서 아래와 같은 명령을 통하여 꾸러미들을 설치할 수 있습니다.

```
sudo apt-get install subversion dpkg-dev autoconf automake libgtk2.0-dev 
```

## 소스 내려받기 (SVN Checkout) ##
  * subversion 꾸러미가 설치되어있다면 code.google.com의 http를 통하여 source를 받을 수 있습니다.
  * 소스는 아래와 같은 방법으로 내려받을 수 있습니다.(read-only)

```
svn checkout http://wibrain-b1l.googlecode.com/svn/trunk wibrain-b1l 
```

  * http에서 받은 소스는 wibrain-b1l 폴더 안에 저장됩니다.

## Mfmgr application build & debian package 생성방법 ##
  * wibrain/Application/mfmgr-1.0.0 폴더로 이동합니다.
  * 받은 source는 read-only로 되어 있기 때문에 아래와 같은 방법으로 source의 권한을 변경합니다.

```
sudo chmod -R 755 *
```

  * 다음 아래와 같은 명령으로 mfmgr application의 debian package를 생성할 수 있습니다.

```
sudo dpkg-buildpackage
```

  * compile이 완료되면 상위 폴더에 debian install package와 source package 파일들이 생성되어 집니다.