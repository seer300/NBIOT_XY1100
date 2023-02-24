��Դ��Ϣ˵��
======
  ARM��ʹ�ÿ�Դϵͳfreertos���汾FreeRTOS Kernel V10.3.1����ҵ��ʹ����CDP sdk��ONENET sdk��
  ͬʱо��SDK������mbedtls��libcoap��wakaama��Դ�⣬��Щ��Դ���Ѿ����Ա���ã��û�����ֱ��ʹ�á����ļ������˿�Դ��ı������Ӻ�ʹ�á�

�û����ο���
========
DEMO��ʹ�ã����Ķ���о��XY1100ƽ̨����ָ��_V1.1�����Լ�demo·���µ�readme

��α������ӿ�Դ�⣿��
===========
  ��SDK�У�targets\xinyiNBSoc_M3\Makefile\feature.mk��ʹ����ؿ�Դ��ĺ궨�弴�ɡ�
  
  ARM mbedtls��Դ�� --- MBEDTLS_SUPPORT
  
  libcoap��Դ��     --- LIBCOAP_SUPPORT
  
  wakaama��Դ��     --- WAKAAMA_SUPPORT
  
  eg��
  	LIBCOAP_SUPPORT=y//��������libcoap��Դ��
  	
  	LIBCOAP_SUPPORT=n//����������libcoap��Դ��


FreeRTOS
======
  ARM��ʹ�ÿ�ԴϵͳFreeRTOS��
�汾�� V10.3.1
·����https://www.freertos.org/a00104.html


CDP sdk
======
  cdpҵ�񿪷�ʹ�õ�sdk��֧�ֶԵ���CTWing����ͨ�ͻ�Ϊ��ƽ̨������
  CDP sdk�Ǽ�����LiteOS�ڵģ��汾���ȡ·������LiteOS
�汾��LiteOSV200R001C50B021
·����https://github.com/LiteOS/LiteOS/releases/tag/tag_LiteOS_V200R001C50B021_20180629  
  
ONENET sdk  
======
  �����ṩ��������ҵ��sdk��֧��������onenet ��ƽ̨�ĶԽ�
�汾��2.3.0
·����https://open.iot.10086.cn/doc/nb-iot/book/device-develop/get_SDK.html
  

ARM mbedtls��Դ��https://tls.mbed.org
======
  ARM mbedtlsʹ������Ա���Էǳ����ɵ��ڣ�Ƕ��ʽ��Ʒ�м�����ܺ� SSL/TLS ���ܣ������ṩ�˾���ֱ�۵� API �Ϳɶ�Դ����� SSL 
�⡣�ù��߼������ã������ڴ󲿷�ϵͳ��ֱ�ӹ�������Ҳ�����ֶ�ѡ������ø���ܡ�

  mbedtls���ṩ��һ��ɵ���ʹ�úͱ���ļ��������������ʹ�õ�������ͷ�ļ�������ų���Щ�����
  
  �ӹ��ܽǶ���������mbedtls��Ϊ������Ҫ���֣�   
	- SSL/TLS Э��ʵʩ�� 	
	- һ�����ܿ⡣ 	
	- һ�� X.509 ֤�鴦��⡣	
  Ŀǰʹ���ṩ��DTLS���Ǽ�����LiteOS�ڵģ��汾���ȡ·������LiteOS
	

libcoap��Դ��https://libcoap.net
======
  libcoap��CoAPЭ���C����ʵ�֣�libcoap�ṩserver��client���ܣ����ǵ���CoAP���������ߣ�sdk��
������libcoap����ͨ���Ѹ��򵥵�����˵��libcoap��ʹ�÷�����
�汾��release-4.2.0  
·����https://github.com/obgm/libcoap/tree/release-4.2.0


wakaama��LWM2M��ԴЭ��ջhttps://github.com/eclipse/wakaama
======
  wakaama������ʽ������һ��C�Ŀ��ļ���������Դ�������ʽ��ֱ�Ӻ���Ŀ�������ϱ��롣  
  SDKֻ�����˿ͻ��˴��룬�û������Լ�����ʹ�á�
�汾��Wakaama v1.0
·����https://github.com/eclipse/wakaama
  

MQTT��mqtt��ԴЭ��ջhttps://github.com/eclipse/paho.mqtt.embedded-c/tree/master
======
  MQTT��һ�����ڿͻ���-����������Ϣ����/���Ĵ���Э�飬��Դ�������ʽ���֣�ֱ�Ӻ���Ŀ�������ϱ��롣
�汾��  
·����https://github.com/eclipse/paho.mqtt.embedded-c/tree/master

HTTP��Դ��
======
  HTTPЭ����һ�����ڴ��䳬�ı���Ӧ�ò�Э�顣
  HTTP��Դ���Ǽ�����AliOS Things�ڵģ��汾���ȡ·������AliOS Things��
�汾��rel_3.3.0
·����http://gitee.com/alios-things-admin/AliOS-Things/tree/rel_3.3.0/

test

 
